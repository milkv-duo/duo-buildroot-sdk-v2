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
int random_seed;
static void print_pooling_param(param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  printf("  Pooling parameters:\n");
  printf("    random_seed : %d \n", random_seed);
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    dilation = (%d, %d)\n",p->dilation_h, p->dilation_w);
  printf("    rshift_bits = %d\n", p->rshift_bits);
  printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == CVK_FMT_I8);
}

static uint16_t *alloc_input(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ifmap->shape, p->ifmap->fmt);
  uint16_t *data = (uint16_t *)malloc(size);
  if (!data)
    return NULL;

  for (uint64_t i = 0; i < size / sizeof(uint16_t); i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    data[i] = cvk_convert_fp32_bf16(val);
  }
  return data;
}

static uint16_t *alloc_weight(param_t *p)
{
  uint64_t size = tl_shape_size(&p->weight->shape, p->weight->fmt);
  uint16_t *data = (uint16_t *)malloc(size);
  if (!data)
    return NULL;

  for (uint64_t i = 0; i < size / sizeof(uint16_t); i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    data[i] = cvk_convert_fp32_bf16(val);
  }
  return data;
}

static uint32_t *alloc_bias(param_t *p)
{
  int c = p->bias->shape.c;
  uint32_t *bias = (uint32_t *)malloc(sizeof(uint32_t) * c);
  if (!bias)
    return NULL;

  for (int i = 0; i < c; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //2 ~ -2
    val = (float)(rand()-RAND_MAX2)*2 / (float)RAND_MAX;
    bias[i] = cvk_convert_fp32_hex(val);
  }
  return bias;
}

static uint16_t *alloc_output(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  return (uint16_t *)malloc(size * 2);
}

static inline void bf16_relu(uint16_t *buf, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (cvk_convert_bf16_fp32(buf[i]) < 0)
      buf[i] = cvk_convert_fp32_bf16(0);
}

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int native_pooling_avg_bf16(
    const uint16_t* i_fmap,
    const void* weight,
    const uint32_t *bias,
    uint16_t * o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    int dh, int dw,
    int const_weight)
{
  if (kh * kw <= 0)
    return -1;

  uint16_t avg_const_weight = *(uint16_t *)weight;
  uint16_t *weight_arr = (uint16_t*)weight;
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int d_kh = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int output_h = calc_output_hw(h_after, d_kh, stride_h);
  int output_w = calc_output_hw(w_after, d_kw, stride_w);
  float *avg_pooling_mac_a = (float *)malloc(d_kh * d_kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(d_kh * d_kw * sizeof(float));

  uint16_t *i_fmap_pad = NULL;
  uint16_t *i_kmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = (uint16_t*)weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, 0,
          pad_w_l, pad_w_r, pad_h_t, pad_h_b,
          ins_h, ins_w, ins_h_last, ins_w_last,
          input_h, input_w);

      //kernel_dilation(
      if (const_weight == 0)
        fill_pad_fmap_bf16(
          (weight_arr ), &i_kmap_pad, 0,
          0, 0, 0, 0,  // no padding
          dh - 1, dw - 1, 0, 0,
          kh, kw);

      float avg_pool_result;
      for (int ph = 0; ph < output_h; ++ph) {
        for (int pw = 0; pw < output_w; ++pw) {
          int hstart = ph * stride_h;
          int wstart = pw * stride_w;
          int pool_index = index_get(ph, output_w, pw);
          int mac_index = 0;

          for (int h = 0; h < d_kh; h++) {
            for (int w = 0; w < d_kw; w++) {
              int index = index_get((hstart+h), w_after, (w+wstart));
              mac_index = h*d_kw + w;

              avg_pooling_mac_a[mac_index] = cvk_convert_bf16_fp32(i_fmap_pad[index]);

              avg_pooling_mac_b[h*d_kw+w] = const_weight ?
                  cvk_convert_bf16_fp32(avg_const_weight) : cvk_convert_bf16_fp32(i_kmap_pad[mac_index]);
            }
          }
          inner_float_product(avg_pooling_mac_a, avg_pooling_mac_b, d_kh * d_kw,
              &avg_pool_result);

          if(bias) {
            avg_pool_result += cvk_convert_hex_fp32(bias[c]);
          }
          *(o_fmap+pool_index) = cvk_convert_fp32_bf16(avg_pool_result);
        }
      }
      weight_arr += kh * kw;
      i_fmap += input_w * input_h;
      o_fmap += output_w * output_h;
    }
  }
  free(i_fmap_pad);
  free(i_kmap_pad);
  free(avg_pooling_mac_a);
  free(avg_pooling_mac_b);

  return 0;
}

static int compare_results(
    param_t *p,
    uint16_t input[],
    uint16_t weight[],
    uint32_t bias[],
    uint16_t output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  uint16_t *output_ref = alloc_output(p);
  int ret = native_pooling_avg_bf16(
      input, weight, p->bias ? bias : NULL, output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      p->dilation_h, p->dilation_w, 0
      );
  if (ret)
    goto fail_exit;

  if(p->relu_enable )
    bf16_relu(output_ref, tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  ret = array_cmp_int8(
      "Comparing results ...\n", (int8_t*) output_ref, (int8_t*) output,
      tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  if (ret) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
    ret = -1;
  }

fail_exit:
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

static int pooling_oh(param_t *p, int ih, int kh, int dh)
{
  int ih_ext = pooling_ih_ext(p, ih);
  int d_h = (kh -1) * dh + 1;
  return (ih_ext - d_h) / p->stride_h + 1;
}

static int pooling_ow(param_t *p, int iw, int kw, int dw)
{
  int iw_ext = pooling_iw_ext(p, iw);
  int d_w = (kw -1) * dw +1;
  return (iw_ext - d_w) / p->stride_w + 1;
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
  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  random_seed = clock();
  srand(random_seed);
  int using_bias = rand() % 2;
  int n = rand() % 5 + 1;
  int c = rand() % (3 * cvk_ctx->info.npu_num) + 1;
  int ih = rand() % 30 + 3 + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
  int iw = rand() % 30 + 6 + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);
  int kh = rand() % 7 + 1;
  int kw = rand() % 7 + 1;

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
  p.dilation_h = rand()%4 + 1;
  p.dilation_w = rand()%4 + 1;

  int oh = pooling_oh(&p, ih, kh, p.dilation_h);
  int ow = pooling_ow(&p, iw, kw, p.dilation_w);
  int d_kh = calc_dilute_hw(kh, p.dilation_h - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, p.dilation_w - 1, 0, 0, 0);

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

  cvk_fmt_t ifmt = CVK_FMT_BF16;
  p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_BF16, 1);
  p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, ifmt, 1);
  p.weight = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, weight_shape, CVK_FMT_BF16, 1);
  p.bias = NULL;
  if (using_bias)
    p.bias = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, bias_shape, CVK_FMT_BF16, 0);

  if ((kh > pooling_ih_ext(&p, ih))
      || (kw > pooling_iw_ext(&p, iw))
      || (oh < d_kh)
      || (ow < d_kw)
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
    goto retry;
  }
  return p;
}

static void put_bias_tensor(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_tl_t *tl,
    uint32_t data[])
{
  int c = tl->shape.c;

  uint16_t *hi_lo = (uint16_t *)malloc(2 * c * 2);
  if (!hi_lo)
    return;

  for (int i = 0; i < c; i++) {
    hi_lo[i] = (data[i] >> 16) & 0xffff;
    hi_lo[i + c] = (data[i]  & 0xffff);
  }
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl, (uint8_t *)hi_lo);

  free(hi_lo);
}

static int _test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int stride_w, int stride_h)
{
  param_t param = random_depthwise_param(cvk_ctx, stride_w, stride_h);
  //print_pooling_param(&param);
  uint16_t *input = alloc_input(&param);
  uint16_t *weight = alloc_weight(&param);
  uint32_t *bias = NULL;
  if (param.bias)
    bias = alloc_bias(&param);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.ifmap, (uint8_t *)input);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.weight, (uint8_t *)weight);
  if (param.bias)
    put_bias_tensor(rt_handle, cvk_ctx, param.bias, bias);

  cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, &param);
  uint16_t *output = (uint16_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, param.ofmap);
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
  for (uint64_t i = 0; i < 20; i++)
    ret |= test_pooling(rt_handle, cvk_ctx);

  // test stride extend (0, 31]
  int stride_list[] = {15, 16, 31};
  int stride_list_len = sizeof(stride_list) / sizeof(stride_list[0]);

  for (int stride_w_idx = 0; stride_w_idx < stride_list_len; stride_w_idx++) {
    for (int stride_h_idx = 0; stride_h_idx < stride_list_len; stride_h_idx++) {
      int stride_w = stride_list[stride_w_idx];
      int stride_h = stride_list[stride_h_idx];
      ret |= _test_pooling(rt_handle, cvk_ctx, stride_w, stride_h);
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
  int round_mode;
  round_mode = cvk_set_store_feround();
  ret |= test_depthwise_pooling(rt_handle, cvk_ctx);
  cvk_restore_feround(round_mode);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
