#include "1822_test_util.h"

typedef struct {
  long index;
  long offset;
}Base_reg;

Base_reg base_reg[]={
 {0, 0x000000 },
 {1, 0x100000 },
 {2, 0x200000 },
 {3, 0x300000 },
 {4, 0x400000 },
 {5, 0x500000 },
 {6, 0x600000 },
 {7, 0x700000 },
};
static void test_tensor_base_selection(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx, u32 reg_index, int offset)
{
  int n = 2;
  int c = 66;
  int h = 3;
  int w = 15;

  int size = n * c * h * w;
  u8 *data_x = (u8 *)xmalloc(size);

  for (int i = 0; i < size; i++)
    data_x[i] = i - 100;

  tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  tg_shape_t ts_shape;
  ts_shape.n = n;
  ts_shape.c = c;
  ts_shape.h = h;
  ts_shape.w = w;

  tl_t *tl_x = alloc_tl(bk_ctx, tl_shape, FMT_I8, 1);

  /*
   * Copy test data to the fixed address.(gaddr + offset)
   */
  bmshape_t bms = BM_TENSOR_INT8((int)n, (int)c, (int)h, (int)w);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  //bmmem_device_t ab_dev_mem = bmmem_device_prealloc(*ctx, NULL, gaddr + offset, &bms);
  bmmem_device_t ab_dev_mem = bmmem_device_prealloc_raw(*ctx, NULL, gaddr + offset, bmshape_get_size(&bms));
  
  int ret = bm_memcpy_s2d(*ctx, ab_dev_mem, data_x);
  assert(ret == BM_SUCCESS);

  /*
   * tensor transfer
   * g2l array base = offset, index = reg_index
   * l2g array base = 0, index = 0
   */
  bm_device_set_base_reg(*ctx, reg_index, offset);
  tg_t tg;
  tg.base_reg_index = 0;
  tg.start_address = gaddr;
  tg.fmt = FMT_I8;
  tg.shape = ts_shape;
  tg.stride = bmk1822_tensor_tgmem_default_stride(ts_shape, tg.fmt);
  tg.base_reg_index = reg_index;

  bmk1822_tdma_tg2l_tensor_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &tg;
  p.dst = tl_x;

  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  bm_device_set_base_reg(*ctx, 0, 0);
  u8 *result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }
  free(result_x);

  /*
   * tensor transfer
   * g2l array base = 0, index = reg_index
   * l2g array base = 0, index = 0
   */
  bm_device_set_base_reg(*ctx, reg_index, 0);
  tg.start_address = gaddr + offset;
  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);
  bm_device_set_base_reg(*ctx, 0, 0);
  result_x = get_tensor_l2g(ctx, bk_ctx, tl_x);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }

  /*
   * tensor transfer
   * g2l, array base = offset, index = reg_index
   * l2g, array_base = offset, index = reg_index
   */
  bm_device_set_base_reg(*ctx, reg_index, offset);
  tg.start_address = gaddr;
  tg.base_reg_index = reg_index;
  bmk1822_tdma_g2l_tensor_copy(bk_ctx, &p);
  test_submit(ctx);

  bm_device_set_base_reg(*ctx, reg_index, offset);
  tg.start_address = gaddr;
  bmk1822_tdma_l2tg_tensor_copy_param_t l2g_p;
  memset(&l2g_p, 0, sizeof(l2g_p));
  l2g_p.src = tl_x;
  l2g_p.dst = &tg;
  bmk1822_tdma_l2g_tensor_copy(bk_ctx, &l2g_p);
  test_submit(ctx);
  ret = bm_memcpy_d2s(*ctx, result_x,ab_dev_mem);
  assert(ret == BM_SUCCESS);

  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }
  free(result_x);

  bm_device_set_base_reg(*ctx, 0, 0);
  bm_device_set_base_reg(*ctx, 1, 0);
  free_tl(bk_ctx, tl_x);
  bmmem_device_free(*ctx, dev_mem);
  bmmem_device_free(*ctx, ab_dev_mem);
  free(data_x);
}
static void test_matrix_base_selection(
    bmctx_t *ctx, bmk_ctx_t *bk_ctx, u32 reg_index, int offset)
{
  int row = 5;
  int col = 16 * 5 + 2;
  int size = row * col;

  u8 *data_x = (u8 *)xmalloc(size);

  for (int i = 0; i < size; i++)
    data_x[i] = i - 100;

  ml_shape_t ml_shape =
    bmk1822_matrix_lmem_default_shape(bk_ctx, row, col, FMT_I8);
  mg_shape_t mg_shape;
  mg_shape.row = row;
  mg_shape.col = col;

  ml_t *ml =
    bmk1822_lmem_alloc_matrix(bk_ctx, ml_shape, FMT_I8, 1);

  /*
   * Copy test data to the specified offset address.
   */

  bmshape_t bms = BM_MATRIX_INT8(row,col);
  bmmem_device_t dev_mem = bmmem_device_alloc_raw(*ctx, bmshape_get_size(&bms));
  gaddr_t gaddr = bmmem_device_addr(dev_mem);
  //bmmem_device_t ab_dev_mem = bmmem_device_prealloc(*ctx, NULL, gaddr + offset, &bms);
  bmmem_device_t ab_dev_mem = bmmem_device_prealloc_raw(*ctx, NULL, gaddr + offset, bmshape_get_size(&bms));
  
  int ret = bm_memcpy_s2d(*ctx, ab_dev_mem, data_x);
  assert(ret == BM_SUCCESS);

  /*
   * matrix transfer
   * g2l array base = offset, index = reg_index
   * l2g array base = 0, index = 0
   */
  bm_device_set_base_reg(*ctx, reg_index, offset);
  mg_t mg;
  mg.base_reg_index = 0;
  mg.start_address = gaddr;
  mg.shape = mg_shape;
  mg.stride.row = mg_shape.col;
  mg.base_reg_index = reg_index;

  bmk1822_tdma_tg2l_matrix_copy_param_t p;
  memset(&p, 0, sizeof(p));
  p.src = &mg;
  p.dst = ml;

  bmk1822_tdma_g2l_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  bm_device_set_base_reg(*ctx, 0, 0);
  u8 *result_x = get_matrix_l2g(ctx, bk_ctx, ml);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }
  free(result_x);

  /*
   * matrix transfer
   * g2l array base = 0, index = reg_index
   * l2g array base = 0, index = 0
   */
  bm_device_set_base_reg(*ctx, reg_index, 0);
  mg.start_address = gaddr + offset;
  bmk1822_tdma_g2l_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  bm_device_set_base_reg(*ctx, 0, 0);
  result_x = get_matrix_l2g(ctx, bk_ctx, ml);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }

 /*
  * Matrix transfer
  * g2l, array base = offset, index = reg_index
  * l2g, array_base = offset, index = reg_index
  */
  bm_device_set_base_reg(*ctx, reg_index, offset);
  mg.start_address = gaddr;
  mg.base_reg_index = reg_index;
  bmk1822_tdma_g2l_matrix_copy(bk_ctx, &p);
  test_submit(ctx);

  mg.start_address = gaddr;
  bmk1822_tdma_l2tg_matrix_copy_param_t l2g_p;
  memset(&l2g_p, 0, sizeof(l2g_p));
  l2g_p.src = ml;
  l2g_p.dst = &mg;

  bm_device_set_base_reg(*ctx, reg_index, offset);

  bmk1822_tdma_l2g_matrix_copy(bk_ctx, &l2g_p);
  test_submit(ctx);

  ret = bm_memcpy_d2s(*ctx, result_x,ab_dev_mem);
  assert(ret == BM_SUCCESS);

  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      fprintf(stderr, "compare failed at result_x[%d]\n", i);
      exit(-1);
    }
  }

  bm_device_set_base_reg(*ctx, 0, 0);
  bm_device_set_base_reg(*ctx, 1, 0);
  free(result_x);
  bmk1822_lmem_free_matrix(bk_ctx, ml);
  bmmem_device_free(*ctx, dev_mem);
  bmmem_device_free(*ctx, ab_dev_mem);
  free(data_x);
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  for(int i=0; i<8; i ++)
  {
    test_matrix_base_selection(&ctx, bk_ctx, base_reg[i].index, base_reg[i].offset );
    test_tensor_base_selection(&ctx, bk_ctx, base_reg[i].index, base_reg[i].offset);
  }
  test_exit(&ctx);
  return 0;
}
