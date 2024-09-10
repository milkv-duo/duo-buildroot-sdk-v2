#include <string.h>
#include "tpu/tpu_add.hpp"

void IveTPUAddBF16::setCoef(float a, float b) {
  m_p_mul.b_const.val = convert_fp32_bf16(a);
  m_p_mac.b_const.val = convert_fp32_bf16(b);
}

int IveTPUAddBF16::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_cmdbuf_subfix = "add_bf16";
  m_slice_info.ping_pong_size = 2;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 3 * 2;  // BF16

  return CVI_SUCCESS;
}

int IveTPUAddBF16::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                            const std::vector<cvk_tg_shape_t> &tg_in_slices,
                            const std::vector<cvk_tg_shape_t> &tg_out_slices,
                            std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                            const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
  }
  auto *dummy = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);

  m_p_mul.b_is_const = 1;
  m_p_mul.rshift_bits = 0;
  m_p_mac.b_is_const = 1;
  m_p_mac.lshift_bits = 0;
  m_p_mac.rshift_bits = 0;
  m_p_mul.res_high = NULL;
  m_p_mac.res_high = dummy;

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 2);
    tl_in_idx->push_back(1 + pp * 2);
    tl_out_idx->push_back(0 + pp * 2);
  }
  return CVI_SUCCESS;
}

void IveTPUAddBF16::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  m_p_mul.a = m_input1[ping_idx];
  m_p_mul.res_low = m_input1[ping_idx];
  m_p_mac.a = m_input2[ping_idx];
  m_p_mac.res_low = m_input1[ping_idx];
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
  cvk_ctx->ops->tiu_mac(cvk_ctx, &m_p_mac);
}