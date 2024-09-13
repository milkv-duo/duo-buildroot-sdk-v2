#include "tpu/tpu_mask.hpp"
#include <string.h>

int IveTPUMask::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_force_use_ext = true;
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "mask";
  m_slice_info.ping_pong_size = 2;
  m_slice_info.ping_pong_share_tl = 1;
  m_slice_info.nums_of_tl = 4;
  m_kernel_info.nums_of_kernel = 1;
  return CVI_SUCCESS;
}

int IveTPUMask::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                         const std::vector<cvk_tg_shape_t> &tg_in_slices,
                         const std::vector<cvk_tg_shape_t> &tg_out_slices,
                         std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                         const bool enable_cext) {
  m_input1.clear();
  m_input2.clear();
  m_mask.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input1.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_input2.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
    m_mask.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1));
  }
  auto *high_bit_zeros = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  constantFillTL(rt_handle, cvk_ctx, 0, high_bit_zeros);
  cvk_tl_shape_t tl_kernel_s = {1, cvk_ctx->info.npu_num, 1, 1};
  cvk_tl_shape_t tl_kernel_s_2 = {2, cvk_ctx->info.npu_num, 1, 1};
  auto *tl_weight = allocTLMem(cvk_ctx, tl_kernel_s, CVK_FMT_I8, 1, IVETLType::KERNEL);
  auto *tl_bias = allocTLMem(cvk_ctx, tl_kernel_s_2, CVK_FMT_I8, 0, IVETLType::KERNEL);
  constantFillTL(rt_handle, cvk_ctx, -1, tl_weight);
  cvk_tl_t tl_bias_tmp = *tl_bias;
  cvk_tdma_g2l_tensor_fill_constant_param_t p_const;
  p_const.dst = &tl_bias_tmp;
  p_const.constant = 1;
  cvk_ctx->ops->tdma_g2l_tensor_fill_constant(cvk_ctx, &p_const);
  tl_bias_tmp.start_address += 1;
  p_const.constant = 0;
  cvk_ctx->ops->tdma_g2l_tensor_fill_constant(cvk_ctx, &p_const);

  m_p_min.b_const.val = 1;
  m_p_min.b_const.is_signed = false;
  m_p_min.b_is_const = true;

  m_p_mul.res_high = NULL;
  m_p_mul.b_is_const = 0;
  m_p_mul.rshift_bits = 0;
  m_p_mul.relu_enable = 0;

  m_p_dconv.ins_h = 0;
  m_p_dconv.ins_w = 0;
  m_p_dconv.ins_last_w = 0;
  m_p_dconv.ins_last_h = 0;
  m_p_dconv.dilation_h = 1;
  m_p_dconv.dilation_w = 1;
  m_p_dconv.pad_top = m_p_dconv.pad_bottom = m_p_dconv.pad_left = m_p_dconv.pad_right = 0;
  m_p_dconv.stride_h = 1;
  m_p_dconv.stride_w = 1;
  m_p_dconv.weight = tl_weight;
  m_p_dconv.bias = tl_bias;
  m_p_dconv.rshift_bits = 0;
  m_p_dconv.relu_enable = 0;
  m_p_dconv.cmd_pre_exe_typ = 0;
  m_p_dconv.cmd_pre_exe = 0;
  m_p_dconv.ps32_mode = 0;
  m_p_dconv.ins_val = 0;
  m_p_dconv.ins_fp = 0;

  m_p_add.res_high = NULL;
  m_p_add.a_high = high_bit_zeros;
  m_p_add.b_is_const = 0;
  m_p_add.b.high = high_bit_zeros;
  m_p_add.rshift_bits = 0;
  m_p_add.relu_enable = 0;

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 3);
    tl_in_idx->push_back(1 + pp * 3);
    tl_in_idx->push_back(2 + pp * 3);
    tl_out_idx->push_back(0 + pp * 3);
  }
  return CVI_SUCCESS;
}

void IveTPUMask::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  m_p_min.min = m_mask[ping_idx];
  m_p_min.a = m_mask[ping_idx];
  cvk_ctx->ops->tiu_min(cvk_ctx, &m_p_min);

  m_p_mul.res_low = m_input1[ping_idx];
  m_p_mul.a = m_input1[ping_idx];
  m_p_mul.b = m_mask[ping_idx];
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);

  m_p_dconv.ofmap = m_mask[ping_idx];
  m_p_dconv.ifmap = m_mask[ping_idx];
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_dconv);

  m_p_mul.res_low = m_input2[ping_idx];
  m_p_mul.a = m_input2[ping_idx];
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);

  m_p_add.res_low = m_input1[ping_idx];
  m_p_add.a_low = m_input1[ping_idx];
  m_p_add.b.low = m_input2[ping_idx];
  cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add);
}