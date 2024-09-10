#include "hand_keypoint.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "cvi_sys.h"
#include "rescale_utils.hpp"

#define R_SCALE (0.003922 / 0.229)
#define G_SCALE (0.003922 / 0.224)
#define B_SCALE (0.003922 / 0.225)
#define R_MEAN (0.485 / 0.229)
#define G_MEAN (0.456 / 0.224)
#define B_MEAN (0.406 / 0.225)

namespace cvitdl {

HandKeypoint::HandKeypoint() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
  m_preprocess_param[0].use_crop = true;
  m_preprocess_param[0].keep_aspect_ratio = false;  // do not keep aspect ratio,resize directly
}

HandKeypoint::~HandKeypoint() {}

int HandKeypoint::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_handpose21_meta_ts *meta) {
  uint32_t img_width = stOutFrame->stVFrame.u32Width;
  uint32_t img_height = stOutFrame->stVFrame.u32Height;
  for (uint32_t i = 0; i < meta->size; i++) {
    int box_x1 = meta->info[i].bbox_x;
    int box_y1 = meta->info[i].bbox_y;
    uint32_t box_w = meta->info[i].bbox_w;
    uint32_t box_h = meta->info[i].bbox_h;

    // expand box with 1.25 scale
    box_x1 = box_x1 - box_w * 0.125;
    box_y1 = box_y1 - box_h * 0.125;
    box_w = box_w * 1.25;
    box_h = box_h * 1.25;

    if (box_x1 < 0) box_x1 = 0;
    if (box_y1 < 0) box_y1 = 0;
    if (box_x1 + box_w > img_width) {
      box_w = img_width - box_x1;
    }
    if (box_y1 + box_h > img_height) {
      box_h = img_height - box_y1;
    }

    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {box_x1, box_y1, box_w, box_h};
    m_vpss_config[0].crop_attr.bEnable = true;

    std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};

    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      LOGW("hand keypoint inference failed\n");
      return ret;
    }

    TensorInfo oinfo = getOutputTensorInfo(0);
    float *out_data = getOutputRawPtr<float>(oinfo.tensor_name);
    for (int k = 0; k < 42; k++) {
      if (k % 2 == 0) {
        meta->info[i].xn[k / 2] = out_data[k];
        meta->info[i].x[k / 2] = out_data[k] * box_w + box_x1;
      } else {
        meta->info[i].yn[k / 2] = out_data[k];
        meta->info[i].y[k / 2] = out_data[k] * box_h + box_y1;
      }
    }
  }
  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl
