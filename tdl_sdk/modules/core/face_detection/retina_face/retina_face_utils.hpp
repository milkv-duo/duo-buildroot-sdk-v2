#pragma once
#include "anchor_generator.h"

#include "core/core/cvtdl_core_types.h"
#include "core_utils.hpp"

#include <vector>

namespace cvitdl {

inline void __attribute__((always_inline))
bbox_pred(const anchor_box &anchor, float *regress, cvtdl_bbox_t &bbox, PROCESS process) {
  float width = anchor.x2 - anchor.x1 + 1;
  float height = anchor.y2 - anchor.y1 + 1;

  if (process == CAFFE) {
    float ctr_x = anchor.x1 + 0.5 * (width - 1.0);
    float ctr_y = anchor.y1 + 0.5 * (height - 1.0);

    float pred_ctr_x = regress[0] * width + ctr_x;
    float pred_ctr_y = regress[1] * height + ctr_y;
    float pred_w = FastExp(regress[2]) * width;
    float pred_h = FastExp(regress[3]) * height;

    bbox.x1 = pred_ctr_x - 0.5 * (pred_w - 1.0);
    bbox.y1 = pred_ctr_y - 0.5 * (pred_h - 1.0);
    bbox.x2 = pred_ctr_x + 0.5 * (pred_w - 1.0);
    bbox.y2 = pred_ctr_y + 0.5 * (pred_h - 1.0);

  } else if (process == PYTORCH) {
    float ctr_x = anchor.x1 + 0.5 * (width);
    float ctr_y = anchor.y1 + 0.5 * (height);

    float pred_ctr_x = regress[0] * 0.1 * width + ctr_x;
    float pred_ctr_y = regress[1] * 0.1 * height + ctr_y;
    float pred_w = FastExp(regress[2] * 0.2) * width;
    float pred_h = FastExp(regress[3] * 0.2) * height;

    bbox.x1 = pred_ctr_x - 0.5 * (pred_w);
    bbox.y1 = pred_ctr_y - 0.5 * (pred_h);
    bbox.x2 = pred_ctr_x + 0.5 * (pred_w);
    bbox.y2 = pred_ctr_y + 0.5 * (pred_h);
  }
}

inline void __attribute__((always_inline))
landmark_pred(const anchor_box &anchor, cvtdl_pts_t &facePt) {
  float width = anchor.x2 - anchor.x1 + 1;
  float height = anchor.y2 - anchor.y1 + 1;
  float ctr_x = anchor.x1 + 0.5 * (width - 1.0);
  float ctr_y = anchor.y1 + 0.5 * (height - 1.0);

  for (size_t j = 0; j < facePt.size; j++) {
    facePt.x[j] = facePt.x[j] * width + ctr_x;
    facePt.y[j] = facePt.y[j] * height + ctr_y;
  }
}

}  // namespace cvitdl
