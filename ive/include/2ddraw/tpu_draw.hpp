#pragma once
#include "core.hpp"
#include "cvi_draw_ive.h"

inline void getYUVColorLimitedUV(IVE_COLOR_S &color, uint8_t *out) {
  // clang-format off
  out[0] =  (0.257 * color.r) + (0.504 * color.g) + (0.098 * color.b) + 16;  // Y
  out[1] = -(0.148 * color.r) - (0.291 * color.g) + (0.439 * color.b) + 128;  // U
  out[2] =  (0.439 * color.r) - (0.368 * color.g) - (0.071 * color.b) + 128;  // V
  // clang-format on
}

class IveTPUDrawHollowRect {
 public:
  static int addCmd(cvk_context_t *cvk_ctx, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    uint8_t *color, CviImg &output);
};