#include "../1822_test_util.h"
#include "1822_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void get_tensor_l2g_stride_unalign_ref(
    void *ref,
    void *a,
    tl_shape_t tl_shape,
    bmk1822_tensor_tgmem_stride_t gmem_stride,
    fmt_t fmt)
{
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;
  int nsrc_byte = 1;
  int new_n = n * 2;
  int new_h = h / 2;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;

  if (fmt == FMT_BF16) {
    nsrc_byte = 2; // FMT_BF16
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
  } else {
    nsrc_byte = 1; // FMT_U8, FMT_I8
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
  }
  int n_str = gmem_stride.n / nsrc_byte;
  int c_str = gmem_stride.c / nsrc_byte;
  int h_str = gmem_stride.h / nsrc_byte;
  /*
   * Same as in get_tensor_l2g_stride_unalign().
   */
  int stride_size = new_n * gmem_stride.n;
  for (int i = 0; i < stride_size; i++) {
    if (fmt == FMT_BF16) {
      u16_ref[i] = 0xcf;
    } else {
      u8_ref[i] = 0xcf;
    }
  }
  /*
   * (n, c, h, w) => (n * 2, c, h / 2, w)
   */
  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = ni * c * h * w + ci * h * w + hi * w + wi;
          u64 dst_i = (ni * 2 + hi / new_h) * n_str +
              ci * c_str + (hi % new_h) * h_str + wi;
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

static inline u8 * get_tensor_l2g_stride(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1822_tensor_tgmem_stride_t tg_stride,
    fmt_t fmt)
{
  bmk1822_tdma_l2tg_tensor_copy_param_t p;
  int n = tl->shape.n;
  int n_stride = tg_stride.n;
  int stride_size = n * n_stride;
  u8 *data = NULL;
  u16 *u16_data = (u16 *)malloc(sizeof(u16) * stride_size);
  u8 *u8_data = (u8 *)malloc(sizeof(u8) * stride_size);
  if (!u16_data || !u8_data) {
    free(u16_data);
    free(u8_data);
    return NULL;
  }

  for (int i = 0; i < stride_size; i++) {
    if (fmt == FMT_BF16) {
      u16_data[i] = 0xcf;
    } else {
      u8_data[i] = 0xcf;
    }
  }

  if (fmt == FMT_BF16) {
    data = (u8 *)u16_data;
    free(u8_data);
  } else {
    data = u8_data;
    free(u16_data);
  }

  bmshape_t bms = BM_TENSOR_WITH_FMT(n, n_stride, 1, 1,
                                     (fmt == FMT_BF16)? BM_FMT_BF16 : BM_FMT_INT8);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  int ret = bm_memcpy_s2d(*ctx, dev_mem, (u8 *)data);
  assert(ret == BM_SUCCESS);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = fmt;
  tg.shape.n = tl->shape.n;
  tg.shape.c = tl->shape.c;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = tg_stride;
  tg.base_reg_index = 0;

  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = &tg;

  bmk1822_tdma_l2g_bf16_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  ret = bm_memcpy_d2s(*ctx, (u8 *)data, dev_mem);
  assert(ret == BM_SUCCESS);
  bmmem_device_free(*ctx, dev_mem);

  return data;
}

static void test_get_tensor_l2g_stride_unalign(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt)
{
  bmk1822_tensor_tgmem_stride_t tg_stride;
  /*
   * Make sure (h / 2 * w) is not eu-aligned.
   */
  int n = 1;
  int c = 5;
  int h = 18;
  int w = 7;
  tl_t *tl_x = NULL;
  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;
  int new_n = n * 2;
  int new_h = h / 2;
  tg_stride.h = w * 2;
  tg_stride.c = w * 2 * new_h * 2;
  tg_stride.n = w * 2 * new_h * 2 * c * 2;

  float val = -100;
  int stride_size = new_n * tg_stride.n;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * stride_size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * stride_size);
  void *src_data = NULL;
  u8 *result_x = NULL;
  void *ref_x = NULL;
  u8 *u8ref_x = NULL;
  u16 *u16ref_x = NULL;

  // prepare source data
  ref_x = (u8 *)xmalloc(stride_size * ((fmt == FMT_BF16) ?2:1) );
  for (int i = 0; i < stride_size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  tl_x = alloc_tl(bk_ctx, tl_shape, fmt, 1);
  put_bf16_tensor_g2l(ctx, bk_ctx, tl_x, (u16 *)src_data, fmt);
  tl_x->shape.n = new_n;
  tl_x->shape.c = c;
  tl_x->shape.h = new_h;
  tl_x->shape.w = w;
  tl_x->stride = bmk1822_tensor_lmem_default_stride(bk_ctx, tl_x->shape, fmt, 0);
  result_x = get_tensor_l2g_stride(ctx, bk_ctx, tl_x, tg_stride, fmt);
  tl_x->shape = tl_shape;
  tl_x->stride = bmk1822_tensor_lmem_default_stride(bk_ctx, tl_x->shape, fmt, 1);
  get_tensor_l2g_stride_unalign_ref(ref_x, (u16 *)src_data, tl_shape, tg_stride, fmt);

  // compare data
  compare_result( ref_x, result_x, fmt, stride_size);

  // free variables
  free_tl(bk_ctx, tl_x);
  free(ref_x);
  free(result_x);
  free(u8ref_x);
  free(u16ref_x);
  free(s8src_data);
  free(u16src_data);
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
      test_get_tensor_l2g_stride_unalign(&ctx, bk_ctx, input_fmt[i].src_fmt);
    }
  }
  test_exit(&ctx);

  return 0;
}
