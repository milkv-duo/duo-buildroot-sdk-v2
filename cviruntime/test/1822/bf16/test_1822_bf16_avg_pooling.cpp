#include "../1822_test_util.h"

#define INVALIDE_STRIDE (-1)
typedef bmk1822_tiu_average_pooling_param_t param_t;
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
    int const_weight)
{
  if (kh * kw <= 0)
    return BM_ERR_INVALID_ARGUMENT;

  float *avg_pooling_mac_a = (float *)malloc(kh * kw * sizeof(float));
  float *avg_pooling_mac_b = (float *)malloc(kh * kw * sizeof(float));

  u16 avg_const_weight = *(u16 *)weight;
  const u16 *weight_arr = (u16*)weight;

  int h_after = calc_dilute_hw(input_h, ins_h, ins_h_last, pad_h_t, pad_h_b);
  int w_after = calc_dilute_hw(input_w, ins_w, ins_w_last, pad_w_l, pad_w_r);

  int output_h = calc_output_hw(h_after, kh, stride_h);
  int output_w = calc_output_hw(w_after, kw, stride_w);
  u16 *i_fmap_pad = NULL;
  for (int n = 0; n < input_n; n++) {
    if (const_weight == 0)
      weight_arr = (u16*)weight;

    for (int c = 0; c < input_c; ++c) {
      fill_pad_fmap_bf16(i_fmap, &i_fmap_pad, convert_fp32_bf16(0),
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
              float a = convert_bf16_fp32(i_fmap_pad[index]);
              float b = const_weight ?
                  convert_bf16_fp32(avg_const_weight) : convert_bf16_fp32(weight_arr[mac_index]);

              avg_pool_result += a*b;
            }
          }

          if(bias) {
            avg_pool_result += convert_hex_fp32(bias[c]);
          }
          *(o_fmap+pool_index) = convert_fp32_bf16(avg_pool_result);
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

  return BM_SUCCESS;
}

static u16 *alloc_input(param_t *p)
{
  u64 size = tl_shape_size(&p->ifmap->shape);
  u16 *data = (u16 *)xmalloc(size*2);
  if (!data)
    return NULL;

  for (u64 i = 0; i < size; i++) {
    float val = 0;
    int RAND_MAX2 = RAND_MAX/2; //100 ~ -100
    val = (float)(rand()-RAND_MAX2)*1000 / (float)RAND_MAX;
    data[i] = convert_fp32_bf16(val);//rand() % 256 - 128;
  }
  return data;
}

static u16 *alloc_output(param_t *p)
{
  u64 size = tl_shape_size(&p->ofmap->shape);
  return (u16 *)xmalloc(size*2);
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
  param_t p;
  memset(&p, 0, sizeof(p));

retry:
  random_seed = clock();
  srand(random_seed);

  int in = rand() % 5 + 1;
  int ic = rand() % (3 * BM1822_HW_NPU_NUM) + 1;
  int ih = rand() % 30 + 3 + (INVALIDE_STRIDE == stride_h ? 0 : stride_h);
  int iw = rand() % 30 + 6 + (INVALIDE_STRIDE == stride_w ? 0 : stride_w);

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
  p.avg_pooling_const = convert_fp32_bf16(rand()%0x1000);//rand() % 256;
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

  p.ofmap = bmk1822_lmem_alloc_tensor(ctx, ofmap_shape, FMT_BF16, 1);
  p.ifmap = bmk1822_lmem_alloc_tensor(ctx, ifmap_shape, FMT_BF16, 1);

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
    u16 input[],
    u16 output[])
{
  int in = p->ifmap->shape.n;
  int ic = p->ifmap->shape.c;
  int ih = p->ifmap->shape.h;
  int iw = p->ifmap->shape.w;

  u16 *output_ref = alloc_output(p);
  p->avg_pooling_const = convert_fp32_bf16(convert_bf16_fp32(p->avg_pooling_const)/(p->kh * p->kw));
  bmerr_t ret = native_pooling_avg_bf16(
      input, &p->avg_pooling_const, NULL, output_ref,
      in, ic, ih, iw, p->kh, p->kw,
      p->pad_top, p->pad_bottom, p->pad_left, p->pad_right,
      p->stride_h, p->stride_w,
      p->ins_h, p->ins_w, p->ins_last_h, p->ins_last_w,1
      );
  assert(ret == BM_SUCCESS);
  int cmp_res = array_cmp_int8(
      "Comparing results ...\n", (s8*)output_ref, (s8*) output,
      tl_shape_size(&p->ofmap->shape)*2);

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
//  print_pooling_param(&p);

  u16 *input = alloc_input(&p);

  put_bf16_tensor_g2l(&ctx, bk_ctx, p.ifmap, (u16 *)input, FMT_BF16);
  bmk1822_tiu_average_pooling(bk_ctx, &p);
  u16 *output = (u16 *)get_bf16_tensor_l2g(&ctx, bk_ctx, p.ofmap, FMT_BF16);

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
  for (u64 i = 0; i < 20; i++)
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
  int round_mode;
  round_mode = set_store_feround();
  test_avg_pooling(&ctx, bk_ctx);
  restore_feround(round_mode);

  test_exit(&ctx);
  return 0;
}
