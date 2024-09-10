#include "tpu/tpu_or.hpp"
#include <string.h>

int IveTPUOr::init(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  m_slice_info.io_fmt = CVK_FMT_U8;
  m_cmdbuf_subfix = "or";
  m_slice_info.nums_of_tl = 2;

  return CVI_SUCCESS;
}

int IveTPUOr::runSetup(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx,
                       const std::vector<cvk_tg_shape_t> &tg_in_slices,
                       const std::vector<cvk_tg_shape_t> &tg_out_slices,
                       std::vector<uint32_t> *tl_in_idx, std::vector<uint32_t> *tl_out_idx,
                       const bool enable_cext) {
  cvk_tl_shape_t tl_shape;
  tl_shape.n = tg_in_slices[0].n;
  tl_shape.c = tg_in_slices[0].c;
  tl_shape.h = tg_in_slices[0].h;
  tl_shape.w = tg_in_slices[0].w;
  auto *tl_input = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);
  auto *tl_input2 = allocTLMem(cvk_ctx, tl_shape, CVK_FMT_U8, 1);

  m_p_or.a = tl_input;
  m_p_or.b = tl_input2;
  m_p_or.res = tl_input;
  m_p_or.layer_id = 0;

  tl_in_idx->push_back(0);
  tl_in_idx->push_back(1);
  tl_out_idx->push_back(0);
  return CVI_SUCCESS;
}

void IveTPUOr::operation(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, uint32_t ping_idx) {
  cvk_ctx->ops->tiu_or_int8(cvk_ctx, &m_p_or);
}