#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_and_int8_ref(int8_t *a, int8_t *b, int8_t *res, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    res[i] = a[i] & b[i];
}

static void tl_and_int16_ref(
    uint8_t *ref_high, uint8_t *ref_low,
    uint8_t *a_high, uint8_t *a_low,
    uint8_t *b_high, uint8_t *b_low,
    uint64_t size)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t ta = ((int8_t)a_high[i] << 8) + a_low[i];
    int32_t tb = ((int8_t)b_high[i] << 8) + b_low[i];
    int32_t res = ta & tb;
    ref_high[i] = (res >> 8) & 0xff;
    ref_low[i] = res & 0xff;
  }
}

static int test_tl_and_int8(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int n = 3;
  int c = 9; // 39 -> 9 for 180x
  int h = 7;
  int w = 37;

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

  tl_and_int8_ref(a_data, b_data, ref_data, size);

  cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_b = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  if (!tl_a || !tl_b || !tl_res) {
    printf("  %s: fail to alloc tl\n", __FUNCTION__);
    ret = -1;
    goto fail_exit_2;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_b, (uint8_t *)b_data);
  cvk_tiu_and_int8_param_t p9;
  p9.res = tl_res;
  p9.a = tl_a;
  p9.b = tl_b;
  cvk_ctx->ops->tiu_and_int8(cvk_ctx, &p9);
  uint8_t *res_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res);

  for (uint32_t i = 0; i < size; i++) {
    if ((int8_t)res_data[i] != ref_data[i]) {
      fprintf(stderr, "comparing failed at ofmap_data[%u], got %x, exp %x\n",
             i, res_data[i], ref_data[i]);
      ret = -1;
      break;
    }
  }

  free(res_data);

fail_exit_2:
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_b);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);

fail_exit:
  free(a_data);
  free(b_data);
  free(ref_data);

  printf("  %s(eu_align=%d) %s\n", __FUNCTION__, eu_align, ret ? "fail" : "pass");

  return ret;
}

static int test_tl_and_int16(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int n = 3;
  int c = 9; // 39 -> 9 for 180x
  int h = 7;
  int w = 37;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h * w;
  uint8_t *a_high_data = (uint8_t *)malloc(size);
  uint8_t *a_low_data = (uint8_t *)malloc(size);
  uint8_t *b_high_data = (uint8_t *)malloc(size);
  uint8_t *b_low_data = (uint8_t *)malloc(size);
  uint8_t *ref_high_data = (uint8_t *)malloc(size);
  uint8_t *ref_low_data = (uint8_t *)malloc(size);
  if (!a_high_data || !a_low_data || !b_high_data || !b_low_data || !ref_high_data || !ref_low_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t i = 0; i < size; i++) {
    a_high_data[i] = i / 10;
    a_low_data[i] = i;
    b_high_data[i] = (i + 250) / 20;
    b_low_data[i] = 100 - i;
  }

  tl_and_int16_ref(
      ref_high_data, ref_low_data,
      a_high_data, a_low_data,
      b_high_data, b_low_data,
      size);

  cvk_tl_t *tl_a_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_a_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_b_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_b_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  cvk_tl_t *tl_res_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
  if (!tl_a_low || !tl_a_high || !tl_b_low || !tl_b_high || !tl_res_low || !tl_res_high){
    printf("  %s: fail to alloc tl\n", __FUNCTION__);
    ret = -1;
    goto fail_exit_2;
  }

  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_low, a_low_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_high, a_high_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_b_low, b_low_data);
  tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_b_high, b_high_data);
  cvk_tiu_and_int16_param_t p8;
  p8.res_high = tl_res_high;
  p8.res_low = tl_res_low;
  p8.a_high = tl_a_high;
  p8.a_low = tl_a_low;
  p8.b_high = tl_b_high;
  p8.b_low = tl_b_low;
  cvk_ctx->ops->tiu_and_int16(cvk_ctx, &p8);
  uint8_t *res_high_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_high);
  uint8_t *res_low_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_low);

  for (uint64_t i = 0; i < size; i++) {
    if (res_high_data[i] != ref_high_data[i]) {
      fprintf(stderr, "comparing failed at res_high_data[%" PRIu64 "], got %d, exp %d\n",
             i, res_high_data[i], ref_high_data[i]);
      ret = 1;
      break;
    }
    if (res_low_data[i] != ref_low_data[i]) {
      fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %d, exp %d\n",
             i, res_low_data[i], ref_low_data[i]);
      ret = -1;
      break;
    }
  }

  free(res_high_data);
  free(res_low_data);

fail_exit_2:
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_high);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_low);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_b_high);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_b_low);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_high);
  cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_low);

fail_exit:
  free(a_high_data);
  free(a_low_data);
  free(b_high_data);
  free(b_low_data);
  free(ref_high_data);
  free(ref_low_data);

  printf("  %s(eu_align=%d) %s\n", __FUNCTION__, eu_align, ret ? "fail" : "pass");

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

  ret |= test_tl_and_int8(rt_handle, cvk_ctx, 0);
  ret |= test_tl_and_int8(rt_handle, cvk_ctx, 1);
  ret |= test_tl_and_int16(rt_handle, cvk_ctx, 0);
  ret |= test_tl_and_int16(rt_handle, cvk_ctx, 1);

  printf("tensor and test %s\n", ret ? "fail" : "pass");

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
