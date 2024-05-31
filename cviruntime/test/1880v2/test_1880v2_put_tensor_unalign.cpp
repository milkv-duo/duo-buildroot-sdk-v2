#include "1880v2_test_util.h"

static void put_tensor_g2l_unalign_ref(
    u8 *ref, u8 *a, tl_shape_t tl_shape)
{
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;

  /*
   * (n, c, h, w) => (1, c, n * h, w)
   */
  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u64 dst_i = ci * n * h * w + ni * h * w + hi * w + wi;
          ref[dst_i] = a[src_i];
        }
      }
    }
  }
}

static void test_put_tensor_g2l_unalign(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx)
{
  int n = 4;
  int c = 9; //just larger than (npu_num/2)
  int h = 1;
  int w = 8;
  int size = n * c * h * w;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  u8 *data_x = (u8 *)xmalloc(size);
  for (int i = 0; i < size; i++)
    data_x[i] = i;

  tl_t *tl_x =
      alloc_tl(bk_ctx, tl_shape, FMT_I8, 0);
  put_tensor_g2l(ctx, bk_ctx, tl_x, data_x);

  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;

  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  u8 *ref_x = (u8 *)xmalloc(size);
  if (!result_x || !ref_x)
    goto fail_exit;

  put_tensor_g2l_unalign_ref(ref_x, data_x, tl_shape);

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
  test_put_tensor_g2l_unalign(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
