#include "1822_test_util.h"

static void test_put_and_get_matrix_l2g(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int row = 5;
  int col = 16 * 5 + 2;
  int size = row * col;

  ml_shape_t s =
      bmk1822_matrix_lmem_default_shape(bk_ctx, row, col, FMT_I8);

  u8 *data_x = (u8 *)xmalloc(size);
  u8 *data_y = (u8 *)xmalloc(size);

  for (int i = 0; i < size; i++)
    data_x[i] = i - 100;

  for (int i = 0; i < size; i++)
    data_y[i] = -i;

  ml_t *ml_x =
      bmk1822_lmem_alloc_matrix(bk_ctx,s, FMT_I8, 1);
  ml_t *ml_y =
      bmk1822_lmem_alloc_matrix(bk_ctx,s, FMT_I8, 1);

  /*
   * Interleave two matrice in case the same devmem is reused between
   * put_matrix_g2l() and get_matrix_l2g(), in which case the content of
   * devmem is already what is expected before bmk1822_gdma_store_matrix().
   */
  put_matrix_g2l(ctx, bk_ctx, ml_x, data_x);
  put_matrix_g2l(ctx, bk_ctx, ml_y, data_y);

  u8 *result_x = get_matrix_l2g(ctx, bk_ctx, ml_x);
  u8 *result_y = get_matrix_l2g(ctx, bk_ctx, ml_y);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      printf("compare failed at result_x[%d]\n", i);
      exit(-1);
    }
    if (result_y[i] != data_y[i]) {
      printf("compare failed at result_y[%d]\n", i);
      exit(-1);
    }
  }
  free(result_x);
  free(result_y);

  /*
   * Get result_y before result_x.
   */
  result_y = get_matrix_l2g(ctx, bk_ctx, ml_y);
  result_x = get_matrix_l2g(ctx, bk_ctx, ml_x);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      printf("compare failed at result_x[%d]\n", i);
      exit(-1);
    }
    if (result_y[i] != data_y[i]) {
      printf("compare failed at result_y[%d]\n", i);
      exit(-1);
    }
  }
  free(result_x);
  free(result_y);

  bmk1822_lmem_free_matrix(bk_ctx, ml_y);
  bmk1822_lmem_free_matrix(bk_ctx, ml_x);
  free(data_x);
  free(data_y);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_put_and_get_matrix_l2g(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
