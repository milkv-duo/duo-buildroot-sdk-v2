#include "../1822_test_util.h"

static void tl_min_ref(u16 *a, u16 *b, u16 *max, u64 size)
{
  for (u64 i = 0; i < size; i++) {
    float fa = convert_bf16_fp32(a[i]);
    float fb = convert_bf16_fp32(b[i]);
    float fmax;
    if (fa > fb)
      fmax = fb;
    else
      fmax = fa;
    max[i] = convert_fp32_bf16(fmax);
  }
}

static void test_tl_min(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  fmt_t fmt_type = FMT_BF16;
  u64 size = n * c * h * w;
  u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1); 

  u16 *a_data = (u16 *)xmalloc(data_size);
  for (u64 i = 0; i < size; i++)
    a_data[i] = convert_fp32_bf16(rand());

  u16 *b_data = (u16 *)xmalloc(data_size);
  for (u64 i = 0; i < size; i++)
    b_data[i] = convert_fp32_bf16(rand()/2);

  u16 *ref_data = (u16 *)xmalloc(data_size);
  tl_min_ref(a_data, b_data, ref_data, size);

  tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_min = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

  put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, (u16 *)a_data, fmt_type);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_b, (u16 *)b_data, fmt_type);
  bmk1822_tiu_element_wise_min_param_t p6;
  p6.min = tl_min;
  p6.a = tl_a;
  p6.b_is_const = 0;
  p6.b = tl_b;
  bmk1822_tiu_element_wise_min(bk_ctx, &p6);
  u16 *min_data = (u16*)get_bf16_tensor_l2g(ctx, bk_ctx, tl_min, fmt_type);

  for (u64 i = 0; i < size; i++) {
    if (min_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, min_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_min);
  free_tl(bk_ctx, tl_b);
  free_tl(bk_ctx, tl_a);

  free(a_data);
  free(b_data);
  free(ref_data);
  free(min_data);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_min(&ctx, bk_ctx, 0);
  test_tl_min(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
