#include "../1822_test_util.h"
#include "1822_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void put_tensor_g2l_unalign_ref(
    void *ref,
    void *a,
    tl_shape_t tl_shape,
    fmt_t fmt)
{
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;

  if (fmt == FMT_BF16) {
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
  } else {
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
  }
  /*
   * (n, c, h, w) => (1, c, n * h, w)
   */
  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u64 dst_i = ci * n * h * w + ni * h * w + hi * w + wi;
          if (fmt == FMT_BF16) {
            u16_ref[dst_i] = u16_src[src_i];
          } else {
            u8_ref[dst_i] = u8_src[src_i];
          }
        }
      }
    }
  }

  if (fmt == FMT_BF16) {
    ref = (void *)u16_ref;
  } else {
    ref = (void *)u8_ref;
  }
}

static void test_put_tensor_g2l_unalign(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  tl_t *tl_x = NULL;
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
  float val = -100;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * size);
  void *src_data;
  void *result_x = NULL;
  void *ref_x = NULL;
  u8 *u8ref_x = NULL;
  u16 *u16ref_x = NULL;

  // prepare source data
  ref_x = (u8 *)xmalloc(size * ((fmt == FMT_BF16) ?2:1) );
  for (int i = 0; i < size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  tl_x = alloc_tl(bk_ctx, tl_shape, fmt, eu_align);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_x, (u16 *)src_data, fmt);
  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;
  result_x = get_bf16_tensor_l2g(ctx, bk_ctx, tl_x, fmt);
  put_tensor_g2l_unalign_ref(ref_x, src_data, tl_shape, fmt);

   // compare data
  compare_result( ref_x, result_x, fmt, size);

  // free variables
  free_tl(bk_ctx, tl_x);
  free(ref_x);
  free(u8ref_x);
  free(u16ref_x);
  free(s8src_data);
  free(u16src_data);
  free(result_x);
}

#define TEST_ALIGNED 1 // 1: test unalign only, 2: test both align/unalign
int main (void)
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
      test_put_tensor_g2l_unalign(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);

  return 0;
}
