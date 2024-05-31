#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_add_const_ref(
    uint8_t *ref_high, uint8_t *ref_low,
    uint8_t *a_high, uint8_t *a_low,
    int16_t b, int b_is_signed,
    int rshift_bits,
    uint64_t size, int relu_enable)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t ta = ((int8_t)a_high[i] << 8) + a_low[i];
    int32_t tb = b_is_signed? b: (uint16_t)b;
    int32_t res = ta + tb;
    res += 1 << (rshift_bits - 1);
    res >>= rshift_bits;

    if(relu_enable)
    {
      if (res > 127)
        res = 127;
      else if (res < -128)
        res = -128;

      if(relu_enable)
        if(res<0)
          res=0;
      ref_high[i] = 0;
      ref_low[i] = res & 0xff;
    }else{
      if (res > 32767)
        res = 32767;
      else if (res < -32768)
        res = -32768;
      ref_high[i] = (res >> 8) & 0xff;
      ref_low[i] = res & 0xff;
    }
  }
}

static int test_tl_add_const(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int n = 3;
  int c = 39;
  int h = 7;
  int w = 37;
  int rshift_bits;
  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  uint32_t size = n * c * h  * w;
  uint8_t *a_high_data = (uint8_t *)malloc(size);
  uint8_t *a_low_data = (uint8_t *)malloc(size);
  uint8_t *ref_high_data = (uint8_t *)malloc(size);
  uint8_t *ref_low_data = (uint8_t *)malloc(size);
  if (!a_high_data || !a_low_data || !ref_high_data || !ref_low_data) {
    ret = -1;
    goto fail_exit;
  }

  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {
    int16_t b;
    int b_is_signed = 1;
    for (uint32_t i = 0; i < size; i++) {
      a_high_data[i] = rand() % 64+ i;
      a_low_data[i] = i;
    }

    if(relu_enable)
    {
      b=-64;
      rshift_bits = 7;
    }
    else
    {
      b=-278;
      rshift_bits = 1;
    }

    tl_add_const_ref(ref_high_data, ref_low_data,
                     a_high_data, a_low_data,
                     b, b_is_signed, rshift_bits, size,relu_enable);

    cvk_tl_t *tl_a_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_a_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_res_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_res_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    uint8_t *res_high_data = NULL, *res_low_data = NULL;
    if (!tl_a_low || !tl_a_high || !tl_res_low || !tl_res_high) {
      ret = -1;
      goto fail_exit_2;
    }

    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_low, a_low_data);
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a_high, a_high_data);

    cvk_tiu_add_param_t p4;
    p4.res_high = relu_enable ? 0 : tl_res_high;
    p4.res_low = tl_res_low;
    p4.a_high = tl_a_high;
    p4.a_low = tl_a_low;
    p4.b_is_const = 1;
    p4.b_const.val = b;
    p4.b_const.is_signed = b_is_signed;
    p4.rshift_bits = rshift_bits;
    p4.relu_enable = relu_enable;
    cvk_ctx->ops->tiu_add(cvk_ctx, &p4);

    res_high_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_high);
    res_low_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_low);
    for (uint64_t i = 0; i < size; i++) {
      if(!relu_enable)
        if (res_high_data[i] != ref_high_data[i]) {
          fprintf(stderr, "comparing failed at res_high_data[%" PRIu64 "], got %d, exp %d\n",
                  i, res_high_data[i], ref_high_data[i]);
          ret = -1;
        }
      if (res_low_data[i] != ref_low_data[i]) {
        fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %d, exp %d\n",
                i, res_low_data[i], ref_low_data[i]);
        ret = -1;
      }
    }

fail_exit_2:
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_high);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_low);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_high);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a_low);
    free(res_high_data);
    free(res_low_data);
  }

fail_exit:
  free(a_high_data);
  free(a_low_data);
  free(ref_high_data);
  free(ref_low_data);

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

  ret |= test_tl_add_const(rt_handle, cvk_ctx, 0);
  ret |= test_tl_add_const(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
