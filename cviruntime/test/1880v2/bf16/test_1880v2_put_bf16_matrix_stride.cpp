#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void put_matrix_g2l_stride_ref(
    void *ref,
    void *a,
    ml_shape_t  lmem_shape,
    bmk1880v2_matrix_tgmem_stride_t gmem_stride,
    fmt_t fmt)
{
  int row = lmem_shape.n;
  int col = lmem_shape.col;
  int row_stride = gmem_stride.row / ((fmt == FMT_BF16) ?2:1);
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

  for (int ri = 0; ri < row; ri++) {
    for (int ci = 0; ci < col; ci++) {
      if (fmt == FMT_BF16) {
        u16_ref[ri * col + ci] = u16_src[ri * row_stride + ci];
      } else {
        u8_ref[ri * col + ci] = u8_src[ri * row_stride + ci];
      }
    }
  }

  if (fmt == FMT_BF16) {
    ref = (void *)u16_ref;
  } else {
    ref = (void *)u8_ref;
  }
}

static void put_matrix_g2l_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    ml_t *ml,
    bmk1880v2_matrix_tgmem_stride_t gmem_stride,
    void *data,
    fmt_t fmt)
{
  int row = ml->shape.n;
  int col = ml->shape.col;
  int row_stride = gmem_stride.row;

  bmshape_t bms = BM_MATRIX_INT16(row, row_stride );
  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, (u8 *)data);
  assert(ret == BM_SUCCESS);

  gaddr_t gaddr = CVI_RT_MemGetPAddr(devmem);
  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = gaddr;
  mg.shape.row = row;
  mg.shape.col = col;
  mg.stride = gmem_stride;
  mg.fmt = fmt;
  mg.base_reg_index = 0;

  bmk1880v2_tdma_tg2l_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.dst = ml;
  p.src = &mg;
  bmk1880v2_tdma_g2l_bf16_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  CVI_RT_MemFree(*ctx, devmem);
  return ;
}

static void test_put_matrix_g2l_stride(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  int row = 80;
  int col = 70;
  float val = -100;
  int size = row * col;
  int row_stride = col * 2;
  int stride_size = row * row_stride;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * stride_size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * stride_size);
  void *src_data = NULL;
  void *result_x = NULL;
  void *ref_x = NULL;

  ml_shape_t mls = bmk1880v2_matrix_lmem_default_shape(bk_ctx, row, col, fmt);
  ml_t *ml = bmk1880v2_lmem_alloc_matrix(bk_ctx, mls, fmt, eu_align);
  bmk1880v2_matrix_tgmem_stride_t gmem_stride;
  gmem_stride.row = row_stride * ((fmt == FMT_BF16) ?2:1);

  // prepare source data
  for (int i = 0; i < stride_size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  ref_x = (u8 *)xmalloc(size * ((fmt == FMT_BF16) ?2:1));
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  put_matrix_g2l_stride(ctx, bk_ctx, ml, gmem_stride, src_data, fmt);
  result_x = get_bf16_matrix_l2g(ctx, bk_ctx, ml, fmt);
  put_matrix_g2l_stride_ref(ref_x, src_data, mls, gmem_stride, fmt);

  // compare data
  if( COMPARE_PASS != compare_result( ref_x, result_x, fmt, size))
    exit(-1);

  // free variables
  bmk1880v2_lmem_free_matrix(bk_ctx, ml);
  free(s8src_data);
  free(u16src_data);
  free(ref_x);
  free(result_x);
}

#define TEST_ALIGNED 2 // 1: test unalign only, 2: test both align/unalign
int main ()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  u32 nr_fmt = sizeof(input_fmt) / sizeof(input_fmt[0]);

  test_init(&ctx, &bk_ctx);
  for (u32 i = 0; i < nr_fmt; i++) {
    for (u8 u8_align = 0; u8_align < TEST_ALIGNED; u8_align++) {
      test_put_matrix_g2l_stride(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);
  return 0;
}
