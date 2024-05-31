#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void get_tensor_l2g_stride_ref(
    void *ref, void *a,
    tl_shape_t tl_shape,
    bmk1880v2_tensor_lmem_stride_t tl_stride,
    bmk1880v2_tensor_tgmem_stride_t tg_stride,
    fmt_t fmt)
{
  int nsrc_byte = 1;
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;
  int stride_size = n * tg_stride.n;

  if (fmt == FMT_BF16) {
    nsrc_byte = 2; // FMT_BF16
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
  } else {
    nsrc_byte = 1; // FMT_U8, FMT_I8
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
  }

  int n_str = tg_stride.n / nsrc_byte;
  int c_str = tg_stride.c / nsrc_byte;
  int h_str = tg_stride.h / nsrc_byte;
  int w_str = 1;

  for (int i = 0; i < stride_size; i++) {
    if (fmt == FMT_BF16) {
      u16_ref[i] = 0xcf;
    } else {
      u8_ref[i] = 0xcf;
    }
  }

  for (int ni = 0; ni < n; ni++) {
    for (int ci = 0; ci < c; ci++) {
      for (int hi = 0; hi < h; hi++) {
        for (int wi = 0; wi < w; wi++) {
          u64 src_i = (ni * c + ci) * tl_stride.c/nsrc_byte + hi * tl_stride.h/nsrc_byte + wi * 1;
          u64 dst_i = ni * n_str + ci * c_str + hi * h_str + wi * w_str;
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
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    bmk1880v2_tensor_tgmem_stride_t tg_stride,
    fmt_t fmt)
{
  u8 *data = NULL;
  int n = tl->shape.n;
  int n_stride = tg_stride.n;
  int stride_size = n * n_stride;
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
  CVI_RT_MEM dev_mem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = CVI_RT_MemGetPAddr(dev_mem);
  int ret = CVI_RT_MemCopyS2D(*ctx, dev_mem, (u8 *)data);
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

  bmk1880v2_tdma_l2tg_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = tl;
  p.dst = &tg;
  bmk1880v2_tdma_l2g_bf16_tensor_copy(bk_ctx, &p);

  test_submit(ctx);

  ret = CVI_RT_MemCopyD2S(*ctx, (u8 *)data, dev_mem);
  assert(ret == BM_SUCCESS);
  CVI_RT_MemFree(*ctx, dev_mem);

  return data;
}

static void test_get_tensor_l2g_gl_stride(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  tl_t *tl_x = NULL;
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
      bmk1880v2_tensor_tgmem_default_stride( tg_shape, fmt);

  int stride_size = n * tg_stride.n;
  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h * w;
  tl_shape.w = 1;
  float val = -100;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * stride_size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * stride_size);
  void *src_data;
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
  tl_x = alloc_tl( bk_ctx, tl_shape, fmt, eu_align);
  put_bf16_tensor_g2l( ctx, bk_ctx, tl_x, (u16 *)src_data, fmt);
  tl_x->shape.n = n;
  tl_x->shape.c = c;
  tl_x->shape.h = h;
  tl_x->shape.w = w;
  tl_x->stride = bmk1880v2_tensor_lmem_default_stride(bk_ctx, tl_x->shape, fmt, eu_align);
  result_x = get_tensor_l2g_stride(ctx, bk_ctx, tl_x, tg_stride, fmt);
  get_tensor_l2g_stride_ref( ref_x, src_data, tl_x->shape, tl_x->stride, tg_stride, fmt);

  // compare data
  compare_result( ref_x, result_x, fmt, stride_size);

  // free variables
  free_tl(bk_ctx, tl_x);
  free(result_x);
  free(u8ref_x);
  free(u16ref_x);
  free(s8src_data);
  free(u16src_data);
  free(ref_x);
}

#define TEST_ALIGNED 1 // 1: test unalign only, 2: test both align/unalign
int main (void)
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
        test_get_tensor_l2g_gl_stride(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);

  return 0;
}
