#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"


typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int cls;
  float score;
  int batch_idx;
} detection;

static const char *coco_names[] = {
    "person",        "bicycle",       "car",           "motorbike",
    "aeroplane",     "bus",           "train",         "truck",
    "boat",          "traffic light", "fire hydrant",  "stop sign",
    "parking meter", "bench",         "bird",          "cat",
    "dog",           "horse",         "sheep",         "cow",
    "elephant",      "bear",          "zebra",         "giraffe",
    "backpack",      "umbrella",      "handbag",       "tie",
    "suitcase",      "frisbee",       "skis",          "snowboard",
    "sports ball",   "kite",          "baseball bat",  "baseball glove",
    "skateboard",    "surfboard",     "tennis racket", "bottle",
    "wine glass",    "cup",           "fork",          "knife",
    "spoon",         "bowl",          "banana",        "apple",
    "sandwich",      "orange",        "broccoli",      "carrot",
    "hot dog",       "pizza",         "donut",         "cake",
    "chair",         "sofa",          "pottedplant",   "bed",
    "diningtable",   "toilet",        "tvmonitor",     "laptop",
    "mouse",         "remote",        "keyboard",      "cell phone",
    "microwave",     "oven",          "toaster",       "sink",
    "refrigerator",  "book",          "clock",         "vase",
    "scissors",      "teddy bear",    "hair drier",    "toothbrush"};

static void usage(char **argv) {
  printf("Usage:\n");
  printf("   %s cvimodel image.jpg image_detected.jpg\n", argv[0]);
}

template <typename T>
int argmax(const T *data,
          size_t len,
          size_t stride = 1)
{
	int maxIndex = 0;
	for (size_t i = stride; i < len; i += stride)
	{
		if (data[maxIndex] < data[i])
		{
			maxIndex = i;
		}
	}
	return maxIndex;
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

static void NMS(std::vector<detection> &dets, int *total, float thresh)
{
  if (*total){
    std::sort(dets.begin(), dets.end(), [](detection &a, detection &b)
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
    std::vector<detection>::iterator it = dets.begin();
    while (it != dets.end()) {
      if (it->score == 0) {
        dets.erase(it);
      } else {
        it++;
      }
    }
    *total = new_count;
  }
}

void correctYoloBoxes(std::vector<detection> &dets,
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
        bbox.x   = (bbox.x - (input_width - restored_w) / 2.) * resize_ratio;
        bbox.y   = (bbox.y - (input_height - restored_h) / 2.) * resize_ratio;
        bbox.w *= resize_ratio;
        bbox.h *= resize_ratio;
        dets[i].bbox = bbox;
    }
}

/**
 * @brief
 * @param output
 * @note scores_shape : [batch , class_num, det_num, 1] 
 * @note des_shape: [batch, 1, 4, det_num]
 * @return int
 */
int getDetections(CVI_TENSOR *output,
                  int32_t input_height,
                  int32_t input_width,
                  int classes_num,
                  CVI_SHAPE output_shape,
                  float conf_thresh,
                  std::vector<detection> &dets) {  
    float *scores_ptr  = (float *)CVI_NN_TensorPtr(&output[1]);
    float *dets_ptr  = (float *)CVI_NN_TensorPtr(&output[0]);
    float stride[3] = {8, 16, 32};
    int count          = 0;
    int batch = output_shape.dim[0];
    int total_box_num = output_shape.dim[3];
    for (int b = 0; b < batch; b++) {
        int score_index = 0;
      for (int i = 0; i < 3; i++) {
        int nh = input_height / stride[i], nw = input_width / stride[i];
        int box_num = nh * nw;
        for (int j = 0; j < box_num; j++) {
          float anchor_x = (float)(j % nw) + 0.5, anchor_y = (float)(j / nw) + 0.5;
          int maxIndex = argmax(scores_ptr, classes_num * total_box_num - score_index, total_box_num);
          if (scores_ptr[maxIndex] <= conf_thresh) {
            scores_ptr = scores_ptr + 1;
            score_index++;
            dets_ptr = dets_ptr + 1;            
            continue; 
          }
          detection det;
          det.score = scores_ptr[maxIndex];
          det.cls = (maxIndex + score_index) / total_box_num;
          det.batch_idx = b;
          float x1 = anchor_x - dets_ptr[0 * total_box_num];
          float y1 = anchor_y - dets_ptr[1 * total_box_num];
          float x2 = anchor_x + dets_ptr[2 * total_box_num];
          float y2 = anchor_y + dets_ptr[3 * total_box_num];
          det.bbox.h = (y2 -y1) * stride[i];
          det.bbox.w = (x2 -x1) * stride[i];
          det.bbox.x = x1 * stride[i] + det.bbox.w / 2.0;
          det.bbox.y = y1 * stride[i] + det.bbox.h / 2.0;
          count++;  
          dets.emplace_back(det);
          scores_ptr = scores_ptr + 1;
          score_index++;
          dets_ptr = dets_ptr + 1;   
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
  //int bbox_len = 84; // classes num + 4
  int classes_num = 80;
  float conf_thresh = 0.5;
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
  assert(height % 32 == 0 && width %32 == 0);
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
  printf("CVI_NN_Forward Succeed...\n");
  // do post proprocess
  int det_num = 0;
  std::vector<detection> dets;
  det_num = getDetections(output, height, width, classes_num, output_shape[0],  
                          conf_thresh, dets);
  // correct box with origin image size
  NMS(dets, &det_num, iou_thresh);
  correctYoloBoxes(dets, det_num, cloned.rows, cloned.cols, height, width);

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
    char content[100];
    sprintf(content, "%s %0.3f", coco_names[dets[i].cls], dets[i].score);
    cv::putText(cloned, content, cv::Point(x1, y1),
                cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
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