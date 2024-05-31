#include "1822_test_util.h"

static void tl_mac_ref(
    u8 *ref_high, u8 *ref_low,
    u8 *a, u8 *b, u8 *c_high, u8 *c_low,
    int lshift_bits, int rshift_bits, u64 size, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    s32 ta = (s8)a[i];
    s32 tb = (s8)b[i];
    s32 tc = ((s8)c_high[i] << 8) + c_low[i];
    tc <<= lshift_bits;
    s32 res = ta * tb + tc;

    res += 1 << (rshift_bits - 1);
    res >>= rshift_bits;
    if(relu_enable)
    {
      if (res > 127)
        res = 127;
      else if (res < -128)
        res = -128;

      if(relu_enable)
        if(res<0)
          res=0;
      ref_high[i] = 0;
      ref_low[i] = res & 0xff;

    }else{
      if (res > 32767)
        res = 32767;
      else if (res < -32768)
        res = -32768;
      ref_high[i] = (res >> 8) & 0xff;
      ref_low[i] = res & 0xff;
    }
  }
}

static void test_tl_mac(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
{
  int lshift_bits;
  int rshift_bits;
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
    u64 size = n * c * h * w;
    u8 *a_data = (u8 *)xmalloc(size);
    u8 *b_data = (u8 *)xmalloc(size);
    u8 *c_high_data = (u8 *)xmalloc(size);
    u8 *c_low_data = (u8 *)xmalloc(size);

    for (u64 i = 0; i < size; i++) {
      a_data[i] = rand() % 128;
      b_data[i] = 100 - i;
      c_high_data[i] = rand() % 64;
      c_low_data[i] = 200 + 2 * i;
    }

    if(relu_enable) {
      lshift_bits= 1;
      rshift_bits = 7;
    }else {
      lshift_bits = 1;
      rshift_bits = 3;
    }

    u8 *ref_high_data = (u8 *)xmalloc(size);
    u8 *ref_low_data = (u8 *)xmalloc(size);

    tl_mac_ref(ref_high_data, ref_low_data,
               a_data, b_data, c_high_data, c_low_data,
               lshift_bits, rshift_bits, size, relu_enable);

    tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_c_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_c_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);

    put_tensor_g2l(ctx, bk_ctx, tl_a, a_data);
    put_tensor_g2l(ctx, bk_ctx, tl_b, b_data);
    put_tensor_g2l(ctx, bk_ctx, tl_c_low, c_low_data);
    put_tensor_g2l(ctx, bk_ctx, tl_c_high, c_high_data);
    bmk1822_tiu_element_wise_mac_param_t p2;
    p2.res_high = tl_c_high;
    p2.res_low = tl_c_low;
    p2.res_is_int8 = relu_enable;
    p2.a = tl_a;
    p2.b_is_const = 0;
    p2.b = tl_b;
    p2.lshift_bits = lshift_bits;
    p2.rshift_bits = rshift_bits;
    p2.relu_enable = relu_enable;
    bmk1822_tiu_element_wise_mac(bk_ctx, &p2);
    u8 *mac_high_data = get_tensor_l2g(ctx, bk_ctx, tl_c_high);
    u8 *mac_low_data = get_tensor_l2g(ctx, bk_ctx, tl_c_low);

    for (u64 i = 0; i < size; i++) {
      if(!relu_enable)
        if (mac_high_data[i] != ref_high_data[i]) {
          fprintf(stderr, "comparing failed at mac_high_data[%" PRIu64 "], got %d, exp %d\n",
                 i, mac_high_data[i], ref_high_data[i]);
          exit(-1);
        }
      if (mac_low_data[i] != ref_low_data[i]) {
        fprintf(stderr, "comparing failed at mac_low_data[%" PRIu64 "], got %d, exp %d\n",
               i, mac_low_data[i], ref_low_data[i]);
        exit(-1);
      }
    }

    free_tl(bk_ctx, tl_c_high);
    free_tl(bk_ctx, tl_c_low);
    free_tl(bk_ctx, tl_b);
    free_tl(bk_ctx, tl_a);

    free(a_data);
    free(b_data);
    free(c_high_data);
    free(c_low_data);
    free(ref_high_data);
    free(ref_low_data);
    free(mac_high_data);
    free(mac_low_data);
  }
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_mac(&ctx, bk_ctx, 0);
  test_tl_mac(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
