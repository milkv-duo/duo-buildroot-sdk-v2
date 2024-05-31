#include "1822_test_util.h"

static void put_tensor_g2l_stride_ref(
    u8 *ref, u8 *a,
    tl_shape_t lmem_shape,
    bmk1822_tensor_tgmem_stride_t gmem_stride)
{
  int n = lmem_shape.n;
  int c = lmem_shape.c;
  int h = lmem_shape.h;
  int w = lmem_shape.w;

  int n_str = gmem_stride.n;
  int c_str = gmem_stride.c;
  int h_str = gmem_stride.h;
  int w_str = 1;

  /*
   * put stride ddr tensor to local memory in default stride.
   */

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * n_str + ci * c_str + hi * h_str + wi * w_str;
          u64 dst_i = ni * c * h * w + ci * h * w + hi * w + wi;
          ref[dst_i] = a[src_i];
        }
      }
    }
  }
}

static inline void put_tensor_g2l_stride(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1822_tensor_tgmem_stride_t tg_stride,
    u8 *data)
{
  int n = tl->shape.n;
  int n_stride = tg_stride.n;
  bmshape_t bms = BM_TENSOR_WITH_FMT(n, n_stride, 1, 1, BM_FMT_INT8);
  bmmem_device_t devmem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  int ret = bm_memcpy_s2d(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = bmmem_device_addr(devmem);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape.n = tl->shape.n;
  tg.shape.c = tl->shape.c;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = tg_stride;
  tg.base_reg_index = 0;

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  bmmem_device_free(*ctx, devmem);
}

static void test_put_tensor_g2l_stride(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 2;
  int c = 15;
  int h = 10;
  int w = 8;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  bmk1822_tensor_tgmem_stride_t gmem_stride;
  gmem_stride.h = w * 2;
  gmem_stride.c = gmem_stride.h * h * 2;
  gmem_stride.n = gmem_stride.c * c * 2;

  int size = n * c * h * w;
  int stride_size = gmem_stride.n * n;

  u8 *data_x = (u8 *)xmalloc(stride_size);
  for (int i = 0; i < stride_size; i++)
    data_x[i] = i;

  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 1);
  put_tensor_g2l_stride(ctx, bk_ctx, tl_x, gmem_stride, data_x);
  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  put_tensor_g2l_stride_ref(ref_x, data_x, tl_shape, gmem_stride);

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
  test_put_tensor_g2l_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
