#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_l2l_tensor_copy_param_t param_t;

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
  cvk_tl_shape_t dst_shape;
} case_t;

static case_t g_cases[] = {
  {
    { 1, 1, 1, 1 },
    { 1, 1, 1, 1 },
  }, {
    { 1, 1, 1, 2 },
    { 1, 1, 2, 1 },
  }, {
    { 1, 1, 7, 2 },
    { 1, 1, 2, 7 },
  }, {
    { 1, 1, 17, 13 },
    { 1, 1, 13, 17 },
  }, {
    { 1, 1, 10, 60 },
    { 1, 1, 120, 5 },
  }, {
    { 1, 2, 1, 1 },
    { 1, 1, 1, 2 },
  }, {
    { 2, 17, 1,  4 },
    { 2,  1, 4, 17 },
  }, {
    { 2, 17, 1,  4 },
    { 2,  1, 17, 4 },
  }, {
    { 3, 16, 1, 1 },
    { 3,  1, 2, 8 },
  }, {
    { 3, 39, 17, 23 },
    { 3, 17, 39, 23 },
  }, {
    { 3, 36, 16,  20 },
    { 3, 18,  1, 640 },
  }, {
    { 5, 39, 17, 23 },
    { 5, 17, 39, 23 },
  }, {
    { 20, 35,  2, 2 },
    { 20,  7, 10, 2 },
  }    
};

static void destroy_param(cvk_context_t *cvk_ctx, param_t *p)
{
  if (p->dst)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
  if (p->src)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->src);
}

static void l2l_tensor_copy_ref(param_t *p, uint8_t ref_data[], uint8_t src_data[])
{
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  for (uint64_t i = 0; i < size; i++)
    ref_data[i] = src_data[i];
}

static int test_param(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = tl_shape_size(&p->src->shape, p->src->fmt);

  uint8_t *src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!src_data)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    src_data[i] = 200 + i;

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p->src, src_data);

  cvk_ctx->ops->tdma_l2l_tensor_copy(cvk_ctx, p);

  uint8_t *dst_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);
  uint8_t *ref_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!dst_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  l2l_tensor_copy_ref(p, ref_data, src_data);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      return -1;
    }
  }

fail_exit:
  free(src_data);
  free(dst_data);
  free(ref_data);

  return ret;
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;

  for (int src_align = 0; src_align < 2; src_align++) {
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      param_t p;
      memset(&p, 0, sizeof(p));
      p.src = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->src_shape, CVK_FMT_I8, src_align);
      p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->dst_shape, CVK_FMT_I8, dst_align);
      if (p.src && p.dst)
        ret = test_param(rt_handle, cvk_ctx, &p);
      else if (!p.src)
        fprintf(stderr, "fail to alloc src (%d, %d, %d, %d)\n",
                c->src_shape.n, c->src_shape.c, c->src_shape.h, c->src_shape.w);
      else if (!p.dst)
        fprintf(stderr, "fail to alloc dst (%d, %d, %d, %d)\n",
                c->dst_shape.n, c->dst_shape.c, c->dst_shape.h, c->dst_shape.w);
      destroy_param(cvk_ctx, &p);
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
  for (uint32_t i = 0; i < nr_cases; i++)
    ret |= test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
