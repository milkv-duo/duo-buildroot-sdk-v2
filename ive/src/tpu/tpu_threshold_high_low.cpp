#include "ive_log.hpp"
#include "tpu/tpu_threshold.hpp"
#include "utils.hpp"

#include <string.h>

void IveTPUThresholdHighLow::setThreshold(int threshold, int low, int high) {
  m_threshold = threshold;
  m_threshold_low = low;
  m_threshold_high = high;
}

int IveTPUThresholdHighLow::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "threshHL";
  m_slice_info.nums_of_tl = 3;

  return CVI_SUCCESS;
}

int IveTPUThresholdHighLow::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                     const std::vector<cvk_tg_shape_t> &tg_in_slices,
                                     const std::vector<cvk_tg_shape_t> &tg_out_slices,
                                     std::vector<uint32_t> *tl_in_idx,
                                     std::vector<uint32_t> *tl_out_idx, const bool enable_cext) {
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  auto *tl_threshold = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  auto *tl_high_bit = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);

  if (m_threshold == -1) {
    LOGE("threshold not set.\n");
  }

  constantFillTL(rt_handle, cvk_ctx, m_threshold - 1, tl_threshold);
  constantFillTL(rt_handle, cvk_ctx, 0, tl_high_bit);

  m_p_mac.res_high = tl_high_bit;
  m_p_mac.res_low = tl_input;
  m_p_mac.a = tl_threshold;
  m_p_mac.b_is_const = 1;
  m_p_mac.b_const.val = -1;
  m_p_mac.b_const.is_signed = 1;
  m_p_mac.lshift_bits = 0;
  m_p_mac.res_is_int8 = 1;
  m_p_mac.rshift_bits = 0;
  m_p_mac.relu_enable = 1;

  m_p_mul.a = tl_input;
  m_p_mul.b_const.val = 255;
  m_p_mul.b_is_const = 1;
  m_p_mul.b_const.is_signed = 0;
  m_p_mul.relu_enable = 0;
  m_p_mul.res_high = NULL;
  m_p_mul.res_low = tl_input;
  m_p_mul.rshift_bits = 0;

  m_p_max.a = tl_input;
  m_p_max.b = NULL;
  m_p_max.max = tl_input;
  m_p_max.b_is_const = 1;
  m_p_max.b_const.is_signed = 0;
  m_p_max.b_const.val = m_threshold_low;

  m_p_min.a = tl_input;
  m_p_min.b = NULL;
  m_p_min.min = tl_input;
  m_p_min.b_is_const = 1;
  m_p_min.b_const.is_signed = 0;
  m_p_min.b_const.val = m_threshold_high;

  tl_in_idx->push_back(0);
  tl_out_idx->push_back(0);
  return CVI_SUCCESS;
}

void IveTPUThresholdHighLow::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                       uint32_t ping_idx) {
  cvk_ctx->ops->tiu_mac(cvk_ctx, &m_p_mac);
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max);
  cvk_ctx->ops->tiu_min(cvk_ctx, &m_p_min);
}