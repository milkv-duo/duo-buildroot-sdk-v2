#include "yolo_v3_detector.h"

YoloV3Detector::YoloV3Detector(const char *model_file) {
  int ret = CVI_NN_RegisterModel(model_file, &model);
  if (ret != CVI_RC_SUCCESS) {
    printf("CVI_NN_RegisterModel failed, err %d\n", ret);
    exit(1);
  }

  // get input output tensors
  CVI_NN_GetInputOutputTensors(model, &input_tensors, &input_num, &output_tensors,
                               &output_num);

  input = CVI_NN_GetTensorByName(CVI_NN_DEFAULT_TENSOR, input_tensors, input_num);
  assert(input);
  output = CVI_NN_GetTensorByName("output", output_tensors, output_num);
  assert(output);

  qscale = CVI_NN_TensorQuantScale(input);
  shape = CVI_NN_TensorShape(input);

  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(shape.dim[2], shape.dim[3], CV_8SC1);
  }
}

YoloV3Detector::~YoloV3Detector() {
  if (model) {
    CVI_NN_CleanupModel(model);
  }
}

void YoloV3Detector::doPreProccess(cv::Mat &image) {
  cv::Mat resized_image;
  // resize & letterbox
  int ih = image.rows;
  int iw = image.cols;
  int oh = shape.dim[2];
  int ow = shape.dim[3];
  double resize_scale = std::min((double)oh / ih, (double)ow / iw);
  int nh = (int)(ih * resize_scale);
  int nw = (int)(iw * resize_scale);
  cv::resize(image, resized_image, cv::Size(nw, nh));
  int top = (oh - nh) / 2;
  int bottom = (oh - nh) - top;
  int left = (ow - nw) / 2;
  int right = (ow - nw) - left;
  cv::copyMakeBorder(resized_image, resized_image, top, bottom, left, right,
                     cv::BORDER_CONSTANT, cv::Scalar::all(0));

  cv::cvtColor(resized_image, resized_image, cv::COLOR_BGR2RGB);
  //Packed2Planar
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(resized_image.rows, resized_image.cols, CV_8SC1);
  }
  cv::split(resized_image, channels);

  // fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = oh * ow;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size);
  }

}

void YoloV3Detector::doInference() {
  // run inference
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
}

int32_t YoloV3Detector::doPostProccess(int32_t image_h, int32_t image_w, detection dets[],
                                      int32_t max_det_num) {
  int32_t det_num = 0;
  float *output_ptr = (float *)CVI_NN_TensorPtr(output);
  for (int i = 0; i < max_det_num; ++i) {
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
  correctYoloBoxes(dets, det_num, image_h, image_w, false);

  return det_num;
}

void YoloV3Detector::correctYoloBoxes(detection *dets, int det_num, int image_h,
                                      int image_w, bool relative_position) {
  int i;
  int restored_w = 0;
  int restored_h = 0;
  if (((float)shape.dim[3] / image_w) < ((float)shape.dim[2] / image_h)) {
    restored_w = shape.dim[3];
    restored_h = (image_h * shape.dim[3]) / image_w;
  } else {
    restored_h = shape.dim[2];
    restored_w = (image_w * shape.dim[2]) / image_h;
  }
  for (i = 0; i < det_num; ++i) {
    box b = dets[i].bbox;
    b.x = (b.x - (shape.dim[3] - restored_w) / 2. / shape.dim[3]) /
          ((float)restored_w / shape.dim[3]);
    b.y = (b.y - (shape.dim[2] - restored_h) / 2. / shape.dim[2]) /
          ((float)restored_h / shape.dim[2]);
    b.w *= (float)shape.dim[3] / restored_w;
    b.h *= (float)shape.dim[2] / restored_h;
    if (!relative_position) {
      b.x *= image_w;
      b.w *= image_w;
      b.y *= image_h;
      b.h *= image_h;
    }
    dets[i].bbox = b;
  }
}
