#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void test_put_and_get_tensor_l2g(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    fmt_t fmt)
{
  int n = 2;
  int c = 66;
  int h = 3;
  int w = 15;
  u64 size = n * c * h * w;
  s8 *s8data_x = (s8 *)malloc(sizeof(s8) * size);
  s8 *s8data_y = (s8 *)malloc(sizeof(s8) * size);
  u16 *u16data_x = (u16 *)malloc(sizeof(u16) * size);
  u16 *u16data_y = (u16 *)malloc(sizeof(u16) * size);
  u8 *u8src_data_x;
  u8 *u8src_data_y;

  if(fmt == FMT_BF16) {
    /* bf16*/
    float val = -100;
    for(u64 i = 0; i < size; i++) {
      u16data_x[i] = generate_bf16_corner_val(val);
      u16data_y[i] = generate_bf16_corner_val(val);
      val += 0.1;
    }
    u8src_data_x = (u8 *)u16data_x;
    u8src_data_y = (u8 *)u16data_y;
  } else {
    /* int8 -> bf16*/
    for(u64 i = 0; i < size; i++) {
      s8data_x[i] = i-100;
      s8data_y[i] = -i;
    }
    u8src_data_x = (u8 *)s8data_x;
    u8src_data_y = (u8 *)s8data_y;
  }
  /*
   * Interleave two tensors in case the same devmem is reused between
   * put_tensor_g2l() and get_tensor_l2g(), in which case the content of
   * devmem is already what is expected before bmk1880v2_gdma_store(bk_ctx, ).
   */
  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  tg_shape_t ts_shape;
  ts_shape.n = n;
  ts_shape.c = c;
  ts_shape.h = h;
  ts_shape.w = w;

  tl_t *tl_x = alloc_tl( bk_ctx, tl_shape, fmt, 1);
  tl_t *tl_y = alloc_tl( bk_ctx, tl_shape, fmt, 1);

  tg_t ts_x;
  ts_x.base_reg_index = 0;
  ts_x.start_address = 0;
  ts_x.shape = ts_shape;
  ts_x.stride = bmk1880v2_tensor_tgmem_default_stride(ts_shape, fmt);

  put_bf16_tensor_g2l( ctx, bk_ctx, tl_x, (u16 *)u8src_data_x, fmt);
  put_bf16_tensor_g2l( ctx, bk_ctx, tl_y, (u16 *)u8src_data_y, fmt);

  u8 *result_x = get_bf16_tensor_l2g( ctx, bk_ctx, tl_x, fmt);
  u8 *result_y = get_bf16_tensor_l2g( ctx, bk_ctx, tl_y, fmt);

  for (u64 i = 0; i < size; i++) {
    if (result_x[i] != u8src_data_x[i]) {
      printf("compare 1 failed at result_x[%d]\n", (int)i);
      exit(-1);
    }
    if (result_y[i] != u8src_data_y[i]) {
      printf("compare 1 failed at result_y[%d]\n", (int)i);
      exit(-1);
    }
  }
  free(result_x);
  free(result_y);

  /*
   * Get result_y before result_x.
   */


  result_y = get_bf16_tensor_l2g(ctx, bk_ctx, tl_y, fmt);
  result_x = get_bf16_tensor_l2g(ctx, bk_ctx, tl_x, fmt);
  for (u64 i = 0; i < size; i++) {
    if (result_x[i] != u8src_data_x[i]) {
      printf("compare 2 failed at result_x[%d]\n", (int)i);
      exit(-1);
    }
    if (result_y[i] != u8src_data_y[i]) {
      printf("compare 2 failed at result_y[%d]\n", (int)i);
      exit(-1);
    }
  }
  free(result_x);
  free(result_y);

  free_tl(bk_ctx, tl_y);
  free_tl(bk_ctx, tl_x);

  free(s8data_x);
  free(s8data_y);
  free(u16data_x);
  free(u16data_y);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  
  for (u32 i = 0; i < nr_fmt; i++) {
    test_put_and_get_tensor_l2g(&ctx, bk_ctx, input_fmt[i].src_fmt);
  }
  test_exit(&ctx);

  return 0;
}
