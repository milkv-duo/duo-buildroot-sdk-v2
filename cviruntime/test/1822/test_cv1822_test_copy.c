#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_copy_ref(int8_t *a, int8_t *res, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    res[i] = a[i];
}

static int test_tl_copy(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;

  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h * w;
  int8_t *a_data = (int8_t *)malloc(size);
  assert(a_data && "Expect allocated a_data");
  for (uint32_t i = 0; i < size; i++)
    a_data[i] = (int8_t)(i % 256);

  int8_t *ref_data = (int8_t *)malloc(size);
  assert(ref_data && "Expect allocated ref_data");
  tl_copy_ref(a_data, ref_data, size);
  
  cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);

  cvk_tiu_copy_param_t p10;
  p10.dst = tl_res;
  p10.src = tl_a;
  cvk_ctx->ops->tiu_copy(cvk_ctx, &p10);
  uint8_t *res_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res);

  for (uint64_t i = 0; i < size; i++) {
    if ((int8_t)res_data[i] != ref_data[i]) {
      printf("comparing failed at ofmap_data[%" PRIu64 "], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      ret = -1;
    }
  }

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

  free(a_data);
  free(ref_data);
  free(res_data);

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
 
  if (!argc)
    return -1;
  if (!argv)
    return -1;

  CVI_RT_HANDLE rt_handle;
  cvk_context_t *cvk_ctx = NULL;

  CVI_RT_Init(&rt_handle);
  cvk_ctx = CVI_RT_RegisterKernel(rt_handle, CMDBUF_SIZE);

  ret = test_tl_copy(rt_handle, cvk_ctx, 0);
  ret |= test_tl_copy(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  if (!ret)
    printf("%s pass\n", __FILENAME__);
  else
    printf("%s fail\n", __FILENAME__);

  return ret;
}
