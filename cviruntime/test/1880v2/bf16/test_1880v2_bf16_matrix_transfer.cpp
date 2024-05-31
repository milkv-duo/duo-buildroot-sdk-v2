#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void test_put_and_get_matrix_l2g(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  int row = 5;
  int col = 16 * 5 + 2;
  int size = row * col;
  float val = -100;

  ml_shape_t s = bmk1880v2_matrix_lmem_default_shape(bk_ctx, row, col, fmt);

  u16 *u16data_x = (u16 *)malloc(sizeof(u16) * size);
  u16 *u16data_y = (u16 *)malloc(sizeof(u16) * size);
  s8 *s8data_x = (s8 *)malloc(sizeof(s8) * size);
  s8 *s8data_y = (s8 *)malloc(sizeof(s8) * size);
  void *data_x = NULL;
  void *data_y = NULL;
  u8 *result_x =NULL;
  u8 *result_y = NULL;

  // prepare source data
  for (int i = 0; i < size; i++) {
    if(fmt == FMT_BF16) {
      u16data_x[i] = generate_bf16_corner_val(val);
      u16data_y[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8data_x[i] = i - 100;
      s8data_y[i] = -i;
    }
  }
  data_x =  (fmt == FMT_BF16) ? (void *)u16data_x : (void *)s8data_x;
  data_y =  (fmt == FMT_BF16) ? (void *)u16data_y : (void *)s8data_y;

 // run tpu operations
  ml_t *ml_x = bmk1880v2_lmem_alloc_matrix(bk_ctx, s, fmt, eu_align);
  ml_t *ml_y = bmk1880v2_lmem_alloc_matrix(bk_ctx, s, fmt, eu_align);
  /*
   * Interleave two matrice in case the same devmem is reused between
   * put_matrix_g2l() and get_matrix_l2g(), in which case the content of
   * devmem is already what is expected before bmk1880v2_gdma_store_matrix().
   */
  put_bf16_matrix_g2l(ctx, bk_ctx, ml_x, (u8 *)data_x, fmt);
  put_bf16_matrix_g2l(ctx, bk_ctx, ml_y, (u8 *)data_y, fmt);


  // compare data
  //// Get result_x before result_y.
  result_x = get_bf16_matrix_l2g(ctx, bk_ctx, ml_x, fmt);
  result_y = get_bf16_matrix_l2g(ctx, bk_ctx, ml_y, fmt);
  if( COMPARE_PASS != compare_result( data_x, result_x, fmt, size))
    exit(-1);
  if( COMPARE_PASS != compare_result( data_y, result_y, fmt, size))
    exit(-1);
  free(result_x);
  free(result_y);

  //// Get result_y before result_x.
  result_y = get_bf16_matrix_l2g(ctx, bk_ctx, ml_y, fmt);
  result_x = get_bf16_matrix_l2g(ctx, bk_ctx, ml_x, fmt);
  if( COMPARE_PASS != compare_result( data_x, result_x, fmt, size))
    exit(-1);
  if( COMPARE_PASS != compare_result( data_y, result_y, fmt, size))
    exit(-1);
  free(result_x);
  free(result_y);

  // free variables
  bmk1880v2_lmem_free_matrix(bk_ctx, ml_y);
  bmk1880v2_lmem_free_matrix(bk_ctx, ml_x);
  free(u16data_x);
  free(u16data_y);
  free(s8data_x);
  free(s8data_y);
}

#define TEST_ALIGNED 2 // 1: test unalign only, 2: test both align/unalign
int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);

  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
      test_put_and_get_matrix_l2g(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }

  test_exit(&ctx);
  return 0;
}
