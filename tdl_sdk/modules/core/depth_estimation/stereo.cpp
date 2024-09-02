#include "stereo.hpp"
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

Stereo::Stereo() : Core(CVI_MEM_DEVICE) {
  setUseMmap(false);
  m_preprocess_param[0].factor[0] = 0.017125;
  m_preprocess_param[0].factor[1] = 0.017507;
  m_preprocess_param[0].factor[2] = 0.017429;
  m_preprocess_param[0].mean[0] = 2.1179;
  m_preprocess_param[0].mean[1] = 2.03571;
  m_preprocess_param[0].mean[2] = 1.8044;
  m_preprocess_param[0].keep_aspect_ratio = true;
  m_preprocess_param[0].rescale_type = RESCALE_CENTER;
  m_preprocess_param.emplace_back(m_preprocess_param[0]);
  std::cout << "setupInputPreprocess done" << std::endl;
}

Stereo::~Stereo() {}

int Stereo::inference(VIDEO_FRAME_INFO_S *frame1, VIDEO_FRAME_INFO_S *frame2,
                      cvtdl_depth_logits_t *depth_logist) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame1, frame2};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    std::cout << "cvi_tdl clip inference run is fail!\n";
    return ret;
  }

  const TensorInfo &oinfo = getOutputTensorInfo(0);
  int byte_per_pixel = oinfo.tensor_size / oinfo.tensor_elem;
  float qscale_output = byte_per_pixel == 1 ? oinfo.qscale : 1;
  depth_logist->w = oinfo.shape.dim[2];
  depth_logist->h = oinfo.shape.dim[1];
  int pix_size = depth_logist->w * depth_logist->h;
  if (depth_logist->int_logits != NULL) {
    delete[] depth_logist->int_logits;
    depth_logist->int_logits = NULL;
  }
  depth_logist->int_logits = new int8_t[pix_size];
  if (byte_per_pixel == 1) {
    int8_t *int8_out_data = getOutputRawPtr<int8_t>(0);
    for (int i = 0; i < pix_size; i++) {
      depth_logist->int_logits[i] = static_cast<int8_t>(int8_out_data[i] * qscale_output);
    }
  } else {
    float *float_out_data = getOutputRawPtr<float>(0);
    for (int i = 0; i < pix_size; i++) {
      depth_logist->int_logits[i] = static_cast<int8_t>(float_out_data[i] * qscale_output);
    }
  }
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}
}  // namespace cvitdl
