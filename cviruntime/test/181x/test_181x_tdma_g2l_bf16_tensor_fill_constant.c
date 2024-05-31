#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"


typedef cvk_tdma_g2l_tensor_fill_constant_param_t param_t;

static void __print_param(const char *tag, FILE *f, param_t *p)
{
  fprintf(
      f, "%s: %u => (%u, %u, %u, %u)\n",
      tag, p->constant,
      p->dst->shape.n, p->dst->shape.c, p->dst->shape.h, p->dst->shape.w);
}

#define print_param(f, p) __print_param(__func__, f, p)

typedef struct {
  uint16_t constant;
  cvk_tl_shape_t dst_shape;
} case_t;

typedef struct {
  cvk_fmt_t src_fmt;
  cvk_fmt_t dst_fmt;
} fmt_type_t;

static fmt_type_t input_fmt[] = {
 {CVK_FMT_BF16, CVK_FMT_BF16},
};

static case_t g_cases[] = {
  {
    37, { 1, 1, 1, 1 }
  }, {
    39, { 1, 1, 1, 2 }
  }, {
    23, { 1, 1, 2, 1 }
  }, {
    19, { 1, 1, 7, 2 }
  }, {
    17, { 1, 1, 2, 7 }
  }, {
    13, { 1, 1, 17, 13 }
  }, {
    11, { 1, 1, 13, 17 }
  }, {
    7, { 1, 1, 10, 60 }
  }, {
    9, { 1, 1, 120, 5 }
  }, {
    2, { 1, 2, 1, 1 }
  }, {
    3, { 1, 1, 1, 2 }
  }, {
    5, { 2, 17, 1,  4 }
  }, {
    41, { 2,  1, 4, 17 }
  }, {
    5, { 2, 17, 1,  4 }
  }, {
    9, { 2,  1, 17, 4 }
  }, {
    17, { 3, 16, 1, 1 }
  }, {
    26, { 3,  1, 2, 8 }
  }, {
    103, { 3, 39, 17, 23 }
  }, {
    255, { 3, 17, 39, 23 }
  }, {
    254, { 3, 36, 16,  20 }
  }, {
    127, { 3, 18,  1, 640 }
#if 0 // No enough local memory for 1810
  }, {
    128, { 5, 39, 17, 23 }
  }, {
    129, { 5, 17, 39, 23 }
  }, {
    55, { 20, 35,  2, 2 }
  }, {
    1, { 20,  7, 10, 2 }
#endif    
  }
};

static void g2l_tensor_fill_constant_ref(param_t *p, uint16_t ref_data[])
{
  uint64_t size = tl_shape_size(&p->dst->shape, p->dst->fmt);

  for (uint64_t i = 0; i < size/sizeof(uint16_t); i++)
    ref_data[i] = p->constant;
}

static void test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  uint64_t size = tl_shape_size(&p->dst->shape, CVK_FMT_I8);
  uint64_t bytesize = size * fmt_size(p->dst->fmt);

  cvk_ctx->ops->tdma_g2l_bf16_tensor_fill_constant(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  uint16_t *dst_data = (uint16_t*) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  uint16_t *ref_data = (uint16_t *)malloc(bytesize);
  g2l_tensor_fill_constant_ref(p, ref_data);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %d, exp %d\n",
              i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free(dst_data);
  free(ref_data);
}

static void destroy_param_g2l(cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->dst);
}

static void test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  uint32_t nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (uint32_t i = 0; i < nr_fmt; i++) {
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      param_t p;
      memset(&p, 0, sizeof(p));
      p.constant = test_generate_bf16_corner_val(c->constant);
      p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->dst_shape, input_fmt[i].src_fmt, dst_align);

      test_param_g2l(rt_handle, cvk_ctx, &p);
      destroy_param_g2l(cvk_ctx, &p);
    }
  }
}

int main(int argc, char **argv)
{
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
    test_one_case(rt_handle, cvk_ctx, &g_cases[i]);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
