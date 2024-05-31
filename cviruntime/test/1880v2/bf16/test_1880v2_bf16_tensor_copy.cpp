#include "../1880v2_test_util.h"

static void tl_copy_ref(u16 *a, u16 *res, u64 size, fmt_t fmt_type)
{
  if(fmt_type == FMT_BF16) {
    for (u64 i = 0; i < size; i++)
      res[i] = a[i];
  } else {
    u8* u8res = (u8*) res;
    u8* u8a = (u8*) a;
    for (u64 i = 0; i < size; i++)
      u8res[i] = u8a[i];
  }
}

static void test_tl_copy(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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

  u16 *ref_data = (u16 *)xmalloc(data_size);
  tl_copy_ref(a_data, ref_data, size, fmt_type);

  tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_res = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

  put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, (u16 *)a_data, fmt_type);
  bmk1880v2_tiu_element_wise_copy_param_t p10;
  memset(&p10, 0, sizeof(p10));
  p10.dst = tl_res;
  p10.src = tl_a;
  bmk1880v2_tiu_element_wise_copy(bk_ctx, &p10);
  u16 *res_data = (u16 *)get_bf16_tensor_l2g(ctx, bk_ctx, tl_res, fmt_type);

  for (u64 i = 0; i < size; i++) {
    if (res_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_res);
  free_tl(bk_ctx, tl_a);

  free(a_data);
  free(ref_data);
  free(res_data);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_copy(&ctx, bk_ctx, 0);
  test_tl_copy(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
