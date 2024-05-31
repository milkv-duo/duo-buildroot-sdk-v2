#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <runtime/kernel_function.hpp>
#include <runtime/debug.h>

namespace cvi {
namespace runtime {

static inline int ceiling_func(int numerator, int denominator) {
  return (numerator + denominator - 1) / denominator;
}

static inline uint64_t align_up(uint64_t x, uint64_t n) {
  return (x + n - 1) / n * n;
}

static void tdma_load_stride(cvk_context_t* cvk, cvk_ml_t* ml,
    uint64_t ga_src, cvk_mg_stride_t mg_stride, uint32_t reg_idx) {
  cvk_mg_t src;
  src.base_reg_index = reg_idx;
  src.fmt = CVK_FMT_U8;
  src.shape = {ml->shape.n, ml->shape.col};
  src.start_address = ga_src;
  src.stride = mg_stride;

  cvk_tdma_g2l_matrix_copy_param_t p;
  p.src = &src;
  p.dst = ml;
  p.layer_id = 0;

  cvk->ops->tdma_g2l_matrix_copy(cvk, &p);
}

static void tdma_store_stride(cvk_context_t *cvk, cvk_ml_t *ml,
    uint64_t ga_dst, cvk_mg_stride_t mg_stride) {
  cvk_mg_t dst;
  dst.base_reg_index = 4;
  dst.fmt = CVK_FMT_U8;
  dst.start_address = ga_dst;
  dst.shape = {ml->shape.n, ml->shape.col};
  dst.stride = mg_stride;

  cvk_tdma_l2g_matrix_copy_param_t p;
  p.src = ml;
  p.dst = &dst;
  p.layer_id = 0;

  cvk->ops->tdma_l2g_matrix_copy(cvk, &p);
}

static void convert_result_and_store(cvk_context_t *cvk, cvk_ml_t *ps32, uint64_t gaddr, uint32_t res_col) {
  auto row = ps32->shape.n;
  auto c = ps32->shape.c;
  auto w = ps32->shape.w;
  auto col = ps32->shape.col;

  cvk_tl_shape_t shape = {row, c, 1, w};
  cvk_tl_stride_t stride = cvk->ops->tl_default_stride(cvk, shape, CVK_FMT_U8, 1);
  int size = cvk->ops->lmem_tensor_to_size(cvk, shape, CVK_FMT_U8, 1);
  auto laddr_src = ps32->start_address;
  auto laddr_dst = laddr_src + 4 * size;

  cvk_tl_t src = {};
  src.shape = shape;
  src.stride = stride;
  src.fmt = CVK_FMT_U8;
  src.start_address = laddr_src;

  cvk_tl_t dst = {};
  dst.shape = shape;
  uint32_t lane_num = cvk->info.npu_num;
  uint32_t c_per_lane = ceiling_func(c, lane_num);
  dst.stride = {c_per_lane * w * 4, w * 4, w * 4, 4};
  dst.fmt = CVK_FMT_U8;
  dst.start_address = laddr_dst;

  for (int i = 0; i < 4; ++i) {
    src.start_address = laddr_src + i * size;
    dst.start_address = laddr_dst + i;

    cvk_tiu_copy_param_t param;
    param.src = &src;
    param.dst = &dst;
    cvk->ops->tiu_copy(cvk, &param);
  }

  cvk_ml_t res = {};
  res.shape = {row, c, 4 * w, 4 * col};
  res.fmt = CVK_FMT_U8;
  res.stride = cvk->ops->ml_default_stride(cvk, res.shape, CVK_FMT_U8, 0);
  res.start_address = laddr_dst;

  cvk_mg_stride_t mg_stride;
  mg_stride.row = 4 * res_col;
  tdma_store_stride(cvk, &res, gaddr, mg_stride);
}

static cvk_fmt_t formatTranslate(CVI_FMT fmt) {
  switch(fmt) {
    case CVI_FMT_INT8:  return CVK_FMT_I8;
    case CVI_FMT_UINT8: return CVK_FMT_U8;
    default:
      TPU_LOG_ERROR("unsupported fmt:%d\n", fmt);
      assert(0);
  }
  return CVK_FMT_U8;
}

CVI_RT_MEM runtimeJitMatrixMul(
    CVI_RT_HANDLE ctx, void* cvk_ctx, CVI_FMT format,
    uint32_t m, uint32_t k, uint32_t n) {
  auto cvk = (cvk_context_t*)cvk_ctx;

  uint64_t l_ga = 0;
  uint64_t r_ga = 0;
  uint64_t y_ga = 0;

  cvk_fmt_t fmt = formatTranslate(format);
  uint32_t max_tiu = (1 << 12) - 1; // 1880v2: 12 bit
  uint32_t m_step = std::min(m, max_tiu);
  uint32_t k_step = std::min(k, max_tiu);
  uint32_t n_step = std::min(n, max_tiu);
  uint32_t lane_num = cvk->info.npu_num;
  uint32_t eu_num = cvk->info.eu_num;
  uint32_t min_n_step = eu_num * lane_num;

  uint32_t total_size = 0;
  for (; k_step > 0; --k_step) {
    for (n_step = n; n_step > 0; n_step = align_up(n_step - min_n_step, min_n_step)) {
      for (m_step = m; m_step > 0; --m_step) {
        total_size = 0;

        cvk_ml_shape_t tiled_L_shape = cvk->ops->ml_default_shape(cvk, m_step, k_step, fmt);
        cvk_ml_shape_t tiled_R_shape = cvk->ops->ml_default_shape(cvk, k_step, n_step, fmt);
        cvk_ml_shape_t tiled_Y_shape = cvk->ops->ml_default_shape(cvk, m_step, n_step, fmt);
        cvk_tl_shape_t result_shape = {tiled_Y_shape.n, tiled_Y_shape.c, 1, 4 * tiled_Y_shape.w};

        total_size += cvk->ops->lmem_matrix_to_size(cvk, tiled_L_shape, fmt, 1);
        total_size += cvk->ops->lmem_matrix_to_size(cvk, tiled_R_shape, fmt, 1);
        total_size += cvk->ops->lmem_ps32_matrix_to_size(cvk, tiled_Y_shape, fmt, 1);
        total_size += cvk->ops->lmem_tensor_to_size(cvk, result_shape, fmt, 1);

        if (total_size < cvk->info.lmem_size) {
          goto start;
        }
      }
    }
  }

start:
  assert(m_step > 0 && k_step > 0 && n_step > 0);
  // printf("split: m:%d, k:%d, n:%d\n", (int)m_step, (int)k_step, (int)n_step);
  assert(k_step >= k && "m or n is too bigger");

  cvk_ml_t tiled_L;
  cvk_ml_t tiled_R;
  cvk_ml_t tiled_Y;

  cvk_mg_stride_t mg_stride;

  for (uint32_t k_pos = 0; k_pos < k; k_pos += k_step) {
    uint32_t cur_k = std::min(k - k_pos, k_step);
    assert(cur_k == k_step);
    uint32_t ps32_mode = 2;

    for (uint32_t m_pos = 0; m_pos < m; m_pos += m_step) {
      uint32_t cur_m = std::min(m - m_pos, m_step);

      uint32_t lmem_address = 0;
      tiled_L.start_address = lmem_address;
      tiled_L.fmt = fmt;
      tiled_L.shape = cvk->ops->ml_default_shape(cvk, cur_m, cur_k, fmt);
      tiled_L.stride = cvk->ops->ml_default_stride(cvk, tiled_L.shape, fmt, 1);
      lmem_address += cvk->ops->lmem_matrix_to_size(cvk, tiled_L.shape, fmt, 1);

      mg_stride.row = k;
      tdma_load_stride(cvk, &tiled_L, l_ga + m_pos * k + k_pos, mg_stride, 2);

      for (uint32_t n_pos = 0; n_pos < n; n_pos += n_step) {
        uint32_t cur_n = std::min(n - n_pos, n_step);

        // std::cout << "tiled L :(" << cur_m << "x" << cur_k << "), "
        //           << "tiled R :(" << cur_k << "x" << cur_n << ")\n";

        uint32_t lmem_address_1 = lmem_address;
        tiled_R.start_address = lmem_address_1;
        tiled_R.fmt = fmt;
        tiled_R.shape = cvk->ops->ml_default_shape(cvk, cur_k, cur_n, fmt);
        tiled_R.stride = cvk->ops->ml_default_stride(cvk, tiled_R.shape, fmt, 1);
        lmem_address_1 += cvk->ops->lmem_matrix_to_size(cvk, tiled_R.shape, fmt, 1);

        mg_stride.row = n;
        tdma_load_stride(cvk, &tiled_R, r_ga + k_pos * n + n_pos, mg_stride, 3);

        tiled_Y.start_address = lmem_address_1;
        tiled_Y.fmt = fmt;
        tiled_Y.shape = cvk->ops->ml_default_shape(cvk, cur_m, cur_n, fmt);
        tiled_Y.stride = cvk->ops->ml_default_stride(cvk, tiled_Y.shape, fmt, 1);

        cvk_tiu_matrix_multiplication_param_t p;
        p.res = &tiled_Y;
        p.left = &tiled_L;
        p.right = &tiled_R;
        p.bias = nullptr;
        p.lshift_bits = 0;
        p.rshift_bits = 0;
        p.res_is_int8 = 0;
        p.add_result = 0;
        p.relu_enable = 0;
        p.ps32_mode = ps32_mode;
        p.layer_id = 0;

        cvk->ops->tiu_matrix_multiplication(cvk, &p);

        convert_result_and_store(cvk, &tiled_Y, y_ga + (m_pos * n + n_pos) * sizeof(int), n);
      }
    }
  }

  CVI_RT_MEM cmdbuf_mem;
  uint32_t size;
  auto cmdbuf = cvk->ops->acquire_cmdbuf(cvk, &size);
  int ret = CVI_RT_LoadCmdbuf(ctx, cmdbuf, size, 0, 0, false, &cmdbuf_mem);
  assert(ret == 0);
  cvk->ops->reset(cvk);
  return cmdbuf_mem;
}


}
}