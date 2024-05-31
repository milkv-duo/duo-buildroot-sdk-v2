#include "1880v2_test_util.h"

static void matrix_tp_ref(
    u8 *ref, u8 *a, ml_shape_t s)
{
  /*
   * ref[] is transposed matrix in lmem.
   * row/col are shape in DDR
   */
  int row = s.col;
  int col = s.n;

  for (int ri = 0; ri < row; ri++) {
    for (int ci = 0; ci < col; ci++) {
      ref[ci * row + ri] = a[ri * col + ci];
    }
  }
}

static void  put_matrix_g2l_tp(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    ml_t *ml,
    u8 *data)
{
  /*
   * raw_row = row of src, raw_col = col of dst.
   * raw and col of ml.shape are transposed raw and col
   */

  int raw_row = ml->shape.col;
  int raw_col = ml->shape.n;

  bmshape_t bms = BM_MATRIX_INT8(raw_row,raw_col);
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, data);
  assert(ret == BM_SUCCESS);

  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = gaddr;
  mg.shape.row = raw_row;
  mg.shape.col = raw_col;
  mg.stride.row = raw_col;
  mg.base_reg_index = 0;

  bmk1880v2_tdma_tg2l_matrix_copy_row_col_transposed_param_t g2lp;
  memset(&g2lp, 0, sizeof(g2lp));
  g2lp.src = &mg;
  g2lp.dst = ml;

  bmk1880v2_tdma_g2l_matrix_copy_row_col_transposed(bk_ctx, &g2lp);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, dev_mem);
  return ;
}

static void test_put_matrix_g2l_tp(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int row = 80;
  int col = 70;
  int size = row * col;

  u8 *data_x = (u8 *)xmalloc(size);
  for (int i = 0; i < size; i++)
    data_x[i] = i;

  ml_shape_t mls =
      bmk1880v2_matrix_lmem_default_shape(bk_ctx, col, row, FMT_I8);
  ml_t *ml =
      bmk1880v2_lmem_alloc_matrix(bk_ctx,mls, FMT_I8, 1);

  put_matrix_g2l_tp(ctx, bk_ctx,ml, data_x);
  u8 *result_x = get_matrix_l2g(ctx, bk_ctx, ml);
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  matrix_tp_ref(ref_x, data_x, mls);

  for (int i = 0; i < size; i++) {
    if (result_x[i] != ref_x[i]) {
      printf("compare failed at result_x[%d], got %d, exp %d\n",
             i, result_x[i], ref_x[i]);
      exit(-1);
    }
  }

  bmk1880v2_lmem_free_matrix(bk_ctx, ml);

fail_exit:
  free(data_x);
  free(result_x);
  free(ref_x);
}

int main (void)
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_put_matrix_g2l_tp(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
