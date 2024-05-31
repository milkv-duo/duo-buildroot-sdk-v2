#include "../1822_test_util.h"
#include "1822_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static int npu_num = BM1822_HW_NPU_NUM;

static u64 shape_size(tl_shape_t s)
{
  return s.n * s.c * s.h * s.w;
}

static tl_shape_t shape_of_stride(
    tl_shape_t tl_shape,
    bmk1822_tensor_lmem_stride_t tl_stride,
    fmt_t fmt)
{
  tl_shape_t shape;
  shape.n = tl_shape.n;
  shape.c = npu_num;
  shape.h = tl_stride.n / ((fmt == FMT_BF16) ?2:1);
  shape.w = 1;
  return shape;
}

static void tl_copy_with_stride_ref(
    void *src,
    void *dst,
    tl_shape_t shape,
    bmk1822_tensor_lmem_stride_t src_stride,
    bmk1822_tensor_lmem_stride_t dst_stride,
    fmt_t fmt)
{
  int nsrc_byte = ((fmt == FMT_BF16) ? 2 : 1);
  int n = shape.n;
  int c = shape.c;
  int h = shape.h;
  int w = shape.w;

  tl_shape_t dst_stride_shape = shape_of_stride(shape, dst_stride, fmt);
  
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;

  int dst_size = 
      dst_stride_shape.n *
      dst_stride_shape.c *
      dst_stride_shape.h *
      dst_stride_shape.w;

  if (fmt == FMT_BF16) {
    u16_ref = (u16 *)dst;
    u16_src = (u16 *)src;
  } else {
    u8_ref = (u8 *)dst;
    u8_src = (u8 *)src;
  }

  for (int i = 0; i < dst_size; i++) {
    if (fmt == FMT_BF16) {
      u16_ref[i] = 0x0;
    } else {
      u8_ref[i] = 0x0;
    }
  }

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          int src_i = (ni * npu_num + ci % npu_num) * src_stride.n / nsrc_byte +
              ci / npu_num * src_stride.c / nsrc_byte +
              hi * src_stride.h / nsrc_byte +
              wi;
          int dst_i = (ni * npu_num + ci % npu_num) * dst_stride.n / nsrc_byte +
              ci / npu_num * dst_stride.c / nsrc_byte +
              hi * dst_stride.h / nsrc_byte +
              wi;
          if (fmt == FMT_BF16) {
            u16_ref[dst_i] = u16_src[src_i];
          } else {
            u8_ref[dst_i] = u8_src[src_i];
          }
        }
      }
    }
  }

  dst =  (fmt == FMT_BF16) ? (void *)u16_ref : (void *)u8_ref;
}

static void test_tl_copy_with_stride(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    fmt_t fmt,
    int eu_align)
{
  int n = 3;
  int c = 38;
  int h = 2;
  int w = 3;
  int c_layers = ALIGN(c, npu_num) / npu_num;
  int nsrc_byte = ((fmt == FMT_BF16) ? 2 : 1);

  bmk1822_tensor_lmem_stride_t src_stride;
  src_stride.w = nsrc_byte;
  src_stride.h = (w + 3) * nsrc_byte;
  src_stride.c = h * src_stride.h + (13 * nsrc_byte);
  src_stride.n = c_layers * src_stride.c + (7 * nsrc_byte);

  bmk1822_tensor_lmem_stride_t dst_stride;
  dst_stride.w = nsrc_byte;
  dst_stride.h = (w + 1) * nsrc_byte;
  dst_stride.c = h * dst_stride.h + (5 * nsrc_byte);
  dst_stride.n = c_layers * dst_stride.c + (19 * nsrc_byte);

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  tl_shape_t src_stride_shape = shape_of_stride(tl_shape, src_stride, fmt);
  tl_shape_t dst_stride_shape = shape_of_stride(tl_shape, dst_stride, fmt);

  int src_size = shape_size(src_stride_shape);
  int dst_size = shape_size(dst_stride_shape);

  float val = -100;
  void *src_data = NULL;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * src_size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * src_size);
  void *dst_data = NULL;
  u16 *u16dst_data = (u16 *)malloc(sizeof(u16) * dst_size);
  s8 *s8dst_data = (s8 *)malloc(sizeof(s8) * dst_size);
  u8 *result_x = NULL;
  u8 *ref_x = NULL;

  // prepare source data
  ref_x = (u8 *)xmalloc(dst_size * nsrc_byte);
  for (int i = 0; i < src_size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = convert_fp32_bf16(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  for (int i = 0; i < dst_size; i++) {
      u16dst_data[i] = s8dst_data[i] = 0;
  }
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;
  dst_data =  (fmt == FMT_BF16) ? (void *)u16dst_data : (void *)s8dst_data;

  // run tpu operations
  tl_t *tl_src = alloc_tl( bk_ctx, src_stride_shape, fmt, eu_align);
  tl_t *tl_dst = alloc_tl( bk_ctx, dst_stride_shape, fmt, eu_align);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_src, (u16 *)src_data, fmt);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_dst, (u16 *)dst_data, fmt);
  {
    tl_t src = *tl_src;
    tl_t dst = *tl_dst;
    src.shape = dst.shape = tl_shape;
    src.stride = src_stride;
    dst.stride = dst_stride;
    bmk1822_tiu_element_wise_copy_param_t p11;
    p11.dst = &dst;
    p11.src = &src;
    bmk1822_tiu_element_wise_copy(bk_ctx, &p11);
    
  }
  result_x = get_bf16_tensor_l2g(ctx, bk_ctx, tl_dst, fmt);

  tl_copy_with_stride_ref(src_data, ref_x, tl_shape, src_stride, dst_stride, fmt);

  // compare data
  if( COMPARE_PASS != compare_result( ref_x, result_x, fmt, dst_size))
    exit(-1);
  
  // free variables
  free_tl(bk_ctx, tl_dst);
  free_tl(bk_ctx, tl_src);
  free(s8src_data);
  free(u16src_data);
  free(s8dst_data);
  free(u16dst_data);
  free(result_x);
  free(ref_x);
}

#define TEST_ALIGNED 1 // 1: test unalign only, 2: test both align/unalign
int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);

  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
      test_tl_copy_with_stride(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }

  test_exit(&ctx);
  return 0;
}
