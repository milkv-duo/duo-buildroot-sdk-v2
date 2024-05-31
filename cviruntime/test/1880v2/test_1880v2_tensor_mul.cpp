#include "1880v2_test_util.h"

static void tl_mul_ref(s8 *ofmap, s8 *a, s8 *b, u64 size, int shift_bits, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    s32 tmp = a[i] * b[i];
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

static void test_tl_mul(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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
  int shift_bits = 1;

  for (u32 relu_enable = 0; relu_enable < 2; relu_enable++)
  {
     s8 *a_data = (s8 *)xmalloc(size);
     s8 *b_data = (s8 *)xmalloc(size);
     for (u64 i = 0; i < size; i++) {
       a_data[i] = random()%0x10;
       b_data[i] = 128 - i;
     }
   
     tl_t *tl_a = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
     tl_t *tl_b = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
     tl_t *tl_res_low = alloc_tl(bk_ctx, tl_shape, FMT_I8, eu_align);
   
     put_tensor_g2l(ctx, bk_ctx, tl_a, (u8 *)a_data);
     put_tensor_g2l(ctx, bk_ctx, tl_b, (u8 *)b_data);
   
     bmk1880v2_tiu_element_wise_mul_param_t p1;
     memset(&p1, 0, sizeof(p1));
     p1.res_high = NULL;
     p1.res_low = tl_res_low;
     p1.a = tl_a;
     p1.b_is_const = 0;
     p1.b = tl_b;
     p1.rshift_bits = shift_bits;
     p1.relu_enable = relu_enable;
     bmk1880v2_tiu_element_wise_mul(bk_ctx, &p1);
   
     u8 *res_low_data = get_tensor_l2g(ctx, bk_ctx, tl_res_low);
   
     s8 *ref_data = (s8 *)xmalloc(size);
     tl_mul_ref(ref_data, a_data, b_data, size, shift_bits, relu_enable);
   
     for (u64 i = 0; i < size; i++) {
       if ((s8)res_low_data[i] != ref_data[i]) {
         fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %x, exp %x\n",
                i, res_low_data[i], ref_data[i]);
         exit(-1);
       }
     }
   
     free_tl(bk_ctx, tl_res_low);
     free_tl(bk_ctx, tl_b);
     free_tl(bk_ctx, tl_a);
   
     free(a_data);
     free(b_data);
     free(ref_data);
     free(res_low_data);
  }
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  test_tl_mul(&ctx, bk_ctx, 0);
  test_tl_mul(&ctx, bk_ctx, 1);

  test_exit(&ctx);
  return 0;
}
