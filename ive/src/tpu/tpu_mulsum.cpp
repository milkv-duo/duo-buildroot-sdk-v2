#include "tpu/tpu_mulsum.hpp"
#include <string.h>

int IveTPUMulSum::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "mulsum";
  m_force_use_ext = true;
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_slice_info.ping_pong_size = 1;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 2 * 2;  // BF16

  return CVI_SUCCESS;
}

int IveTPUMulSum::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                           const std::vector<cvk_tg_shape_t> &tg_in_slices,
                           const std::vector<cvk_tg_shape_t> &tg_out_slices,
                           std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                           const bool enable_cext) {
  m_input.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
  }
  mp_tl_mulsum = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  m_tl_mulsum_shape = tl_shape;
  m_tl_mulsum_stride = mp_tl_mulsum->stride;
  constantFillTL(rt_handle, cvk_ctx, convert_fp32_bf16(1.f), mp_tl_mulsum);

  m_p_mul.b_is_const = 0;
  m_p_mul.b = mp_tl_mulsum;
  m_p_mul.relu_enable = 0;
  m_p_mul.rshift_bits = 0;
  m_p_mul.res_high = NULL;

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp);
  }
  return CVI_SUCCESS;
}

void IveTPUMulSum::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  m_p_mul.a = m_input[ping_idx];
  m_p_mul.res_low = mp_tl_mulsum;
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
}

void IveTPUMulSum::beforeSubmit(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                const std::vector<CviImg *> &input, std::vector<CviImg *> &output) {
  uint32_t total_data_size = m_tl_mulsum_shape.h * m_tl_mulsum_shape.w;
  uint32_t data_size = total_data_size;
  uint32_t fmt_size = getFmtSize(mp_tl_mulsum->fmt);
  cvk_tiu_mul_param_t p_mul;
  cvk_tl_t tl_1;
  cvk_tl_t tl_2;
  tl_1.fmt = mp_tl_mulsum->fmt;
  tl_2.fmt = mp_tl_mulsum->fmt;
  while (data_size > 1) {
    uint32_t start_addr = mp_tl_mulsum->start_address;
    bool add_1 = false;
    if (data_size % 2 != 0) {
      add_1 = true;
      data_size -= 1;
      start_addr += fmt_size;
    }
    data_size /= 2;
    uint32_t w = data_size;
    uint32_t h = 1;
    auto m = w / 2;
    for (size_t i = 2; i < m; i++) {
      if (data_size % i == 0) {
        w = data_size / i;
        h = i;
        if (w < 4063) {
          break;
        }
      }
    }
    tl_1.start_address = start_addr;
    tl_2.start_address = start_addr + (h * w * fmt_size);
    tl_1.shape = {1, m_tl_mulsum_shape.c, h, w};
    tl_1.stride = cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_1.shape, tl_1.fmt, 1);
    tl_2.shape = tl_1.shape;
    tl_2.stride = tl_1.stride;
    p_mul.a = &tl_1;
    p_mul.b = &tl_2;
    p_mul.res_low = &tl_1;
    p_mul.res_high = NULL;
    p_mul.b_is_const = 0;
    p_mul.rshift_bits = 0;
    p_mul.relu_enable = 0;
    cvk_ctx->ops->tiu_mul(cvk_ctx, &p_mul);
    if (add_1) {
      data_size += 1;
    }
  }
  mp_tl_mulsum->shape = {m_tl_mulsum_shape.n, m_tl_mulsum_shape.c, 1, 1};
  mp_tl_mulsum->stride =
      cvk_ctx->ops->tl_default_stride(cvk_ctx, mp_tl_mulsum->shape, mp_tl_mulsum->fmt, 1);
  m_sum = 1.f;
  m_rt_dev = get_tensor_l2g(rt_handle, cvk_ctx, mp_tl_mulsum);
}

int IveTPUMulSum::postProcess(CVI_RT_HANDLE rt_handle) {
  uint8_t *data = get_rt_vaddr(rt_handle, m_rt_dev);
  if (data == nullptr) {
    return CVI_FAILURE;
  }
  uint16_t *bf16_data = (uint16_t *)data;
  size_t total_size = m_tl_mulsum_shape.c;
  for (size_t i = 0; i < total_size; i++) {
    float val = convert_bf16_fp32(bf16_data[i]);
    m_sum *= val;
  }
  CVI_RT_MemFree(rt_handle, m_rt_dev);
  m_rt_dev = NULL;
  return CVI_SUCCESS;
}

double IveTPUMulSum::getSum() { return m_sum; }