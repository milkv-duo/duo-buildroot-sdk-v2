#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"

// #define SAVE_FILE_FOR_DEBUG
// #define DO_IMSHOW

#define MAX_DET 200

typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int cls;
  float score;
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
  CVI_SHAPE shape;
  int32_t height;
  int32_t width;

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
  output = CVI_NN_GetTensorByName("output", output_tensors, output_num);
  assert(output);

  // nchw
  shape = CVI_NN_TensorShape(input);
  height = shape.dim[2];
  width = shape.dim[3];

  // imread
  cv::Mat image;
  image = cv::imread(argv[2]);
  if (!image.data) {
    printf("Could not open or find the image\n");
    return -1;
  }
  cv::Mat cloned = image.clone();

  detection dets[MAX_DET];
  int32_t det_num = 0;

  /* preprocess */
  int ih = image.rows;
  int iw = image.cols;
  int oh = height;
  int ow = width;
  double scale = std::min((double)oh / ih, (double)ow / iw);
  int nh = (int)(ih * scale);
  int nw = (int)(iw * scale);
  // resize & letterbox
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

  /* run inference */
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
  printf("CVI_NN_Forward succeeded\n");

  /* post process */
  float *output_ptr = (float *)CVI_NN_TensorPtr(output);
  for (int i = 0; i < MAX_DET; ++i) {
    // filter real det with score > 0
    if (output_ptr[i * 6 + 5] > 0) {
      // output: [x,y,w,h,cls,score]
      dets[det_num].bbox.x = output_ptr[i * 6 + 0];
      dets[det_num].bbox.y = output_ptr[i * 6 + 1];
      dets[det_num].bbox.w = output_ptr[i * 6 + 2];
      dets[det_num].bbox.h = output_ptr[i * 6 + 3];
      dets[det_num].cls = output_ptr[i * 6 + 4];
      dets[det_num].score = output_ptr[i * 6 + 5];
      det_num++;
    }
  }
  printf("get detection num: %d\n", det_num);

  // correct box with origin image size
  int restored_w = 0;
  int restored_h = 0;
  bool relative_position = false;
  if (((float)width / cloned.cols) < ((float)height / cloned.rows)) {
    restored_w = width;
    restored_h = (cloned.rows * width) / cloned.cols;
  } else {
    restored_h = height;
    restored_w = (cloned.cols * height) / cloned.rows;
  }
  for (int i = 0; i < det_num; ++i) {
    box b = dets[i].bbox;
    b.x = (b.x - (width - restored_w) / 2. / width) /
          ((float)restored_w / width);
    b.y = (b.y - (height - restored_h) / 2. / height) /
          ((float)restored_h / height);
    b.w *= (float)width / restored_w;
    b.h *= (float)height / restored_h;
    if (!relative_position) {
      b.x *= cloned.cols;
      b.w *= cloned.cols;
      b.y *= cloned.rows;
      b.h *= cloned.rows;
    }
    dets[i].bbox = b;
  }

  /* draw bbox on image */
  for (int i = 0; i < det_num; i++) {
    box b = dets[i].bbox;
    // xywh2xyxy
    int x1 = (b.x - b.w / 2);
    int y1 = (b.y - b.h / 2);
    int x2 = (b.x + b.w / 2);
    int y2 = (b.y + b.h / 2);
    cv::rectangle(cloned, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 0),
                  3, 8, 0);
    cv::putText(cloned, coco_names[dets[i].cls], cv::Point(x1, y1),
                cv::FONT_HERSHEY_DUPLEX, 1.0, cv::Scalar(0, 0, 255), 2);
  }

  // save or show picture
  cv::imwrite(argv[3], cloned);

  printf("------\n");
  printf("%d objects are detected\n", det_num);
  printf("------\n");

  CVI_NN_CleanupModel(model);
  printf("CVI_NN_CleanupModel succeeded\n");
  return 0;
}
