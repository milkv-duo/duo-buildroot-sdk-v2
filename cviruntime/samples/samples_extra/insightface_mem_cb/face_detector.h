#ifndef FACE_DETECTOR_H
#define FACE_DETECTOR_H

#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"

class FaceDetector {
public:
  FaceDetector(const char *model_file, bool nhwc = false);
  ~FaceDetector();

  void doPreProccess(cv::Mat &image);
  void doPreProccess_ResizeOnly(cv::Mat &image);
  void doInference();
  cv::Mat doPostProccess(cv::Mat &image);

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
  float scale_w;
  float scale_h;
  float qscale;
};

#endif
