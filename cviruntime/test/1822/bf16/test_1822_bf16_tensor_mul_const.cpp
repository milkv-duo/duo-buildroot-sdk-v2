#include "../1822_test_util.h"

static void tl_mul_const_ref(
    u16 *ofmap, u16 *ifmap, u64 size, u16 mul_const, int shift_bits, int relu_enable, fmt_t fmt_type)
{

  if(fmt_type == FMT_BF16) {
    for (u64 i = 0; i < size; i++) {
      float tmp = convert_bf16_fp32(ifmap[i]) * convert_bf16_fp32(mul_const);
      if(relu_enable)
        if(tmp<0)
          tmp=0;
      ofmap[i] = convert_fp32_bf16(tmp);
    }
  } else {
    for (u64 i = 0; i < size; i++) {
      s32 tmp = ifmap[i] * (s16) mul_const;
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
}

static void test_tl_mul_const(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  fmt_t fmt_type = FMT_BF16;
  u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1);
  int shift_bits = 1;

  for (u32 relu_enable = 0; relu_enable < 2; relu_enable++)
  {
    u16 *ifmap_data = (u16 *)xmalloc(data_size);
    for (u64 i = 0; i < size; i++)
      ifmap_data[i] = convert_fp32_bf16(random() % 256);
  
    u16 mul_const = convert_fp32_bf16(20);
  
    u16 *ref_data = (u16 *)xmalloc(data_size);
    tl_mul_const_ref(ref_data, ifmap_data, size, mul_const, shift_bits, relu_enable, fmt_type);
  
    tl_t *tl_ifmap = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_ofmap = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
  
    put_bf16_tensor_g2l(ctx, bk_ctx, tl_ifmap, (u16 *)ifmap_data, fmt_type);
  
    bmk1822_tiu_element_wise_mul_param_t p;
    memset(&p, 0, sizeof(p));
    p.res_high = NULL;
    p.res_low = tl_ofmap;
    p.a = tl_ifmap;
    p.b_is_const = 1;
    p.b_const.val = mul_const;
    p.relu_enable = relu_enable;

    bmk1822_tiu_element_wise_mul(bk_ctx, &p);
  
    u16 *ofmap_data = (u16*) get_bf16_tensor_l2g(ctx, bk_ctx, tl_ofmap, fmt_type);
  
    for (u64 i = 0; i < size; i++) {
      if (ofmap_data[i] != ref_data[i]) {
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
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_mul_const(&ctx, bk_ctx, 0);
  test_tl_mul_const(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
