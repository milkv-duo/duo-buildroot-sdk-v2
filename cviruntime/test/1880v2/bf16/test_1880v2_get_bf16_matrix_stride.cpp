#include "../1880v2_test_util.h"
#include "1880v2_bf16_util.h"

static fmt_type input_fmt[] = {
 {FMT_BF16, FMT_BF16},
 {FMT_I8, FMT_I8},
};

static void get_matrix_l2g_stride_ref(
    void *ref,
    void *a,
    ml_shape_t ml_shape,
    bmk1880v2_matrix_tgmem_stride_t gmem_stride,
    fmt_t fmt)
{
  int row = ml_shape.n;
  int col = ml_shape.col;
  int row_stride = gmem_stride.row / ((fmt == FMT_BF16) ?2:1);
  int stride_size = row * row_stride;
  u16 *u16_ref = NULL;
  u16 *u16_src = NULL;
  u8 *u8_ref = NULL;
  u8 *u8_src = NULL;

  if (fmt == FMT_BF16) {
    u16_ref = (u16 *)ref;
    u16_src = (u16 *)a;
    for (int i = 0; i < stride_size; i++)
      u16_ref[i] = 0xaf;
  } else {
    u8_ref = (u8 *)ref;
    u8_src = (u8 *)a;
    for (int i = 0; i < stride_size; i++)
      u8_ref[i] = 0xaf;
  }

  for (int ri = 0; ri < row; ri++) {
    for (int ci = 0; ci < col; ci++) {
      if (fmt == FMT_BF16) {
        u16_ref[ri * row_stride + ci] = u16_src[ri * col + ci];
      } else {
        u8_ref[ri * row_stride + ci] = u8_src[ri * col + ci];
      }
    }
  }

  if (fmt == FMT_BF16) {
    ref = (void *)u16_ref;
  } else {
    ref = (void *)u8_ref;
  }
}

static u8 * get_matrix_l2g_stride(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    ml_t *ml,
    bmk1880v2_matrix_tgmem_stride_t mg_stride,
    fmt_t fmt)
{
  int row = ml->shape.n;
  int row_stride = mg_stride.row;
  int col = ml->shape.col;
  int stride_size = row * row_stride;

  u8 *data = NULL;
  u8 *u8data = (u8 *)malloc(sizeof(u8) * stride_size);
  u16 *u16data = (u16 *)malloc(sizeof(u16) * stride_size);
  if (!u8data || !u16data) {
    free(u8data);
    free(u16data);
    return NULL;
  }

  for (int i = 0; i < stride_size; i++)
  {
    if(fmt == FMT_BF16) {
      u16data[i] = 0xaf;
    } else {
      u8data[i] = 0xaf;
    }
  }

  if (fmt == FMT_BF16) {
    data = (u8 *)u16data;
    free(u8data);
  } else {
    data = u8data;
    free(u16data);
  }

  bmshape_t bms = BM_TENSOR_WITH_FMT( row, row_stride, 1, 1,
                                     (fmt == FMT_BF16)? BM_FMT_BF16 : BM_FMT_INT8);
  CVI_RT_MEM devmem = CVI_RT_MemAlloc(*ctx, bmshape_get_size(&bms));
  int ret = CVI_RT_MemCopyS2D(*ctx, devmem, data);
  assert(ret == BM_SUCCESS);

  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = CVI_RT_MemGetPAddr(devmem);
  mg.shape.row = row;
  mg.shape.col = col;
  mg.stride = mg_stride;
  mg.fmt = fmt;

  bmk1880v2_tdma_l2tg_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = ml;
  p.dst = &mg;

  bmk1880v2_tdma_l2g_bf16_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  ret = CVI_RT_MemCopyD2S(*ctx, data, devmem);
  assert(ret == BM_SUCCESS);

  CVI_RT_MemFree(*ctx, devmem);
  return data;
}

static void test_get_matrix_l2g_stride(
    CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, fmt_t fmt, int eu_align)
{
  int row = 80;
  int col = 70;
  float val = -100;
  int size = row * col;
  int row_stride = col * 2;
  int stride_size = row * row_stride;
  u16 *u16src_data = (u16 *)malloc(sizeof(u16) * size);
  s8 *s8src_data = (s8 *)malloc(sizeof(s8) * size);
  void *src_data = NULL;
  u8 *result_x = NULL;
  void *ref_x = NULL;

  ml_shape_t ml_shape = bmk1880v2_matrix_lmem_default_shape(bk_ctx, row, col, fmt);
  bmk1880v2_matrix_tgmem_stride_t gmem_stride;
  gmem_stride.row = row_stride  * ((fmt == FMT_BF16) ?2:1);

  // prepare source data
  for (int i = 0; i < size; i++) {
    if(fmt == FMT_BF16) {
      u16src_data[i] = generate_bf16_corner_val(val);
      val += 0.1;
    } else {
      s8src_data[i] = i;
    }
  }
  ref_x = (u8 *)xmalloc(stride_size * ((fmt == FMT_BF16) ?2:1));
  src_data =  (fmt == FMT_BF16) ? (void *)u16src_data : (void *)s8src_data;

  // run tpu operations
  ml_t *ml_x = bmk1880v2_lmem_alloc_matrix(bk_ctx,ml_shape, fmt, eu_align);
  put_bf16_matrix_g2l(ctx, bk_ctx, ml_x, (u8 *)src_data, fmt);
  result_x = get_matrix_l2g_stride(ctx, bk_ctx, ml_x, gmem_stride, fmt);
  get_matrix_l2g_stride_ref(ref_x, src_data, ml_shape, gmem_stride, fmt);

   // compare data
  if( COMPARE_PASS != compare_result( ref_x, result_x, fmt, stride_size))
    exit(-1);

  // free variables
  bmk1880v2_lmem_free_matrix(bk_ctx, ml_x);
  free(s8src_data);
  free(u16src_data);
  free(ref_x);
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
      test_get_matrix_l2g_stride(&ctx, bk_ctx, input_fmt[i].src_fmt, u8_align);
    }
  }
  test_exit(&ctx);
  return 0;
}
