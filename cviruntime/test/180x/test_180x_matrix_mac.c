#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tiu_matrix_multiplication_param_t param_t;

static uint64_t matrix_size(const cvk_ml_t *ml)
{
  uint64_t row = ml->shape.n;
  uint64_t col = ml->shape.col;
  return row * col;
}

static uint64_t res_size(param_t *p)
{
  if (p->res_is_int8 && !p->add_result)
    return matrix_size(p->res);
  else
    return matrix_size(p->res) / 2;
}

static uint8_t * alloc_left(param_t *p)
{
  uint64_t size = matrix_size(p->left);

  uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    buf[i] = i % 17 - 9;

  return buf;
}

static uint8_t * alloc_right(param_t *p)
{
  uint64_t size = matrix_size(p->right);

  uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    buf[i] = i % 13 - 6;

  return buf;
}

static uint16_t * alloc_bias(param_t *p)
{
  if (!p->bias)
    return NULL;

  uint64_t size = matrix_size(p->bias) / 2;

  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    buf[i] = 5 - (i % 7);

  return buf;
}

static uint16_t * alloc_res(param_t *p)
{
  uint64_t size = res_size(p);

  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++)
    buf[i] = 17 - (i % 35);

  return buf;
}

static void right_shift(param_t *p, int32_t *buf, uint64_t size)
{
  int shift_bits = p->rshift_bits;
  int round_up = 1;
  if (1)
    arith_right_shift(buf, size, shift_bits, round_up);
  else
    logic_right_shift(buf, size, shift_bits, round_up);
}

static void matrix_mac_ref(
    param_t *p, uint8_t left[], uint8_t right[], uint16_t bias[], uint16_t res[])
{
  uint64_t size = res_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == CVK_FMT_I8);
  int right_sign = (p->right->fmt == CVK_FMT_I8);
  int res_sign = (p->res->fmt == CVK_FMT_I8);

  int32_t *tmp_res = (int32_t *)malloc(sizeof(int32_t) * size);
  if (!tmp_res)
    return;

  if (p->add_result) {
    for (uint32_t i = 0; i < res_row * res_col; i++) {
      tmp_res[i] = res_sign? (int16_t)res[i]: res[i];
      tmp_res[i] <<= p->lshift_bits;
    }
  } else {
    for (uint32_t i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
  }

  for (uint32_t row = 0; row < res_row; row++) {
    for (uint32_t col = 0; col < res_col; col++) {
      for (uint32_t i = 0; i < left_col; i++) {
        uint32_t li = row * left_col + i;
        uint32_t ri = i * right_col + col;
        int32_t l = left_sign? (int8_t)left[li]: left[li];
        int32_t r = right_sign? (int8_t)right[ri]: right[ri];
        tmp_res[row * res_col + col] += l * r;
      }
    }
  }

  if (p->bias && bias) {
    for (uint32_t row = 0; row < res_row; row++) {
      for (uint32_t col = 0; col < res_col; col++) {
        int bias_sign = (p->bias->fmt == CVK_FMT_I8);
        int32_t b = bias_sign? (int16_t)bias[col]: bias[col];
        tmp_res[row * res_col + col] += b;
      }
    }
  }

  if (p->relu_enable)
    relu(tmp_res, size);

  right_shift(p, tmp_res, size);

  if (p->res_is_int8)
    saturate_to_int8(tmp_res, size, res_sign);
  else
    saturate_to_int16(tmp_res, size, res_sign);

  for (uint64_t i = 0; i < size; i++)
    res[i] = tmp_res[i];

  free(tmp_res);
}

static void put_bias(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint16_t data[])
{
  uint64_t size = ml->shape.col;

  uint8_t *tmp = (uint8_t *)malloc(sizeof(uint8_t) * size * 2);
  if (!tmp)
    return;

  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = data[i] & 0xff;
    tmp[i + size] = (data[i] >> 8) & 0xff;
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, ml, tmp);

  free(tmp);
}

static void put_res(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint16_t data[])
{
  uint64_t size = ml->shape.n / 2 * ml->shape.col;

  uint8_t *tmp = (uint8_t *)malloc(sizeof(uint8_t) * size * 2);
  if (!tmp)
    return;

  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = data[i] & 0xff;
    tmp[i + size] = (data[i] >> 8) & 0xff;
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, ml, tmp);

  free(tmp);
}

static uint16_t * get_res(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    param_t *p)
{
  uint64_t size = res_size(p);
  uint16_t *res = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!res)
    return NULL;

  uint8_t *tmp = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, p->res);
  if (p->res_is_int8) {
    int res_sign = (p->res->fmt == CVK_FMT_I8);
    for (uint64_t i = 0; i < size; i++)
      res[i] = res_sign? (int8_t)tmp[i]: tmp[i];
  } else {
    for (uint64_t i = 0; i < size; i++)
      res[i] = tmp[i] + (tmp[i + size] << 8);
  }

  free(tmp);
  return res;
}

static int test_param(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  int ret = 0;
  uint8_t *left = alloc_left(p);
  uint8_t *right = alloc_right(p);
  uint16_t *bias = alloc_bias(p);
  uint16_t *ref = alloc_res(p);
  if (!left || !right || (p->bias && !bias) || !ref) {
    ret = -1;
    goto fail_exit;
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->left, left);
  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->right, right);
  if (bias)
    put_bias(rt_handle, cvk_ctx, p->bias, bias);
  if (p->add_result)
    put_res(rt_handle, cvk_ctx, p->res, ref);

  cvk_ctx->ops->tiu_matrix_multiplication(cvk_ctx, p);
  uint16_t *res = get_res(rt_handle, cvk_ctx, p);

  matrix_mac_ref(p, left, right, bias, ref);

  uint64_t size = res_size(p);
  for (uint64_t i = 0; i < size; i++) {
    if (res[i] != ref[i]) {
      fprintf(stderr, "comparing failed at out[%" PRIu64 "], got %d, exp %d\n",
              i, (int16_t)res[i], (int16_t)ref[i]);
      ret = -1;
      break;
    }
  }

  free(res);

fail_exit:
  free(left);
  free(right);
  free(bias);
  free(ref);

  return ret;
}

static void destroy_param(cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->res);
  if (p->bias)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->bias);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->right);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->left);
}

static cvk_ml_t *alloc_param_res(
    cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ml_shape_t s;
  s.n = p->left->shape.n;
  if (p->add_result || !p->res_is_int8)
    s.n *= 2;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;

  cvk_fmt_t fmt = CVK_FMT_U8;
  if (p->left->fmt == CVK_FMT_I8)
    fmt = CVK_FMT_I8;
  if (p->right->fmt == CVK_FMT_I8)
    fmt = CVK_FMT_I8;
  if (p->bias)
    if (p->bias->fmt == CVK_FMT_I8)
      fmt = CVK_FMT_I8;

  if (p->relu_enable)
    fmt = CVK_FMT_U8;

  return cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, s, fmt, 1);
}

static param_t param_0(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;
  p.ps32_mode = 0;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_1(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 6;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_2(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 6;
  uint32_t left_col = 25;
  uint32_t left_c = 1;
  uint32_t left_w = 25;

  uint32_t right_row = 25;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_3(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 6;
  uint32_t left_col = 25;
  uint32_t left_c = 2;
  uint32_t left_w = 18;

  uint32_t right_row = 25;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_4(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 6;
  uint32_t left_col = 39;
  uint32_t left_c = 4;
  uint32_t left_w = 10;

  uint32_t right_row = 39;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_5(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 2;
  uint32_t right_c = 1;
  uint32_t right_w = 2;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_6(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 2;
  uint32_t right_c = 2;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_7(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_8(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_9(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_10(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_11(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_12(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_13(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 2;
  uint32_t right_c = 1;
  uint32_t right_w = 2;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_14(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_15(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_16(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_17(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = true;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_18(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_19(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_20(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 4;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_21(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_22(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_23(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_24(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 1;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_25(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 4;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_26(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 4;
  uint32_t left_col = 1;
  uint32_t left_c = 1;
  uint32_t left_w = 1;

  uint32_t right_row = 1;
  uint32_t right_col = 1;
  uint32_t right_c = 1;
  uint32_t right_w = 1;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_I8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_27(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_28(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 2;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_29(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 2;
  p.rshift_bits = 3;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 477;
  uint32_t left_c = 60;
  uint32_t left_w = 8;

  uint32_t right_row = 477;
  uint32_t right_col = 10;
  uint32_t right_c = 3;
  uint32_t right_w = 4;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_30(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_31(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 3;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_32(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 6;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_33(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 6;
  p.rshift_bits = 2;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_I8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_34(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 13;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_U8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_U8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_35(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_U8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_U8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_36(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  cvk_ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_U8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_U8, 1);
  p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_U8, 1);
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_37(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_U8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_U8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

static param_t param_38(cvk_context_t *cvk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 6;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  uint32_t left_row = 7;
  uint32_t left_col = 23;
  uint32_t left_c = 3;
  uint32_t left_w = 8;

  uint32_t right_row = 23;
  uint32_t right_col = 477;
  uint32_t right_c = 60;
  uint32_t right_w = 8;

  cvk_ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  cvk_ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_U8, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(cvk_ctx, &p);

  return p;
}

#define test_one_param(n)                               \
  do {                                                  \
    param_t p = param_##n(cvk_ctx);                     \
    ret |= test_param(rt_handle, cvk_ctx, &p);          \
    destroy_param(cvk_ctx, &p);                         \
  } while (0)

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

  test_one_param(0);
  test_one_param(1);
  test_one_param(2);
  test_one_param(3);
  test_one_param(4);
  test_one_param(5);
  test_one_param(6);
  test_one_param(7);
  test_one_param(8);
  test_one_param(9);
  test_one_param(10);
  test_one_param(11);
  test_one_param(12);
  test_one_param(13);
  test_one_param(14);
  test_one_param(15);
  test_one_param(16);
  test_one_param(17);
  test_one_param(18);
  test_one_param(19);
  test_one_param(20);
  test_one_param(21);
  test_one_param(22);
  test_one_param(23);
  test_one_param(24);
  test_one_param(25);
  test_one_param(26);
  test_one_param(27);
  test_one_param(28);
  test_one_param(29);
  test_one_param(30);
  test_one_param(31);
  test_one_param(32);
  test_one_param(33);
  test_one_param(34);
  test_one_param(35);
  test_one_param(36);
  test_one_param(37);
  test_one_param(38);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
