#include "1880v2_test_util.h"

static void put_matrix_g2l_stride_ref(u8 *ref,
    u8 *a,
    ml_shape_t  lmem_shape,
    bmk1880v2_matrix_tgmem_stride_t gmem_stride)
{
  int row = lmem_shape.n;
  int col = lmem_shape.col;
  int row_stride = gmem_stride.row;

  for (int ri = 0; ri < row; ri++)
    for (int ci = 0; ci < col; ci++)
      ref[ri * col + ci] = a[ri * row_stride + ci];
}

static void put_matrix_g2l_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    ml_t *ml,
    bmk1880v2_matrix_tgmem_stride_t gmem_stride,
    u8 *data)
{
  int row = ml->shape.n;
  int col = ml->shape.col;
  int row_stride = gmem_stride.row;

  bmshape_t bms = BM_MATRIX_INT8(row, row_stride);
  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = CVI_RT_MemGetPAddr(devmem);
  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = gaddr;
  mg.shape.row = row;
  mg.shape.col = col;
  mg.stride = gmem_stride;
  mg.base_reg_index = 0;

  bmk1880v2_tdma_tg2l_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.dst = ml;
  p.src = &mg;

  bmk1880v2_tdma_g2l_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, devmem);
  return ;
}

static void test_put_matrix_g2l_stride(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int row = 80;
  int col = 70;
  int size = row * col;
  ml_shape_t mls =
      bmk1880v2_matrix_lmem_default_shape(bk_ctx, row, col, FMT_I8);
  ml_t *ml =
      bmk1880v2_lmem_alloc_matrix(bk_ctx,mls, FMT_I8, 0);

  int row_stride = col * 2;
  bmk1880v2_matrix_tgmem_stride_t gmem_stride;
  gmem_stride.row = row_stride;
  int stride_size = row * row_stride;

  u8 *data_x = (u8 *)xmalloc(stride_size);
  for (int i = 0; i < stride_size; i++)
    data_x[i] = i;

  put_matrix_g2l_stride(ctx, bk_ctx, ml, gmem_stride, data_x);
  u8 *result_x = get_matrix_l2g(ctx, bk_ctx, ml);
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  put_matrix_g2l_stride_ref(ref_x, data_x, mls, gmem_stride);

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

int main ()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_put_matrix_g2l_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
