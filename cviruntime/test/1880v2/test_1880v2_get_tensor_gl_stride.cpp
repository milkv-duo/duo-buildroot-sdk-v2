#include "1880v2_test_util.h"

static void get_tensor_l2g_stride_ref(
    u8 *ref, u8 *a,
    int n, int c, int h, int w,
    bmk1880v2_tensor_lmem_stride_t tl_stride,
    bmk1880v2_tensor_tgmem_stride_t tg_stride)
{
  /*
   * Same as in get_tensor_l2g_stride().
   */
  int stride_size = n * tg_stride.n;
  for (int i = 0; i < stride_size; i++)
    ref[i] = 0xcf;

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i =
              (ni * c + ci) * tl_stride.c +
              hi * tl_stride.h +
              wi * 1;
          u64 dst_i =
              ni * tg_stride.n +
              ci * tg_stride.c +
              hi * tg_stride.h +
              wi * 1;
          ref[dst_i] = a[src_i];
        }
      }
    }
  }
}

static inline u8 * get_tensor_l2g_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1880v2_tensor_tgmem_stride_t tg_stride)
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
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, data);
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
  tg.base_reg_index = 0;

  bmk1880v2_tdma_l2tg_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = &tg;
  bmk1880v2_tdma_l2g_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  ret = CVI_RT_MemCopyD2S(*ctx, data, dev_mem);
  assert(ret == BM_SUCCESS);

  CVI_RT_MemFree(*ctx, dev_mem);
  return data;
}

static void test_get_tensor_l2g_gl_stride(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 2;
  int c = 35;
  int h = 2;
  int w = 3;

  tg_shape_t tg_shape;
  tg_shape.n = n;
  tg_shape.c = c;
  tg_shape.h = h;
  tg_shape.w = w;

  bmk1880v2_tensor_tgmem_stride_t tg_stride =
      bmk1880v2_tensor_tgmem_default_stride(tg_shape, FMT_I8);

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h * w;
  tl_shape.w = 1;

  int size = tl_shape.n * tl_shape.c * tl_shape.h * tl_shape.w;
  u8 *data_x = (u8 *)xmalloc(size);
  for (int i = 0; i < size; i++)
    data_x[i] = i;

  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 0);
  put_tensor_g2l(ctx, bk_ctx, tl_x, data_x);

  tl_x->shape.n = n;
  tl_x->shape.c = c;
  tl_x->shape.h = h;
  tl_x->shape.w = w;

  tl_x->stride = bmk1880v2_tensor_lmem_default_stride(bk_ctx, tl_x->shape, FMT_I8, 0);

  u8 *result_x = get_tensor_l2g_stride(ctx, bk_ctx, tl_x, tg_stride);
  int stride_size = tg_shape.n * tg_stride.n;
  u8 *ref_x = (u8 *)xmalloc(stride_size);
  if (!result_x || !ref_x)
    goto fail_exit;

  get_tensor_l2g_stride_ref(ref_x,
      data_x, tg_shape.n,
      tg_shape.c, tg_shape.h,
      tg_shape.w, tl_x->stride, tg_stride);

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
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_get_tensor_l2g_gl_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
