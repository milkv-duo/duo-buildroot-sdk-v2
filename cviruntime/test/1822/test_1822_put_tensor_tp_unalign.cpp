#include "1822_test_util.h"

static void put_tensor_g2l_tp_unalign_ref(
    u8 *ref, u8 *a, tl_shape_t tl_shape)
{
  /*
   * (c, n, h, w) => (n, c, h, w) => (1, c, n * h, w)
   */

  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;

  int size = n * c * h * w;
  for (int i = 0; i < size; i++)
    ref[i] = a[i];
}


static void put_tensor_g2l_tp(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    u8 *data)
{
  int n = tl->shape.n;
  int c = tl->shape.c;
  int h = tl->shape.h;
  int w = tl->shape.w;

  bmshape_t bms = BM_TENSOR_WITH_FMT(n, c, h, w, BM_FMT_INT8);
  bmmem_device_t devmem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  int ret = bm_memcpy_s2d(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = bmmem_device_addr(devmem);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape.n = tl->shape.c;
  tg.shape.c = tl->shape.n;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = bmk1822_tensor_tgmem_default_stride(tg.shape, tg.fmt);
  tg.base_reg_index = 0 ;

  bmk1822_tdma_tg2l_tensor_copy_nc_transposed_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1822_tdma_g2l_tensor_copy_nc_transposed(bk_ctx, &p);
  test_submit(ctx);

  bmmem_device_free(*ctx, devmem);
}

static void test_put_tensor_g2l_tp_unalign(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 2;
  int c = (BM1822_HW_NPU_NUM - 1);
  int h = 1;
  int w = 8;
  int size = n * c * h * w;

  u8 *data_x = (u8 *)xmalloc(size);
  for (int i = 0; i < size; i++)
    data_x[i] = i;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 0);

  put_tensor_g2l_tp(ctx, bk_ctx, tl_x, data_x);

  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;

  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  tl_x->shape = tl_shape;
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  put_tensor_g2l_tp_unalign_ref(ref_x, data_x, tl_shape);

  for (int i = 0; i < size; i++) {
    if (result_x[i] != ref_x[i]) {
      printf("compare failed at result_x[%d], got %d, exp %d\n",
             i, result_x[i], ref_x[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_x);

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
  test_put_tensor_g2l_tp_unalign(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
