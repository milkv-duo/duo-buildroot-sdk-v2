#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <float.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tiu_min_pooling_param_t param_t;
int random_seed;
static void print_pooling_param(param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int on = p->ofmap->shape.n;
  int oc = p->ofmap->shape.c;
  int oh = p->ofmap->shape.h;
  int ow = p->ofmap->shape.w;

  printf("  Pooling parameters:\n");
  printf("    random_seed : %d \n", random_seed);
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == CVK_FMT_I8);
  printf("    weight = (%d, %d)\n", p->kh, p->kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ofmap = (%d, %d, %d, %d)\n", on, oc, oh, ow);
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

static uint16_t *alloc_input(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ifmap->shape, p->ifmap->fmt);
  uint16_t *data = (uint16_t *)malloc(size);
  if (!data)
    return NULL;

  for (uint64_t i = 0; i < size/sizeof(uint16_t); i++) {
    float val;
    int RAND_MAX2 = RAND_MAX/2; //100 ~ -100
    val = (float)(rand()-RAND_MAX2)*100 / (float)RAND_MAX;
    data[i] = cvk_convert_fp32_bf16(val);
  }
  return data;
}

static uint16_t *alloc_output(param_t *p)
{
  uint64_t size = tl_shape_size(&p->ofmap->shape, p->ofmap->fmt);
  return (uint16_t *)malloc(size);
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

  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  random_seed = clock();
//  random_seed = 3058538;
  srand(random_seed);

#if 0
  int in = 1;
  int ic = 1;
  int ih = 6;
  int iw = 6;
  //int opd0_sign = rand() % 2;

  p.kh = 3;
  p.kw = 3;
  p.stride_h = p.kh;
  p.stride_w = p.kw;
  p.pad_top = 3;//rand() % p.kh;
  p.pad_bottom = 3;//rand() % p.kh;
  p.pad_left = 3;//rand() % p.kw;
  p.pad_right = 3;//rand() % p.kw;

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

#else
  int in = rand() % 5 + 1;
  int ic = rand() % (3 * cvk_ctx->info.npu_num) + 1;
  int ih = rand() % 30 + 3;
  int iw = rand() % 30 + 6;
  //int opd0_sign = rand() % 2;

  p.kh = rand() % 5 + 1;
  p.kw = rand() % 5 + 1;
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
#endif
//  cvk_fmt_t fmt = opd0_sign? CVK_FMT_I8: CVK_FMT_U8;
  p.ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ofmap_shape, CVK_FMT_BF16, 1);
  p.ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, ifmap_shape, CVK_FMT_BF16, 1);

  int RAND_MAX2 = RAND_MAX/2; //20 ~ -20
  float ins_val = (float)(rand()-RAND_MAX2)*20 / (float)RAND_MAX;
  p.ins_fp = cvk_convert_fp32_bf16(ins_val);

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
    goto retry;
  }

  return p;
}
static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int native_pooling_min_bf16(
    const uint16_t* i_fmap,
    uint16_t* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    uint16_t ins_fp
    )
{
  if (ins_h != 0 || ins_w != 0 || ins_h_last != 0  || ins_w_last !=0)
    return -1;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);

  const float min_init = FLT_MAX;//cvk_convert_bf16_fp32(ins_fp);
  uint16_t *i_fmap_pad = NULL;
  for (int nc = 0; nc < input_n * input_c; nc++) {
    fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, ins_fp,
      pad_w_l, pad_w_r, pad_h_t, pad_h_b,
      0, 0, 0, 0, input_h, input_w);

    for (int ph = 0; ph < output_h; ++ph) {
      for (int pw = 0; pw < output_w; ++pw) {
        int hstart = ph * stride_h;
        int wstart = pw * stride_w;
        int pool_index = index_get(ph, output_w, pw);
        float min = min_init;
        for (int h = 0; h < kh; h++) {
          for (int w = 0; w < kw; w++) {
            int index = index_get((hstart + h), (input_w + pad_w_l + pad_w_r),
                            (w + wstart));
            float val = cvk_convert_bf16_fp32(i_fmap_pad[index]);
            min = (val < min)? val: min;
          }
        }
        o_fmap[pool_index] = cvk_convert_fp32_bf16(min);
      }
    }
    i_fmap += input_w * input_h;
    o_fmap += output_w * output_h;
  }
  free(i_fmap_pad);

  return 0;
}


static int compare_results(
    param_t *p,
    uint16_t input[],
    uint16_t output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  uint16_t *output_ref = alloc_output(p);
  int ret = native_pooling_min_bf16(
      input, output_ref, in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w, 0, 0, 0, 0, p->ins_fp);
  if (ret)
    goto fail_exit;

  ret = array_cmp_int8(
      "Comparing results ...\n", (int8_t*) output_ref, (int8_t*)output,
      tl_shape_size(&p->ofmap->shape, p->ofmap->fmt));
  if (ret != 0) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
    ret = -1;;
  }

fail_exit:
  free(output_ref);

  return ret;
}

static int test_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  param_t param = random_pooling_param(cvk_ctx);
  //print_pooling_param(&param);
  uint16_t *input = alloc_input(&param);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, param.ifmap, (uint8_t *)input);
  cvk_ctx->ops->tiu_min_pooling(cvk_ctx, &param);

  uint16_t *output = (uint16_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, param.ofmap);

  int ret = compare_results(&param, input, output);

  free_pooling_param(cvk_ctx, &param);
  free(output);
  free(input);

  return ret;
}

static int test_min_pooling(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int ret = 0;
  for (uint64_t i = 0; i < 20; i++)
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
