#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void put_tensor_g2l_tp_unalign_ref(
    void *ref,
    void *a,
    tl_shape_t tl_shape,
    fmt_t fmt)
{
  /*
   * (c, n, h, w) => (n, c, h, w) => (1, c, n * h, w)
   */
  int n = tl_shape.n;
  int c = tl_shape.c;
  int h = tl_shape.h;
  int w = tl_shape.w;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;

  int size = n * c * h * w;

  if (fmt == FMT_BF16) {
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
  } else {
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
  }

  for (int i = 0; i < size; i++)
  {
    if (fmt == FMT_BF16) {
      u16_ref[i] = u16_src[i];
    } else {
      u8_ref[i] = u8_src[i];
    }
  }

  if (fmt == FMT_BF16) {
    ref = (void *)u16_ref;
  } else {
    ref = (void *)u8_ref;
  }
}

static void put_tensor_g2l_tp(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    tl_t *tl,
    void *data,
    fmt_t fmt)
{
  int n = tl->shape.n;
  int c = tl->shape.c;
  int h = tl->shape.h;
  int w = tl->shape.w;

  bmshape_t bms = BM_TENSOR_WITH_FMT(n, c, h, w,
                                     (fmt == FMT_BF16)? BM_FMT_BF16 : BM_FMT_INT8);

  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, (u8 *)data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = CVI_RT_MemGetPAddr(devmem);

  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = fmt;
  tg.shape.n = tl->shape.c;
  tg.shape.c = tl->shape.n;
  tg.shape.h = tl->shape.h;
  tg.shape.w = tl->shape.w;
  tg.stride = bmk1880v2_tensor_tgmem_default_stride(tg.shape, fmt);
  tg.base_reg_index = 0;

  bmk1880v2_tdma_tg2l_tensor_copy_nc_transposed_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl;

  bmk1880v2_tdma_g2l_bf16_tensor_copy_nc_transposed(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, devmem);
}

static void test_put_tensor_g2l_tp_unalign(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  tl_t *tl_x = NULL;
  int n = 2;
  int c = 15;
  int h = 1;
  int w = 8;
  int size = n * c * h * w;
  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  float val = -100;
  void *src_data = NULL;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * size);
  void *result_x = NULL;
  void *ref_x = NULL;

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
  src_data = (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  tl_x = alloc_tl(bk_ctx, tl_shape, fmt, eu_align);
  put_tensor_g2l_tp(ctx, bk_ctx, tl_x, src_data, fmt);
  tl_x->shape.n = 1;
  tl_x->shape.c = c;
  tl_x->shape.h = n * h;
  tl_x->shape.w = w;
  result_x = get_bf16_tensor_l2g(ctx, bk_ctx, tl_x, fmt);
  tl_x->shape = tl_shape;
  put_tensor_g2l_tp_unalign_ref( ref_x, src_data, tl_shape, fmt);

  // compare data
  compare_result( ref_x, result_x, fmt, size);

  // free variables
  free_tl(bk_ctx, tl_x);
  free(ref_x);
  free(s8src_data);
  free(u16src_data);
  free(result_x);
}

#define TEST_ALIGNED 2 // 1: test unalign only, 2: test both align/unalign
int main (void)
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
      test_put_tensor_g2l_tp_unalign(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);

  return 0;
}
