#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_l2g_tensor_copy_cw_transposed_param_t param_t;

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
  cvk_tl_shape_t src_shape;
  cvk_tg_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 2, 1, 1 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 2, 7, 1 },
  }, {
    { 1,  1, 17, 13 },
    { 1, 13, 17,  1 },
  }, {
    { 1,  1, 10, 60 },
    { 1, 60, 10,  1 },
  }, {
    { 1, 2, 1, 1 },
    { 1, 1, 1, 2 },
  }, {
    {  2, 17, 1,  4 },
    {  2,  4, 1, 17 },
  }, {
    {  2, 17, 3,  4 },
    {  2,  4, 3, 17 },
  }, {
    {  3, 16, 7,  1 },
    {  3,  1, 7, 16 },
  }, {
    {  3, 39, 17, 23 },
    {  3, 23, 17, 39 },
  }, {
    {  3, 36,  16, 20 },
    {  3, 20,  16, 36 },
  }, {
    #if 0 // No enough local memory for 180x 
    {  5, 39, 17, 23 },
    {  5, 23, 17, 39 },
  }, {
    #endif
    { 20, 35,  2,  2 },
    { 20,  2,  2, 35 },
  }, {
    { 20, 35,  3,  2 },
    { 20,  2,  3, 35 },
  }    
};

static void l2g_tensor_copy_cw_transposed_ref(
    param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  cvk_tl_shape_t s = p->src->shape;
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = s.w;

  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hi = 0; hi < h; hi++) {
        for (uint32_t wi = 0; wi < w; wi++) {
          uint32_t src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          uint32_t dst_i = ni * c * h * w + wi * h * c + hi * c + ci;
          ref_data[dst_i] = src_data[src_i];
        }
      }
    }
  }
}

static int test_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);
  int ret = 0;

  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, src_data);
  cvk_ctx->ops->tdma_l2g_tensor_copy_cw_transposed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint8_t *dst_data = tensor_copy_d2s(rt_handle, p->dst);

  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!dst_data || !ref_data)
    goto fail_exit;

  l2g_tensor_copy_cw_transposed_ref(p, ref_data, src_data);

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


static void destroy_param_l2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  free_tensor_dev_mem(rt_handle, p->dst);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;

  for (int src_align = 0; src_align < 2; src_align++) {
    param_t p;
    memset(&p, 0, sizeof(p));

    p.src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->src_shape, CVK_FMT_I8, src_align);
    p.dst = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->dst_shape, CVK_FMT_I8);
    if (p.src && p.dst)
      ret |= test_param_l2g(rt_handle, cvk_ctx, &p);
    else if (!p.src)
      fprintf(stderr, "allocate tl shape(%d, %d, %d, %d) failed\n",
              c->src_shape.n, c->src_shape.c, c->src_shape.h,
              c->src_shape.w);
    else if (!p.dst)
      fprintf(stderr, "allocate tg failed\n");
    destroy_param_l2g(rt_handle, cvk_ctx, &p);
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
    ret = test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  printf("tdma l2g tensor copy cw tp test %s\n", ret ? "fail" : "pass");

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
