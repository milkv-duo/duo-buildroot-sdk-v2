#include "../1880v2_test_util.h"

typedef bmk1880v2_tiu_depthwise_convolution_param_t param_t;
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
  printf("    opd0_sign = %d\n", p->ifmap->fmt == FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    dilation = (%d, %d)\n",p->dilation_h, p->dilation_w);
  printf("    rshift_bits = %d\n", p->rshift_bits);
  printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == FMT_I8);
}

static u16 *alloc_input(param_t *p)
{
  u64 size = tl_shape_size(&p->ifmap->shape);
  u16 *data = (u16 *)xmalloc(size * 2);
  if (!data)
    return NULL;

  for (u64 i = 0; i < size; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    data[i] = convert_fp32_bf16(val);
  }
  return data;
}

static u16 *alloc_weight(param_t *p)
{
  int size = tl_shape_size(&p->weight->shape);
  u16 *data = (u16 *)xmalloc(size * 2);
  if (!data)
    return NULL;

  for (int i = 0; i < size; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //5 ~ -5
    val = (float)(rand()-RAND_MAX2)*5 / (float)RAND_MAX;
    data[i] = convert_fp32_bf16(val);
  }
  return data;
}

static u32 *alloc_bias(param_t *p)
{
  int c = p->bias->shape.c;
  u32 *bias = (u32 *)malloc(sizeof(u32) * c);
  if (!bias)
    return NULL;

  for (int i = 0; i < c; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //2 ~ -2
    val = (float)(rand()-RAND_MAX2)*2 / (float)RAND_MAX;
    bias[i] = convert_fp32_hex(val);
  }
  return bias;
}

static u16 *alloc_output(param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  return (u16 *)xmalloc(size * 2);
}

static inline void bf16_relu(u16 *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (convert_bf16_fp32(buf[i]) < 0)
      buf[i] = convert_fp32_bf16(0);
}

static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int native_pooling_avg_bf16(
    const u16* i_fmap,
    const void* weight,
    const u32 *bias,
    u16 * o_fmap,
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
    return BM_ERR_INVALID_ARGUMENT;

  u16 avg_const_weight = *(u16 *)weight;
  u16 *weight_arr = (u16*)weight;
  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);
  int d_kh = calc_dilute_hw(kh, dh - 1, 0, 0, 0);
  int d_kw = calc_dilute_hw(kw, dw - 1, 0, 0, 0);

  int output_h = calc_output_hw(h_after, d_kh, stride_h);
  int output_w = calc_output_hw(w_after, d_kw, stride_w);
  float *avg_pooling_mac_a = (float *)malloc(d_kh * d_kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(d_kh * d_kw * sizeof(float));

  u16 *i_fmap_pad = NULL;
  u16 *i_kmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = (u16*)weight;

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

              avg_pooling_mac_a[mac_index] = convert_bf16_fp32(i_fmap_pad[index]);

              avg_pooling_mac_b[h*d_kw+w] = const_weight ?
                  convert_bf16_fp32(avg_const_weight) : convert_bf16_fp32(i_kmap_pad[mac_index]);
            }
          }
          inner_float_product(avg_pooling_mac_a, avg_pooling_mac_b, d_kh * d_kw,
              &avg_pool_result);

          if(bias) {
            avg_pool_result += convert_hex_fp32(bias[c]);
          }
          *(o_fmap+pool_index) = convert_fp32_bf16(avg_pool_result);
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

  return BM_SUCCESS;
}

static void compare_results(
    param_t *p,
    u16 input[],
    u16 weight[],
    u32 bias[],
    u16 output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;

  u16 *output_ref = alloc_output(p);
  bmerr_t ret = native_pooling_avg_bf16(
      input, weight, p->bias ? bias : NULL, output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      p->dilation_h, p->dilation_w, 0
      );
  assert(ret == BM_SUCCESS);

  if(p->relu_enable )
    bf16_relu(output_ref, tl_shape_size(&p->ofmap->shape));

  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", (s8*) output_ref, (s8*) output,
      tl_shape_size(&p->ofmap->shape) * 2);

  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
    exit(-1);
  }

  free(output_ref);
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
    bmk_ctx_t *ctx,
    param_t *p)
{
  if (p->bias)
    free_tl(ctx, p->bias);

  if (p->weight)
    free_tl(ctx, p->weight);

  if (p->ifmap)
    free_tl(ctx, p->ifmap);

  if (p->ofmap)
    free_tl(ctx, p->ofmap);
}

static param_t random_depthwise_param(bmk_ctx_t *ctx)
{
  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  random_seed = clock();
  srand(random_seed);
  int using_bias = rand() % 2;
  int n = rand() % 5 + 1;
  int c = rand() % (3 * BM1880V2_HW_NPU_NUM) + 1;
  int ih = rand() % 30 + 3;
  int iw = rand() % 30 + 6;
  int kh = rand() % 7 + 1;
  int kw = rand() % 7 + 1;

  p.ins_h = rand() % kh;
  p.ins_w = rand() % kw;
  p.ins_last_h = rand() % kh;
  p.ins_last_w = rand() % kw;
  p.stride_h = rand() % kh + 1;
  p.stride_w = rand() % kw + 1;
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

  tl_shape_t ofmap_shape;
  ofmap_shape.n = n;
  ofmap_shape.c = c;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;
  tl_shape_t ifmap_shape;
  ifmap_shape.n = n;
  ifmap_shape.c = c;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;
  tl_shape_t weight_shape;
  weight_shape.n = 1;
  weight_shape.c = c;
  weight_shape.h = kh;
  weight_shape.w = kw;
  tl_shape_t bias_shape;
  bias_shape.n = 2;
  bias_shape.c = c;
  bias_shape.h = 1;
  bias_shape.w = 1;
  p.relu_enable = rand()%2;

  fmt_t ifmt = FMT_BF16;
  p.ofmap = bmk1880v2_lmem_alloc_tensor(ctx, ofmap_shape, FMT_BF16, 1);
  p.ifmap = bmk1880v2_lmem_alloc_tensor(ctx, ifmap_shape, ifmt, 1);
  p.weight = bmk1880v2_lmem_alloc_tensor(ctx, weight_shape, FMT_BF16, 1);
  p.bias = NULL;
  if (using_bias)
    p.bias = bmk1880v2_lmem_alloc_tensor(ctx, bias_shape, FMT_BF16, 0);

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
    free_depthwise_param(ctx, &p);
    goto retry;
  }
  return p;
}

static void put_bias_tensor(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    u32 data[])
{
  int c = tl->shape.c;

  u16 *hi_lo = (u16 *)xmalloc(2 * c * 2);
  if (!hi_lo)
    return;

  for (int i = 0; i < c; i++) {
    hi_lo[i] = (data[i] >> 16) & 0xffff;
    hi_lo[i + c] = (data[i]  & 0xffff);
  }
  put_bf16_tensor_g2l(ctx, bk_ctx, tl, (u16 *)hi_lo, FMT_BF16);

  free(hi_lo);
}

static int test_pooling(CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  param_t param = random_depthwise_param(bk_ctx);
  //print_pooling_param(&param);
  u16 *input = alloc_input(&param);
  u16 *weight = alloc_weight(&param);
  u32 *bias = NULL;
  if (param.bias)
    bias = alloc_bias(&param);

  put_bf16_tensor_g2l(&ctx, bk_ctx, param.ifmap, (u16 *)input, FMT_BF16);
  put_bf16_tensor_g2l(&ctx, bk_ctx, param.weight, (u16 *)weight, FMT_BF16);
  if (param.bias)
    put_bias_tensor(&ctx, bk_ctx, param.bias, bias);

  bmk1880v2_tiu_depthwise_convolution(bk_ctx, &param);
  u16 *output = (u16 *)get_bf16_tensor_l2g(&ctx, bk_ctx, param.ofmap, FMT_BF16);
  compare_results(&param, input, weight, bias, output);

  free_depthwise_param(bk_ctx, &param);
  free(input);
  free(weight);
  free(bias);
  free(output);

  return 1;
}

static void test_depthwise_pooling(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int test_finished_num = 0;
  for (u64 i = 0; i < 20; i++)
    test_finished_num += test_pooling(*ctx, bk_ctx);
  printf("Test finished %d\n", test_finished_num);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();
  test_depthwise_pooling(&ctx, bk_ctx);
  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
