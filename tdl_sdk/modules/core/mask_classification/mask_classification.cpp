#include "mask_classification.hpp"

#include "core/cvi_tdl_types_mem.h"
#include "rescale_utils.hpp"

#include <cmath>
#include "core/core/cvtdl_errno.h"
#include "cvi_sys.h"

#define R_SCALE (1 / (256.0 * 0.229))
#define G_SCALE (1 / (256.0 * 0.224))
#define B_SCALE (1 / (256.0 * 0.225))
#define R_MEAN (0.485 / 0.229)
#define G_MEAN (0.456 / 0.224)
#define B_MEAN (0.406 / 0.225)
#define CROP_PCT 0.875
#define MASK_OUT_NAME "logits_dequant"

namespace cvitdl {

MaskClassification::MaskClassification() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = R_SCALE;
  m_preprocess_param[0].factor[1] = G_SCALE;
  m_preprocess_param[0].factor[2] = B_SCALE;
  m_preprocess_param[0].mean[0] = R_MEAN;
  m_preprocess_param[0].mean[1] = G_MEAN;
  m_preprocess_param[0].mean[2] = B_MEAN;
  m_preprocess_param[0].use_crop = true;
}

MaskClassification::~MaskClassification() {}

int MaskClassification::inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta) {
  uint32_t img_width = stOutFrame->stVFrame.u32Width;
  uint32_t img_height = stOutFrame->stVFrame.u32Height;
  for (uint32_t i = 0; i < meta->size; i++) {
    cvtdl_face_info_t face_info = info_rescale_c(img_width, img_height, *meta, i);
    int box_x1 = face_info.bbox.x1;
    int box_y1 = face_info.bbox.y1;
    uint32_t box_w = face_info.bbox.x2 - face_info.bbox.x1;
    uint32_t box_h = face_info.bbox.y2 - face_info.bbox.y1;
    CVI_TDL_FreeCpp(&face_info);
    uint32_t min_edge = std::min(box_w, box_h);
    float new_edge = min_edge * CROP_PCT;
    int box_new_x1 = (box_w - new_edge) / 2.f + box_x1;
    int box_new_y1 = (box_h - new_edge) / 2.f + box_y1;

    m_vpss_config[0].crop_attr.enCropCoordinate = VPSS_CROP_RATIO_COOR;
    m_vpss_config[0].crop_attr.stCropRect = {box_new_x1, box_new_y1, (uint32_t)new_edge,
                                             (uint32_t)new_edge};

    std::vector<VIDEO_FRAME_INFO_S *> frames = {stOutFrame};
    int ret = run(frames);
    if (ret != CVI_TDL_SUCCESS) {
      return ret;
    }

    float *out_data = getOutputRawPtr<float>(MASK_OUT_NAME);

    float max = std::max(out_data[0], out_data[1]);
    float f0 = std::exp(out_data[0] - max);
    float f1 = std::exp(out_data[1] - max);
    float score = f0 / (f0 + f1);

    meta->info[i].mask_score = score;
  }

  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl