#include "1880v2_test_util.h"

typedef bmk1880v2_tiu_depthwise_convolution_param_t param_t;

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
  printf("    opd0_sign = %d\n", p->ifmap->fmt == FMT_I8);
  printf("    weight = (%d, %d)\n", kh, kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    rshift_bits = %d\n", p->rshift_bits);
  printf("    relu_enable = %d\n", p->relu_enable);
  printf("    res0_sign = %d\n", p->ofmap->fmt == FMT_I8);
}

static s8 *alloc_input(param_t *p)
{
  u64 size = tl_shape_size(&p->ifmap->shape);
  s8 *data = (s8 *)xmalloc(size);
  if (!data)
    return NULL;

  for (u64 i = 0; i < size; i++)
    data[i] = rand() % 256 - 128;
  return data;
}

static s8 *alloc_weight(param_t *p)
{
  int size = tl_shape_size(&p->weight->shape);
  s8 *data = (s8 *)xmalloc(size);
  if (!data)
    return NULL;

  for (int i = 0; i < size; i++)
    data[i] = rand() % 256 - 128;
  return data;
}

static s16 *alloc_bias(param_t *p)
{
  int c = p->bias->shape.c;
  s16 *bias = (s16 *)malloc(sizeof(s16) * c);
  if (!bias)
    return NULL;

  for (int i = 0; i < c; i++)
    bias[i] = rand() % 65536 - 32768;
  return bias;
}

static s8 *alloc_output(param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  return (s8 *)xmalloc(size);
}

static inline void relu8(s8 *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}


static void compare_results(
    param_t *p,
    s8 input[],
    s8 weight[],
    s16 bias[],
    s8 output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int kh = p->weight->shape.h;
  int kw = p->weight->shape.w;
  int opd0_sign = (p->ifmap->fmt == FMT_I8);
  int res0_sign = (p->ofmap->fmt == FMT_I8);
  s8 *output_ref = alloc_output(p);
  bmerr_t ret = native_pooling_ave_int8(
      input, weight, p->bias ? bias : NULL, output_ref,
      in, ic, ih, iw, kh, kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      opd0_sign, res0_sign, p->rshift_bits, 0);
  assert(ret == BM_SUCCESS);

  if(p->relu_enable )
    relu8(output_ref, tl_shape_size(&p->ofmap->shape));

  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", output_ref, output,
      tl_shape_size(&p->ofmap->shape));

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
  srand(clock());
  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  int using_bias = rand() % 2;
  int n = rand() % 5 + 1;
  int c = rand() % (3 * BM1880V2_HW_NPU_NUM) + 1;
  int ih = rand() % 30 + 3;
  int iw = rand() % 30 + 6;
  int kh = rand() % 7 + 1;
  int kw = rand() % 7 + 1;
  int opd0_sign = rand() % 2;

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

  int oh = pooling_oh(&p, ih, kh);
  int ow = pooling_ow(&p, iw, kw);
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
  /*test case ref does not support dilation !=1*/
  p.dilation_h = 1;
  p.dilation_w = 1;
  fmt_t ifmt = opd0_sign ? FMT_I8: FMT_U8;

  p.ofmap = bmk1880v2_lmem_alloc_tensor(ctx, ofmap_shape, FMT_I8, 1);
  p.ifmap = bmk1880v2_lmem_alloc_tensor(ctx, ifmap_shape, ifmt, 1);
  p.weight = bmk1880v2_lmem_alloc_tensor(ctx, weight_shape, FMT_I8, 1);
  p.bias = NULL;
  if (using_bias)
    p.bias = bmk1880v2_lmem_alloc_tensor(ctx, bias_shape, FMT_I8, 0);

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
    free_depthwise_param(ctx, &p);
    goto retry;
  }
  return p;
}

static void put_bias_tensor(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const tl_t *tl,
    s16 data[])
{
  int c = tl->shape.c;

  u8 *lo_hi = (u8 *)xmalloc(2 * c);
  if (!lo_hi)
    return;

  for (int i = 0; i < c; i++) {
    lo_hi[i] = data[i] & 0xff;
    lo_hi[i + c] = (data[i] >> 8) & 0xff;
  }

  put_tensor_g2l(ctx, bk_ctx, tl, (u8 *)lo_hi);

  free(lo_hi);
}

static int test_pooling(CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  param_t param = random_depthwise_param(bk_ctx);

  s8 *input = alloc_input(&param);
  s8 *weight = alloc_weight(&param);
  s16 *bias = NULL;
  if (param.bias)
    bias = alloc_bias(&param);

  put_tensor_g2l(&ctx, bk_ctx, param.ifmap, (u8 *)input);
  put_tensor_g2l(&ctx, bk_ctx, param.weight, (u8 *)weight);
  if (param.bias)
    put_bias_tensor(&ctx, bk_ctx, param.bias, bias);

  bmk1880v2_tiu_depthwise_convolution(bk_ctx, &param);
  s8 *output = (s8 *)get_tensor_l2g(&ctx, bk_ctx, param.ofmap);

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
  for (u64 i = 0; i < 16; i++)
    test_finished_num += test_pooling(*ctx, bk_ctx);
  printf("Test finished %d\n", test_finished_num);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_depthwise_pooling(&ctx, bk_ctx);

  test_exit(&ctx);
  return 0;
}
