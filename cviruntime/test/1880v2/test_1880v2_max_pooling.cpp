#include "1880v2_test_util.h"

typedef bmk1880v2_tiu_max_pooling_param_t param_t;

static void print_pooling_param(param_t *p)
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

static void free_pooling_param(
    bmk_ctx_t *ctx,
    param_t *r)
{
  if (r->ifmap)
    free_tl(ctx, r->ifmap);
  if (r->ofmap)
    free_tl(ctx, r->ofmap);
}

static param_t random_pooling_param(bmk_ctx_t *ctx)
{
  srand(clock());
  param_t p;

  memset(&p, 0, sizeof(p));

retry:
  int in = rand() % 5 + 1;
  int ic = rand() % (3 * BM1880V2_HW_NPU_NUM) + 1;
  int ih = rand() % 30 + 3;
  int iw = rand() % 30 + 6;
  int opd0_sign = rand() % 2;

  p.kh = rand() % 7 + 1;
  p.kw = rand() % 7 + 1;
  p.stride_h = rand() % (p.kh) + 1;
  p.stride_w = rand() % (p.kw) + 1;
  p.pad_top = rand() % p.kh;
  p.pad_bottom = rand() % p.kh;
  p.pad_left = rand() % p.kw;
  p.pad_right = rand() % p.kw;

  tl_shape_t ifmap_shape;
  ifmap_shape.n = in;
  ifmap_shape.c = ic;
  ifmap_shape.h = ih;
  ifmap_shape.w = iw;
  tl_shape_t ofmap_shape;
  ofmap_shape.n = in;
  ofmap_shape.c = ic;
  ofmap_shape.h = pooling_oh(&p, ih);
  ofmap_shape.w = pooling_ow(&p, iw);

  fmt_t fmt = opd0_sign? FMT_I8: FMT_U8;
  p.ofmap = bmk1880v2_lmem_alloc_tensor(ctx, ofmap_shape, FMT_I8, 1);
  p.ifmap = bmk1880v2_lmem_alloc_tensor(ctx, ifmap_shape, fmt, 1);

  if ((p.kh > pooling_ih_ext(&p, ih))
      || (p.kw > pooling_iw_ext(&p, iw))
      || (p.pad_top >= (1 << 4))
      || (p.pad_bottom >= (1 << 4))
      || (p.pad_left >= (1 << 4))
      || (p.pad_right >= (1 << 4))
      || (p.kh * p.kw == 1)
      || !p.ofmap || !p.ifmap) {
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
  int sign = (p->ifmap->fmt == FMT_I8);

  s8 *output_ref = alloc_output(p);
  bmerr_t ret = native_pooling_max_int8(
      input, output_ref, in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w, 0, 0, 0, 0, sign);
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

static int test_pooling(CVI_RT_HANDLE ctx, bmk_ctx_t *bk_ctx)
{
  param_t param = random_pooling_param(bk_ctx);
  s8 *input = alloc_input(&param);

  put_tensor_g2l(&ctx, bk_ctx, param.ifmap, (u8 *)input);
  bmk1880v2_tiu_max_pooling(bk_ctx, &param);
  s8 *output = (s8 *)get_tensor_l2g(&ctx, bk_ctx, param.ofmap);

  compare_results(&param, input, output);

  free_pooling_param(bk_ctx, &param);
  free(output);
  free(input);

  return 1;
}

static void test_max_pooling(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
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
  test_max_pooling(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
