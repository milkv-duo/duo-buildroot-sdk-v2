#include "face_landmarker_det2.hpp"
#include <core/core/cvtdl_errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <error_msg.hpp>
#include <iostream>
#include <iterator>
#include <string>
#include "coco_utils.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

namespace cvitdl {

FaceLandmarkerDet2::FaceLandmarkerDet2() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = 1 / 127.5;
  m_preprocess_param[0].factor[1] = 1 / 127.5;
  m_preprocess_param[0].factor[2] = 1 / 127.5;
  m_preprocess_param[0].mean[0] = 1.0;
  m_preprocess_param[0].mean[1] = 1.0;
  m_preprocess_param[0].mean[2] = 1.0;
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].rescale_type = RESCALE_NOASPECT;
  m_preprocess_param[0].keep_aspect_ratio = false;
}

int FaceLandmarkerDet2::onModelOpened() {
  for (size_t j = 0; j < getNumOutputTensor(); j++) {
    TensorInfo oinfo = getOutputTensorInfo(j);
    CVI_SHAPE output_shape = oinfo.shape;
    if (output_shape.dim[1] <= 2) {
      out_names_["score"] = oinfo.tensor_name;
    } else if (out_names_.count("point_x") == 0) {
      out_names_["point_x"] = oinfo.tensor_name;
    } else {
      out_names_["point_y"] = oinfo.tensor_name;
    }
  }
  if (out_names_.count("score") == 0 || out_names_.count("point_x") == 0 ||
      out_names_.count("point_y") == 0) {
    return CVI_TDL_FAILURE;
  }
  return CVI_TDL_SUCCESS;
}

FaceLandmarkerDet2::~FaceLandmarkerDet2() {}

int FaceLandmarkerDet2::vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                                       VPSSConfig &vpss_config) {
  auto &vpssChnAttr = vpss_config.chn_attr;
  auto &factor = vpssChnAttr.stNormalize.factor;
  auto &mean = vpssChnAttr.stNormalize.mean;
  vpssChnAttr.stNormalize.bEnable = false;
  vpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;

  VPSS_CHN_SQ_RB_HELPER(&vpssChnAttr, srcFrame->stVFrame.u32Width, srcFrame->stVFrame.u32Height,
                        vpssChnAttr.u32Width, vpssChnAttr.u32Height, PIXEL_FORMAT_RGB_888_PLANAR,
                        factor, mean, false);
  int ret = mp_vpss_inst->sendFrame(srcFrame, &vpssChnAttr, &vpss_config.chn_coeff, 1);
  if (ret != CVI_SUCCESS) {
    LOGE("vpssPreprocess Send frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_SEND_FRAME;
  }

  ret = mp_vpss_inst->getFrame(dstFrame, 0, m_vpss_timeout);
  if (ret != CVI_SUCCESS) {
    LOGE("get frame failed: %s!\n", get_vpss_error_msg(ret));
    return CVI_TDL_ERR_VPSS_GET_FRAME;
  }
  return CVI_TDL_SUCCESS;
}

int FaceLandmarkerDet2::inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *facemeta) {
  std::vector<VIDEO_FRAME_INFO_S *> frames = {srcFrame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("FaceLandmarkerDet2 run inference failed\n");
    return ret;
  }

  CVI_SHAPE shape = getInputShape(0);

  outputParser(shape.dim[3], shape.dim[2], srcFrame->stVFrame.u32Width,
               srcFrame->stVFrame.u32Height, facemeta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

void FaceLandmarkerDet2::outputParser(const int image_width, const int image_height,
                                      const int frame_width, const int frame_height,
                                      cvtdl_face_t *facemeta) {
  TensorInfo oinfo_x = getOutputTensorInfo(out_names_["point_x"]);
  float *output_point_x = getOutputRawPtr<float>(oinfo_x.tensor_name);

  TensorInfo oinfo_y = getOutputTensorInfo(out_names_["point_y"]);
  float *output_point_y = getOutputRawPtr<float>(oinfo_y.tensor_name);

  TensorInfo oinfo_cls = getOutputTensorInfo(out_names_["score"]);
  float *output_score = getOutputRawPtr<float>(oinfo_cls.tensor_name);

  CVI_TDL_MemAllocInit(1, 5, facemeta);
  facemeta->width = frame_width;
  facemeta->height = frame_height;

  facemeta->info[0].pts.score = 1.0 / (1.0 + exp(-output_score[0]));
  // blur model
  if (oinfo_cls.shape.dim[1] == 2) {
    facemeta->info[0].blurness = 1.0 / (1.0 + exp(-output_score[1]));
  }

  for (int i = 0; i < 5; i++) {
    float x = output_point_x[i] * frame_width;
    float y = output_point_y[i] * frame_height;
    facemeta->info[0].pts.x[i] = x;
    facemeta->info[0].pts.y[i] = y;
  }
}
// namespace cvitdl
}  // namespace cvitdl
