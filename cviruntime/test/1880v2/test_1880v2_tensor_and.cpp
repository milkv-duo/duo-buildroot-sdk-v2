#include "1880v2_test_util.h"

static void tl_and_int8_ref(s8 *a, s8 *b, s8 *res, u64 size)
{
  for (u64 i = 0; i < size; i++)
    res[i] = a[i] & b[i];
}

static void tl_and_int16_ref(
    u8 *ref_high, u8 *ref_low,
    u8 *a_high, u8 *a_low,
    u8 *b_high, u8 *b_low,
    u64 size)
{
  for (u64 i = 0; i < size; i++) {
    s32 ta = ((s8)a_high[i] << 8) + a_low[i];
    s32 tb = ((s8)b_high[i] << 8) + b_low[i];
    s32 res = ta & tb;
    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static void test_tl_and_int8(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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

  s8 *a_data = (s8 *)xmalloc(size);
  for (u64 i = 0; i < size; i++)
    a_data[i] = (s8)(i % 256);

  s8 *b_data = (s8 *)xmalloc(size);
  for (u64 i = 0; i < size; i++)
    b_data[i] = (s8)(100 - i % 256);

  s8 *ref_data = (s8 *)xmalloc(size);
  tl_and_int8_ref(a_data, b_data, ref_data, size);

  tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  tl_t *tl_res = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);

  put_tensor_g2l(ctx, bk_ctx, tl_a, (u8 *)a_data);
  put_tensor_g2l(ctx, bk_ctx, tl_b, (u8 *)b_data);
  bmk1880v2_tiu_element_wise_and_int8_param_t p9;
  memset(&p9, 0, sizeof(p9));
  p9.res = tl_res;
  p9.a = tl_a;
  p9.b = tl_b;
  bmk1880v2_tiu_element_wise_and_int8(bk_ctx, &p9);
  u8 *res_data = get_tensor_l2g(ctx, bk_ctx, tl_res);

  for (u64 i = 0; i < size; i++) {
    if ((s8)res_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_res);
  free_tl(bk_ctx, tl_b);
  free_tl(bk_ctx, tl_a);

  free(a_data);
  free(b_data);
  free(ref_data);
  free(res_data);
}

static void test_tl_and_int16(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  tl_and_int16_ref(
      ref_high_data, ref_low_data,
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
  bmk1880v2_tiu_element_wise_and_int16_param_t p8;
  memset(&p8, 0, sizeof(p8));
  p8.res_high = tl_res_high;
  p8.res_low = tl_res_low;
  p8.a_high = tl_a_high;
  p8.a_low = tl_a_low;
  p8.b_high = tl_b_high;
  p8.b_low = tl_b_low;
  bmk1880v2_tiu_element_wise_and_int16(bk_ctx, &p8);
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
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_and_int8(&ctx, bk_ctx, 0);
  test_tl_and_int8(&ctx, bk_ctx, 1);
  test_tl_and_int16(&ctx, bk_ctx, 0);
  test_tl_and_int16(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
