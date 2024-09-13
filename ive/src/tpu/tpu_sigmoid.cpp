#include "tpu/tpu_sigmoid.hpp"
#include <string.h>

int IveTPUSigmoid::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_BF16;
  m_slice_info.ping_pong_size = 2;
  m_slice_info.ping_pong_share_tl = 0;
  m_slice_info.nums_of_tl = 3 * 2;
  m_slice_info.nums_of_table = 2 * 2;

  return CVI_SUCCESS;
}

int IveTPUSigmoid::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                            const std::vector<cvk_tg_shape_t> &tg_in_slices,
                            const std::vector<cvk_tg_shape_t> &tg_out_slices,
                            std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                            const bool enable_cext) {
  m_input.clear();
  m_output.clear();
  m_buf.clear();
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  for (size_t i = 0; i < m_slice_info.ping_pong_size; i++) {
    m_input.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
    m_buf.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
    m_output.emplace_back(allocTLMem(cvk_ctx, tl_shape, CVK_FMT_BF16, 1));
  }

  int range_start = -8;
  int range_end = 8;
  float scale = cvm_sigmoid_scale(range_start, range_end);

  cvk_tl_shape_t tl_table_s = {1, cvk_ctx->info.npu_num, 32, 8};
  auto *tl_table_answer = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
  auto *tl_table_answer_slope = allocTLMem(cvk_ctx, tl_table_s, CVK_FMT_BF16, 1, IVETLType::TABLE);
  table = new CviImg(rt_handle, tl_table_s.c, tl_table_s.h, tl_table_s.w, CVK_FMT_BF16);
  table_slope = new CviImg(rt_handle, tl_table_s.c, tl_table_s.h, tl_table_s.w, CVK_FMT_BF16);
  cvm_sigmoid_tbl((uint16_t *)table->GetVAddr(), (uint16_t *)table_slope->GetVAddr(), &tl_table_s,
                  range_start, range_end);
  cviImgFlush2TL(rt_handle, cvk_ctx, *table, tl_table_answer);
  cviImgFlush2TL(rt_handle, cvk_ctx, *table_slope, tl_table_answer_slope);

  m_p_sig.scale = scale;
  m_p_sig.table_answer = tl_table_answer;
  m_p_sig.table_answer_slope = tl_table_answer_slope;

  for (size_t pp = 0; pp < m_slice_info.ping_pong_size; pp++) {
    tl_in_idx->push_back(0 + pp * 3);
    tl_out_idx->push_back(2 + pp * 3);
  }
  return CVI_SUCCESS;
}

void IveTPUSigmoid::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  m_p_sig.ifmap = m_input[ping_idx];
  m_p_sig.buf = m_buf[ping_idx];
  m_p_sig.ofmap = m_output[ping_idx];
  cvm_emit_sigmoid(cvk_ctx, m_p_sig.ifmap, m_p_sig.buf, m_p_sig.table_answer,
                   m_p_sig.table_answer_slope, m_p_sig.ofmap, m_p_sig.scale);
}

int IveTPUSigmoid::postProcess(CVI_RT_HANDLE rt_handle) {
  if (table) {
    table->Free(rt_handle);
    delete table;
    table = nullptr;
  }
  if (table_slope) {
    table_slope->Free(rt_handle);
    delete table_slope;
    table_slope = nullptr;
  }
  return CVI_SUCCESS;
}