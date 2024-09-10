#include "ir_liveness.hpp"

#include "core/cvi_tdl_types_mem.h"
#include "rescale_utils.hpp"

#include <cmath>
#include <iostream>
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "cvi_sys.h"

#define R_SCALE (float)(1 / 255.0)
#define G_SCALE (float)(1 / 255.0)
#define B_SCALE (float)(1 / 255.0)
#define R_MEAN 0
#define G_MEAN 0
#define B_MEAN 0

namespace cvitdl {

IrLiveness::IrLiveness() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
  m_preprocess_param[0].rescale_type = RESCALE_NOASPECT;
  m_preprocess_param[0].use_crop = true;
}

IrLiveness::~IrLiveness() {}

int IrLiveness::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta) {
  uint32_t img_width = stOutFrame->stVFrame.u32Width;
  uint32_t img_height = stOutFrame->stVFrame.u32Height;
  for (uint32_t i = 0; i < meta->size; i++) {
    cvtdl_face_info_t face_info = info_rescale_c(img_width, img_height, *meta, i);
    int box_x1 = face_info.bbox.x1;
    int box_y1 = face_info.bbox.y1;
    uint32_t box_w = face_info.bbox.x2 - face_info.bbox.x1;
    uint32_t box_h = face_info.bbox.y2 - face_info.bbox.y1;
    CVI_TDL_FreeCpp(&face_info);

    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {box_x1, box_y1, box_w, box_h};

    std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    float *out_data = getOutputRawPtr<float>(0);

    meta->info[i].liveness_score = out_data[0];
  }

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl