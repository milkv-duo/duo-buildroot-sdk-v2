#include "tpu/tpu_sub.hpp"
#include "utils.hpp"

#include <string.h>

int IveTPUSubAbs::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "subAbs";
  // 2 - in
  // 1 -tmp
  // 1 -high bit
  m_slice_info.ping_pong_size = 2;
  m_slice_info.ping_pong_share_tl = 1;
  m_slice_info.nums_of_tl = 4;
  return CVI_SUCCESS;
}

int IveTPUSubAbs::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                           const std::vector<cvk_tg_shape_t> &tg_in_slices,
                           const std::vector<cvk_tg_shape_t> &tg_out_slices,
                           std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                           const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  m_min.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_min.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }
  auto *tl_high_bit = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  constantFillTL(rt_handle, cvk_ctx, 0, tl_high_bit);

  m_p_min.b_is_const = 0;

  m_p_max.b_is_const = 0;

  m_p_mac.res_high = tl_high_bit;
  m_p_mac.b_is_const = 1;
  m_p_mac.b_const.val = -1;
  m_p_mac.b_const.is_signed = 1;
  m_p_mac.lshift_bits = 0;
  m_p_mac.res_is_int8 = 1;
  m_p_mac.rshift_bits = 0;
  m_p_mac.relu_enable = 1;

  if (m_is_binary_output) {
    m_p_mul.b_const.val = 255;
    m_p_mul.b_is_const = 1;
    m_p_mul.b_const.is_signed = 0;
    m_p_mul.relu_enable = 0;
    m_p_mul.res_high = NULL;
    m_p_mul.rshift_bits = 0;
  }

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 3);
    tl_in_idx->push_back(1 + pp * 3);
    tl_out_idx->push_back(0 + pp * 3);
  }
  return CVI_SUCCESS;
}

void IveTPUSubAbs::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  m_p_min.a = m_input1[ping_idx];
  m_p_min.b = m_input2[ping_idx];
  m_p_min.min = m_min[ping_idx];
  m_p_max.a = m_input1[ping_idx];
  m_p_max.b = m_input2[ping_idx];
  m_p_max.max = m_input1[ping_idx];
  m_p_mac.res_low = m_input1[ping_idx];
  m_p_mac.a = m_min[ping_idx];
  cvk_ctx->ops->tiu_min(cvk_ctx, &m_p_min);
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max);
  cvk_ctx->ops->tiu_mac(cvk_ctx, &m_p_mac);

  if (m_clip_128) {
    cvk_tiu_min_param_t pmin;
    pmin.min = m_input1[ping_idx];
    pmin.a = m_input1[ping_idx];
    pmin.b_is_const = 1;
    pmin.b_const.val = 128;
    pmin.b_const.is_signed = 0;
    cvk_ctx->ops->tiu_min(cvk_ctx, &pmin);
  }
  if (m_is_binary_output) {
    m_p_mul.a = m_input1[ping_idx];
    m_p_mul.res_low = m_input1[ping_idx];
    cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
  }
}
