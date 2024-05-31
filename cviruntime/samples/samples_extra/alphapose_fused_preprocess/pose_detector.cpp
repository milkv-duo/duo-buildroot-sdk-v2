#include <vector>
#include "pose_detector.h"
#include "pose_utils.h"

PoseDetector::PoseDetector(const char *model_file) {
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
  output = &output_tensors[0];
  assert(output);

  shape = CVI_NN_TensorShape(input);
  height = shape.dim[2];
  width = shape.dim[3];

}

PoseDetector::~PoseDetector() {
  if (model) {
    CVI_NN_CleanupModel(model);
  }
}

void PoseDetector::doPreProccess_ResizeOnly(cv::Mat &image, detection &det,
                                            std::vector<bbox_t> &align_bbox_list) {
  int pose_h = height;
  int pose_w = width;
  box b = det.bbox;
  bbox_t b_pose;
  b_pose.x1 = (b.x - b.w / 2);
  b_pose.y1 = (b.y - b.h / 2);
  b_pose.x2 = (b.x + b.w / 2);
  b_pose.y2 = (b.y + b.h / 2);

  float aspect_ratio = (float)pose_w / pose_h;

  float x = b_pose.x1;
  float y = b_pose.y1;
  float w = b_pose.x2 - b_pose.x1;
  float h = b_pose.y2 - b_pose.y1;

  std::vector<float> center(2, 0);
  std::vector<float> scale(2, 0);
  box_to_center_scale(x, y, w, h, aspect_ratio, scale, center);

  cv::Mat trans = get_affine_transform(center, scale, {(float)pose_h, (float)pose_w});
  cv::Mat align_image;
  cv::warpAffine(image, align_image, trans, cv::Size(int(pose_w), int(pose_h)),
                 cv::INTER_LINEAR);
  align_bbox_list.push_back(center_scale_to_box(center, scale));

  cv::cvtColor(align_image, align_image, cv::COLOR_BGR2RGB);

  //Packed2Planar
  cv::Mat channels[3];
  for (int i = 0; i < 3; i++) {
    channels[i] = cv::Mat(align_image.rows, align_image.cols, CV_8SC1);
  }
  cv::split(align_image, channels);

  // fill data
  int8_t *ptr = (int8_t *)CVI_NN_TensorPtr(input);
  int channel_size = height * width;
  for (int i = 0; i < 3; ++i) {
    memcpy(ptr + i * channel_size, channels[i].data, channel_size);
  }
}

void PoseDetector::doInference() {
  CVI_NN_Forward(model, input_tensors, input_num, output_tensors, output_num);
}
