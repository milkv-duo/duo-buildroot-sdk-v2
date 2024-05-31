#include "1822_test_util.h"

static void test_put_and_get_tensor_l2g(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 2;
  int c = 66;
  int h = 3;
  int w = 15;
  int size = n * c * h * w;
  u8 *data_x = (u8 *)xmalloc(size);
  u8 *data_y = (u8 *)xmalloc(size);

  for (int i = 0; i < size; i++)
    data_x[i] = i - 100;

  for (int i = 0; i < size; i++)
    data_y[i] = -i;

  /*
   * Interleave two tensors in case the same devmem is reused between
   * put_tensor_g2l() and get_tensor_l2g(), in which case the content of
   * devmem is already what is expected before bmk1822_gdma_store(bk_ctx, ).
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

  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 1);
  tl_t *tl_y =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 1);

  tg_t ts_x;
  ts_x.base_reg_index = 0;
  ts_x.start_address = 0;
  ts_x.shape = ts_shape;
  ts_x.stride = bmk1822_tensor_tgmem_default_stride(ts_shape, FMT_I8);

  put_tensor_g2l(ctx, bk_ctx, tl_x, data_x);
  put_tensor_g2l(ctx, bk_ctx, tl_y, data_y);

  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  u8 *result_y = get_tensor_l2g(ctx, bk_ctx, tl_y);

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


  result_y = get_tensor_l2g(ctx, bk_ctx, tl_y);
  result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
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

  free_tl(bk_ctx, tl_y);
  free_tl(bk_ctx, tl_x);
  free(data_x);
  free(data_y);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_put_and_get_tensor_l2g(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
