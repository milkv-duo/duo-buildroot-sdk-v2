#include "clip_image.hpp"
#include <core/core/cvtdl_errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <error_msg.hpp>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

namespace cvitdl {

Clip_Image::Clip_Image() : Core(CVI_MEM_DEVICE) {
  setUseMmap(false);
  // m_preprocess_param[0].factor[0] = 0.0145984266;
  // m_preprocess_param[0].factor[1] = 0.0150077685;
  // m_preprocess_param[0].factor[2] = 0.0142200657;
  // m_preprocess_param[0].mean[0] = 1.7922625;
  // m_preprocess_param[0].mean[1] = 1.7465649;
  // m_preprocess_param[0].mean[2] = 1.4802198;

  //use fused_precess w4f16 model
  m_preprocess_param[0].factor[0] = 1;
  m_preprocess_param[0].factor[1] = 1;
  m_preprocess_param[0].factor[2] = 1;
  m_preprocess_param[0].mean[0] = 0;
  m_preprocess_param[0].mean[1] = 0;
  m_preprocess_param[0].mean[2] = 0;
  m_preprocess_param[0].use_crop = true;
  m_preprocess_param[0].keep_aspect_ratio = true;
}

Clip_Image::~Clip_Image() {}

int Clip_Image::inference(VIDEO_FRAME_INFO_S* frame, cvtdl_clip_feature* clip_feature) {
  int height = frame->stVFrame.u32Height;
  int width = frame->stVFrame.u32Width;
  m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
  int left_up = 0;
  if (width>height){
    left_up = (width - height) / 2;
    m_vpss_config[0].crop_attr.stCropRect = {left_up, 0, height, height};
  }else{
    left_up = (height-width) / 2;
    m_vpss_config[0].crop_attr.stCropRect = {0, left_up, width, width};
  }
  std::vector<VIDEO_FRAME_INFO_S*> frames = {frame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    std::cout << "cvi_tdl clip inference run is fail!\n";
    return ret;
  }
  float* out_feature = getOutputRawPtr<float>(0);
  CVI_SHAPE output_shape = getOutputShape(0);
  clip_feature->feature_dim = output_shape.dim[1];
  clip_feature->out_feature = (float*)malloc(clip_feature->feature_dim * sizeof(float));
  memcpy(clip_feature->out_feature, out_feature, clip_feature->feature_dim * sizeof(float));
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}
}  // namespace cvitdl
