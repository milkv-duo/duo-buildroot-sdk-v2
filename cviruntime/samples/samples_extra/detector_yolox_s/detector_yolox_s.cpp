#include <math.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"

#define MAX_DET 200
#define YOLOX_NMS_THRESH  0.45 // nms threshold
#define YOLOX_CONF_THRESH 0.25 // threshold of bounding box prob

typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int cls;
  float score;
} detection;

struct Object
{
    cv::Rect_<float> rect;
    int label;
    float prob;
};

struct GridAndStride
{
    int grid0;
    int grid1;
    int stride;
};


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

static inline float intersection_area(const Object &a, const Object &b) {
  cv::Rect_<float> inter = a.rect & b.rect;
  return inter.area();
}

static void
generate_grids_and_stride(const int target_size_w, const int target_size_h, std::vector<int> &strides,
                          std::vector<GridAndStride> &grid_strides) {
  for (int i = 0; i < (int)strides.size(); i++) {
    int stride = strides[i];
    int num_grid_w = target_size_w / stride;
    int num_grid_h = target_size_h / stride;
    for (int g1 = 0; g1 < num_grid_h; g1++) {
      for (int g0 = 0; g0 < num_grid_w; g0++) {
        GridAndStride gs;
        gs.grid0 = g0;
        gs.grid1 = g1;
        gs.stride = stride;
        grid_strides.push_back(gs);
      }
    }
  }
}

static void generate_yolox_proposals(std::vector<GridAndStride> grid_strides,
                                     CVI_TENSOR *out_tensor,
                                     float prob_threshold,
                                     std::vector<Object> &objects) {
  CVI_SHAPE shape = CVI_NN_TensorShape(out_tensor);
  const int num_grid = shape.dim[1];
  const int num_class = shape.dim[2] - 5;
  const int num_anchors = grid_strides.size();

  const float *feat_ptr = (float *)CVI_NN_TensorPtr(out_tensor);
  for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
    int max_class_idx = 0;
    float max_prob = 0.0f;
    float box_objectness = feat_ptr[4];
    for (int class_idx = 0; class_idx < num_class; class_idx++) {
      float box_cls_score = feat_ptr[5 + class_idx];
      float box_prob = box_objectness * box_cls_score;
      if (box_prob > max_prob) {
        max_class_idx = class_idx;
        max_prob = box_prob;
      }
    } // class loop
    if (max_prob > prob_threshold) {
      const int grid0 = grid_strides[anchor_idx].grid0;
      const int grid1 = grid_strides[anchor_idx].grid1;
      const int stride = grid_strides[anchor_idx].stride;

      // yolox/models/yolo_head.py decode logic
      //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
      //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
      float x_center = (feat_ptr[0] + grid0) * stride;
      float y_center = (feat_ptr[1] + grid1) * stride;
      float w = exp(feat_ptr[2]) * stride;
      float h = exp(feat_ptr[3]) * stride;
      float x0 = x_center - w * 0.5f;
      float y0 = y_center - h * 0.5f;
      Object obj;
      obj.rect.x = x0;
      obj.rect.y = y0;
      obj.rect.width = w;
      obj.rect.height = h;
      obj.label = max_class_idx;
      obj.prob = max_prob;

      objects.push_back(obj);
    }

    feat_ptr += shape.dim[2];

  } // point anchor loop
}

static void qsort_descent_inplace(std::vector<Object> &faceobjects, int left,
                                  int right) {
  int i = left;
  int j = right;
  float p = faceobjects[(left + right) / 2].prob;

  while (i <= j) {
    while (faceobjects[i].prob > p)
      i++;

    while (faceobjects[j].prob < p)
      j--;

    if (i <= j) {
      // swap
      std::swap(faceobjects[i], faceobjects[j]);

      i++;
      j--;
    }
  }

#pragma omp parallel sections
  {
#pragma omp section
    {
      if (left < j)
        qsort_descent_inplace(faceobjects, left, j);
    }
#pragma omp section
    {
      if (i < right)
        qsort_descent_inplace(faceobjects, i, right);
    }
  }
}

static void qsort_descent_inplace(std::vector<Object> &objects) {
  if (objects.empty())
    return;

  qsort_descent_inplace(objects, 0, objects.size() - 1);
}

static void nms_sorted_bboxes(const std::vector<Object> &faceobjects,
                              std::vector<int> &picked, float nms_threshold) {
  picked.clear();

  const int n = faceobjects.size();

  std::vector<float> areas(n);
  for (int i = 0; i < n; i++) {
    areas[i] = faceobjects[i].rect.area();
  }

  for (int i = 0; i < n; i++) {
    const Object &a = faceobjects[i];

    int keep = 1;
    for (int j = 0; j < (int)picked.size(); j++) {
      const Object &b = faceobjects[picked[j]];

      // intersection over union
      float inter_area = intersection_area(a, b);
      float union_area = areas[i] + areas[picked[j]] - inter_area;
      // float IoU = inter_area / union_area
      if (inter_area / union_area > nms_threshold)
        keep = 0;
    }

    if (keep)
      picked.push_back(i);
  }
}

int post_process(CVI_TENSOR *out_tensor, int img_w, int img_h, int width, int height, float scale,
                 std::vector<Object> &objects) {
  std::vector<Object> proposals;
  static const int stride_arr[] = {8, 16, 32}; // might have stride=64 in YOLOX
  std::vector<int> strides(stride_arr, stride_arr + sizeof(stride_arr) /
                                                        sizeof(stride_arr[0]));
  std::vector<GridAndStride> grid_strides;
  generate_grids_and_stride(width, height, strides, grid_strides);
  generate_yolox_proposals(grid_strides, out_tensor, YOLOX_CONF_THRESH, proposals);
  // sort all proposals by score from highest to lowest
  qsort_descent_inplace(proposals);

  // apply nms with nms_threshold
  std::vector<int> picked;
  nms_sorted_bboxes(proposals, picked, YOLOX_NMS_THRESH);

  int count = picked.size();

  objects.resize(count);
  for (int i = 0; i < count; i++) {
    objects[i] = proposals[picked[i]];

    // adjust offset to original unpadded
    float x0 = (objects[i].rect.x) / scale;
    float y0 = (objects[i].rect.y) / scale;
    float x1 = (objects[i].rect.x + objects[i].rect.width) / scale;
    float y1 = (objects[i].rect.y + objects[i].rect.height) / scale;

    // clip
    x0 = std::max(std::min(x0, (float)(img_w - 1)), 0.f);
    y0 = std::max(std::min(y0, (float)(img_h - 1)), 0.f);
    x1 = std::max(std::min(x1, (float)(img_w - 1)), 0.f);
    y1 = std::max(std::min(y1, (float)(img_h - 1)), 0.f);

    objects[i].rect.x = x0;
    objects[i].rect.y = y0;
    objects[i].rect.width = x1 - x0;
    objects[i].rect.height = y1 - y0;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_SHAPE shape;
  int32_t height;
  int32_t width;
  float qscale;
  CVI_TENSOR *input;
  CVI_TENSOR *output;

  if (argc != 4) {
    usage(argv);
    exit(-1);
  }

  const char *model_file = argv[1];
  cv::Mat image = cv::imread(argv[2]);
  if (!image.data) {
    printf("Could not open or find the image:%s\n", argv[2]);
    return -1;
  }
  cv::Mat cloned = image.clone();

  int ret = CVI_NN_RegisterModel(model_file, &model);
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
  output = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, output_tensors, output_num);
  assert(output);

  qscale = CVI_NN_TensorQuantScale(input);
  shape = CVI_NN_TensorShape(input);
  height = shape.dim[2];
  width = shape.dim[3];

  // do preprocess
  // resize & letterbox
  int ih = image.rows;
  int iw = image.cols;
  int oh = height;
  int ow = width;
  double resize_scale = std::min((double)oh / ih, (double)ow / iw);
  int nh = (int)(ih * resize_scale);
  int nw = (int)(iw * resize_scale);
  cv::resize(image, image, cv::Size(nw, nh));
  int top = 0;
  int bottom = oh - nh;
  int left = 0;
  int right = ow - nw;
  cv::copyMakeBorder(image, image, top, bottom, left, right, cv::BORDER_CONSTANT,
                     cv::Scalar::all(0));
  // split
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(height, width, CV_8SC1);
  }
  cv::split(image, channels);
  // normalize
  float scale = qscale;
  float mean = 0.0f;
  for (int i = 0; i < 3; i++) {
    channels[i].convertTo(channels[i], CV_8SC1, scale, mean);
  }
  // fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  memcpy(ptr, channels[0].data, channel_size);
  memcpy(ptr + channel_size, channels[1].data, channel_size);
  memcpy(ptr + 2 * channel_size, channels[2].data, channel_size);

  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);

  std::vector<Object> objects;

  // post process
  ret = post_process(output, iw, ih, width, height, resize_scale, objects);

  // draw bbox on image
  for (size_t i = 0; i < objects.size(); i++) {
    const Object &obj = objects[i];

    fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
            obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

    cv::rectangle(cloned, obj.rect, cv::Scalar(255, 0, 0));

    char text[256];
    sprintf(text, "%s %.1f%%", coco_names[obj.label], obj.prob * 100);

    int baseLine = 0;
    cv::Size label_size =
        cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int x = obj.rect.x;
    int y = obj.rect.y - label_size.height - baseLine;
    if (y < 0)
      y = 0;
    if (x + label_size.width > image.cols)
      x = image.cols - label_size.width;

    cv::rectangle(
        cloned,
        cv::Rect(cv::Point(x, y),
                 cv::Size(label_size.width, label_size.height + baseLine)),
        cv::Scalar(255, 255, 255), -1);

    cv::putText(cloned, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
  }

  // save or show picture
  cv::imwrite(argv[3], cloned);

  printf("------\n");
  printf("%zu objects are detected\n", objects.size());
  printf("------\n");

  CVI_NN_CleanupModel(model);
  printf("CVI_NN_CleanupModel succeeded\n");
  return 0;
}