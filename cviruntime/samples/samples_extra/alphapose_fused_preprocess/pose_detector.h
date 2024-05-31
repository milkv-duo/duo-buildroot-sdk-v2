#ifndef POSE_DETECTOR_H
#define POSE_DETECTOR_H

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"
#include "yolo_v3_detector.h"
#include "pose_utils.h"

class PoseDetector {
public:
  PoseDetector(const char *model_file);
  ~PoseDetector();

  void doPreProccess_ResizeOnly(cv::Mat &image, detection &det,
                                std::vector<bbox_t> &align_bbox_list);
  void doInference();

public:
  CVI_TENSOR *input;
  CVI_TENSOR *output;

private:
  CVI_MODEL_HANDLE model = nullptr;
  CVI_TENSOR *input_tensors;
  CVI_TENSOR *output_tensors;
  int32_t input_num;
  int32_t output_num;
  CVI_SHAPE shape;
  int32_t height;
  int32_t width;
};

#endif
