#include "1880v2_test_util.h"

static void tl_mul_const_ref(
    s8 *ofmap, s8 *ifmap, u64 size, s8 mul_const, int shift_bits, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    s32 tmp = ifmap[i] * mul_const;
    tmp += 1 << (shift_bits - 1);
    tmp >>= shift_bits;
    if (tmp > 127)
      tmp = 127;
    else if (tmp < -128)
      tmp = -128;
    if(relu_enable)
      if(tmp<0)
        tmp=0;

    ofmap[i] = tmp;
  }
}

static void test_tl_mul_const(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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

  u64 size = n * c * h  * w;

  for (u32 relu_enable = 0; relu_enable < 2; relu_enable++)
  {
    s8 *ifmap_data = (s8 *)xmalloc(size);
    for (u64 i = 0; i < size; i++)
      ifmap_data[i] = (u8)(random() % 256);
  
    s8 mul_const = 20;
    int shift_bits = 1;
  
    s8 *ref_data = (s8 *)xmalloc(size);
    tl_mul_const_ref(ref_data, ifmap_data, size, mul_const, shift_bits, relu_enable);
  
    tl_t *tl_ifmap = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
    tl_t *tl_ofmap = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
  
    put_tensor_g2l(ctx, bk_ctx, tl_ifmap, (u8 *)ifmap_data);
  
    bmk1880v2_tiu_element_wise_mul_param_t p;
    memset(&p, 0, sizeof(p));
    p.res_high = NULL;
    p.res_low = tl_ofmap;
    p.a = tl_ifmap;
    p.b_is_const = 1;
    p.b_const.val = mul_const;
    p.b_const.is_signed = 1;
    p.rshift_bits = shift_bits;
    p.relu_enable = relu_enable;

    bmk1880v2_tiu_element_wise_mul(bk_ctx, &p);
  
    u8 *ofmap_data = get_tensor_l2g(ctx, bk_ctx, tl_ofmap);
  
    for (u64 i = 0; i < size; i++) {
      if ((s8)ofmap_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
               i, ofmap_data[i], ref_data[i]);
        exit(-1);
      }
    }
  
    free_tl(bk_ctx, tl_ofmap);
    free_tl(bk_ctx, tl_ifmap);
  
    free(ifmap_data);
    free(ref_data);
    free(ofmap_data);
  }
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_mul_const(&ctx, bk_ctx, 0);
  test_tl_mul_const(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
