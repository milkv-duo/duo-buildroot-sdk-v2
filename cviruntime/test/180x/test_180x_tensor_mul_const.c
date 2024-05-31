#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_mul_const_ref(
    int8_t *ofmap, int8_t *ifmap, uint64_t size, int8_t mul_const, int shift_bits, int relu_enable)
{
  for (uint64_t i = 0; i < size; i++) {
    int32_t tmp = ifmap[i] * mul_const;
    tmp += 1 << (shift_bits - 1);
    tmp >>= shift_bits;
    if (tmp > 127)
      tmp = 127;
    else if (tmp < -128)
      tmp = -128;
    if(relu_enable)
      if(tmp<0)
        tmp=0;

    ofmap[i] = tmp;
  }
}

static int test_tl_mul_const(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
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

  uint32_t size = n * c * h  * w;

  int8_t *ifmap_data = (int8_t *)malloc(size);
  int8_t *ref_data = (int8_t *)malloc(size);
  if (!ifmap_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t relu_enable = 0; relu_enable < 2; relu_enable++)
  {
    for (uint32_t i = 0; i < size; i++)
      ifmap_data[i] = (uint8_t)(random() % 256);
  
    int8_t mul_const = 20;
    int shift_bits = 1;
  
    tl_mul_const_ref(ref_data, ifmap_data, size, mul_const, shift_bits, relu_enable);
  
    cvk_tl_t *tl_ifmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    cvk_tl_t *tl_ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, CVK_FMT_I8, eu_align);
    uint8_t *ofmap_data = NULL;
    if (!tl_ifmap || !tl_ofmap) {
      ret = -1;
      goto fail_exit_2;
    }
  
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_ifmap, (uint8_t *)ifmap_data);
  
    cvk_tiu_mul_param_t p;
    memset(&p, 0, sizeof(p));
    p.res_high = NULL;
    p.res_low = tl_ofmap;
    p.a = tl_ifmap;
    p.b_is_const = 1;
    p.b_const.val = mul_const;
    p.b_const.is_signed = 1;
    p.rshift_bits = shift_bits;
    p.relu_enable = relu_enable;

    cvk_ctx->ops->tiu_mul(cvk_ctx, &p);
  
    ofmap_data = tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_ofmap);
  
    for (uint32_t i = 0; i < size; i++) {
      if ((int8_t)ofmap_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing failed at ofmap_data[%u], got %x, exp %x\n",
               i, ofmap_data[i], ref_data[i]);
        ret = -1;
        break;
      }
    }
  
fail_exit_2:
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_ofmap);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_ifmap);
    free(ofmap_data);
  }

fail_exit:
  free(ifmap_data);
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

  ret |= test_tl_mul_const(rt_handle, cvk_ctx, 0);
  ret |= test_tl_mul_const(rt_handle, cvk_ctx, 1);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
