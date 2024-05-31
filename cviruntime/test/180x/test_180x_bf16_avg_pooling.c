#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

#define INVALIDE_STRIDE (-1)
typedef cvk_tiu_average_pooling_param_t param_t;
int random_seed;
static void print_pooling_param(const param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  printf("  Pooling parameters:\n");
  printf("    random_seed : %d \n", random_seed);
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", p->kh, p->kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    avg_pooling_const = %d\n", p->avg_pooling_const);
  printf("    rshift_bits = %d\n", p->rshift_bits);
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
    int const_weight)
{
  if (kh * kw <= 0)
    return -1;

  float *avg_pooling_mac_a = (float *)malloc(kh * kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(kh * kw * sizeof(float));

  uint16_t avg_const_weight = *(uint16_t *)weight;
  const uint16_t *weight_arr = (uint16_t*)weight;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);
  uint16_t *i_fmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = (uint16_t*)weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, cvk_convert_fp32_bf16(0),
          pad_w_l, pad_w_r, pad_h_t, pad_h_b,
          ins_h, ins_w, ins_h_last, ins_w_last,
          input_h, input_w);

      for (int ph = 0; ph < output_h; ++ph) {
        for (int pw = 0; pw < output_w; ++pw) {
          int hstart = ph * stride_h;
          int wstart = pw * stride_w;
          int pool_index = index_get(ph, output_w, pw);
          int mac_index = 0;
          float avg_pool_result=0;
          for (int h = 0; h < kh; h++) {
            for (int w = 0; w < kw; w++) {
              int index = index_get((hstart+h), w_after, (w+wstart));
              mac_index = index_get(h, kw, w);
              float a = cvk_convert_bf16_fp32(i_fmap_pad[index]);
              float b = const_weight ?
                  cvk_convert_bf16_fp32(avg_const_weight) : cvk_convert_bf16_fp32(weight_arr[mac_index]);

              avg_pool_result += a*b;
            }
          }

          if(bias) {
            avg_pool_result += cvk_convert_hex_fp32(bias[c]);
          }
          *(o_fmap+pool_index) = cvk_convert_fp32_bf16(avg_pool_result);
        }
      }
      i_fmap += input_w * input_h;
      if (const_weight == 0)
        weight_arr += kh * kw;

      o_fmap += output_w * output_h;
    }
  }
  free(i_fmap_pad);

  free(avg_pooling_mac_a);
  free(avg_pooling_mac_b);

  return 0;
}

static uint16_t *alloc_input(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ifmap->shape, p->ifmap->fmt);
  uint16_t *data = (uint16_t *)malloc(size);
  if (!data)
    return NULL;

  for (uint64_t i = 0; i < size / sizeof(uint16_t); i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //100 ~ -100
    val = (float)(rand()-RAND_MAX2)*1000 / (float)RAND_MAX;
    data[i] = cvk_convert_fp32_bf16(val);//rand() % 256 - 128;
  }
  return data;
}

static uint16_t *alloc_output(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  return (uint16_t *)malloc(size);
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

static void free_pooling_param(
    cvk_context_t *cvk_ctx,
    param_t *p)
{
  if (p->ifmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ifmap);
  if (p->ofmap)
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, p->ofmap);
}

static param_t random_pooling_param(cvk_context_t *cvk_ctx, int stride_w, int stride_h)
{
  param_t p;

retry:
  random_seed = clock();
  srand(random_seed);

  int in = rand() % 5 + 1;
  int ic = rand() % (3 * cvk_ctx->info.npu_num) + 1;
  int ih = rand() % 30 + 3 + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
  int iw = rand() % 30 + 6 + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);

  memset(&p, 0, sizeof(p));
  p.kh = rand() % 7 + 1;
  p.kw = rand() % 7 + 1;
  p.stride_h = INVALIDE_STRIDE == stride_h ? rand() % (p.kh) + 1 : stride_h;
  p.stride_w = INVALIDE_STRIDE == stride_w ? rand() % (p.kh) + 1 : stride_w;
  p.ins_h = rand() % p.kh;
  p.ins_w = rand() % p.kw;
  p.ins_last_h = rand() % p.kh;
  p.ins_last_w = rand() % p.kw;
  p.pad_top = rand() % p.kh;
  p.pad_bottom = rand() % p.kh;
  p.pad_left = rand() % p.kw;
  p.pad_right= rand() % p.kw;
  p.rshift_bits = rand() % 32;
  p.avg_pooling_const = cvk_convert_fp32_bf16(rand()%0x1000);//rand() % 256;
  cvk_tl_shape_t ifmap_shape;
  ifmap_shape.n = in;
  ifmap_shape.c = ic;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;

  int on = in;
  int oc = ic;
  int oh = pooling_oh(&p, ih);
  int ow = pooling_ow(&p, iw);
  cvk_tl_shape_t ofmap_shape;
  ofmap_shape.n = on;
  ofmap_shape.c = oc;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;

  p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_BF16, 1);
  p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, CVK_FMT_BF16, 1);

  if ((p.kh > pooling_ih_ext(&p, ih))
      || (p.kw > pooling_iw_ext(&p, iw))
      || (p.pad_top >= (1 << 4))
      || (p.pad_bottom >= (1 << 4))
      || (p.pad_left >= (1 << 4))
      || (p.pad_right >= (1 << 4))
      || !p.ofmap
      || !p.ifmap) {
    printf("retry init_pooling_param\n");
    free_pooling_param(cvk_ctx, &p);
    goto retry;
  }

  return p;
}

static int compare_results(
    param_t *p,
    uint16_t input[],
    uint16_t output[])
{
  int ret = 0;
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  uint16_t *output_ref = alloc_output(p);
  p->avg_pooling_const = cvk_convert_fp32_bf16(cvk_convert_bf16_fp32(p->avg_pooling_const)/(p->kh * p->kw));
  ret = native_pooling_avg_bf16(
      input, &p->avg_pooling_const, NULL, output_ref,
      in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,1
      );
  if (ret) {
    free(output_ref);
    return ret;
  }

  ret = array_cmp_int8(
      "Comparing results ...\n", (int8_t*)output_ref, (int8_t*) output,
      tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));

  if (ret) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
    ret = -1;
  }

  free(output_ref);

  return ret;
}

static int _test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int stride_w, int stride_h)
{
  param_t p = random_pooling_param(cvk_ctx, stride_w, stride_h);
//  print_pooling_param(&p);

  uint16_t *input = alloc_input(&p);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, p.ifmap, (uint8_t *)input);
  cvk_ctx->ops->tiu_average_pooling(cvk_ctx, &p);
  uint16_t *output = (uint16_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, p.ofmap);

  int ret = compare_results(&p, input, output);

  free_pooling_param(cvk_ctx, &p);
  free(output);
  free(input);

  return ret;
}


static int test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx) {
  return _test_pooling(rt_handle, cvk_ctx, INVALIDE_STRIDE, INVALIDE_STRIDE);
}

static int test_avg_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
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
  ret = test_avg_pooling(rt_handle, cvk_ctx);
  cvk_restore_feround(round_mode);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
