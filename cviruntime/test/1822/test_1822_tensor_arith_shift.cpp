#include "1822_test_util.h"

static void tl_arith_shift_ref(
    u8 *ref_high, u8 *ref_low,
    u8 *a_high, u8 *a_low,
    u8 *bits, u64 size)
{
  for (u64 i = 0; i < size; i++) {
    s32 ta = ((s8)a_high[i] << 8) + a_low[i];
    s32 tbits = (s8)bits[i];

    /*
     * Yes, a @tbits bigger than zero means shifting LEFT,
     * no matter whether the shift type is arithmetic
     * RIGHT shift or logic RIGHT shift.
     */
    s32 res;
    if (tbits >= 0)
      res = ta << tbits;
    else
      res = ta >> -tbits;

    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static void test_tl_arith_shift(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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

  u64 size = n * c * h * w;

  u8 *a_high_data = (u8 *)xmalloc(size);
  u8 *a_low_data = (u8 *)xmalloc(size);
  u8 *bits_data = (u8 *)xmalloc(size);
  for (u64 i = 0; i < size; i++) {
    a_high_data[i] = 240 + i;
    a_low_data[i] = 200 + i;
    bits_data[i] = (i % 33) - 16;
  }

  u8 *ref_high_data = (u8 *)xmalloc(size);
  u8 *ref_low_data = (u8 *)xmalloc(size);
  tl_arith_shift_ref(
      ref_high_data, ref_low_data,
      a_high_data, a_low_data,
      bits_data, size);

  tl_t *tl_a_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_a_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_bits = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_res_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_res_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);

  put_tensor_g2l(ctx, bk_ctx, tl_a_low, a_low_data);
  put_tensor_g2l(ctx, bk_ctx, tl_a_high, a_high_data);
  put_tensor_g2l(ctx, bk_ctx, tl_bits, bits_data);
  bmk1822_tiu_element_wise_arith_shift_param_t p8;
  p8.res_high = tl_res_high;
  p8.res_low = tl_res_low;
  p8.a_high = tl_a_high;
  p8.a_low = tl_a_low;
  p8.bits = tl_bits;
  bmk1822_tiu_element_wise_arith_shift(bk_ctx, &p8);
  u8 *res_high_data = get_tensor_l2g(ctx, bk_ctx, tl_res_high);
  u8 *res_low_data = get_tensor_l2g(ctx, bk_ctx, tl_res_low);

  for (u64 i = 0; i < size; i++) {
    if (res_high_data[i] != ref_high_data[i]) {
      fprintf(stderr, "comparing failed at res_high_data[%" PRIu64 "], got %d, exp %d\n",
             i, res_high_data[i], ref_high_data[i]);
      exit(-1);
    }
    if (res_low_data[i] != ref_low_data[i]) {
      fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %d, exp %d\n",
             i, res_low_data[i], ref_low_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_res_high);
  free_tl(bk_ctx, tl_res_low);
  free_tl(bk_ctx, tl_bits);
  free_tl(bk_ctx, tl_a_high);
  free_tl(bk_ctx, tl_a_low);

  free(a_high_data);
  free(a_low_data);
  free(bits_data);
  free(ref_high_data);
  free(ref_low_data);
  free(res_high_data);
  free(res_low_data);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_arith_shift(&ctx, bk_ctx, 0);
  test_tl_arith_shift(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
