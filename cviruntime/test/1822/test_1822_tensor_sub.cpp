#include "1822_test_util.h"

static void tl_sub_ref(
    u8 *ref_high, u8 *ref_low,
    u8 *a_high, u8 *a_low,
    u8 *b_high, u8 *b_low,
    u64 size)
{
  for (u64 i = 0; i < size; i++) {
    s32 ta = ((s8)a_high[i] << 8) + a_low[i];
    s32 tb = ((s8)b_high[i] << 8) + b_low[i];
    s32 res = ta - tb;
    if (res > 32767)
      res = 32767;
    else if (res < -32768)
      res = -32768;
    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static void test_tl_sub(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  u8 *b_high_data = (u8 *)xmalloc(size);
  u8 *b_low_data = (u8 *)xmalloc(size);
  for (u64 i = 0; i < size; i++) {
    a_high_data[i] = i / 10;
    a_low_data[i] = i;
    b_high_data[i] = (i + 250) / 20;
    b_low_data[i] = 100 - i;
  }

  u8 *ref_high_data = (u8 *)xmalloc(size);
  u8 *ref_low_data = (u8 *)xmalloc(size);
  tl_sub_ref(ref_high_data, ref_low_data,
             a_high_data, a_low_data,
             b_high_data, b_low_data,
             size);

  tl_t *tl_a_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_a_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_b_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_b_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_res_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_res_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);

  put_tensor_g2l(ctx, bk_ctx, tl_a_low, a_low_data);
  put_tensor_g2l(ctx, bk_ctx, tl_a_high, a_high_data);
  put_tensor_g2l(ctx, bk_ctx, tl_b_low, b_low_data);
  put_tensor_g2l(ctx, bk_ctx, tl_b_high, b_high_data);
  bmk1822_tiu_element_wise_sub_param_t p5;
  p5.res_high = tl_res_high;
  p5.res_low = tl_res_low;
  p5.a_high = tl_a_high;
  p5.a_low = tl_a_low;
  p5.b_high = tl_b_high;
  p5.b_low = tl_b_low;
  p5.rshift_bits = 0;
  bmk1822_tiu_element_wise_sub(bk_ctx, &p5);
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
  free_tl(bk_ctx, tl_b_high);
  free_tl(bk_ctx, tl_b_low);
  free_tl(bk_ctx, tl_a_high);
  free_tl(bk_ctx, tl_a_low);

  free(a_high_data);
  free(a_low_data);
  free(b_high_data);
  free(b_low_data);
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

  test_tl_sub(&ctx, bk_ctx, 0);
  test_tl_sub(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
