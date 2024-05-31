#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

static int test_put_and_get_tensor_l2g(
    CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx)
{
  int n = 2;
  int c = 66;
  int h = 3;
  int w = 15;
  int size = n * c * h * w;
  uint8_t *data_x = (uint8_t *)malloc(size);
  uint8_t *data_y = (uint8_t *)malloc(size);
  if (!data_x || !data_y)
    return -1;

  for (int i = 0; i < size; i++)
    data_x[i] = i - 100;

  for (int i = 0; i < size; i++)
    data_y[i] = -i;

  /*
   * Interleave two tensors in case the same devmem is reused between
   * tensor_copy_s2d_g2l() and tensor_copy_l2g_d2s(), in which case the content of
   * devmem is already what is expected before tdma_store(cvk_ctx, ).
   */


  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  cvk_tl_t *tl_x =
      cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, 1);
  cvk_tl_t *tl_y =
      cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, 1);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_x, data_x);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_y, data_y);

  uint8_t *result_x = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_x);
  uint8_t *result_y = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_y);

  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      printf("compare failed at result_x[%d]\n", i);
      return -1;
    }
    if (result_y[i] != data_y[i]) {
      printf("compare failed at result_y[%d]\n", i);
      return -1;
    }
  }
  free(result_x);
  free(result_y);

  /*
   * Get result_y before result_x.
   */


  result_y = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_y);
  result_x = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_x);
  for (int i = 0; i < size; i++) {
    if (result_x[i] != data_x[i]) {
      printf("compare failed at result_x[%d]\n", i);
      return -1;
    }
    if (result_y[i] != data_y[i]) {
      printf("compare failed at result_y[%d]\n", i);
      return -1;
    }
  }
  free(result_x);
  free(result_y);

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_y);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_x);
  free(data_x);
  free(data_y);

  return 0;
}

int main(int argc, char **argv)
{
  int ret = 0;
  CVI_RT_HANDLE rt_handle = NULL;
  cvk_context_t *cvk_ctx = NULL;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);
  if (!rt_handle || !cvk_ctx) {
    printf("%s fail\n", __FILENAME__);
    return -1;
  }
  
  ret |= test_put_and_get_tensor_l2g(rt_handle, cvk_ctx);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
