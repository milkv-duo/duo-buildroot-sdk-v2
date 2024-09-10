#include "tpu/tpu_threshold.hpp"
#include "utils.hpp"

#include <string.h>

void IveTPUThresholdSlope::setThreshold(int low, int high) {
  m_threshold_low = low;
  m_threshold_high = high;
}

int IveTPUThresholdSlope::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "threshSlope";
  m_slice_info.nums_of_tl = 1;

  return CVI_SUCCESS;
}

int IveTPUThresholdSlope::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
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

void IveTPUThresholdSlope::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                     uint32_t ping_idx) {
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max);
  cvk_ctx->ops->tiu_min(cvk_ctx, &m_p_min);
}