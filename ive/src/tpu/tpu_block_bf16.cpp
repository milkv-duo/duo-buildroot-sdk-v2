#include "ive_log.hpp"
#include "tpu/tpu_block.hpp"

#include <string.h>

void IveTPUBlockBF16::setScaleNum(const float scale_num) { m_scale_num = scale_num; }

void IveTPUBlockBF16::setCellSize(const int cell_size, const int channel) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_kernel_info.size = cell_size;
  m_kernel_info.default_stride_x = cell_size;
  m_kernel_info.default_stride_y = cell_size;
  int pad = 0;
  m_kernel_info.pad[0] = pad;
  m_kernel_info.pad[1] = pad;
  m_kernel_info.pad[2] = pad;
  m_kernel_info.pad[3] = pad;
  m_channel = channel;
}

int IveTPUBlockBF16::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "blockBF16";
  m_slice_info.nums_of_tl = 3 * 2;
  m_kernel_info.nums_of_kernel = 1;

  return CVI_SUCCESS;
}

int IveTPUBlockBF16::sliceSetup(SliceRes &slice_res, SliceRes *tg_in_res, SliceRes *tg_out_res) {
  *tg_in_res = slice_res;
  *tg_out_res = slice_res;
  tg_out_res->h.skip /= m_kernel_info.size;
  tg_out_res->w.skip /= m_kernel_info.size;
  tg_out_res->h.slice /= m_kernel_info.size;
  tg_out_res->w.slice /= m_kernel_info.size;
  tg_out_res->h.left /= m_kernel_info.size;
  tg_out_res->w.left /= m_kernel_info.size;
  return CVI_SUCCESS;
}

int IveTPUBlockBF16::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                              const std::vector<cvk_tg_shape_t> &tg_in_slices,
                              const std::vector<cvk_tg_shape_t> &tg_out_slices,
                              std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                              const bool enable_cext) {
  if (m_channel != tg_in_slices[0].c) {
    LOGE("Channel changed, slicing result may not be suitable.\n");
  }
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  cvk_tl_shape_t tl_shape2;
  tl_shape2.n = tg_out_slices[0].n;
  tl_shape2.c = tg_out_slices[0].c;
  tl_shape2.h = tg_out_slices[0].h;
  tl_shape2.w = tg_out_slices[0].w;
  auto *tl_tmp = allocTLMem(cvk_ctx, tl_shape2, CVK_FMT_BF16, 1);
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape2, CVK_FMT_BF16, 1);

  cvk_tl_shape_t tl_block_shape;
  tl_block_shape.n = tg_out_slices[0].n;
  tl_block_shape.c = tg_out_slices[0].c;
  tl_block_shape.h = m_kernel_info.size;
  tl_block_shape.w = m_kernel_info.size;
  auto *block_kernel = allocTLMem(cvk_ctx, tl_block_shape, CVK_FMT_BF16, 1, IVETLType::KERNEL);
  constantFillTL(rt_handle, cvk_ctx, convert_fp32_bf16(1.f), block_kernel);
  float real_multiplier = 1.f / (m_kernel_info.size * m_kernel_info.size * m_scale_num);

  m_p_conv.pad_top = m_kernel_info.pad[2];
  m_p_conv.pad_bottom = m_kernel_info.pad[3];
  m_p_conv.pad_left = m_kernel_info.pad[0];
  m_p_conv.pad_right = m_kernel_info.pad[1];
  m_p_conv.stride_w = m_kernel_info.size;
  m_p_conv.stride_h = m_kernel_info.size;
  m_p_conv.relu_enable = 0;
  m_p_conv.ins_h = 0;
  m_p_conv.ins_w = 0;
  m_p_conv.ins_last_h = 0;
  m_p_conv.ins_last_w = 0;
  m_p_conv.dilation_h = 1;
  m_p_conv.dilation_w = 1;
  m_p_conv.bias = NULL;
  m_p_conv.rshift_bits = 0;
  m_p_conv.ifmap = tl_input;
  m_p_conv.ofmap = tl_tmp;
  m_p_conv.weight = block_kernel;

  m_p_mul.res_high = NULL;
  m_p_mul.res_low = tl_output;
  m_p_mul.a = tl_tmp;
  m_p_mul.b_is_const = 1;
  m_p_mul.b_const.val = convert_fp32_bf16(real_multiplier);
  m_p_mul.relu_enable = 0;

  tl_in_idx->push_back(0);
  tl_out_idx->push_back(2);
  return CVI_SUCCESS;
}

void IveTPUBlockBF16::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                uint32_t ping_idx) {
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_conv);
  cvk_ctx->ops->tiu_mul(cvk_ctx, &m_p_mul);
}