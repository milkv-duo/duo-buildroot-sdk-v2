#include "1880v2_test_util.h"

static int npu_num = 32;

static u64 shape_size(tl_shape_t s)
{
  return s.n * s.c * s.h * s.w;
}

static tl_shape_t shape_of_stride(
    tl_shape_t tl_shape,
    bmk1880v2_tensor_lmem_stride_t tl_stride)
{
  tl_shape_t shape;
  shape.n = tl_shape.n;
  shape.c = npu_num;
  shape.h = tl_stride.n;
  shape.w = 1;

  return shape;
}

static void tl_copy_with_stride_ref(
    s8 *src,
    s8 *dst,
    tl_shape_t shape,
    bmk1880v2_tensor_lmem_stride_t src_stride,
    bmk1880v2_tensor_lmem_stride_t dst_stride)
{
  int n = shape.n;
  int c = shape.c;
  int h = shape.h;
  int w = shape.w;

  tl_shape_t dst_stride_shape = shape_of_stride(shape, dst_stride);

  u64 dst_size =
      dst_stride_shape.n *
      dst_stride_shape.c *
      dst_stride_shape.h *
      dst_stride_shape.w;

  for (u64 i = 0; i < dst_size; i++)
    dst[i] = 0;

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          int src_i = (ni * npu_num + ci % npu_num) * src_stride.n +
              ci / npu_num * src_stride.c +
              hi * src_stride.h +
              wi;
          int dst_i = (ni * npu_num + ci % npu_num) * dst_stride.n +
              ci / npu_num * dst_stride.c +
              hi * dst_stride.h +
              wi;
          dst[dst_i] = src[src_i];
        }
      }
    }
  }
}

static void test_tl_copy_with_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx)
{
  int n = 3;
  int c = 38;
  int h = 2;
  int w = 3;
  int c_layers = ALIGN(c, npu_num) / npu_num;

  bmk1880v2_tensor_lmem_stride_t src_stride;
  src_stride.w = 1;
  src_stride.h = w + 3;
  src_stride.c = h * src_stride.h + 13;
  src_stride.n = c_layers * src_stride.c + 7;

  bmk1880v2_tensor_lmem_stride_t dst_stride;
  dst_stride.w = 1;
  dst_stride.h = w + 1;
  dst_stride.c = h * dst_stride.h + 5;
  dst_stride.n = c_layers * dst_stride.c + 19;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  tl_shape_t src_stride_shape =
      shape_of_stride(tl_shape, src_stride);

  tl_shape_t dst_stride_shape =
      shape_of_stride(tl_shape, dst_stride);

  u64 src_size = shape_size(src_stride_shape);
  u64 dst_size = shape_size(dst_stride_shape);

  s8 *src_data = (s8 *)xmalloc(src_size);
  for (u64 i = 0; i < src_size; i++)
    src_data[i] = i;

  s8 *dst_init_data = (s8 *)xmalloc(dst_size);
  for (u64 i = 0; i < dst_size; i++)
    dst_init_data[i] = 0;

  tl_t *tl_src = alloc_tl(
      bk_ctx, src_stride_shape, FMT_I8, /*eu_align*/0);

  tl_t *tl_dst = alloc_tl(
      bk_ctx, dst_stride_shape, FMT_I8, /*eu_align*/0);

  put_tensor_g2l(ctx, bk_ctx, tl_src, (u8 *)src_data);
  put_tensor_g2l(ctx, bk_ctx, tl_dst, (u8 *)dst_init_data);

  {
    tl_t src = *tl_src;
    tl_t dst = *tl_dst;
    src.shape = dst.shape = tl_shape;
    src.stride = src_stride;
    dst.stride = dst_stride;
    bmk1880v2_tiu_element_wise_copy_param_t p11;
    memset(&p11, 0, sizeof(p11));
    p11.dst = &dst;
    p11.src = &src;
    bmk1880v2_tiu_element_wise_copy(bk_ctx, &p11);
  }

  u8 *dst_data = get_tensor_l2g(ctx, bk_ctx, tl_dst);

  s8 *ref_data = (s8 *)xmalloc(dst_size);
  tl_copy_with_stride_ref(src_data, ref_data,
                          tl_shape, src_stride, dst_stride);

  for (u64 i = 0; i < dst_size; i++) {
    if ((s8)dst_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at dst_data[%" PRIu64 "], got %x, exp %x\n",
             i, dst_data[i], ref_data[i]);
      exit(-1);
    }
  }

  free_tl(bk_ctx, tl_dst);
  free_tl(bk_ctx, tl_src);

  free(src_data);
  free(dst_init_data);
  free(dst_data);
  free(ref_data);
}

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  test_tl_copy_with_stride(&ctx, bk_ctx);
  test_exit(&ctx);
  return 0;
}
