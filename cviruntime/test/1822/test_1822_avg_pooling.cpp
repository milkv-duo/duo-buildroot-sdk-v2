#include "1822_test_util.h"

#define INVALIDE_STRIDE (-1)
typedef bmk1822_tiu_average_pooling_param_t param_t;

static void print_pooling_param(const param_t *p)
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  printf("  Pooling parameters:\n");
  printf("    ifmap = (%d, %d, %d, %d)\n", in, ic, ih, iw);
  printf("    opd0_sign = %d\n", p->ifmap->fmt == FMT_I8);
  printf("    weight = (%d, %d)\n", p->kh, p->kw);
  printf("    padding = (%d, %d, %d, %d)\n",
         p->pad_top, p->pad_bottom, p->pad_left, p->pad_right);
  printf("    stride = (%d, %d)\n", p->stride_h, p->stride_w);
  printf("    ins0 = (%d, %d, %d, %d)\n",
         p->ins_h, p->ins_last_h, p->ins_w, p->ins_last_w);
  printf("    avg_pooling_const = %d\n", p->avg_pooling_const);
  printf("    rshift_bits = %d\n", p->rshift_bits);
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

static s8 *alloc_output(param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  return (s8 *)xmalloc(size);
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
    bmk_ctx_t *ctx,
    param_t *p)
{
  if (p->ifmap)
    free_tl(ctx, p->ifmap);
  if (p->ofmap)
    free_tl(ctx, p->ofmap);
}

static param_t random_pooling_param(bmk_ctx_t *ctx, int stride_w, int stride_h)
{
  srand(clock());
  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  int in = rand() % 5 + 1;
  int ic = rand() % (3 * BM1822_HW_NPU_NUM) + 1;
  int ih = rand() % 30 + 3 + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
  int iw = rand() % 30 + 6 + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);
  int opd0_sign = rand() % 2;

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
  p.avg_pooling_const = rand() % 256;
  p.rshift_bits = rand() % 32;

  tl_shape_t ifmap_shape;
  ifmap_shape.n = in;
  ifmap_shape.c = ic;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;

  int on = in;
  int oc = ic;
  int oh = pooling_oh(&p, ih);
  int ow = pooling_ow(&p, iw);
  tl_shape_t ofmap_shape;
  ofmap_shape.n = on;
  ofmap_shape.c = oc;
  ofmap_shape.h = oh;
  ofmap_shape.w = ow;

  fmt_t fmt = opd0_sign? FMT_I8: FMT_U8;
  p.ofmap = bmk1822_lmem_alloc_tensor(ctx, ofmap_shape, FMT_I8, 1);
  p.ifmap = bmk1822_lmem_alloc_tensor(ctx, ifmap_shape, fmt, 1);

  if ((p.kh > pooling_ih_ext(&p, ih))
      || (p.kw > pooling_iw_ext(&p, iw))
      || (p.pad_top >= (1 << 4))
      || (p.pad_bottom >= (1 << 4))
      || (p.pad_left >= (1 << 4))
      || (p.pad_right >= (1 << 4))
      || !p.ofmap
      || !p.ifmap) {
    printf("retry init_pooling_param\n");
    free_pooling_param(ctx, &p);
    goto retry;
  }

  return p;
}

static void compare_results(
    param_t *p,
    s8 input[],
    s8 output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;
  int opd0_sign = (p->ifmap->fmt == FMT_I8);

  s8 *output_ref = alloc_output(p);
  bmerr_t ret = native_pooling_ave_int8(
      input, &p->avg_pooling_const, NULL, output_ref,
      in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,
      opd0_sign, opd0_sign, p->rshift_bits, 1);
  assert(ret == BM_SUCCESS);

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

static int _test_pooling(bmctx_t ctx, bmk_ctx_t *bk_ctx, int stride_w, int stride_h)
{
  param_t p = random_pooling_param(bk_ctx, stride_w, stride_h);
  s8 *input = alloc_input(&p);

  put_tensor_g2l(&ctx, bk_ctx, p.ifmap, (u8 *)input);
  bmk1822_tiu_average_pooling(bk_ctx, &p);
  s8 *output = (s8 *)get_tensor_l2g(&ctx, bk_ctx, p.ofmap);

  compare_results(&p, input, output);

  free_pooling_param(bk_ctx, &p);
  free(output);
  free(input);

  return 1;
}

static int test_pooling(bmctx_t ctx, bmk_ctx_t *bk_ctx) {
  return _test_pooling(ctx, bk_ctx, INVALIDE_STRIDE, INVALIDE_STRIDE);
}

static void test_avg_pooling(bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int test_finished_num = 0;
  for (u64 i = 0; i < 16; i++)
    test_finished_num += test_pooling(*ctx, bk_ctx);

  // test stride extend (0, 31]
  int stride_list[] = {15, 16, 31};
  int stride_list_len = sizeof(stride_list) / sizeof(stride_list[0]);

  for (int stride_w_idx = 0; stride_w_idx < stride_list_len; stride_w_idx++) {
    for (int stride_h_idx = 0; stride_h_idx < stride_list_len; stride_h_idx++) {
      int stride_w = stride_list[stride_w_idx];
      int stride_h = stride_list[stride_h_idx];

      test_finished_num += _test_pooling(*ctx, bk_ctx, stride_w, stride_h);
    }
  }
  printf("Test finished %d\n", test_finished_num);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_avg_pooling(&ctx, bk_ctx);

  test_exit(&ctx);
  return 0;
}
