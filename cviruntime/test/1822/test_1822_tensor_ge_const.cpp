#include "1822_test_util.h"

static void tl_ge_const_ref(s8 *a, s8 b, s8 *result, u64 size, fmt_t fmt)
{
  for (u64 i = 0; i < size; i++) {
    s32 a32 = (fmt == FMT_I8) ? (s8)a[i] : (u8)a[i];
    s32 b32 = (fmt == FMT_I8) ? (s8)b : (u8)b;
    if (a32 >= b32)
      result[i] = 1;
    else
      result[i] = 0;
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

  u64 size = n * c * h * w;

  for (int i = 0; i < 2; i++) {
    s8 *a_data = (s8 *)xmalloc(size);
    for (u64 i = 0; i < size; i++)
      a_data[i] = (s8)(i % 256);

    s8 b = 47;
    s8 *ref_data = (s8 *)xmalloc(size);

    fmt_t fmt = (i == 1) ? FMT_I8 : FMT_U8;
    tl_ge_const_ref(a_data, b, ref_data, size, fmt);
  
    tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, fmt, eu_align);
    tl_t *tl_ge = alloc_tl(bk_ctx, tl_shape, fmt, eu_align);
  
    put_tensor_g2l(ctx, bk_ctx, tl_a, (u8 *)a_data);
    bmk1822_tiu_element_wise_ge_param_t p;
    memset(&p, 0, sizeof(p));
    p.ge = tl_ge;
    p.a = tl_a;
    p.b_is_const = 1;
    p.b_const.val = b;
    p.b_const.is_signed = i;
    bmk1822_tiu_element_wise_ge(bk_ctx, &p);
    u8 *ge_data = get_tensor_l2g(ctx, bk_ctx, tl_ge);

    for (u64 i = 0; i < size; i++) {
      if ((s8)ge_data[i] != (s8)ref_data[i]) {
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
