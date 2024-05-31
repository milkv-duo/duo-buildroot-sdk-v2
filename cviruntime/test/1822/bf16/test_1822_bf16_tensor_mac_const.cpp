#include "../1822_test_util.h"

static void tl_mac_const_ref(
    u16 *ref_low,
    u16 *a, u16 b_const,
    u16 *c_low,
    u64 size, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    float ta = convert_bf16_fp32(a[i]);
    float tb = convert_bf16_fp32(b_const);
    float tc = convert_bf16_fp32(c_low[i]);
    float res = ta * tb + tc;

    if(relu_enable)
    {
      if(res<0)
        res=0;
    }
    ref_low[i] = convert_fp32_bf16(res);
  }
}

static void test_tl_mac_const(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
{
  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {
    fmt_t fmt_type = FMT_BF16;
    u64 size = n * c * h  * w;
    u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1);

    u16 *a_data = (u16 *)xmalloc(data_size);
    u16 *c_low_data = (u16 *)xmalloc(data_size);
    for (u64 i = 0; i < size; i++) {
      a_data[i] = convert_fp32_bf16(rand() % 256);
      c_low_data[i] = convert_fp32_bf16(i);
    }

    u16 b_const = convert_fp32_bf16(37);

    u16 *ref_low_data = (u16 *)xmalloc(data_size);
    tl_mac_const_ref(ref_low_data,
                     a_data, b_const, c_low_data,
                     size, relu_enable);

    tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_c_low = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

    put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, (u16*) a_data, fmt_type);
    put_bf16_tensor_g2l(ctx, bk_ctx, tl_c_low, (u16*) c_low_data, fmt_type);
    bmk1822_tiu_element_wise_mac_param_t p3;
    p3.res_high = 0;
    p3.res_low = tl_c_low;
    p3.res_is_int8 = 1;//relu_enable;
    p3.a = tl_a;
    p3.b_is_const = 1;
    p3.b_const.val = b_const;
    p3.relu_enable = relu_enable;

    bmk1822_tiu_element_wise_mac(bk_ctx, &p3);
    u16 *mac_low_data = (u16*) get_bf16_tensor_l2g(ctx, bk_ctx, tl_c_low, fmt_type);
    for (u64 i = 0; i < size; i++) {
      if (mac_low_data[i] != ref_low_data[i]) {
        fprintf(stderr, "comparing failed at mac_low_data[%" PRIu64 "], got %d, exp %d\n",
               i, mac_low_data[i], ref_low_data[i]);
        exit(-1);
      }
    }

    free_tl(bk_ctx, tl_c_low);
    free_tl(bk_ctx, tl_a);

    free(a_data);
    free(c_low_data);
    free(ref_low_data);
    free(mac_low_data);
  }
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();

  test_tl_mac_const(&ctx, bk_ctx, 0);
  test_tl_mac_const(&ctx, bk_ctx, 1);

  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
