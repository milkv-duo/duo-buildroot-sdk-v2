#include "../1822_test_util.h"

static void tl_add_ref(
    u16 *ref_low,
    u16 *a_low,
    u16 *b_low,
    u64 size, int relu_enable)
{
  for (u64 i = 0; i < size; i++) {
    float ta = convert_bf16_fp32(a_low[i]);
    float tb = convert_bf16_fp32(b_low[i]);
    float res = ta + tb;
    if(relu_enable && res <0)
        res = 0;
    ref_low[i] = convert_fp32_bf16(res);
  }
}

static void test_tl_add(bmctx_t *ctx, bmk_ctx_t *bk_ctx, int eu_align)
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

  fmt_t fmt_type = FMT_BF16;
  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {
    u64 size = n * c * h  * w;
    u64 data_size = size * (fmt_type == FMT_BF16 ? 2 : 1);
    fmt_t fmt_type = FMT_BF16;
    u16 *a_low_data = (u16 *)xmalloc(data_size);
    u16 *b_low_data = (u16 *)xmalloc(data_size);

    for (u64 i = 0; i < size; i++) {
      a_low_data[i] = convert_fp32_bf16(i);
      b_low_data[i] = convert_fp32_bf16(i);
    }
    rshift_bits = 0;

    u16 *ref_low_data = (u16 *)xmalloc(data_size);

    tl_add_ref(ref_low_data,
               a_low_data,
               b_low_data,
               size, relu_enable);

    tl_t *tl_a_low = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_b_low = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);
    tl_t *tl_res_low = alloc_tl(bk_ctx, tl_shape, fmt_type, eu_align);

    put_bf16_tensor_g2l(ctx, bk_ctx, tl_a_low, (u16*)a_low_data, fmt_type);
    put_bf16_tensor_g2l(ctx, bk_ctx, tl_b_low, (u16*)b_low_data, fmt_type);
    bmk1822_tiu_element_wise_add_param_t p4;
    p4.res_high = 0;
    p4.res_low = tl_res_low;
    p4.a_high = 0;
    p4.a_low = tl_a_low;
    p4.b_is_const = 0;
    p4.b_high = 0;
    p4.b_low = tl_b_low;
    p4.rshift_bits = rshift_bits;
    p4.relu_enable = relu_enable;
    bmk1822_tiu_element_wise_add(bk_ctx, &p4);
    u16 *res_low_data = (u16*)get_bf16_tensor_l2g(ctx, bk_ctx, tl_res_low, fmt_type);
    
    for (u64 i = 0; i < size; i++) {
      if (res_low_data[i] != ref_low_data[i]) {
        fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %x, exp %x\n",
               i, res_low_data[i], ref_low_data[i]);
        exit(-1);
      }
    }

    free_tl(bk_ctx, tl_res_low);
    free_tl(bk_ctx, tl_b_low);
    free_tl(bk_ctx, tl_a_low);

    free(a_low_data);
    free(b_low_data);
    free(ref_low_data);
    free(res_low_data);
  }
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();

  test_tl_add(&ctx, bk_ctx, 0);
  test_tl_add(&ctx, bk_ctx, 1);

  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
