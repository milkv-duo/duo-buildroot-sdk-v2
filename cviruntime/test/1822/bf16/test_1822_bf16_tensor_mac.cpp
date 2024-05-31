#include "../1822_test_util.h"

static void tl_mac_ref(
    u16 *ref,
    u16 *a, u16 *b, u16 *c,
    u64 size, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    float ta = convert_bf16_fp32(a[i]);
    float tb = convert_bf16_fp32(b[i]);
    float tc = convert_bf16_fp32(c[i]);
    float res = ta * tb + tc;

    if(relu_enable)
      if(res<0)
        res=0;
    ref[i] = convert_fp32_bf16(res);
  }
}

static void test_tl_mac(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
{
  int lshift_bits = 1;
  int rshift_bits = 3;
  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;
  fmt_t fmt_type = FMT_BF16;
  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {
    u64 size = n * c * h * w;
    u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1);
    u16 *a_data = (u16 *)xmalloc(data_size);
    u16 *b_data = (u16 *)xmalloc(data_size);
    u16 *c_data = (u16 *)xmalloc(data_size);

    for (u64 i = 0; i < size; i++) {
      a_data[i] = convert_fp32_bf16(rand());
      b_data[i] = convert_fp32_bf16(rand());
      c_data[i] = convert_fp32_bf16(rand());
    }

    u16 *ref_data = (u16 *)xmalloc(data_size);

    tl_mac_ref(ref_data,
               a_data, b_data, c_data,
               size, relu_enable);

    tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_c = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

    put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, a_data, fmt_type);
    put_bf16_tensor_g2l(ctx, bk_ctx, tl_b, b_data, fmt_type);
    put_bf16_tensor_g2l(ctx, bk_ctx, tl_c, c_data, fmt_type);

    bmk1822_tiu_element_wise_mac_param_t p2;
    p2.res_high = 0;
    p2.res_low = tl_c;
    p2.res_is_int8 = relu_enable;
    p2.a = tl_a;
    p2.b_is_const = 0;
    p2.b = tl_b;
    p2.lshift_bits = lshift_bits;
    p2.rshift_bits = rshift_bits;
    p2.relu_enable = relu_enable;
    bmk1822_tiu_element_wise_mac(bk_ctx, &p2);
    u16 *mac_data = (u16 *)get_bf16_tensor_l2g(ctx, bk_ctx, tl_c, fmt_type);

    for (u64 i = 0; i < size; i++) {
      if (mac_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing failed at mac_data[%" PRIu64 "], got %d, exp %d\n",
               i, mac_data[i], ref_data[i]);
        exit(-1);
      }
    }

    free_tl(bk_ctx, tl_c);
    free_tl(bk_ctx, tl_b);
    free_tl(bk_ctx, tl_a);

    free(a_data);
    free(b_data);
    free(c_data);
    free(ref_data);
    free(mac_data);
  }
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();
  test_tl_mac(&ctx, bk_ctx, 0);
  test_tl_mac(&ctx, bk_ctx, 1);
  restore_feround(round_mode);

  test_exit(&ctx);
  return 0;
}
