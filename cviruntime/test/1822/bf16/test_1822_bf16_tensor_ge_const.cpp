#include "../1822_test_util.h"

static void tl_ge_const_ref(u16 *a, u16 b, u16 *result, u64 size)
{
  for (u64 i = 0; i < size; i++) {
    float fa = convert_bf16_fp32(a[i]);
    float fb = convert_bf16_fp32(b);
    float fge;
    if (fa >= fb)
      fge = 1;
    else
      fge = 0;
    result[i] = convert_fp32_bf16(fge);
  }
}

static void test_tl_ge_const(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  u64 size = n * c * h  * w;
  u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1);

  u16 *a_data = (u16 *)xmalloc(data_size);
  for (u64 i = 0; i < size; i++)
    a_data[i] = convert_fp32_bf16(i);
    //a_data[i] = convert_fp32_bf16(rand()%100 - 50);

  u16 b = convert_fp32_bf16(20);
  u16 *ref_data = (u16 *)xmalloc(data_size);
  tl_ge_const_ref(a_data, b, ref_data, size);

  tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  tl_t *tl_ge = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_a, (u16 *)a_data, fmt_type);
  bmk1822_tiu_element_wise_ge_param_t p;
  memset(&p, 0, sizeof(p));
  p.ge = tl_ge;
  p.a = tl_a;
  p.b_is_const = 1;
  p.b_const.val = b;
  p.b_const.is_signed = 1;

  bmk1822_tiu_bf16_element_wise_ge(bk_ctx, &p);

  u16 *ge_data = (u16*) get_bf16_tensor_l2g(ctx, bk_ctx, tl_ge, fmt_type);

  for (u64 i = 0; i < size; i++) {
    if (ge_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, ge_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_ge);
  free_tl(bk_ctx, tl_a);

  free(a_data);
  free(ref_data);
  free(ge_data);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_ge_const(&ctx, bk_ctx, 0);
  test_tl_ge_const(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
