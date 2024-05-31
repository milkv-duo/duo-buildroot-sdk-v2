#include "../1880v2_test_util.h"

static void tl_max_ref(u16 *a, u16 *b, u16 *max, u64 size)
{
  for (u64 i = 0; i < size; i++) {
    float fa = convert_bf16_fp32(a[i]);
    float fb = convert_bf16_fp32(b[i]);
    float fmax;
    if (fa > fb)
      fmax = fa;
    else
      fmax = fb;
    max[i] = convert_fp32_bf16(fmax);
  }
}

static void test_tl_max(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
    a_data[i] = convert_fp32_bf16((s8)(i % 256));

  u16 *b_data = (u16 *)xmalloc(data_size);
  for (u64 i = 0; i < size; i++)
    b_data[i] = convert_fp32_bf16((s8)(100 - i % 256));

  u16 *ref_data = (u16 *)xmalloc(data_size);
  tl_max_ref(a_data, b_data, ref_data, size);

  tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_max = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

  put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, (u16 *)a_data, fmt_type);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_b, (u16 *)b_data, fmt_type);

  bmk1880v2_tiu_element_wise_max_param_t p;
  memset(&p, 0, sizeof(p));
  p.max = tl_max;
  p.a = tl_a;
  p.b_is_const = 0;
  p.b = tl_b;
  bmk1880v2_tiu_element_wise_max(bk_ctx, &p);
  u16 *max_data = (u16*)get_bf16_tensor_l2g(ctx, bk_ctx, tl_max, fmt_type);

  for (u64 i = 0; i < size; i++) {
    if (max_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, max_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_max);
  free_tl(bk_ctx, tl_b);
  free_tl(bk_ctx, tl_a);

  free(a_data);
  free(b_data);
  free(ref_data);
  free(max_data);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_max(&ctx, bk_ctx, 0);
  test_tl_max(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
