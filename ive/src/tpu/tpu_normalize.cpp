#include "tpu/tpu_normalize.hpp"
#include <string.h>
#include "ive_log.hpp"

void IveTPUNormalize::setMinMax(float min, float max) {
  m_min = min;
  m_max = max;
}

void IveTPUNormalize::setOutputFMT(cvk_fmt_t fmt) { m_fmt = fmt; }

int IveTPUNormalize::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_cmdbuf_subfix = "normalize";
  m_slice_info.ping_pong_size = 2;
  m_slice_info.nums_of_tl = 1 * 2;

  return CVI_SUCCESS;
}

int IveTPUNormalize::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                              const std::vector<cvk_tg_shape_t> &tg_in_slices,
                              const std::vector<cvk_tg_shape_t> &tg_out_slices,
                              std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                              const bool enable_cext) {
  m_input.clear();
  if (m_fmt != CVK_FMT_U8 && m_fmt != CVK_FMT_I8) {
    LOGE("TPUT normalize only supports U8/ I8.\n");
  }
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
  }

  m_p_add.res_high = NULL;
  m_p_add.a_high = NULL;
  m_p_add.b_is_const = 1;
  m_p_add.b_const.val = convert_fp32_bf16(-1.f * m_min);
  m_p_add.rshift_bits = 0;
  m_p_add.relu_enable = 0;

  m_p_mul.res_high = NULL;
  m_p_mul.b_is_const = 1;
  m_p_mul.b_const.val = convert_fp32_bf16(255.f / (float)(m_max - m_min));
  m_p_mul.rshift_bits = 0;
  m_p_mul.relu_enable = 0;

  if (m_fmt == CVK_FMT_I8) {
    m_p_add_offset.res_high = NULL;
    m_p_add_offset.a_high = NULL;
    m_p_add_offset.b_is_const = 1;
    m_p_add_offset.b_const.val = convert_fp32_bf16(-128.f);
    m_p_add_offset.rshift_bits = 0;
    m_p_add_offset.relu_enable = 0;
  }

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp);
    tl_out_idx->push_back(0 + pp);
  }
  return CVI_SUCCESS;
}

void IveTPUNormalize::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                uint32_t ping_idx) {
  m_p_add.res_low = m_input[ping_idx];
  m_p_add.a_low = m_input[ping_idx];
  m_p_mul.res_low = m_input[ping_idx];
  m_p_mul.a = m_input[ping_idx];
  cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add);
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
  if (m_fmt == CVK_FMT_I8) {
    m_p_add_offset.res_low = m_input[ping_idx];
    m_p_add_offset.a_low = m_input[ping_idx];
    cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add_offset);
  }
}