#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

static void tl_mul_ref(uint16_t *ofmap, uint16_t *a, uint16_t *b, uint64_t size, int shift_bits, int relu_enable, cvk_fmt_t fmt_type)
{
  if(fmt_type == CVK_FMT_BF16) {
    for (uint64_t i = 0; i < size; i++) {
      float tmp = cvk_convert_bf16_fp32(a[i]) * cvk_convert_bf16_fp32(b[i]);
      if(relu_enable)
        if(tmp<0)
          tmp=0;
      ofmap[i] = cvk_convert_fp32_bf16(tmp);
    }
  } else {
    for (uint64_t i = 0; i < size; i++) {
      int32_t tmp = a[i] * b[i];
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
}

static int test_tl_mul(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, int eu_align)
{
  int ret = 0;
  int n = 2; // 3 -> 2 for 1810
  int c = 39;
  int h = 7;
  int w = 37;

  cvk_tl_shape_t tl_shape;
  tl_shape.n = n;
  tl_shape.c = c;
  tl_shape.h = h;
  tl_shape.w = w;

  cvk_fmt_t fmt_type = CVK_FMT_BF16;
  uint32_t size = n * c * h  * w;
  uint64_t data_size = size * (fmt_type == CVK_FMT_BF16 ? 2 : 1);
  int shift_bits = 1;

  uint16_t *a_data = (uint16_t *)malloc(data_size);
  uint16_t *b_data = (uint16_t *)malloc(data_size);
  uint16_t *ref_data = (uint16_t *)malloc(data_size);
  if (!a_data || !b_data || !ref_data) {
    ret = -1;
    goto fail_exit;
  }

  for (uint32_t relu_enable = 0; relu_enable < 2; relu_enable++)
  {
    for (uint32_t i = 0; i < size; i++) {
      a_data[i] = cvk_convert_fp32_bf16(random()%0x10);
      b_data[i] = cvk_convert_fp32_bf16(random());
    }

    cvk_tl_t *tl_a = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, fmt_type, eu_align);
    cvk_tl_t *tl_b = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, fmt_type, eu_align);
    cvk_tl_t *tl_res_low = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_shape, fmt_type, eu_align);
    uint16_t *res_low_data = NULL;
    if (!tl_a || !tl_b || !tl_res_low) {
      ret = -1;
      goto fail_exit_2;
    }

    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_a, (uint8_t *)a_data);
    tensor_copy_s2d_g2l(rt_handle, cvk_ctx, tl_b, (uint8_t *)b_data);

    cvk_tiu_mul_param_t p1;
    p1.res_high = NULL;
    p1.res_low = tl_res_low;
    p1.a = tl_a;
    p1.b_is_const = 0;
    p1.b = tl_b;
    p1.rshift_bits = shift_bits;
    p1.relu_enable = relu_enable;
    cvk_ctx->ops->tiu_mul(cvk_ctx, &p1);

    res_low_data = (uint16_t *)tensor_copy_l2g_d2s(rt_handle, cvk_ctx, tl_res_low);

    tl_mul_ref(ref_data, a_data, b_data, size, shift_bits, relu_enable, fmt_type);

    for (uint64_t i = 0; i < size; i++) {
      if (res_low_data[i] != ref_data[i]) {
        fprintf(stderr, "comparing failed at res_low_data[%" PRIu64 "], got %x, exp %x\n",
               i, res_low_data[i], ref_data[i]);
        ret = -1;
        break;
      }
    }

fail_exit_2:
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_res_low);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_b);
    cvk_ctx->ops->lmem_free_tensor(cvk_ctx, tl_a);
    free(res_low_data);
  }

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

  int round_mode;
  round_mode = cvk_set_store_feround();

  ret |= test_tl_mul(rt_handle, cvk_ctx, 0);
  ret |= test_tl_mul(rt_handle, cvk_ctx, 1);

  cvk_restore_feround(round_mode);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
