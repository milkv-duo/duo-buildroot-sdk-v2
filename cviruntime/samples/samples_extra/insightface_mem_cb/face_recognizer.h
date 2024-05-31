#ifndef FACE_RECOGNIZER_H
#define FACE_RECOGNIZER_H

#include <iostream>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include "cviruntime.h"


cv::Mat similarTransform(cv::Mat src, cv::Mat dst);

class FaceRecognizer {
public:
  FaceRecognizer(const char *model_file, bool nhwc = false);
  ~FaceRecognizer();

  void doPreProccess(cv::Mat &image, cv::Mat &det);
  void doPreProccess_ResizeOnly(cv::Mat &image, cv::Mat &det);
  void doInference();
  cv::Mat doPostProccess();

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
  float qscale;
};

#endif
