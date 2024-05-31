#include "1822_test_util.h"

static void get_tensor_l2g_stride_ref(
    u8 *ref, u8 *a,
    tl_shape_t tl_shape,
    bmk1822_tensor_tgmem_stride_t tg_stride)
{
  uint32_t n = tl_shape.n;
  uint32_t c = tl_shape.c;
  uint32_t h = tl_shape.h;
  uint32_t w = tl_shape.w;

  uint32_t n_str = tg_stride.n;
  uint32_t c_str = tg_stride.c;
  uint32_t h_str = tg_stride.h;
  uint32_t w_str = 1;

  /*
   * Same as in get_tensor_l2g_stride().
   */
  int stride_size = n * tg_stride.n;
  for (int i = 0; i < stride_size; i++)
    ref[i] = 0xcf;

  for (uint32_t ni = 0; ni < n; ni++) {
    for (uint32_t ci = 0; ci < c; ci++) {
      for (uint32_t hi = 0; hi < h; hi++) {
        for (uint32_t wi = 0; wi < w; wi++) {
          uint32_t src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          uint32_t dst_i = ni * n_str + ci * c_str + hi * h_str + wi * w_str;
          ref[dst_i] = a[src_i];
        }
      }
    }
  }
}

static inline u8 * get_tensor_l2g_stride(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1822_tensor_tgmem_stride_t tg_stride)
{
  int n = tl->shape.n;
  int n_stride = tg_stride.n;

  int stride_size = n * n_stride;
  u8 *data = (u8 *)malloc(sizeof(u8) * stride_size);
  if (!data)
    return NULL;

  for (int i = 0; i < stride_size; i++)
    data[i] = 0xcf;

  bmshape_t bms = BM_TENSOR_WITH_FMT(n, n_stride, 1, 1, BM_FMT_INT8);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape.n = tl->shape.n;
  tg.shape.c = tl->shape.c;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = tg_stride;

  bmk1822_tdma_l2tg_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = &tg;
  bmk1822_tdma_l2g_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  ret = bm_memcpy_d2s(*ctx, data, dev_mem);
  assert(ret == BM_SUCCESS);

  bmmem_device_free(*ctx, dev_mem);
  return data;
}

static void test_get_tensor_l2g_stride(
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

  int size = n * c * h * w;
  u8 *data_x = (u8 *)xmalloc(size);
  for (int i = 0; i < size; i++)
    data_x[i] = i;

  bmk1822_tensor_tgmem_stride_t tg_stride;
  tg_stride.h = w * 2;
  tg_stride.c = tg_stride.h * h * 2;
  tg_stride.n = tg_stride.c * c * 2;
  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 1);

  put_tensor_g2l(ctx, bk_ctx, tl_x, data_x);
  u8 *result_x = get_tensor_l2g_stride(ctx, bk_ctx ,tl_x, tg_stride);

  int stride_size = n * tg_stride.n;
  u8 *ref_x = (u8 *)xmalloc(stride_size);
  if (!result_x || !ref_x)
    goto fail_exit;

  get_tensor_l2g_stride_ref(ref_x, data_x, tl_shape, tg_stride);

  for (int i = 0; i < stride_size; i++) {
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
  test_get_tensor_l2g_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
