#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_matrix_copy_row_col_transposed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.row, p->src->shape.col,
      p->dst->shape.n, p->dst->shape.c,
      p->dst->shape.w, p->dst->shape.col);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_mg_shape_t src_shape;
  cvk_ml_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 2 },
    { 2, 1, 1, 1 },
  }, {
    { 1, 7 },
    { 7, 1, 1, 1 },
  }, {
    { 1, 17 },
    { 17, 1, 1, 1 },
  }, {
    { 1, 60 },
    { 60, 1, 1, 1 },
  }, {
    { 1, 139 },
    { 139, 1, 1, 1 },
  }, {
    { 2, 1 },
    { 1, 1, 2, 2 },
  }, {
    { 2, 1 },
    { 1, 2, 1, 2 },
  }, {
    { 2, 2 },
    { 2, 1, 2, 2 },
  }, {
    { 2, 2 },
    { 2, 2, 1, 2 },
  }, {
    { 2, 7 },
    { 7, 1, 2, 2 },
  }, {
    { 2, 7 },
    { 7, 2, 1, 2 },
  }, {
    { 2, 17 },
    { 17, 1, 2, 2 },
  }, {
    { 2, 17 },
    { 17, 2, 1, 2 },
  }, {
    { 2, 60 },
    { 60, 1, 2, 2 },
  }, {
    { 2, 60 },
    { 60, 2, 1, 2 },
  }, {
    { 2, 139 },
    { 139, 1, 2, 2 },
  }, {
    { 2, 139 },
    { 139, 2, 1, 2 },
  }, {
    { 7, 1 },
    { 1, 1, 7, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 4, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 5, 7 },
  }, {
    { 7, 1 },
    { 1, 2, 6, 7 },
  }, {
    { 7, 1 },
    { 1, 3, 3, 7 },
  }, {
    { 7, 1 },
    { 1, 4, 2, 7 },
  }, {
    { 7, 1 },
    { 1, 7, 1, 7 },
  }, {
    { 7, 2 },
    { 2, 1, 7, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 4, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 5, 7 },
  }, {
    { 7, 2 },
    { 2, 2, 6, 7 },
  }, {
    { 7, 2 },
    { 2, 3, 3, 7 },
  }, {
    { 7, 2 },
    { 2, 4, 2, 7 },
  }, {
    { 7, 2 },
    { 2, 7, 1, 7 },
  }, {
    { 7, 7 },
    { 7, 1, 7, 7 },
  }, {
    { 7, 7 },
    { 7, 3, 3, 7 },
  }, {
    { 7, 7 },
    { 7, 4, 2, 7 },
  }, {
    { 7, 7 },
    { 7, 7, 1, 7 },
  }, {
    { 7, 17 },
    { 17, 1, 7, 7 },
  }, {
    { 7, 17 },
    { 17, 4, 2, 7 },
  }, {
    { 7, 17 },
    { 17, 7, 1, 7 },
  }, {
    { 7, 60 },
    { 60, 1, 7, 7 },
  }, {
    { 7, 60 },
    { 60, 3, 3, 7 },
  }, {
    { 7, 60 },
    { 60, 7, 1, 7 },
  }, {
    { 7, 139 },
    { 139, 1, 7, 7 },
  }, {
    { 7, 139 },
    { 139, 3, 3, 7 },
  }, {
    { 7, 139 },
    { 139, 7, 1, 7 },
  }, {
    { 43, 1 },
    { 1, 1, 43, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 22, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 25, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 37, 43 },
  }, {
    { 43, 1 },
    { 1, 2, 41, 43 },
  }, {
    { 43, 1 },
    { 1, 5, 9, 43 },
  }, {
    { 43, 1 },
    { 1, 5, 10, 43 },
  }, {
    { 43, 1 },
    { 1, 9, 5, 43 },
  }, {
    { 43, 1 },
    { 1, 22, 2, 43 },
  }, {
    { 43, 1 },
    { 1, 43, 1, 43 },
  }, {
    { 43, 2 },
    { 2, 1, 43, 43 },
  }, {
    { 43, 2 },
    { 2, 2, 27, 43 },
  }, {
    { 43, 2 },
    { 2, 22, 2, 43 },
  }, {
    { 43, 2 },
    { 2, 43, 1, 43 },
  }, {
    { 57, 7 },
    { 7, 1, 57, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 37, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 43, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 55, 57 },
  }, {
    { 57, 7 },
    { 7, 2, 56, 57 },
  }, {
    { 57, 7 },
    { 7, 7, 9, 57 },
  }, {
    { 57, 7 },
    { 7, 8, 8, 57 },
  }, {
    { 57, 7 },
    { 7, 29, 2, 57 },
  }, {
    { 57, 7 },
    { 7, 57, 1, 57 },
  }, {
    { 67, 17 },
    { 17, 1, 67, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 34, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 49, 67 },
  }, {
    { 67, 17 },
    { 17, 2, 66, 67 },
  }, {
    { 67, 17 },
    { 17, 6, 12, 67 },
  }, {
    { 67, 17 },
    { 17, 6, 13, 67 },
  }, {
    { 67, 17 },
    { 17, 17, 4, 67 },
  }, {
    { 67, 17 },
    { 17, 34, 2, 67 },
  }, {
    { 67, 17 },
    { 17, 67, 1, 67 },
  }, {
    { 129, 139 },
    { 139, 1, 129, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 65, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 80, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 120, 129 },
  }, {
    { 129, 139 },
    { 139, 2, 128, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 43, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 47, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 59, 129 },
  }, {
    { 129, 139 },
    { 139, 3, 64, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 19, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 20, 129 },
  }, {
    { 129, 139 },
    { 139, 7, 21, 129 },
#if 0 // Not enough lmem size for 1810
  }, {
    { 129, 139 },
    { 139, 43, 3, 129 },
  }, {
    { 129, 139 },
    { 139, 65, 2, 129 },
#endif
  }
// out of lmem size
//  , {
//    { 129, 139 },
//    { 139, 129, 1, 129 },
//  }
};

static void g2l_matrix_copy_row_col_transposed_ref(
    param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t row = p->src->shape.row;
  uint64_t col = p->src->shape.col;

  for (uint64_t ri = 0; ri < row; ri++) {
    for (uint64_t ci = 0; ci < col; ci++) {
      uint64_t src_i = ri * col + ci;
      uint64_t dst_i = ci * row + ri;
      ref_data[dst_i] = src_data[src_i];
    }
  }
}

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  matrix_copy_s2d(rt_handle, p->src, src_data);
  cvk_ctx->ops->tdma_g2l_matrix_copy_row_col_transposed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint8_t *dst_data = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!dst_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  g2l_matrix_copy_row_col_transposed_ref(p, ref_data, src_data);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

fail_exit:
  free(src_data);
  free(dst_data);
  free(ref_data);

  return ret;
}


static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_matrix_dev_mem(rt_handle, p->src);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dst);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;
  param_t p;
  /*
   * Matrix transpose must be n/c stride alignment
   * for TDMA limitation
   */
  int dst_align = 1;
  cvk_fmt_t fmt = CVK_FMT_I8;

  memset(&p, 0, sizeof(p));

  p.src = alloc_matrix_dev_mem(rt_handle, c->src_shape, fmt);
  p.dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, c->dst_shape, fmt, dst_align);
  ret = test_param_g2l(rt_handle, cvk_ctx, &p);
  destroy_param_g2l(rt_handle, cvk_ctx, &p);

  return ret;
}

int main(int argc, char **argv)
{
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
  int ret = 0;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }

  uint32_t nr_cases = sizeof(g_cases) / sizeof(g_cases[0]);
  for (uint32_t i = 0; i < nr_cases; i++)
    ret |= test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
