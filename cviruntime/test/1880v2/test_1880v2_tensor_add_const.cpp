#include "1880v2_test_util.h"

static void tl_add_const_ref(
    u8 *ref_high, u8 *ref_low,
    u8 *a_high, u8 *a_low,
    s16 b, int b_is_signed,
    int rshift_bits,
    u64 size, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    s32 ta = ((s8)a_high[i] << 8) + a_low[i];
    s32 tb = b_is_signed? b: (u16)b;
    s32 res = ta + tb;
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

static void test_tl_add_const(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
{
  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;
  int rshift_bits;
  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {
    u64 size = n * c * h  * w;

    u8 *a_high_data = (u8 *)xmalloc(size);
    u8 *a_low_data = (u8 *)xmalloc(size);
    s16 b;
    int b_is_signed = 1;
    for (u64 i = 0; i < size; i++) {
      a_high_data[i] = rand() % 64+ i;
      a_low_data[i] = i;
    }

    if(relu_enable)
    {
      b=-64;
      rshift_bits = 7;
    }
    else
    {
      b=-278;
      rshift_bits = 1;
    }
    u8 *ref_high_data = (u8 *)xmalloc(size);
    u8 *ref_low_data = (u8 *)xmalloc(size);
    tl_add_const_ref(ref_high_data, ref_low_data,
                     a_high_data, a_low_data,
                     b, b_is_signed, rshift_bits, size,relu_enable);

    tl_t *tl_a_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_a_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_res_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_res_high = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);

    put_tensor_g2l(ctx, bk_ctx, tl_a_low, a_low_data);
    put_tensor_g2l(ctx, bk_ctx, tl_a_high, a_high_data);

    bmk1880v2_tiu_element_wise_add_param_t p4;
    memset(&p4, 0, sizeof(p4));
    p4.res_high = relu_enable ? 0 : tl_res_high;
    p4.res_low = tl_res_low;
    p4.a_high = tl_a_high;
    p4.a_low = tl_a_low;
    p4.b_is_const = 1;
    p4.b_const.val = b;
    p4.b_const.is_signed = b_is_signed;
    p4.rshift_bits = rshift_bits;
    p4.relu_enable = relu_enable;
    bmk1880v2_tiu_element_wise_add(bk_ctx, &p4);

    u8 *res_high_data = get_tensor_l2g(ctx, bk_ctx, tl_res_high);
    u8 *res_low_data = get_tensor_l2g(ctx, bk_ctx, tl_res_low);
    for (u64 i = 0; i < size; i++) {
      if(!relu_enable)
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
    free_tl(bk_ctx, tl_a_high);
    free_tl(bk_ctx, tl_a_low);

    free(a_high_data);
    free(a_low_data);
    free(ref_high_data);
    free(ref_low_data);
    free(res_high_data);
    free(res_low_data);
  }
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_add_const(&ctx, bk_ctx, 0);
  test_tl_add_const(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
