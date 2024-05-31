#include "face_detector.h"

FaceDetector::FaceDetector(const char *model_file, bool nhwc) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }

  // get input output tensors
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors,
                               &output_num);
  input = &input_tensors[0];
  output = &output_tensors[0];

  qscale = CVI_NN_TensorQuantScale(input);
  shape = CVI_NN_TensorShape(input);
  if (nhwc) {
    height = shape.dim[1];
    width = shape.dim[2];
  } else {
    height = shape.dim[2];
    width = shape.dim[3];
  }
  scale_w = scale_h = 1.0;
}

FaceDetector::~FaceDetector() {
  if (model) {
    CVI_NN_CleanupModel(model);
  }
}

void FaceDetector::doPreProccess(cv::Mat &image) {
  cv::Mat resized_image;
  scale_w = 1.0 * width / image.cols;
  scale_h = 1.0 * height / image.rows;
  cv::resize(image, resized_image, cv::Size(), scale_w, scale_h);
  // split
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(height, width, CV_8SC1);
  }
  cv::split(resized_image, channels);
  // normalize
  for (int i = 0; i < 3; i++) {
    channels[i].convertTo(channels[i], CV_8SC1, qscale);
  }
  // BGR -> RGB & fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  memcpy(ptr + 2 * channel_size, channels[0].data, channel_size);
  memcpy(ptr + channel_size, channels[1].data, channel_size);
  memcpy(ptr, channels[2].data, channel_size);
}

void FaceDetector::doPreProccess_ResizeOnly(cv::Mat &image) {
  cv::Mat resized_image;
  scale_w = 1.0 * width / image.cols;
  scale_h = 1.0 * height / image.rows;
  cv::resize(image, resized_image, cv::Size(), scale_w, scale_h);
  memcpy(CVI_NN_TensorPtr(input), resized_image.data, CVI_NN_TensorSize(input));
}

void FaceDetector::doInference() {
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
}

cv::Mat FaceDetector::doPostProccess(cv::Mat &image) {
  int32_t output_h = output_tensors[0].shape.dim[2];
  int32_t output_w = output_tensors[0].shape.dim[3];

  cv::Mat dets(output_h, output_w, CV_32FC1);
  memcpy(dets.data, CVI_NN_TensorPtr(output), CVI_NN_TensorSize(output));

  // multiply scale to origin image size
  for (int i = 0; i < output_h; ++i) {
    dets.at<float>(i, 0) = dets.at<float>(i, 0) / scale_w;
    dets.at<float>(i, 1) = dets.at<float>(i, 1) / scale_h;
    dets.at<float>(i, 2) = dets.at<float>(i, 2) / scale_w;
    dets.at<float>(i, 3) = dets.at<float>(i, 3) / scale_h;

    for (int j = 0; j < 10; j = j + 2) {
      dets.at<float>(i, 5 + j) = dets.at<float>(i, 5 + j) / scale_w;
      dets.at<float>(i, 6 + j) = dets.at<float>(i, 6 + j) / scale_h;
    }
  }
  return dets;
}
