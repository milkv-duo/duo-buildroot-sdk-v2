#include "../1822_test_util.h"
#include <float.h>

typedef bmk1822_tiu_min_pooling_param_t param_t;
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
  printf("    opd0_sign = %d\n", p->ifmap->fmt == FMT_I8);
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

static u16 *alloc_input(param_t *p)
{
  u64 size = tl_shape_size(&p->ifmap->shape);
  u16 *data = (u16 *)xmalloc(size*2);
  if (!data)
    return NULL;

  for (u64 i = 0; i < size; i++) {
    float val;
    int RAND_MAX2 = RAND_MAX/2; //100 ~ -100
    val = (float)(rand()-RAND_MAX2)*100 / (float)RAND_MAX;
    data[i] = convert_fp32_bf16(val);
  }
  return data;
}

static u16 *alloc_output(param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  return (u16 *)xmalloc(size * 2);
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

#else
  int in = rand() % 5 + 1;
  int ic = rand() % (3 * BM1822_HW_NPU_NUM) + 1;
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
#endif
//  fmt_t fmt = opd0_sign? FMT_I8: FMT_U8;
  p.ofmap = bmk1822_lmem_alloc_tensor(ctx, ofmap_shape, FMT_BF16, 1);
  p.ifmap = bmk1822_lmem_alloc_tensor(ctx, ifmap_shape, FMT_BF16, 1);

  int RAND_MAX2 = RAND_MAX/2; //20 ~ -20
  float ins_val = (float)(rand()-RAND_MAX2)*20 / (float)RAND_MAX;
  p.ins_fp = convert_fp32_bf16(ins_val);

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
static int index_get(int h, int w1, int w2)
{
  return h * w1 + w2;
}

int native_pooling_min_bf16(
    const u16* i_fmap,
    u16* o_fmap,
    int input_n, int input_c, int input_h, int input_w,
    int kh, int kw,
    int pad_h_t, int pad_h_b, int pad_w_l, int pad_w_r,
    int stride_h, int stride_w,
    int ins_h, int ins_w,
    int ins_h_last, int ins_w_last,
    u16 ins_fp
    )
{
  if (ins_h != 0 || ins_w != 0 || ins_h_last != 0  || ins_w_last !=0)
    return BM_ERR_INVALID_ARGUMENT;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);

  const float min_init = FLT_MAX;//convert_bf16_fp32(ins_fp);
  u16 *i_fmap_pad = NULL;
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
            float val = convert_bf16_fp32(i_fmap_pad[index]);
            min = (val < min)? val: min;
          }
        }
        o_fmap[pool_index] = convert_fp32_bf16(min);
      }
    }
    i_fmap += input_w * input_h;
    o_fmap += output_w * output_h;
  }
  free(i_fmap_pad);

  return BM_SUCCESS;
}


static void compare_results(
    param_t *p,
    u16 input[],
    u16 output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  u16 *output_ref = alloc_output(p);
  bmerr_t ret = native_pooling_min_bf16(
      input, output_ref, in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w, 0, 0, 0, 0, p->ins_fp);
  assert(ret == BM_SUCCESS);
  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", (s8*) output_ref, (s8*)output,
      tl_shape_size(&p->ofmap->shape)*2);
  if (cmp_res != 0) {
    printf("Comparison FAILED!!!\n");
    print_pooling_param(p);
    exit(-1);
  }
  free(output_ref);
}

static int test_pooling(bmctx_t ctx, bmk_ctx_t *bk_ctx)
{
  param_t param = random_pooling_param(bk_ctx);
  //print_pooling_param(&param);
  u16 *input = alloc_input(&param);
  put_bf16_tensor_g2l(&ctx, bk_ctx, param.ifmap, (u16 *)input, FMT_BF16);
  bmk1822_tiu_bf16_min_pooling(bk_ctx, &param);

  u16 *output = (u16 *)get_bf16_tensor_l2g(&ctx, bk_ctx, param.ofmap, FMT_BF16);

  compare_results(&param, input, output);

  free_pooling_param(bk_ctx, &param);
  free(output);
  free(input);

  return 1;
}

static void test_min_pooling(bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int test_finished_num = 0;
  for (u64 i = 0; i < 20; i++)
    test_finished_num += test_pooling(*ctx, bk_ctx);
  printf("Test finished %d\n", test_finished_num);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_min_pooling(&ctx, bk_ctx);

  test_exit(&ctx);
  return 0;
}
