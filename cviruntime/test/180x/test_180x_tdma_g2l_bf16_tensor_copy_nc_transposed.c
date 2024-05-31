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
  cvk_fmt_t src_fmt;
  cvk_fmt_t dst_fmt;
} fmt_type_t;

static fmt_type_t input_fmt[] = {
 {CVK_FMT_BF16, CVK_FMT_BF16},
 {CVK_FMT_I8, CVK_FMT_BF16},
 {CVK_FMT_U8, CVK_FMT_BF16},
};

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
#if 0 // No enough local memory for 180x
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
#endif
  }
};

static void g2l_tensor_copy_nc_transposed_ref(
    param_t *p, uint16_t ref_data[], uint16_t src_data[])
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
        if(p->src->fmt == CVK_FMT_BF16 && p->dst->fmt == CVK_FMT_BF16)
          ref_data[dst_i] = src_data[src_i];
        else {
          uint8_t* u8src_data = (uint8_t*)src_data;
          uint8_t sign = p->src->fmt == CVK_FMT_I8 ? 1 : 0;
          ref_data[dst_i] = cvk_convert_int8_bf16(u8src_data[src_i], sign);
        }
      }
    }
  }
}

static int test_param_g2l(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  print_param(stderr, p);
  int ret = 0;
  uint64_t size = tl_shape_size(&p->dst->shape, CVK_FMT_I8);

  uint16_t *u16src_data = (uint16_t *)malloc(sizeof(uint16_t) * size);
  uint8_t *u8src_data = (uint8_t *)malloc(sizeof(uint16_t) * size);
  uint16_t *dst_data = NULL, *ref_data = NULL;
  if (!u16src_data || !u8src_data) {
    ret = -1;
    goto fail_exit;
  }

  uint8_t *src_data;
  if(p->src->fmt == CVK_FMT_BF16) {
    float val = -100;
    for(uint64_t i = 0; i < size; i++) {
      u16src_data[i] = test_generate_bf16_corner_val(val);
      val += 0.1;
    }
    src_data = (uint8_t*)u16src_data;
  } else {
    for(uint64_t i = 0; i < size; i++) {
      u8src_data[i] = 200 + i;
    }
    src_data = u8src_data;
  }

  tensor_copy_s2d(rt_handle, p->src, (uint8_t*) src_data);
  cvk_ctx->ops->tdma_g2l_bf16_tensor_copy_nc_transposed(cvk_ctx, p);
  CVI_RT_Submit(cvk_ctx);
  dst_data = (uint16_t *) tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p->dst);

  ref_data = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!dst_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  g2l_tensor_copy_nc_transposed_ref(p, ref_data, (uint16_t*) src_data);

  for (uint64_t i = 0; i < size; i++) {
    if (dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst[%" PRIu64 "], got %x, exp %x\n",
              i, dst_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

fail_exit:
  free(u8src_data);
  free(u16src_data);
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
  uint32_t nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);
  for (uint32_t i = 0; i < nr_fmt; i++) {
    for (int dst_align = 0; dst_align < 2; dst_align++) {
      param_t p;
      memset(&p, 0, sizeof(p));

      p.src = alloc_tensor_dev_mem(rt_handle, cvk_ctx, c->src_shape, input_fmt[i].src_fmt);
      p.dst = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, c->dst_shape, input_fmt[i].dst_fmt, dst_align);
      if (p.src && p.dst)
        ret |= test_param_g2l(rt_handle, cvk_ctx, &p);
      else if (!p.src)
          fprintf(stderr, "allocate tg failed\n");
      else if (!p.dst)
          fprintf(stderr, "allocate tl shape(%d, %d, %d, %d) failed\n",
                  c->dst_shape.n, c->dst_shape.c, c->dst_shape.h,
                  c->dst_shape.w);
      destroy_param_g2l(rt_handle, cvk_ctx, &p);
    }
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

  printf("tdma g2l bf16 tensor copy nc tp test %s\n", ret ? "fail" : "pass");

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
