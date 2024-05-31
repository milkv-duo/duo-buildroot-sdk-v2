#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_matrix_copy_param_t param_t;

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
    { 0, 1 },
    { 0, 1, 1, 1 },
  }, {
    { 0, 2 },
    { 0, 1, 2, 2 },
  }, {
    { 0, 2 },
    { 0, 2, 1, 2 },
  }, {
    { 0, 7 },
    { 0, 1, 7, 7 },
  }, {
    { 0, 7 },
    { 0, 2, 4, 7 },
  }, {
    { 0, 7 },
    { 0, 7, 1, 7 },
  }, {
    { 0, 17 },
    { 0, 1, 17, 17 },
  }, {
    { 0, 17 },
    { 0, 3, 7, 17 },
  }, {
    { 0, 17 },
    { 0, 17, 1, 17 },
  }, {
    { 0, 60 },
    { 0, 1, 60, 60 },
  }, {
    { 0, 60 },
    { 0, 30, 2, 60 },
  }, {
    { 0, 60 },
    { 0, 60, 1, 60 },
  }
};

static void g2l_matrix_copy_ref(param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  for (uint64_t i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  uint64_t size = ml_shape_size(&p->dst->shape, p->dst->fmt);

  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  matrix_copy_s2d(rt_handle, p->src, src_data);

  cvk_ctx->ops->tdma_g2l_matrix_copy(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);

  uint8_t *dst_data = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!ref_data)
    return -1;

  g2l_matrix_copy_ref(p, ref_data, src_data);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      return -1;
    }
  }

  free(src_data);
  free(dst_data);
  free(ref_data);

  return 0;
}

static void destroy_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_matrix_dev_mem(rt_handle, p->src);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->dst);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;
  cvk_fmt_t fmt = CVK_FMT_I8;

  for (uint32_t row = 1; row < 13; row += 2) {
    c->src_shape.row = row;
    c->dst_shape.n = row;
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      param_t p;
      memset(&p, 0, sizeof(p));

      p.src = alloc_matrix_dev_mem(rt_handle, c->src_shape, fmt);
      p.dst = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, c->dst_shape, fmt, dst_align);
      ret |= test_param_g2l(rt_handle, cvk_ctx, &p);
      destroy_param_g2l(rt_handle, cvk_ctx, &p);

      if (ret)
        return ret;
    }
  }

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
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
  for (uint32_t i = 0; i < nr_cases; i++) {
    ret |= test_one_case(rt_handle, cvk_ctx, &g_cases[i]);
    if (ret)
      break;
  }

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  if (!ret)
    printf("%s pass\n", __FILENAME__);
  else
    printf("%s fail\n", __FILENAME__);

  return ret;
}
