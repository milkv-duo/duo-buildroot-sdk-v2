#include "tpu/tpu_alpha_blend.hpp"
#include <string.h>

int IveTPUBlend::init(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "blend";
  m_slice_info.ping_pong_size = 1;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 4;

  return CVI_SUCCESS;
}

int IveTPUBlend::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx,
                          const std::vector<cvk_tg_shape_t>& tg_in_slices,
                          const std::vector<cvk_tg_shape_t>& tg_out_slices,
                          std::vector<uint32_t>* tl_in_idx, std::vector<uint32_t>* tl_out_idx,
                          const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  m_buf1.clear();
  m_buf2.clear();

  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }

  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_buf1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_buf2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 2);
    tl_in_idx->push_back(1 + pp * 2);
    tl_out_idx->push_back(0 + pp * 2);
  }
  return CVI_SUCCESS;
}

void IveTPUBlend::operation(CVI_RT_HANDLE rt_handle, cvk_context_t* cvk_ctx, uint32_t ping_idx) {
  // buf = m_input1 * w1
  cvk_tiu_mul_param_t p;
  p.res_high = m_buf2[ping_idx];
  p.res_low = m_buf1[ping_idx];
  p.a = m_input1[ping_idx];
  p.b_is_const = 1;
  p.b_const.val = m_weight;
  p.b_const.is_signed = 0;
  p.rshift_bits = 0;
  p.relu_enable = 0;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &p);

  // buf = buf + w2 * m_input2, i16 output, it should be >= 0
  cvk_tiu_mac_param_t p2;
  p2.res_high = m_buf2[ping_idx];
  p2.res_low = m_input1[ping_idx];
  p2.res_is_int8 = 1;  // keep origin
  p2.a = m_input2[ping_idx];
  p2.b_is_const = 1;
  p2.b_const.val = 255 - m_weight;
  p2.b_const.is_signed = 0;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 8;  // rshift_bits;
  p2.relu_enable = 0;
  cvk_ctx->ops->tiu_mac(cvk_ctx, &p2);
}