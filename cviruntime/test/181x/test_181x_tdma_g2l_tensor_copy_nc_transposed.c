#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2l_tensor_copy_nc_transposed_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: (%u, %u, %u, %u) => (%u, %u, %u, %u)\n",
      tag,
      p->src->shape.n, p->src->shape.c, p->src->shape.h, p->src->shape.w,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  cvk_tg_shape_t src_shape;
  cvk_tl_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 1, 1, 2 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 1, 2, 1 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 7, 2 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 2, 7 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 17, 13 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 13, 17 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 10, 60 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 2, 300 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 3, 200 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 4, 150 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 5, 120 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 60, 10 },
  }, {
    { 1, 1, 120, 5 },
    { 1, 1, 120, 5 },
  }, {
    { 1, 2, 1, 1 },
    { 2, 1, 1, 1 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 1, 4 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 2, 2 },
  }, {
    { 2, 17, 1, 4 },
    { 17, 2, 4, 1 },
  }, {
    { 17, 2, 2, 2 },
    { 2, 17, 2, 2 },
  }, {
    { 17, 2, 4, 1 },
    { 2, 17, 4, 1 },
  }, {
    {  3, 16, 1, 1 },
    { 16,  3, 1, 1 },
  }, {
    { 3, 39, 23, 17 },
    { 39, 3, 23, 17 },
  }, {
    { 3, 39, 17, 23 },
    { 39, 3, 17, 23 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  16, 20 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  2, 160 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  4, 80 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  8, 40 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  20, 16 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  32, 10 },
  }, {
    { 3, 36,  16, 20 },
    { 36, 3,  64, 5 },
  }, {
    { 5, 39, 17, 23 },
    { 39, 5, 17, 23 },
  }, {
    { 20, 35, 2, 2 },
    { 35, 20, 2, 2 },
  }, {
    { 35, 20, 2, 2 },
    { 20, 35, 2, 2 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 160, 2 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 2, 160 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 4, 80 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 8, 40 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 10, 32 },
  }, {
    { 36, 3, 160, 2 },
    { 3, 36, 20, 16 },
  }, {
    { 39, 5, 23, 17 },
    { 5, 39, 23, 17 },
  }    
};

static void g2l_tensor_copy_nc_transposed_ref(
    param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  cvk_tg_shape_t s = p->src->shape;
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t hw = s.h * s.w;

  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hwi = 0; hwi < hw; hwi++) {
        uint32_t src_i = ni * c * hw + ci * hw + hwi;
        uint32_t dst_i = ci * n * hw + ni * hw + hwi;
        ref_data[dst_i] = src_data[src_i];
      }
    }
  }
}

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = tl_shape_size(&p->dst->shape, p->dst->fmt);

  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  tensor_copy_s2d(rt_handle, p->src, src_data);
  cvk_ctx->ops->tdma_g2l_tensor_copy_nc_transposed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint8_t *dst_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!dst_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  g2l_tensor_copy_nc_transposed_ref(p, ref_data, src_data);

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
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
  free_tensor_dev_mem(rt_handle, p->src);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;

  for (int dst_align = 0; dst_align < 2; dst_align++) {
    param_t p;
    memset(&p, 0, sizeof(p));

    p.src = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->src_shape, CVK_FMT_I8);
    p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->dst_shape, CVK_FMT_I8, dst_align);
    ret |= test_param_g2l(rt_handle, cvk_ctx, &p);
    destroy_param_g2l(rt_handle, cvk_ctx, &p);
  }

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
