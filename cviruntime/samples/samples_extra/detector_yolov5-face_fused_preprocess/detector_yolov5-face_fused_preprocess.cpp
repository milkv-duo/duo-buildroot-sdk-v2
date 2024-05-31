#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"

#define MAX_DET 20000

typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int cls;
  float score;
  int batch_idx;
  std::vector<std::pair<float, float>> vlandmark;
} detection;

typedef struct {
  CVI_TENSOR *output;
  int num_anchors, h, w, bbox_len, batch = 1, layer_idx;
} detectLayer;

static float anchors_[3][3][2] = {{{4, 5}, {8, 10}, {13, 16}},
                                  {{23, 29}, {43, 55}, {73, 105}},
                                  {{146, 217}, {231, 300}, {335, 433}}};

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel image.jpg image_detected.jpg\n", argv[0]);
}

static float sigmoid(float x)
{
  return 1.0 / (1 + expf(-x));
}

float calIou(box a, box b)
{
  float area1 = a.w * a.h;
  float area2 = b.w * b.h;
  float wi = std::min((a.x + a.w / 2), (b.x + b.w / 2)) - std::max((a.x - a.w / 2), (b.x - b.w / 2));
  float hi = std::min((a.y + a.h / 2), (b.y + b.h / 2)) - std::max((a.y - a.h / 2), (b.y - b.h / 2));
  float area_i = std::max(wi, 0.0f) * std::max(hi, 0.0f);
  return area_i / (area1 + area2 - area_i);
}

void correctYoloBoxes(detection *dets,
                      int det_num,
                      int image_h,
                      int image_w,
                      int input_height,
                      int input_width) {
    int restored_w;
    int restored_h;
    float resize_ratio;
    if (((float)input_width / image_w) < ((float)input_height / image_h)) {
        restored_w = input_width;
        restored_h = (image_h * input_width) / image_w;
    } else {
        restored_h = input_height;
        restored_w = (image_w * input_height) / image_h;
    }
    resize_ratio = ((float)image_w / restored_w);

    for (int i = 0; i < det_num; ++i) {
        box bbox = dets[i].bbox;
        int b    = dets[i].batch_idx;
        std::vector<std::pair<float, float>> &vlandmark = dets[i].vlandmark;
        bbox.x   = (bbox.x - (input_width - restored_w) / 2.) * resize_ratio;
        bbox.y   = (bbox.y - (input_height - restored_h) / 2.) * resize_ratio;
        bbox.w *= resize_ratio;
        bbox.h *= resize_ratio;
        dets[i].bbox = bbox;
        for (uint32_t j = 0; j < vlandmark.size(); ++j) {
          vlandmark[j].first =
            (vlandmark[j].first - (input_width - restored_w) / 2.) * resize_ratio;
          vlandmark[j].second =
            (vlandmark[j].second - (input_height - restored_h) / 2.) * resize_ratio;
        }
    }
}

void NMS(detection *dets, int *total, float thresh)
{
  if (*total){
    std::sort(dets, dets + *total - 1, [](detection &a, detection &b)
              { return b.score < a.score; });
    int new_count = *total;
    for (int i = 0; i < *total; ++i)
    {
      detection &a = dets[i];
      if (a.score == 0)
        continue;
      for (int j = i + 1; j < *total; ++j)
      {
        detection &b = dets[j];
        if (dets[i].batch_idx == dets[j].batch_idx &&
            b.score != 0 && dets[i].cls == dets[j].cls &&
            calIou(a.bbox, b.bbox) > thresh)
        {
          b.score = 0;
          new_count--;
        }
      }
    }
    for (int i = 0, j = 0 ; i < *total && j < new_count; ++i) {
      detection &a = dets[i];
      if (a.score == 0)
        continue;
      dets[j] = dets[i];
      ++j;
    }
    *total = new_count;
  }
}

/**
 * @brief
 *
 * @note work as long as output shape [n, a, h, w, cls + 5]
 * @param layer
 * @param input_height
 * @param input_width
 * @param classes_num
 * @param conf_thresh
 * @param dets
 * @return int
 */
int getDetections(detectLayer *layer,
                  int32_t input_height,
                  int32_t input_width,
                  int classes_num,
                  float conf_thresh,
                  detection *dets) {
    CVI_TENSOR *output = layer->output;
    float *output_ptr  = (float *)CVI_NN_TensorPtr(output);
    int count          = 0;
    int w_stride       = layer->bbox_len;
    int h_stride       = layer->w * w_stride;
    int a_stride       = layer->h * h_stride;
    int b_stride       = layer->num_anchors * a_stride;
    float down_stride  = input_width / layer->w;
    for (int b = 0; b < layer->batch; b++) {
        for (int a = 0; a < layer->num_anchors; ++a) {
            for (int i = 0; i < layer->w * layer->h; ++i) {
                int col          = i % layer->w;
                int row          = i / layer->w;
                float *obj       = output_ptr + b * b_stride + a * a_stride + row * h_stride + col * w_stride + 4;
                // -4,-3,-2,-1 box_xywh
                // 0           obj_conf
                // 1,2         landmark x1 y1
                // 3,4         landmark x2 y2
                // 5,6         landmark x3 y3
                // 7,8         landmark x4 y4
                // 9,10        landmark x5 y5
                // 11          cls_conf
                float objectness = sigmoid(obj[0]);
                if (objectness <= conf_thresh) {
                    continue;
                }
                objectness *= sigmoid(obj[11]);

                float x               = *(obj - 4);
                float y               = *(obj - 3);
                float w               = *(obj - 2);
                float h               = *(obj - 1);

                dets[count].score     = objectness;
                dets[count].cls       = 0;
                dets[count].batch_idx = b;

                dets[count].bbox.x = (sigmoid(x) * 2 + col - 0.5) * down_stride;
                dets[count].bbox.y = (sigmoid(y) * 2 + row - 0.5) * down_stride;
                dets[count].bbox.w = pow(sigmoid(w) * 2, 2) * anchors_[layer->layer_idx][a][0];
                dets[count].bbox.h = pow(sigmoid(h) * 2, 2) * anchors_[layer->layer_idx][a][1];
                // store landmark
                std::vector<std::pair<float, float>> vlandmark;
                for (int i = 1; i < 6; ++i) {
                  float lx = *(obj + i * 2 - 1) * anchors_[layer->layer_idx][a][0] + col * down_stride;
                  float ly = *(obj + i * 2) * anchors_[layer->layer_idx][a][1] + row * down_stride;
                  vlandmark.push_back(std::make_pair(lx, ly));
                }
                dets[count].vlandmark = vlandmark;
                ++count;
            }
        }
    }
    return count;
}


int main(int argc, char **argv) {
  int ret = 0;
  CVI_MODEL_HANDLE model;

  if (argc != 4) {
    usage(argv);
    exit(-1);
  }
  CVI_TENSOR *input;
  CVI_TENSOR *output;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_SHAPE input_shape;
  CVI_SHAPE* output_shape;
  int32_t height;
  int32_t width;
  float qscale;
  int bbox_len = 16;
  int classes_num = 1;
  float conf_thresh = 0.6;
  float iou_thresh = 0.5;

  ret = CVI_NN_RegisterModel(argv[1], &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }
  printf("CVI_NN_RegisterModel succeeded\n");

  // get input output tensors
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors,
                               &output_num);

  input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, input_tensors, input_num);
  assert(input);
  output = output_tensors;

  output_shape = reinterpret_cast<CVI_SHAPE *>(calloc(output_num, sizeof(CVI_SHAPE)));
  for (int i = 0; i < output_num; i++)
  {
    output_shape[i] = CVI_NN_TensorShape(&output[i]);
  }

  // nchw
  input_shape = CVI_NN_TensorShape(input);
  height = input_shape.dim[2];
  width = input_shape.dim[3];

  // imread
  cv::Mat image;
  image = cv::imread(argv[2]);
  if (!image.data) {
    printf("Could not open or find the image\n");
    return -1;
  }
  cv::Mat cloned = image.clone();

  // resize & letterbox
  int ih = image.rows;
  int iw = image.cols;
  int oh = height;
  int ow = width;
  double resize_scale = std::min((double)oh / ih, (double)ow / iw);
  int nh = (int)(ih * resize_scale);
  int nw = (int)(iw * resize_scale);
  cv::resize(image, image, cv::Size(nw, nh));
  int top = (oh - nh) / 2;
  int bottom = (oh - nh) - top;
  int left = (ow - nw) / 2;
  int right = (ow - nw) - left;
  cv::copyMakeBorder(image, image, top, bottom, left, right, cv::BORDER_CONSTANT,
                     cv::Scalar::all(0));
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  //Packed2Planar
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(image.rows, image.cols, CV_8SC1);
  }
  cv::split(image, channels);

  // fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size);
  }

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);

  // do post proprocess
  int det_num = 0;
  int count = 0;
  detection dets[MAX_DET];
  std::vector<detectLayer> layers;
  detection * dets_ptr = dets;

  int stride[3] = {8, 16, 32};
  // for each detect layer
  for (int i = 0; i < output_num; i++)
  {
    // layer init
    detectLayer layer;
    layer.output = &output[i];
    layer.bbox_len = bbox_len;
    layer.num_anchors = output_shape[i].dim[1];
    layer.h = output_shape[i].dim[2];
    layer.w = (int)(output_shape[i].dim[3] / bbox_len);
    layer.layer_idx = i;
    layers.push_back(layer);

    count = getDetections(&layer, height, width,
                          classes_num, conf_thresh, dets_ptr);
    det_num += count;
    dets_ptr += count;
    float *output_ptr = (float *)CVI_NN_TensorPtr(&output[i]);
  }
  // correct box with origin image size
  NMS(dets, &det_num, iou_thresh);
  correctYoloBoxes(dets, det_num, cloned.rows, cloned.cols, height, width);
  printf("get detection num: %d\n", det_num);

  // draw bbox on image
  for (int i = 0; i < det_num; i++) {
    box b = dets[i].bbox;
    // xywh2xyxy
    int x1 = (b.x - b.w / 2);
    int y1 = (b.y - b.h / 2);
    int x2 = (b.x + b.w / 2);
    int y2 = (b.y + b.h / 2);
    cv::rectangle(cloned, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 0),
                  3, 8, 0);
    std::vector<std::pair<float, float>> vlandmark = dets[i].vlandmark;
    for (uint32_t j = 0; j < vlandmark.size(); ++j) {
      cv::circle(cloned, cv::Point((int)vlandmark[j].first, (int)vlandmark[j].second), 1,
                 cv::Scalar(0, 255, 255), 8, 0);
    }
  }

  // save or show picture
  cv::imwrite(argv[3], cloned);

  printf("------\n");
  printf("%d objects are detected\n", det_num);
  printf("------\n");

  CVI_NN_CleanupModel(model);
  printf("CVI_NN_CleanupModel succeeded\n");
  free(output_shape);
  return 0;
}
