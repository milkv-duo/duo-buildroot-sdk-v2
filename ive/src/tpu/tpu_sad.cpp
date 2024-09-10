#include "tpu/tpu_sad.hpp"

#include <string.h>

void IveTPUSAD::setTblMgr(TblMgr *tblmgr) { mp_tblmgr = tblmgr; }

void IveTPUSAD::outputThresholdOnly(bool value) { m_output_thresh_only = value; }
void IveTPUSAD::doThreshold(bool value) { m_do_threshold = value; }

void IveTPUSAD::setThreshold(const uint16_t threshold, const uint8_t min_val,
                             const uint8_t max_val) {
  m_threshold = threshold;
  m_min_value = min_val;
  m_max_value = max_val;
}

void IveTPUSAD::setWindowSize(const int window_size) {
  m_kernel_info.size = window_size;
  m_kernel_info.default_stride_x = 1;
  m_kernel_info.default_stride_y = 1;
  int pad = (window_size - 1) / 2;
  m_kernel_info.pad[0] = pad;
  m_kernel_info.pad[1] = pad + 1;
  m_kernel_info.pad[2] = pad;
  m_kernel_info.pad[3] = pad + 1;
}

int IveTPUSAD::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_cmdbuf_subfix = "sad";
  m_slice_info.io_fmt = CVK_FMT_BF16;
  if (m_do_threshold) {
    m_slice_info.nums_of_tl = 5 * 2;
    m_slice_info.nums_of_table = 1 * 2;
  } else {
    m_slice_info.nums_of_tl = 4 * 2;
    m_slice_info.nums_of_table = 0;
  }
  m_kernel_info.nums_of_kernel = 1;

  return CVI_SUCCESS;
}

int IveTPUSAD::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                        const std::vector<cvk_tg_shape_t> &tg_in_slices,
                        const std::vector<cvk_tg_shape_t> &tg_out_slices,
                        std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                        const bool enable_cext) {
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  auto *tl_input2 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);
  cvk_tl_shape_t tl_shape2;
  tl_shape2.n = tg_out_slices[0].n;
  tl_shape2.c = tg_out_slices[0].c;
  tl_shape2.h = tg_out_slices[0].h;
  tl_shape2.w = tg_out_slices[0].w;
  auto *tl_output = allocTLMem(cvk_ctx, tl_shape2, CVK_FMT_BF16, 1);
  auto *tl_output_thresh =
      m_do_threshold ? allocTLMem(cvk_ctx, tl_shape2, CVK_FMT_BF16, 1) : nullptr;

  auto *tl_abs_tmp = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1);

  cvk_tl_shape_t tl_block_shape;
  tl_block_shape.n = tg_out_slices[0].n;
  tl_block_shape.c = tg_out_slices[0].c;
  tl_block_shape.h = m_kernel_info.size;
  tl_block_shape.w = m_kernel_info.size;
  auto *block_kernel = allocTLMem(cvk_ctx, tl_block_shape, CVK_FMT_BF16, 1, IVETLType::KERNEL);
  constantFillTL(rt_handle, cvk_ctx, convert_fp32_bf16(1.f), block_kernel);

  m_p_min.a = tl_input;
  m_p_min.b = tl_input2;
  m_p_min.b_is_const = 0;
  m_p_min.min = tl_abs_tmp;

  m_p_max.a = tl_input;
  m_p_max.b = tl_input2;
  m_p_max.b_is_const = 0;
  m_p_max.max = tl_input;

  m_p_sub.a_high = NULL;
  m_p_sub.a_low = tl_input;
  m_p_sub.b_high = NULL;
  m_p_sub.b_low = tl_abs_tmp;
  m_p_sub.res_high = NULL;
  m_p_sub.res_low = tl_input;
  m_p_sub.rshift_bits = 0;

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
  m_p_conv.weight = block_kernel;

  tl_in_idx->push_back(0);
  tl_in_idx->push_back(1);
  if (!m_output_thresh_only) {
    tl_out_idx->push_back(2);
  }

  if (m_do_threshold) {
    const cvk_tl_shape_t tl_table_s = mp_tblmgr->getTblTLShape(CVK_FMT_BF16);
    auto *tl_pos_neg_table = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
    {
      mp_table_pos_neg =
          new CviImg(rt_handle, tl_table_s.c, tl_table_s.h, tl_table_s.w, CVK_FMT_BF16);
      genTableBF16(tl_table_s, (float)m_min_value, (float)m_max_value,
                   (uint16_t *)mp_table_pos_neg->GetVAddr());
      cviImgFlush2TL(rt_handle, cvk_ctx, *mp_table_pos_neg, tl_pos_neg_table);
    }

    m_p_add_thresh.a_high = NULL;
    m_p_add_thresh.a_low = tl_output;
    m_p_add_thresh.b_is_const = 1;
    m_p_add_thresh.b_const.val = convert_fp32_bf16((-1 * m_threshold));
    m_p_add_thresh.res_high = NULL;
    m_p_add_thresh.res_low = tl_output_thresh;
    m_p_add_thresh.rshift_bits = 0;
    m_p_add_thresh.relu_enable = 0;
    m_p_mask.ifmap = tl_output_thresh;
    m_p_mask.ofmap = tl_output_thresh;
    m_p_mask.pos_neg_table = tl_pos_neg_table;
    m_p_mask.fmt = CVK_FMT_BF16;

    tl_out_idx->push_back(3);
  }
  return CVI_SUCCESS;
}

void IveTPUSAD::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  cvk_ctx->ops->tiu_min(cvk_ctx, &m_p_min);
  cvk_ctx->ops->tiu_max(cvk_ctx, &m_p_max);
  cvk_ctx->ops->tiu_sub(cvk_ctx, &m_p_sub);
  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &m_p_conv);
  if (m_do_threshold) {
    cvk_ctx->ops->tiu_add(cvk_ctx, &m_p_add_thresh);
    bf16LookupTable(cvk_ctx, &m_p_mask);
  }
}

int IveTPUSAD::postProcess(CVI_RT_HANDLE rt_handle) {
  if (mp_table_pos_neg) {
    mp_table_pos_neg->Free(rt_handle);
    delete mp_table_pos_neg;
    mp_table_pos_neg = nullptr;
  }
  return CVI_SUCCESS;
}