#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_mac_const_ref(
    uint8_t *ref_high, uint8_t *ref_low,
    uint8_t *a, uint8_t b_const, int b_is_signed,
    uint8_t *c_high, uint8_t *c_low,
    int lshift_bits, int rshift_bits, uint64_t size, int relu_enable)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t ta = (int8_t)a[i];
    int32_t tb = b_is_signed? (int8_t)b_const: (uint8_t)b_const;
    int32_t tc = ((int8_t)c_high[i] << 8) + c_low[i];
    tc <<= lshift_bits;
    int32_t res = ta * tb + tc;

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

static int test_tl_mac_const(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int lshift_bits;
  int rshift_bits;
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
  uint8_t *a_data = (uint8_t *)malloc(size);
  uint8_t *c_high_data = (uint8_t *)malloc(size);
  uint8_t *c_low_data = (uint8_t *)malloc(size);
  uint8_t *ref_high_data = (uint8_t *)malloc(size);
  uint8_t *ref_low_data = (uint8_t *)malloc(size);
  if (!a_data || !c_high_data || !c_low_data || !ref_high_data || !ref_low_data) {
    ret = -1;
    goto fail_exit;
  }

  for(int relu_enable = 0; relu_enable < 2; relu_enable++) {

    for (uint64_t i = 0; i < size; i++) {
      a_data[i] = rand() % 256;
      c_high_data[i] = rand() % 64;
      c_low_data[i] = 200 + 2 * i;
    }

    uint8_t b_const = 37;
    int b_is_signed = 1;
     if(relu_enable) {
      lshift_bits = 1;
      rshift_bits = 8;
    }else {
      lshift_bits = 1;
      rshift_bits = 3;
    }

    tl_mac_const_ref(ref_high_data, ref_low_data,
                     a_data, b_const, b_is_signed, c_high_data, c_low_data,
                     lshift_bits, rshift_bits, size, relu_enable);

    cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_c_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_c_high = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    uint8_t *mac_high_data = NULL, *mac_low_data = NULL;
    if (!tl_a || !tl_c_low || !tl_c_high) {
      ret = -1;
      goto fail_exit_2;
    }

    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, a_data);
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_c_low, c_low_data);
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_c_high, c_high_data);
    cvk_tiu_mac_param_t p3;
    p3.res_high = tl_c_high;
    p3.res_low = tl_c_low;
    p3.res_is_int8 = relu_enable;
    p3.a = tl_a;
    p3.b_is_const = 1;
    p3.b_const.val = b_const;
    p3.b_const.is_signed = b_is_signed;
    p3.lshift_bits = lshift_bits;
    p3.rshift_bits = rshift_bits;
    p3.relu_enable = relu_enable;
    cvk_ctx->ops->tiu_mac(cvk_ctx, &p3);
    mac_high_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_c_high);
    mac_low_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_c_low);
    for (uint64_t i = 0; i < size; i++) {
      if(!relu_enable)
        if (mac_high_data[i] != ref_high_data[i]) {
          fprintf(stderr, "comparing failed at mac_high_data[%" PRIu64 "], got %d, exp %d\n",
                 i, mac_high_data[i], ref_high_data[i]);
          ret = -1;
          break;
        }
      if (mac_low_data[i] != ref_low_data[i]) {
        fprintf(stderr, "comparing failed at mac_low_data[%" PRIu64 "], got %d, exp %d\n",
               i, mac_low_data[i], ref_low_data[i]);
        ret = -1;
        break;
      }
    }

fail_exit_2:
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_c_high);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_c_low);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);
    free(mac_high_data);
    free(mac_low_data);
  }

fail_exit:
  free(a_data);
  free(c_high_data);
  free(c_low_data);
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

  ret |= test_tl_mac_const(rt_handle, cvk_ctx, 0);
  ret |= test_tl_mac_const(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
