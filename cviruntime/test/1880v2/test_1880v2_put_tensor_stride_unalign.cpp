#include "1880v2_test_util.h"

static void put_tensor_g2l_stride_unalign_ref(
    u8 *ref, u8 *a, tl_shape_t tl_shape,
    bmk1880v2_tensor_tgmem_stride_t gmem_stride)
{
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;

  int n_str = gmem_stride.n;
  int c_str = gmem_stride.c;
  int h_str = gmem_stride.h;
  int w_str = 1;

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * n_str + ci * c_str + hi * h_str + wi * w_str;
          u64 dst_i = ci * n * h * w + ni * h * w + hi * w + wi;
          ref[dst_i] = a[src_i];
        }
      }
    }
  }
}

static inline void put_tensor_g2l_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1880v2_tensor_tgmem_stride_t tg_stride,
    u8 *data)
{
  int n = tl->shape.n;
  int n_stride = tg_stride.n;
  bmshape_t bms = BM_TENSOR_WITH_FMT(n, n_stride, 1, 1, BM_FMT_INT8);
  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = CVI_RT_MemGetPAddr(devmem);

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

  bmk1880v2_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1880v2_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, devmem);
}

static void test_put_tensor_g2l_stride_unalign(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 6;
  int c = 9; //just larger than (npu_num/2)
  int h = 1;
  int w = 8;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  bmk1880v2_tensor_tgmem_stride_t gmem_stride;
  gmem_stride.h = w * 2;
  gmem_stride.c = gmem_stride.h * h * 2;
  gmem_stride.n = gmem_stride.c * c * 2;

  int size = n * c * h * w;
  int stride_size = gmem_stride.n * n;

  u8 *data_x = (u8 *)xmalloc(stride_size);
  for (int i = 0; i < stride_size; i++)
    data_x[i] = i;


  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 0);

  put_tensor_g2l_stride(ctx, bk_ctx, tl_x, gmem_stride, data_x);

  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;

  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  put_tensor_g2l_stride_unalign_ref(ref_x, data_x, tl_shape, gmem_stride);

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
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_put_tensor_g2l_stride_unalign(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
