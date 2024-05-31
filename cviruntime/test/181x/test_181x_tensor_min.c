#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_min_ref(int8_t *a, int8_t *b, int8_t *max, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++) {
    if (a[i] > b[i])
      max[i] = b[i];
    else
      max[i] = a[i];
  }
}

static int test_tl_min(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;
  int ret = 0;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h * w;
  int8_t *a_data = (int8_t *)malloc(size);
  int8_t *b_data = (int8_t *)malloc(size);
  int8_t *ref_data = (int8_t *)malloc(size);
  if (!a_data || !b_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t i = 0; i < size; i++)
    a_data[i] = (int8_t)(i % 256);

  for (uint32_t i = 0; i < size; i++)
    b_data[i] = (int8_t)(100 - i % 256);

  tl_min_ref(a_data, b_data, ref_data, size);

  cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_b = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_min = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  if (!tl_a || !tl_b || !tl_min) {
    ret = -1;
    goto fail_exit;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_b, (uint8_t *)b_data);
  cvk_tiu_min_param_t p6;
  p6.min = tl_min;
  p6.a = tl_a;
  p6.b_is_const = 0;
  p6.b = tl_b;
  cvk_ctx->ops->tiu_min(cvk_ctx, &p6);
  uint8_t *min_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_min);

  for (uint32_t i = 0; i < size; i++) {
    if ((int8_t)min_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%u], got %x, exp %x\n",
             i, min_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_min);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_b);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

  free(min_data);

fail_exit:
  free(a_data);
  free(b_data);
  free(ref_data);

  return ret;
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

  ret |= test_tl_min(rt_handle, cvk_ctx, 0);
  ret |= test_tl_min(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
