#include "face_recognizer.h"


FaceRecognizer::FaceRecognizer(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }

  // get input output tensors
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num,
                               &output_tensors, &output_num);
  input = &input_tensors[0];
  output = &output_tensors[0];

  shape = CVI_NN_TensorShape(input);
  height = shape.dim[2];
  width = shape.dim[3];
  
}

FaceRecognizer::~FaceRecognizer() {
  if (model) {
    CVI_NN_CleanupModel(model);
  }
}

void FaceRecognizer::doPreProccess_ResizeOnly(cv::Mat &image, cv::Mat &det) {
  cv::Mat aligned_face = image.clone();

  float ref_pts[5][2] = {
    { 30.2946f, 51.6963f },
    { 65.5318f, 51.5014f },
    { 48.0252f, 71.7366f },
    { 33.5493f, 92.3655f },
    { 62.7299f, 92.2041f }
  };

  cv::Mat ref(5, 2, CV_32FC1, ref_pts);

  float dst_pts[5][2] = {
    det.at<float>(0, 5), det.at<float>(0, 6),
    det.at<float>(0, 7), det.at<float>(0, 8),
    det.at<float>(0, 9), det.at<float>(0, 10),
    det.at<float>(0, 11), det.at<float>(0, 12),
    det.at<float>(0, 13), det.at<float>(0, 14)
  };

  cv::Mat dst(5, 2, CV_32FC1, dst_pts);
  auto m = similarTransform(dst, ref);
  cv::warpPerspective(image, aligned_face, m, cv::Size(96, 112), cv::INTER_LINEAR);
  cv::resize(aligned_face, aligned_face, cv::Size(112, 112), 0, 0, cv::INTER_LINEAR);
  cv::cvtColor(aligned_face, aligned_face, cv::COLOR_BGR2RGB);

  //Packed2Planar
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(aligned_face.rows, aligned_face.cols, CV_8SC1);
  }
  cv::split(aligned_face, channels);

  // fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size);
  }
}

void FaceRecognizer::doInference() {
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
}

cv::Mat FaceRecognizer::doPostProccess() {
  cv::Mat feature(512, 1, CV_32FC1);
  memcpy(feature.data, CVI_NN_TensorPtr(output), CVI_NN_TensorSize(output));
  return feature;
}