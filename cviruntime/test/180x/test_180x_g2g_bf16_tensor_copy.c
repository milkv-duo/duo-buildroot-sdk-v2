#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tdma_g2g_tensor_copy_param_t param_t;

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
  cvk_tg_stride_t src_stride;
  cvk_tg_shape_t dst_shape;
  cvk_tg_stride_t dst_stride;
} case_t;

typedef struct {
  cvk_fmt_t src_fmt;
  cvk_fmt_t dst_fmt;
} fmt_type_t;

static fmt_type_t input_fmt[] = {
 {CVK_FMT_BF16, CVK_FMT_BF16},
 {CVK_FMT_I8, CVK_FMT_I8},
};

static case_t g_cases[] = {
  {
    {1, 3, 3, 3}, {27, 9, 3, 1},
    {1, 3, 3, 3}, {27, 9, 3, 1},
  },
  {
    // YOLOv2 concat layer
    {1, 256, 19, 19}, {92416, 361, 19, 1},
    {1, 256, 19, 19}, {462080, 361, 19, 1},
  }
};

static int test_param_g2g(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = p->src->shape.c * p->src->shape.h * p->src->shape.w;

  uint16_t *u16src_data = (uint16_t *)malloc(sizeof(uint16_t) * size);
  uint8_t *u8src_data = (uint8_t *)malloc(sizeof(uint8_t) * size);
  uint8_t *src_data, *dst_data = NULL;
  if (!u16src_data || !u8src_data) {
    ret = -1;
    goto fail_exit;
  }

  if(p->src->fmt == CVK_FMT_BF16) {
    /* bf16*/
    float val = -100;
    for(uint64_t i = 0; i < size; i++) {
      u16src_data[i] = test_generate_bf16_corner_val(val);
      val += 0.1;
    }
    src_data = (uint8_t*)u16src_data;
  } else {
    /* int8 -> bf16*/
    for(uint64_t i = 0; i < size; i++) {
      u8src_data[i] = 200 + i;
    }
    src_data = u8src_data;
  }

  tensor_copy_s2d(rt_handle, p->src, src_data);

  cvk_ctx->ops->tdma_g2g_bf16_tensor_copy(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  
  dst_data = tensor_copy_d2s(rt_handle, p->dst);
  if (!dst_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != src_data[i]) {
      fprintf(stderr, "comparing(%d->%d) failed at dst[%" PRIu64 "], got %x, exp %x\n",
              p->src->fmt, p->dst->fmt, i, dst_data[i], src_data[i]);
      ret = -1;
      break;
    }
  }

fail_exit:
  free(u8src_data);
  free(u16src_data);
  free(dst_data);

  return ret;
}

static void destroy_param_g2g(CVI_RT_HANDLE rt_handle, param_t *p)
{
  free_tensor_dev_mem(rt_handle, p->src);
  free_tensor_dev_mem(rt_handle, p->dst);
}

static int test_one_case(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, case_t *c)
{
  int ret = 0;
  uint32_t nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  for (uint32_t i = 0; i < nr_fmt; i++) {
        param_t p;
        memset(&p, 0, sizeof(p));
        p.src = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->src_shape, input_fmt[i].src_fmt);
        p.dst = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->dst_shape, input_fmt[i].dst_fmt);
        ret |= test_param_g2g(rt_handle, cvk_ctx, &p);
        destroy_param_g2g(rt_handle, &p);
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