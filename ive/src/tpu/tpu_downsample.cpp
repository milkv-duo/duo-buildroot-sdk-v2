#include "tpu/tpu_downsample.hpp"
#include <string.h>
#include "ive_log.hpp"
void IveTPUDownSample::setScaleNum(const float scale_num) { m_scale_num = scale_num; }
void IveTPUDownSample::setCellSize(const int cell_size, const int channel) {
  m_slice_info.io_fmt = CVK_FMT_U8;
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
int IveTPUDownSample::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "downsample";
  m_slice_info.nums_of_tl = 2;
  m_slice_info.fix_lmem_size = m_channel * MULTIPLIER_ONLY_PACKED_DATA_SIZE;
  m_kernel_info.nums_of_kernel = 1;
  return CVI_SUCCESS;
}
void IveTPUDownSample::GetSliceUnitProperty(const uint32_t length, const uint32_t slice,
                                            const int kernel_sz, const int default_stride,
                                            sliceUnit *unit) {
  unit->slice = slice > length ? length : slice;
  unit->slice = default_stride * (int)(unit->slice / default_stride);
  unit->skip = unit->slice - kernel_sz + default_stride;
  unit->skip = (uint32_t)default_stride > unit->skip ? default_stride : unit->skip;
  unit->turn = ((int64_t)length - unit->slice) / unit->skip + 1;
  int64_t result = (int64_t)length - (int64_t)((unit->turn) * unit->slice);
  LOGD("GetSliceUnitProperty,length:%d,slice:%d,ksize:%d,dstride:%d,res:%d\n", (int)length,
       (int)slice, (int)kernel_sz, (int)default_stride, (int)result);
  if (result >= kernel_sz) {
    // x + (x - 1) * default_stride
    int res_left = result - kernel_sz;
    int res_div = res_left / default_stride;
    int result_2 = default_stride * res_div + kernel_sz;
    unit->left = result_2;
    unit->turn++;
  } else if (result < 0 && unit->slice > std::abs(result)) {
    unit->left = unit->slice + result;
  } else {
    unit->left = 0;
  }
}
int IveTPUDownSample::sliceSetup(SliceRes &slice_res, SliceRes *tg_in_res, SliceRes *tg_out_res) {
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
int IveTPUDownSample::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
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
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  cvk_tl_shape_t tl_shape2;
  tl_shape2.n = tg_out_slices[0].n;
  tl_shape2.c = tg_out_slices[0].c;
  tl_shape2.h = tg_out_slices[0].h;
  tl_shape2.w = tg_out_slices[0].w;
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape2, CVK_FMT_U8, 1);
  cvk_tl_shape_t tl_block_shape;
  tl_block_shape.n = tg_out_slices[0].n;
  tl_block_shape.c = tg_out_slices[0].c;
  tl_block_shape.h = m_kernel_info.size;
  tl_block_shape.w = m_kernel_info.size;
  auto *block_kernel = allocTLMem(cvk_ctx, tl_block_shape, CVK_FMT_U8, 1, IVETLType::KERNEL);
  constantFillTL(rt_handle, cvk_ctx, 1, block_kernel);
  float real_multiplier = 1.f / (m_kernel_info.size * m_kernel_info.size * m_scale_num);
  cvk_tl_shape_t packed_s = {1, tl_shape.c, 1, MULTIPLIER_ONLY_PACKED_DATA_SIZE};
  auto *tl_multiplier = allocTLMem(cvk_ctx, packed_s, CVK_FMT_U8, 1);
  {
    uint32_t quantized_multiplier;
    int right_shift;
    QuantizeMultiplierSmallerThanOne(real_multiplier, &quantized_multiplier, &right_shift);
    mp_multiplier =
        new CviImg(rt_handle, tl_shape.c, 1, MULTIPLIER_ONLY_PACKED_DATA_SIZE, CVK_FMT_U8);
    getPackedMultiplierArrayBuffer(tl_shape.c, quantized_multiplier, right_shift,
                                   mp_multiplier->GetVAddr());
    cviImgFlush2TL(rt_handle, cvk_ctx, *mp_multiplier, tl_multiplier);
    tl_multiplier->shape = {1, tl_shape.c, 1, 1};
    tl_multiplier->stride =
        cvk_ctx->ops->tl_default_stride(cvk_ctx, tl_multiplier->shape, CVK_FMT_U8, 0);
  }
  m_p_conv.pad_top = m_kernel_info.pad[2];
  m_p_conv.pad_bottom = m_kernel_info.pad[3];
  m_p_conv.pad_left = m_kernel_info.pad[0];
  m_p_conv.pad_right = m_kernel_info.pad[1];
  m_p_conv.stride_w = m_kernel_info.size;
  m_p_conv.stride_h = m_kernel_info.size;
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
  m_p_conv.weight = block_kernel;
  m_p_conv.chl_quan_param = tl_multiplier;
  tl_in_idx->push_back(0);
  tl_out_idx->push_back(1);
  return CVI_SUCCESS;
}
void IveTPUDownSample::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                                 uint32_t ping_idx) {
  cvk_ctx->ops->tiu_depthwise_convolution(cvk_ctx, &m_p_conv);
}
int IveTPUDownSample::postProcess(CVI_RT_HANDLE rt_handle) {
  mp_multiplier->Free(rt_handle);
  delete mp_multiplier;
  mp_multiplier = nullptr;
  return CVI_SUCCESS;
}