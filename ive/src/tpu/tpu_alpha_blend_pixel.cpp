#include "tpu/tpu_alpha_blend_pixel.hpp"
#include <string.h>

int IveTPUBlendPixel::init(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "blend_pixel";
  m_slice_info.ping_pong_size = 1;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 7;

  return CVI_SUCCESS;
}

int IveTPUBlendPixel::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                               const std::vector<cvk_tg_shape_t>& tg_in_slices,
                               const std::vector<cvk_tg_shape_t>& tg_out_slices,
                               std::vector<uint32_t>* tl_in_idx, std::vector<uint32_t>* tl_out_idx,
                               const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  m_alpha.clear();
  m_buf_low.clear();
  m_buf_high.clear();
  m_alpha2.clear();
  m_hight_zeros.clear();

  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, m_input_fmts[0], 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, m_input_fmts[1], 1));
    m_alpha.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }

  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_buf_low.emplace_back(allocTLMem(cvk_ctx, tl_shape, m_output_fmts[0], 1));
    m_buf_high.emplace_back(allocTLMem(cvk_ctx, tl_shape, m_output_fmts[0], 1));
    auto mask_255 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
    constantFillTL(rt_handle, cvk_ctx, 255, mask_255);
    m_alpha2.emplace_back(mask_255);

    auto mask_buf = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
    constantFillTL(rt_handle, cvk_ctx, 0, mask_buf);
    m_hight_zeros.emplace_back(mask_buf);
  }

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 3);
    tl_in_idx->push_back(1 + pp * 3);
    tl_in_idx->push_back(2 + pp * 3);
    tl_out_idx->push_back(3 + pp * 3);
  }
  return CVI_SUCCESS;
}

void IveTPUBlendPixel::operation(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                                 uint32_t ping_idx) {
  // buf = m_input1 * w1
  cvk_tiu_mul_param_t p;
  p.res_high = m_buf_high[ping_idx];
  p.res_low = m_buf_low[ping_idx];
  p.a = m_input1[ping_idx];
  p.b_is_const = 0;
  p.b = m_alpha[ping_idx];
  p.rshift_bits = 0;
  p.relu_enable = 0;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &p);

  constantFillTL(rt_handle, cvk_ctx, 255, m_alpha2[ping_idx]);
  // m_alpha2 = 255 + -1 * m_alpha
  cvk_tiu_mac_param_t sub_alpha;
  sub_alpha.res_high = m_hight_zeros[ping_idx];
  sub_alpha.res_low = m_alpha2[ping_idx];
  sub_alpha.a = m_alpha[ping_idx];
  sub_alpha.b_is_const = 1;
  sub_alpha.b_const.val = -1;
  sub_alpha.b_const.is_signed = 1;
  sub_alpha.lshift_bits = 0;
  sub_alpha.res_is_int8 = 1;
  sub_alpha.rshift_bits = 0;
  sub_alpha.relu_enable = 1;
  cvk_ctx->ops->tiu_mac(cvk_ctx, &sub_alpha);

  // // buf = buf + alpha2 * m_input2, i16 output, it should be >= 0
  cvk_tiu_mac_param_t p2;
  p2.res_high = m_buf_high[ping_idx];
  p2.res_low = m_buf_low[ping_idx];
  p2.res_is_int8 = 1;  // keep origin
  p2.a = m_input2[ping_idx];
  p2.b_is_const = 0;
  p2.b = m_alpha2[ping_idx];
  p2.lshift_bits = 0;           // lshift_bits;
  p2.rshift_bits = m_ishift_r;  // rshift_bits;
  p2.relu_enable = 0;
  cvk_ctx->ops->tiu_mac(cvk_ctx, &p2);
}
