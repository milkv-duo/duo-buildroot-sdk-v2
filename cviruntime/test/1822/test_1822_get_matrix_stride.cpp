#include "1822_test_util.h"

static void get_matrix_l2g_stride_ref(
    u8 *ref, u8 *a,
    ml_shape_t ml_shape,
    bmk1822_matrix_tgmem_stride_t gmem_stride)
{
  int row = ml_shape.n;
  int col = ml_shape.col;
  int row_stride = gmem_stride.row;

  /*
   * Same as in get_matrix_l2g_stride().
   */
  int stride_size = row * row_stride;
  for (int i = 0; i < stride_size; i++)
    ref[i] = 0xaf;

  for (int ri = 0; ri < row; ri++) {
    for (int ci = 0; ci < col; ci++) {
      int src_i = ri * col + ci;
      int dst_i = ri * row_stride + ci;
      ref[dst_i] = a[src_i];
    }
  }
}

static u8 * get_matrix_l2g_stride(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    ml_t *ml,
    bmk1822_matrix_tgmem_stride_t mg_stride)
{
  int row = ml->shape.n;
  int row_stride = mg_stride.row;
  int col = ml->shape.col;
  int stride_size = row * row_stride;

  u8 *data = (u8 *)malloc(sizeof(u8) * stride_size);
  if (!data)
    return NULL;

  for (int i = 0; i < stride_size; i++)
    data[i] = 0xaf;

  bmshape_t bms = BM_TENSOR_WITH_FMT(row, row_stride, 1, 1, BM_FMT_INT8);
  bmmem_device_t devmem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  int ret = bm_memcpy_s2d(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = bmmem_device_addr(devmem);
  mg.shape.row = row;
  mg.shape.col = col;
  mg.stride = mg_stride;

  bmk1822_tdma_l2tg_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = ml;
  p.dst = &mg;

  bmk1822_tdma_l2g_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  ret = bm_memcpy_d2s(*ctx, data, devmem);
  assert(ret == BM_SUCCESS);

  bmmem_device_free(*ctx, devmem);
  return data;
}

static void test_get_matrix_l2g_stride(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int row = 80;
  int col = 70;
  int size = row * col;
  int row_stride = col * 2;

  ml_shape_t ml_shape =
      bmk1822_matrix_lmem_default_shape(bk_ctx, row, col, FMT_I8);
  bmk1822_matrix_tgmem_stride_t gmem_stride;
  gmem_stride.row = row_stride;
  int stride_size = row * row_stride;

  u8 *data_x = (u8 *)xmalloc(size);
  if (!data_x)
    return;

  for (int i = 0; i < size; i++)
    data_x[i] = i;

  ml_t *ml_x =
      bmk1822_lmem_alloc_matrix(bk_ctx,ml_shape, FMT_I8, 1);
  put_matrix_g2l(ctx, bk_ctx, ml_x, data_x);
  u8 *result_x = get_matrix_l2g_stride(ctx, bk_ctx, ml_x, gmem_stride);
  u8 *ref_x = (u8 *)xmalloc(stride_size);
  if (!result_x || !ref_x)
    goto fail_exit;

  get_matrix_l2g_stride_ref(ref_x, data_x, ml_shape, gmem_stride);

  for (int i = 0; i < stride_size; i++) {
    if (result_x[i] != ref_x[i]) {
      printf("compare failed at result_x[%d], got %d, exp %d\n",
             i, result_x[i], ref_x[i]);
      exit(-1);
    }
  }

  bmk1822_lmem_free_matrix(bk_ctx, ml_x);

fail_exit:
  free(data_x);
  free(result_x);
  free(ref_x);
}

int main (void)
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_get_matrix_l2g_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
