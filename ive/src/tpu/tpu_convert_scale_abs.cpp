#include "tpu/tpu_convert_scale_abs.hpp"
#include <string.h>

int IveTPUConvertScaleAbs::init(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_cmdbuf_subfix = "convert_scale_abs";
  m_slice_info.ping_pong_size = 1;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 2 * 2;

  return CVI_SUCCESS;
}

int IveTPUConvertScaleAbs::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                                    const std::vector<cvk_tg_shape_t>& tg_in_slices,
                                    const std::vector<cvk_tg_shape_t>& tg_out_slices,
                                    std::vector<uint32_t>* tl_in_idx,
                                    std::vector<uint32_t>* tl_out_idx, const bool enable_cext) {
  m_input.clear();
  m_buf1.clear();

  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    m_input.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
    m_buf1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
    tl_in_idx->push_back(0 + pp * 2);
    tl_out_idx->push_back(0 + pp * 2);
  }
  return CVI_SUCCESS;
}

void IveTPUConvertScaleAbs::operation(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                                      uint32_t ping_idx) {
  bool skip_scale =
      (m_beta == convert_fp32_bf16(-1.0 * 0)) && m_alpha == convert_fp32_bf16(-1.0 * 1.0);

  if (skip_scale) {
    // skip scale and offset, just do ABS.
    // abs it, multiply -1
    cvk_tiu_mul_param_t p1;
    p1.res_high = NULL;
    p1.res_low = m_buf1[ping_idx];
    p1.a = m_input[ping_idx];
    p1.b_is_const = 1;
    p1.b_const.val = convert_fp32_bf16(-1.0);
    p1.b_const.is_signed = 1;
    p1.rshift_bits = 0;
    p1.relu_enable = 0;

    cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);

    // abs it, get max
    cvk_tiu_max_param_t p;
    p.max = m_input[ping_idx];
    p.a = m_input[ping_idx];
    p.b_is_const = 0;
    p.b = m_buf1[ping_idx];

    cvk_ctx->ops->tiu_max(cvk_ctx, &p);
  } else {
    // m_buf = alpha * m_input + beta
    constantFillTL(rt_handle, cvk_ctx, m_beta, m_buf1[ping_idx]);
    cvk_tiu_mac_param_t p_mac;
    p_mac.res_high = NULL;
    p_mac.res_low = m_buf1[ping_idx];
    p_mac.res_is_int8 = 0;
    p_mac.a = m_input[ping_idx];
    p_mac.b_is_const = 1;
    p_mac.b_const.val = m_alpha;
    p_mac.b_const.is_signed = 1;
    p_mac.lshift_bits = 0;
    p_mac.rshift_bits = 0;
    p_mac.relu_enable = 0;
    cvk_ctx->ops->tiu_mac(cvk_ctx, &p_mac);

    // abs it, multiply -1
    cvk_tiu_mul_param_t p1;
    p1.res_high = NULL;
    p1.res_low = m_input[ping_idx];
    p1.a = m_buf1[ping_idx];
    p1.b_is_const = 1;
    p1.b_const.val = convert_fp32_bf16(-1.0);
    p1.b_const.is_signed = 1;
    p1.rshift_bits = 0;
    p1.relu_enable = 0;

    cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);

    // abs it, get max
    cvk_tiu_max_param_t p;
    p.max = m_input[ping_idx];
    p.a = m_input[ping_idx];
    p.b_is_const = 0;
    p.b = m_buf1[ping_idx];

    cvk_ctx->ops->tiu_max(cvk_ctx, &p);
  }
}