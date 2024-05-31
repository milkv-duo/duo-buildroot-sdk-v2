#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

#define INVALIDE_STRIDE (-1)
typedef cvk_tiu_depthwise_pt_convolution_param_t param_t;

static void print_pooling_param(param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  printf("  Pooling parameters:\n");
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    rshift_bits = %d\n", p->rshift_bits);
  printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == CVK_FMT_I8);
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

static int8_t *alloc_weight(param_t *p)
{
  int size = tl_shape_size(&p->weight->shape, p->weight->fmt);
  int8_t *data = (int8_t *)malloc(size);
  if (!data)
    return NULL;

  for (int i = 0; i < size; i++)
    data[i] = rand() % 256 - 128;
  return data;
}

static int16_t *alloc_bias(param_t *p)
{
  int c = p->bias->shape.c;
  int16_t *bias = (int16_t *)malloc(sizeof(int16_t) * c);
  if (!bias)
    return NULL;

  for (int i = 0; i < c; i++)
    bias[i] = rand() % 65536 - 32768;
  return bias;
}

static int8_t *alloc_output(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  return (int8_t *)malloc(size);
}

static inline void relu8(int8_t *buf, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}


static int compare_results(
    param_t *p,
    int8_t input[],
    int8_t weight[],
    int16_t bias[],
    int8_t output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;
  int opd0_sign = (p->ifmap->fmt == CVK_FMT_I8);
  int res0_sign = (p->ofmap->fmt == CVK_FMT_I8);
  int8_t *output_ref = alloc_output(p);
  int ret = native_pooling_ave_int8(
      input, weight, p->bias ? bias : NULL, output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      opd0_sign, res0_sign, p->rshift_bits, 0);
  if (ret)
    return ret;

  if(p->relu_enable )
    relu8(output_ref, tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  ret = array_cmp_int8(
      "Comparing results ...\n", output_ref, output,
      tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  if (ret) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
  }

  free(output_ref);

  return ret;
}

static int pooling_ih_ext(param_t *p, int ih)
{
  int ins = p->ins_h;
  int ins_last = p->ins_last_h;
  int pad = p->pad_top + p->pad_bottom;
  return (ih - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_iw_ext(param_t *p, int iw)
{
  int ins = p->ins_w;
  int ins_last = p->ins_last_w;
  int pad = p->pad_left + p->pad_right;
  return (iw - 1) * (ins + 1) + ins_last + 1 + pad;
}

static int pooling_oh(param_t *p, int ih, int kh)
{
  int ih_ext = pooling_ih_ext(p, ih);
  return (ih_ext - kh) / p->stride_h + 1;
}

static int pooling_ow(param_t *p, int iw, int kw)
{
  int iw_ext = pooling_iw_ext(p, iw);
  return (iw_ext - kw) / p->stride_w + 1;
}

static void free_depthwise_param(
    cvk_context_t *cvk_ctx,
    param_t *p)
{
  if (p->bias)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->bias);

  if (p->weight)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->weight);

  if (p->ifmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ifmap);

  if (p->ofmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ofmap);
}

static param_t random_depthwise_param(cvk_context_t *cvk_ctx, int stride_w, int stride_h)
{
  srand(clock());
  param_t p;
  int retry_cnt = 100;

  for (int i = 0; i < retry_cnt; i++) {
    int using_bias = rand() % 2;
    int n = rand() % 5 + 1;
    int c = rand() % (3 * cvk_ctx->info.npu_num) + 1;
    int ih = rand() % 30 + 3 + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
    int iw = rand() % 30 + 6 + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);
    int kh = rand() % 7 + 1;
    int kw = rand() % 7 + 1;
    int opd0_sign = rand() % 2;

    memset(&p, 0, sizeof(p));
    p.ins_h = rand() % kh;
    p.ins_w = rand() % kw;
    p.ins_last_h = rand() % kh;
    p.ins_last_w = rand() % kw;
    p.stride_h = INVALIDE_STRIDE == stride_h ? rand() % (kh) + 1 : stride_h;
    p.stride_w = INVALIDE_STRIDE == stride_w ? rand() % (kh) + 1 : stride_w;
    p.pad_top = rand() % kh;
    p.pad_bottom = rand() % kh;
    p.pad_left = rand() % kw;
    p.pad_right = rand() % kw;
    p.rshift_bits = rand() % 32;

    int oh = pooling_oh(&p, ih, kh);
    int ow = pooling_ow(&p, iw, kw);
    cvk_tl_shape_t ofmap_shape;
    ofmap_shape.n = n;
    ofmap_shape.c = c;
    ofmap_shape.h = oh;
    ofmap_shape.w = ow;
    cvk_tl_shape_t ifmap_shape;
    ifmap_shape.n = n;
    ifmap_shape.c = c;
    ifmap_shape.h = ih;
    ifmap_shape.w = iw;
    cvk_tl_shape_t weight_shape;
    weight_shape.n = 1;
    weight_shape.c = c;
    weight_shape.h = kh;
    weight_shape.w = kw;
    cvk_tl_shape_t bias_shape;
    bias_shape.n = 2;
    bias_shape.c = c;
    bias_shape.h = 1;
    bias_shape.w = 1;
    p.relu_enable = rand()%2;
    /*test case ref does not support dilation !=1*/
    p.dilation_h = 1;
    p.dilation_w = 1;
    cvk_fmt_t ifmt = opd0_sign ? CVK_FMT_I8: CVK_FMT_U8;

    p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_I8, 1);
    p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, ifmt, 1);
    p.weight = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, weight_shape, CVK_FMT_I8, 1);
    p.bias = NULL;
    if (using_bias)
      p.bias = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, bias_shape, CVK_FMT_I8, 0);

    if ((kh > pooling_ih_ext(&p, ih))
        || (kw > pooling_iw_ext(&p, iw))
        || (p.pad_top >= (1 << 4))
        || (p.pad_bottom >= (1 << 4))
        || (p.pad_left >= (1 << 4))
        || (p.pad_right >= (1 << 4))
        || !p.ofmap
        || !p.ifmap
        || !p.weight
        || (using_bias && !p.bias)) {
      printf("retry init_pooling_param\n");
      free_depthwise_param(cvk_ctx, &p);
    } else
        break;
  }

  return p;
}

static void put_bias_tensor(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    int16_t data[])
{
  int c = tl->shape.c;

  uint8_t *lo_hi = (uint8_t *)malloc(2 * c);
  if (!lo_hi)
    return;

  for (int i = 0; i < c; i++) {
    lo_hi[i] = data[i] & 0xff;
    lo_hi[i + c] = (data[i] >> 8) & 0xff;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl, (uint8_t *)lo_hi);

  free(lo_hi);
}

static int _test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int stride_w, int stride_h)
{
  param_t param = random_depthwise_param(cvk_ctx, stride_w, stride_h);

  int8_t *input = alloc_input(&param);
  int8_t *weight = alloc_weight(&param);
  int16_t *bias = NULL;
  if (param.bias)
    bias = alloc_bias(&param);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.ifmap, (uint8_t *)input);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.weight, (uint8_t *)weight);
  if (param.bias)
    put_bias_tensor(rt_handle, cvk_ctx, param.bias, bias);

  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &param);
  int8_t *output = (int8_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, param.ofmap);

  int ret = compare_results(&param, input, weight, bias, output);

  free_depthwise_param(cvk_ctx, &param);
  free(input);
  free(weight);
  free(bias);
  free(output);

  return ret;
}


static int test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  return _test_pooling(rt_handle, cvk_ctx, INVALIDE_STRIDE, INVALIDE_STRIDE);
}

static int test_depthwise_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  for (uint64_t i = 0; i < 16; i++)
    ret |= test_pooling(rt_handle, cvk_ctx);

  // test stride extend (0, 31]
  int stride_list[] = {15, 16, 31};
  int stride_list_len = sizeof(stride_list) / sizeof(stride_list[0]);

  for (int stride_w_idx = 0; stride_w_idx < stride_list_len; stride_w_idx++) {
    for (int stride_h_idx = 0; stride_h_idx < stride_list_len; stride_h_idx++) {
      int stride_w = stride_list[stride_w_idx];
      int stride_h = stride_list[stride_h_idx];

      ret |= _test_pooling(rt_handle, cvk_ctx, stride_w, stride_h);
      if (ret)
        break;
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

  ret = test_depthwise_pooling(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
