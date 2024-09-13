#include "tpu/tpu_cmp_sat.hpp"
#include <string.h>

int IveTPUCmpSat::init(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "s8_cmp_sat";
  m_slice_info.ping_pong_size = 1;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 6;

  return CVI_SUCCESS;
}

int IveTPUCmpSat::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                           const std::vector<cvk_tg_shape_t>& tg_in_slices,
                           const std::vector<cvk_tg_shape_t>& tg_out_slices,
                           std::vector<uint32_t>* tl_in_idx, std::vector<uint32_t>* tl_out_idx,
                           const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  m_minus1.clear();
  m_minus2.clear();
  m_abs1.clear();
  m_abs2.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_I8, 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_I8, 1));
    m_minus1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_minus2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_abs1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_abs2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 3);
    tl_in_idx->push_back(1 + pp * 3);
    tl_out_idx->push_back(5 + pp * 3);
  }
  return CVI_SUCCESS;
}

void IveTPUCmpSat::operation(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx, uint32_t ping_idx) {
  // clip(-x,0,255)
  cvk_tiu_mul_param_t p1;
  p1.res_high = NULL;
  p1.res_low = m_minus1[ping_idx];
  p1.a = m_input1[ping_idx];
  p1.b_is_const = 1;
  p1.b_const.val = -1;
  p1.b_const.is_signed = 1;
  p1.rshift_bits = 0;
  p1.relu_enable = 1;

  cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);
  // clip(x,0,255)
  p1.res_high = NULL;
  p1.res_low = m_minus2[ping_idx];
  p1.a = m_input1[ping_idx];
  p1.b_is_const = 1;
  p1.b_const.val = 1;  // convert_fp32_s8(-1.0);
  p1.b_const.is_signed = 1;
  p1.rshift_bits = 0;
  p1.relu_enable = 1;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);
  // abs s8_a, get max
  cvk_tiu_max_param_t pmax;
  pmax.max = m_abs1[ping_idx];
  pmax.a = m_minus1[ping_idx];
  pmax.b_is_const = 0;
  pmax.b = m_minus2[ping_idx];
  cvk_ctx->ops->tiu_max(cvk_ctx, &pmax);

  // clip(-x,0,255)
  // cvk_tiu_mul_param_t p1;
  p1.res_high = NULL;
  p1.res_low = m_minus1[ping_idx];
  p1.a = m_input2[ping_idx];
  p1.b_is_const = 1;
  p1.b_const.val = -1;
  p1.b_const.is_signed = 1;
  p1.rshift_bits = 0;
  p1.relu_enable = 1;

  cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);
  // clip(x,0,255)
  p1.res_high = NULL;
  p1.res_low = m_minus2[ping_idx];
  p1.a = m_input2[ping_idx];
  p1.b_is_const = 1;
  p1.b_const.val = 1;  // convert_fp32_s8(-1.0);
  p1.b_const.is_signed = 1;
  p1.rshift_bits = 0;
  p1.relu_enable = 1;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);
  // abs s8_a, get max
  // cvk_tiu_max_param_t pmax;
  pmax.max = m_abs2[ping_idx];
  pmax.a = m_minus1[ping_idx];
  pmax.b_is_const = 0;
  pmax.b = m_minus2[ping_idx];
  cvk_ctx->ops->tiu_max(cvk_ctx, &pmax);

  // get min of abs1,abs2
  cvk_tiu_min_param_t pmin;
  pmin.min = m_minus1[ping_idx];
  pmin.a = m_abs1[ping_idx];
  pmin.b_is_const = 0;
  pmin.b = m_abs2[ping_idx];
  cvk_ctx->ops->tiu_min(cvk_ctx, &pmin);
  // return;

  // abs_sa = (abs_sa - min(abs_a,abs_b))*255
  cvk_tiu_mac_param_t pmac;
  pmac.res_high = m_abs2[ping_idx];
  pmac.res_low = m_abs1[ping_idx];
  pmac.res_is_int8 = 0;
  pmac.a = m_minus1[ping_idx];  // a is min
  pmac.b_is_const = 1;
  pmac.b_const.is_signed = 1;
  pmac.b_const.val = -1;
  pmac.lshift_bits = 0;
  pmac.rshift_bits = 0;
  pmac.relu_enable = 0;
  cvk_ctx->ops->tiu_mac(cvk_ctx, &pmac);
  // return;

  p1.res_high = NULL;
  p1.res_low = m_abs2[ping_idx];
  p1.a = m_abs1[ping_idx];
  p1.b_is_const = 1;
  p1.b_const.val = 255;
  p1.b_const.is_signed = 0;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);
}
