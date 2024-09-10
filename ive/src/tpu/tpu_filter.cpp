#include "tpu/tpu_filter.hpp"
#include <string.h>
#include "ive_log.hpp"

void IveTPUFilter::setKernel(IveKernel &kernel) {
  m_kernel = &kernel;
  m_kernel_info.size = m_kernel->img.m_tg.shape.h;
  int pad = (m_kernel_info.size - 1) / 2;
  m_kernel_info.pad[0] = pad;
  m_kernel_info.pad[1] = pad;
  m_kernel_info.pad[2] = pad;
  m_kernel_info.pad[3] = pad;
}

int IveTPUFilter::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_slice_info.nums_of_tl = 2;
  m_kernel_info.nums_of_kernel = 1;
  return CVI_SUCCESS;
}

int IveTPUFilter::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                           const std::vector<cvk_tg_shape_t> &tg_in_slices,
                           const std::vector<cvk_tg_shape_t> &tg_out_slices,
                           std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                           const bool enable_cext) {
  cvk_tl_shape_t tl_shape, tl_shape_out;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  tl_shape_out.n = tg_out_slices[0].n;
  tl_shape_out.c = tg_out_slices[0].c;
  tl_shape_out.h = tg_out_slices[0].h;
  tl_shape_out.w = tg_out_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape_out, CVK_FMT_U8, 1);

  // Kernel
  if (m_kernel == nullptr) {
    LOGE("Error! kernel not set.\n");
  }
  cvk_tl_shape_t tl_kernel_s = {1, tl_shape.c, m_kernel_info.size, m_kernel_info.size};
  cvk_tl_shape_t packed_s = {1, tl_shape.c, 1, MULTIPLIER_ONLY_PACKED_DATA_SIZE};
  if (m_kernel->img.m_tg.fmt != CVK_FMT_U8 && m_kernel->img.m_tg.fmt != CVK_FMT_I8) {
    LOGE("Kernel fmt type must be U8 or I8.\n");
    return CVI_FAILURE;
  }
  auto *tl_kernel = allocTLMem(cvk_ctx, tl_kernel_s, m_kernel->img.m_tg.fmt, 1, IVETLType::KERNEL);
  if (m_kernel->img.m_tg.shape.c < tl_shape.c) {
    LOGE("kernel size must larger than tl_shape.c\n");
    return CVI_FAILURE;
  }
  int tmp_c = m_kernel->img.m_tg.shape.c;
  m_kernel->img.m_tg.shape.c = tl_shape.c;
  cviImgFlush2TL(rt_handle, cvk_ctx, m_kernel->img, tl_kernel);
  m_kernel->img.m_tg.shape.c = tmp_c;

  auto *tl_multiplier = allocTLMem(cvk_ctx, packed_s, CVK_FMT_U8, 1);
  {
    mp_multiplier =
        new CviImg(rt_handle, tl_shape.c, 1, MULTIPLIER_ONLY_PACKED_DATA_SIZE, CVK_FMT_U8);
    getPackedMultiplierArrayBuffer(tl_shape.c, m_kernel->multiplier.base,
                                   m_kernel->multiplier.shift, mp_multiplier->GetVAddr());
    cviImgFlush2TL(rt_handle, cvk_ctx, *mp_multiplier, tl_multiplier);
    tl_multiplier->shape = {1, tl_shape.c, 1, 1};
    tl_multiplier->stride =
        cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_multiplier->shape, tl_multiplier->fmt, 0);
  }

  if (enable_cext) {
    m_p_conv.pad_top = 0;
    m_p_conv.pad_bottom = 0;
  } else {
    m_p_conv.pad_top = m_kernel_info.pad[2];
    m_p_conv.pad_bottom = m_kernel_info.pad[3];
  }
  m_p_conv.pad_left = m_kernel_info.pad[0];
  m_p_conv.pad_right = m_kernel_info.pad[1];
  m_p_conv.stride_w = 1;
  m_p_conv.stride_h = 1;
  m_p_conv.relu_enable = 1;
  m_p_conv.ins_h = 0;
  m_p_conv.ins_w = 0;
  m_p_conv.ins_last_h = 0;
  m_p_conv.ins_last_w = 0;
  m_p_conv.dilation_h = 1;
  m_p_conv.dilation_w = 1;
  m_p_conv.has_bias = 0;

  m_p_conv.ifmap = tl_input;
  m_p_conv.ofmap = tl_output;
  m_p_conv.weight = tl_kernel;
  m_p_conv.chl_quan_param = tl_multiplier;

  tl_in_idx->push_back(0);
  tl_out_idx->push_back(1);
  return CVI_SUCCESS;
}

void IveTPUFilter::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  cvk_ctx->ops->tiu_depthwise_convolution(cvk_ctx, &m_p_conv);
}

int IveTPUFilter::postProcess(CVI_RT_HANDLE rt_handle) {
  mp_multiplier->Free(rt_handle);
  delete mp_multiplier;
  mp_multiplier = nullptr;
  return CVI_SUCCESS;
}

void IveTPUFilterBF16::setKernel(IveKernel &kernel) {
  m_kernel = &kernel;
  m_kernel_info.size = m_kernel->img.m_tg.shape.h;
  int pad = (m_kernel_info.size - 1) / 2;
  m_kernel_info.pad[0] = pad;
  m_kernel_info.pad[1] = pad;
  m_kernel_info.pad[2] = pad;
  m_kernel_info.pad[3] = pad;
}

int IveTPUFilterBF16::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_cmdbuf_subfix = "filter";
  m_slice_info.nums_of_tl = 2 * 2;
  m_kernel_info.nums_of_kernel = 1 * 2;
  return CVI_SUCCESS;
}

int IveTPUFilterBF16::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                               const std::vector<cvk_tg_shape_t> &tg_in_slices,
                               const std::vector<cvk_tg_shape_t> &tg_out_slices,
                               std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                               const bool enable_cext) {
  cvk_tl_shape_t tl_shape, tl_shape_out;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  tl_shape_out.n = tg_out_slices[0].n;
  tl_shape_out.c = tg_out_slices[0].c;
  tl_shape_out.h = tg_out_slices[0].h;
  tl_shape_out.w = tg_out_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape_out, CVK_FMT_BF16, 1);

  // Kernel
  if (m_kernel == nullptr) {
    LOGE("Error! kernel not set.\n");
  }
  cvk_tl_shape_t tl_kernel_s = {1, m_kernel->img.m_tg.shape.c, m_kernel_info.size,
                                m_kernel_info.size};
  auto *tl_kernel = allocTLMem(cvk_ctx, tl_kernel_s, CVK_FMT_BF16, 1, IVETLType::KERNEL);
  cviImgFlush2TL(rt_handle, cvk_ctx, m_kernel->img, tl_kernel);

  if (enable_cext) {
    m_p_conv.pad_top = 0;
    m_p_conv.pad_bottom = 0;
  } else {
    m_p_conv.pad_top = m_kernel_info.pad[2];
    m_p_conv.pad_bottom = m_kernel_info.pad[3];
  }
  m_p_conv.pad_left = m_kernel_info.pad[0];
  m_p_conv.pad_right = m_kernel_info.pad[1];
  m_p_conv.stride_w = 1;
  m_p_conv.stride_h = 1;
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
  m_p_conv.ofmap = tl_output;
  m_p_conv.weight = tl_kernel;

  tl_in_idx->push_back(0);
  tl_out_idx->push_back(1);
  return CVI_SUCCESS;
}

void IveTPUFilterBF16::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                 uint32_t ping_idx) {
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_conv);
}