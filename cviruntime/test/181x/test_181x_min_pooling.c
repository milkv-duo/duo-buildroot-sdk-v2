#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tiu_min_pooling_param_t param_t;

static void print_pooling_param(param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  printf("  Pooling parameters:\n");
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", p->kh, p->kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
}

static int pooling_ih_ext(param_t *p, int ih)
{
  int pad = p->pad_top + p->pad_bottom;
  return ih + pad;
}

static int pooling_iw_ext(param_t *p, int iw)
{
  int pad = p->pad_left + p->pad_right;
  return iw + pad;
}

static int pooling_oh(param_t *p, int ih)
{
  int ih_ext = pooling_ih_ext(p, ih);
  return (ih_ext - p->kh) / p->stride_h + 1;
}

static int pooling_ow(param_t *p, int iw)
{
  int iw_ext = pooling_iw_ext(p, iw);
  return (iw_ext - p->kw) / p->stride_w + 1;
}

static int8_t *alloc_input(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ifmap->shape, p->ifmap->fmt);
  int8_t *data = (int8_t *)malloc(size);
  if (!data)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    data[i] = rand() % 256 - 128;
  return data;
}

static int8_t *alloc_output(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  return (int8_t *)malloc(size);
}

static void free_pooling_param(
    cvk_context_t *cvk_ctx,
    param_t *r)
{
  if (r->ifmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ifmap);
  if (r->ofmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, r->ofmap);
}

static param_t random_pooling_param(cvk_context_t *cvk_ctx)
{
  srand(clock());
  param_t p;
  int retry_cnt = 100;

  for (int i = 0; i < retry_cnt; i++) {
    int in = rand() % 5 + 1;
    int ic = rand() % (3 * cvk_ctx->info.npu_num) + 1;
    int ih = rand() % 30 + 3;
    int iw = rand() % 30 + 6;
    int opd0_sign = rand() % 2;

    memset(&p, 0, sizeof(p));
    p.kh = rand() % 7 + 1;
    p.kw = rand() % 7 + 1;
    p.stride_h = rand() % (p.kh) + 1;
    p.stride_w = rand() % (p.kw) + 1;
    p.pad_top = rand() % p.kh;
    p.pad_bottom = rand() % p.kh;
    p.pad_left = rand() % p.kw;
    p.pad_right = rand() % p.kw;

    cvk_tl_shape_t ifmap_shape;
    ifmap_shape.n = in;
    ifmap_shape.c = ic;
    ifmap_shape.h = ih;
    ifmap_shape.w = iw;
    cvk_tl_shape_t ofmap_shape;
    ofmap_shape.n = in;
    ofmap_shape.c = ic;
    ofmap_shape.h = pooling_oh(&p, ih);
    ofmap_shape.w = pooling_ow(&p, iw);

    cvk_fmt_t fmt = opd0_sign? CVK_FMT_I8: CVK_FMT_U8;
    p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_I8, 1);
    p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, fmt, 1);

    if ((p.kh > pooling_ih_ext(&p, ih))
        || (p.kw > pooling_iw_ext(&p, iw))
        || (p.pad_top >= (1 << 4))
        || (p.pad_bottom >= (1 << 4))
        || (p.pad_left >= (1 << 4))
        || (p.pad_right >= (1 << 4))
        || (p.kh * p.kw == 1)
        || !p.ofmap || !p.ifmap) {
      printf("retry init_pooling_param\n");
      free_pooling_param(cvk_ctx, &p);
    } else
      break;
  }

  return p;
}

static int compare_results(
    param_t *p,
    int8_t input[],
    int8_t output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int sign = (p->ifmap->fmt == CVK_FMT_I8);

  int8_t *output_ref = alloc_output(p);
  int ret = native_pooling_min_int8(
      input, output_ref, in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w, 0, 0, 0, 0, sign);
  if (ret)
    return ret;

  ret = array_cmp_int8(
      "Comparing results ...\n", output_ref, output,
      tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  if (ret != 0) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
  }

  free(output_ref);

  return ret;
}

static int test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  param_t param = random_pooling_param(cvk_ctx);
  int8_t *input = alloc_input(&param);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.ifmap, (uint8_t *)input);
  cvk_ctx->ops->tiu_min_pooling(cvk_ctx, &param);
  int8_t *output = (int8_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, param.ofmap);

  int ret = compare_results(&param, input, output);

  free_pooling_param(cvk_ctx, &param);
  free(output);
  free(input);

  return ret;
}

static int test_min_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  for (uint64_t i = 0; i < 16; i++)
    ret |= test_pooling(rt_handle, cvk_ctx);

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

  ret = test_min_pooling(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
