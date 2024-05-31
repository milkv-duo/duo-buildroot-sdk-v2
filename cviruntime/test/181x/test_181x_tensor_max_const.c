#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_max_const_ref(int8_t *a, int8_t b, int8_t *max, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++) {
    if (a[i] > b)
      max[i] = a[i];
    else
      max[i] = b;
  }
}

static int test_tl_max_const(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
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
  int8_t *ref_data = (int8_t *)malloc(size);
  uint8_t *max_data = NULL;
  if (!a_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t i = 0; i < size; i++)
    a_data[i] = (int8_t)(i % 256);

  int8_t b = 47;

  tl_max_const_ref(a_data, b, ref_data, size);

  cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_max = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);
  cvk_tiu_max_param_t p;
  memset(&p, 0, sizeof(p));
  p.max = tl_max;
  p.a = tl_a;
  p.b_is_const = 1;
  p.b_const.val = b;
  p.b_const.is_signed = 1;
  cvk_ctx->ops->tiu_max(cvk_ctx, &p);
  max_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_max);

  for (uint32_t i = 0; i < size; i++) {
    if ((int8_t)max_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%u], got %x, exp %x\n",
             i, max_data[i], ref_data[i]);
      ret = -1;
      goto fail_exit;
    }
  }

  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_max);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

fail_exit:
  free(a_data);
  free(ref_data);
  free(max_data);

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

  ret |= test_tl_max_const(rt_handle, cvk_ctx, 0);
  ret |= test_tl_max_const(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
