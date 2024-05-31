#ifndef YOLO_V3_DETECTOR_H
#define YOLO_V3_DETECTOR_H

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"

#define MAX_DET 200

typedef struct {
  float x, y, w, h;
} box;

typedef struct {
  box bbox;
  int cls;
  float score;
} detection;

class YoloV3Detector {
public:
  YoloV3Detector(const char *model_file);
  ~YoloV3Detector();

  void doPreProccess(cv::Mat &image);
  void doInference();
  int32_t doPostProccess(int32_t image_h, int32_t image_w, detection det[],
                        int32_t max_det_num);

private:
  void correctYoloBoxes(detection *dets, int det_num, int image_h, int image_w,
                        bool relative_position);
public:
  CVI_TENSOR *input;
  CVI_TENSOR *output;
  cv::Mat channels[3];

private:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_SHAPE shape;
  float qscale;
};

#endif
