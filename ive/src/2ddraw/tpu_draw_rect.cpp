#include <string.h>
#include "2ddraw/tpu_draw.hpp"

inline void DrawPairLine(cvk_context_t *cvk_ctx, const uint8_t value, const cvk_tg_t &out) {
  cvk_tdma_l2g_tensor_fill_constant_param_t fill_param;
  fill_param.constant = (uint16_t)value;
  if (out.shape.w > 4096 || out.shape.h > 4096) {
    cvk_tg_t tmp_out = out;
    const uint32_t hw_max = 4096;
    for (uint32_t i = 0; i < out.shape.h; i += hw_max) {
      for (uint32_t j = 0; j < out.shape.w; j += hw_max) {
        uint32_t tmp_h = tmp_out.shape.h - hw_max * i;
        uint32_t tmp_w = tmp_out.shape.w - hw_max * j;
        tmp_out.shape.h = tmp_h > 4096 ? 4096 : tmp_h;
        tmp_out.shape.w = tmp_w > 4096 ? 4096 : tmp_w;
        tmp_out.start_address = out.start_address + i * out.stride.h + j * 4096;
        fill_param.dst = &tmp_out;
        cvk_ctx->ops->tdma_l2g_tensor_fill_constant(cvk_ctx, &fill_param);
      }
    }
  } else {
    fill_param.dst = &out;
    cvk_ctx->ops->tdma_l2g_tensor_fill_constant(cvk_ctx, &fill_param);
  }
}

CVI_S32 generateCMD_YUV420SP(cvk_context_t *cvk_ctx, uint16_t x1, uint16_t y1, uint16_t x2,
                             uint16_t y2, uint8_t *color, CviImg &output) {
  if (output.m_tg.shape.w % 2 != 0 && output.m_tg.shape.h % 2 != 0) {
    LOGE("Currently does not support odd width or height for YUV CVI_YUV420SP\n");
    return CVI_FAILURE;
  }
  uint16_t rect_width[2] = {0};
  uint16_t rect_height[2] = {0};
  uint8_t shift[2] = {0};
  shift[1] = 1;
  x1 = (x1 >> 1) << 1;
  y1 = (y1 >> 1) << 1;
  x2 = (x2 >> 1) << 1;
  y2 = (y2 >> 1) << 1;
  rect_width[0] = x2 - x1;
  rect_width[1] = (x2 - x1) >> 1;
  rect_height[0] = y2 - y1;
  rect_height[1] = (y2 - y1) >> 1;

  // Y plane
  cvk_tg_t out;
  memset(&out, 0, sizeof(cvk_tg_t));
  {
    out.fmt = output.m_tg.fmt;
    out.start_address = output.GetPAddr() + output.GetImgCOffsets()[0] +
                        output.GetImgStrides()[0] * (y1 >> shift[0]) + (x1 >> shift[0]);
    out.shape = {1, 2, (uint32_t)(2 >> shift[0]), rect_width[0]};
    out.stride.h = output.GetImgStrides()[0];
    out.stride.c = output.GetImgStrides()[0] * rect_height[0];
    out.stride.n = 2 * out.stride.c;
    DrawPairLine(cvk_ctx, color[0], out);
    out.shape = {1, rect_height[0], 2, (uint32_t)(2 >> shift[0])};
    out.stride.h = rect_width[0];
    out.stride.c = output.GetImgStrides()[0];
    out.stride.n = out.stride.c;
    DrawPairLine(cvk_ctx, color[0], out);
  }

  // v plane
  memset(&out, 0, sizeof(cvk_tg_t));
  {
    out.fmt = output.m_tg.fmt;
    out.start_address = output.GetPAddr() + output.GetImgCOffsets()[1] +
                        output.GetImgStrides()[1] * (y1 >> shift[1]) + (x1);

    out.shape = {1, 2, (uint32_t)(rect_width[0] >> 1), (uint32_t)(2 >> shift[1])};
    out.stride.h = 2;
    out.stride.c = output.GetImgStrides()[1] * rect_height[1];
    out.stride.n = out.stride.c;
    DrawPairLine(cvk_ctx, color[2], out);

    out.shape = {1, rect_height[1], 2, (uint32_t)(2 >> shift[1])};
    out.stride.h = rect_width[0];
    out.stride.c = output.GetImgStrides()[1];
    out.stride.n = out.stride.c;
    DrawPairLine(cvk_ctx, color[2], out);
  }

  // u plane
  memset(&out, 0, sizeof(cvk_tg_t));
  {
    out.fmt = output.m_tg.fmt;
    out.start_address = output.GetPAddr() + output.GetImgCOffsets()[1] +
                        output.GetImgStrides()[1] * (y1 >> shift[1]) + (x1 + 1);

    out.shape = {1, 2, (uint32_t)(rect_width[0] >> 1), (uint32_t)(2 >> shift[1])};
    out.stride.h = 2;
    out.stride.c = output.GetImgStrides()[1] * rect_height[1];
    out.stride.n = out.stride.c;
    DrawPairLine(cvk_ctx, color[1], out);

    out.shape = {1, rect_height[1], 2, (uint32_t)(2 >> shift[1])};
    out.stride.h = rect_width[0];
    out.stride.c = output.GetImgStrides()[1];
    out.stride.n = out.stride.c;
    DrawPairLine(cvk_ctx, color[1], out);
  }
  return CVI_SUCCESS;
}

int IveTPUDrawHollowRect::addCmd(cvk_context_t *cvk_ctx, uint16_t x1, uint16_t y1, uint16_t x2,
                                 uint16_t y2, uint8_t *color, CviImg &output) {
  if (x2 >= output.GetImgWidth()) x2 = output.GetImgWidth() - 1;
  if (y2 >= output.GetImgHeight()) y2 = output.GetImgHeight() - 1;
  if (((int)x2 - x1) < 4 || ((int)y2 - y1) < 4) {
    LOGE("Invalid rect area\n");
    return CVI_FAILURE;
  }
  if (output.IsPlanar()) {
    uint16_t rect_width[3] = {0};
    uint16_t rect_height[3] = {0};
    uint8_t shift[3] = {0};
    if (output.GetImgType() == CVI_RGB_PLANAR) {
      rect_width[0] = rect_width[1] = rect_width[2] = x2 - x1;
      rect_height[0] = rect_height[1] = rect_height[2] = y2 - y1;
    } else if (output.GetImgType() == CVI_YUV420P) {
      if (output.m_tg.shape.w % 2 != 0 && output.m_tg.shape.h % 2 != 0) {
        LOGE("Currently does not support odd width or height for YUV CVI_YUV420P\n");
        return CVI_FAILURE;
      }
      shift[1] = shift[2] = 1;
      x1 = (x1 >> 1) << 1;
      y1 = (y1 >> 1) << 1;
      x2 = (x2 >> 1) << 1;
      y2 = (y2 >> 1) << 1;
      rect_width[0] = x2 - x1;
      rect_width[1] = rect_width[2] = (x2 - x1) >> 1;
      rect_height[0] = y2 - y1;
      rect_height[1] = rect_height[2] = (y2 - y1) >> 1;

    } else {
      LOGE("Unsupported copy type %d.\n", output.GetImgType());
      return CVI_FAILURE;
    }
    cvk_tg_t out;
    memset(&out, 0, sizeof(cvk_tg_t));
    for (uint8_t i = 0; i < 3; i++) {
      out.fmt = output.m_tg.fmt;
      out.start_address = output.GetPAddr() + output.GetImgCOffsets()[i] +
                          output.GetImgStrides()[i] * (y1 >> shift[i]) + (x1 >> shift[i]);
      out.shape = {1, 2, (uint32_t)(2 >> shift[i]), rect_width[i]};
      out.stride.h = output.GetImgStrides()[i];
      out.stride.c = output.GetImgStrides()[i] * rect_height[i];
      out.stride.n = 2 * out.stride.c;
      DrawPairLine(cvk_ctx, color[i], out);
      out.shape = {1, rect_height[i], 2, (uint32_t)(2 >> shift[i])};
      out.stride.h = rect_width[i];
      out.stride.c = output.GetImgStrides()[i];
      out.stride.n = out.stride.c;
      DrawPairLine(cvk_ctx, color[i], out);
    }
  } else {
    if (output.GetImgType() == CVI_YUV420SP) {
      return generateCMD_YUV420SP(cvk_ctx, x1, y1, x2, y2, color, output);
    } else {
      LOGE("Unsupported images type: %d.\n", output.GetImgType());
      return CVI_FAILURE;
    }
  }
  return CVI_SUCCESS;
}