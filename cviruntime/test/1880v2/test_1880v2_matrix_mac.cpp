#include "1880v2_test_util.h"

typedef bmk1880v2_tiu_matrix_multiplication_param_t param_t;

static u64 matrix_size(const ml_t *ml)
{
  u64 row = ml->shape.n;
  u64 col = ml->shape.col;
  return row * col;
}

static u64 res_size(param_t *p)
{
  if (p->res_is_int8 && !p->add_result)
    return matrix_size(p->res);
  else
    return matrix_size(p->res) / 2;
}

static u8 * alloc_left(param_t *p)
{
  u64 size = matrix_size(p->left);

  u8 *buf = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = i % 17 - 9;

  return buf;
}

static u8 * alloc_right(param_t *p)
{
  u64 size = matrix_size(p->right);

  u8 *buf = (u8 *)malloc(sizeof(u8) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = i % 13 - 6;

  return buf;
}

static u16 * alloc_bias(param_t *p)
{
  if (!p->bias)
    return NULL;

  u64 size = matrix_size(p->bias) / 2;

  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = 5 - (i % 7);

  return buf;
}

static u16 * alloc_res(param_t *p)
{
  u64 size = res_size(p);

  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = 17 - (i % 35);

  return buf;
}

static void right_shift(param_t *p, s32 *buf, u64 size)
{
  int shift_bits = p->rshift_bits;
  int round_up = 1;
  if (1)
    arith_right_shift(buf, size, shift_bits, round_up);
  else
    logic_right_shift(buf, size, shift_bits, round_up);
}

static void matrix_mac_ref(
    param_t *p, u8 left[], u8 right[], u16 bias[], u16 res[])
{
  u64 size = res_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == FMT_I8);
  int right_sign = (p->right->fmt == FMT_I8);
  int res_sign = (p->res->fmt == FMT_I8);

  s32 *tmp_res = (s32 *)malloc(sizeof(s32) * size);
  if (p->add_result) {
    for (u32 i = 0; i < res_row * res_col; i++) {
      tmp_res[i] = res_sign? (s16)res[i]: res[i];
      tmp_res[i] <<= p->lshift_bits;
    }
  } else {
    for (u32 i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
  }

  for (u32 row = 0; row < res_row; row++) {
    for (u32 col = 0; col < res_col; col++) {
      for (u32 i = 0; i < left_col; i++) {
        u32 li = row * left_col + i;
        u32 ri = i * right_col + col;
        s32 l = left_sign? (s8)left[li]: left[li];
        s32 r = right_sign? (s8)right[ri]: right[ri];
        tmp_res[row * res_col + col] += l * r;
      }
    }
  }

  if (p->bias && bias) {
    for (u32 row = 0; row < res_row; row++) {
      for (u32 col = 0; col < res_col; col++) {
        int bias_sign = (p->bias->fmt == FMT_I8);
        s32 b = bias_sign? (s16)bias[col]: bias[col];
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

  for (u64 i = 0; i < size; i++)
    res[i] = tmp_res[i];

  free(tmp_res);
}

static void put_bias(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const ml_t *ml,
    u16 data[])
{
  u64 size = ml->shape.col;

  u8 *tmp = (u8 *)malloc(sizeof(u8) * size * 2);
  if (!tmp)
    return;

  for (u64 i = 0; i < size; i++) {
    tmp[i] = data[i] & 0xff;
    tmp[i + size] = (data[i] >> 8) & 0xff;
  }

  put_matrix_g2l(ctx, bk_ctx, ml, tmp);

  free(tmp);
}

static void put_res(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const ml_t *ml,
    u16 data[])
{
  u64 size = ml->shape.n / 2 * ml->shape.col;

  u8 *tmp = (u8 *)malloc(sizeof(u8) * size * 2);
  if (!tmp)
    return;

  for (u64 i = 0; i < size; i++) {
    tmp[i] = data[i] & 0xff;
    tmp[i + size] = (data[i] >> 8) & 0xff;
  }

  put_matrix_g2l(ctx, bk_ctx, ml, tmp);

  free(tmp);
}

static u16 * get_res(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    param_t *p)
{
  u64 size = res_size(p);
  u16 *res = (u16 *)malloc(sizeof(u16) * size);

  u8 *tmp = get_matrix_l2g(ctx, bk_ctx, p->res);
  if (p->res_is_int8) {
    int res_sign = (p->res->fmt == FMT_I8);
    for (u64 i = 0; i < size; i++)
      res[i] = res_sign? (s8)tmp[i]: tmp[i];
  } else {
    for (u64 i = 0; i < size; i++)
      res[i] = tmp[i] + (tmp[i + size] << 8);
  }

  free(tmp);
  return res;
}

static void test_param(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, param_t *p)
{
  u8 *left = alloc_left(p);
  u8 *right = alloc_right(p);
  u16 *bias = alloc_bias(p);
  u16 *ref = alloc_res(p);

  put_matrix_g2l(ctx, bk_ctx, p->left, left);
  put_matrix_g2l(ctx, bk_ctx, p->right, right);
  if (bias)
    put_bias(ctx, bk_ctx, p->bias, bias);
  if (p->add_result)
    put_res(ctx, bk_ctx, p->res, ref);

  bmk1880v2_tiu_matrix_multiplication(bk_ctx, p);
  u16 *res = get_res(ctx, bk_ctx, p);

  matrix_mac_ref(p, left, right, bias, ref);

  u64 size = res_size(p);
  for (u64 i = 0; i < size; i++) {
    if (res[i] != ref[i]) {
      fprintf(stderr, "comparing failed at out[%" PRIu64 "], got %d, exp %d\n",
              i, (s16)res[i], (s16)ref[i]);
      exit(-1);
    }
  }

  free(left);
  free(right);
  free(bias);
  free(ref);
  free(res);
}

static void destroy_param(bmk_ctx_t *bk_ctx, param_t *p)
{
  bmk1880v2_lmem_free_matrix(bk_ctx, p->res);
  if (p->bias)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->bias);
  bmk1880v2_lmem_free_matrix(bk_ctx, p->right);
  bmk1880v2_lmem_free_matrix(bk_ctx, p->left);
}

static ml_t *alloc_param_res(
    bmk_ctx_t *bk_ctx, param_t *p)
{
  ml_shape_t s;
  s.n = p->left->shape.n;
  if (p->add_result || !p->res_is_int8)
    s.n *= 2;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;

  fmt_t fmt = FMT_U8;
  if (p->left->fmt == FMT_I8)
    fmt = FMT_I8;
  if (p->right->fmt == FMT_I8)
    fmt = FMT_I8;
  if (p->bias)
    if (p->bias->fmt == FMT_I8)
      fmt = FMT_I8;

  if (p->relu_enable)
    fmt = FMT_U8;

  return bmk1880v2_lmem_alloc_matrix(bk_ctx, s, fmt, 1);
}

static param_t param_0(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;
  p.ps32_mode = 0;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_1(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 6;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_2(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 6;
  u32 left_col = 25;
  u32 left_c = 1;
  u32 left_w = 25;

  u32 right_row = 25;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_3(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 6;
  u32 left_col = 25;
  u32 left_c = 2;
  u32 left_w = 18;

  u32 right_row = 25;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_4(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 6;
  u32 left_col = 39;
  u32 left_c = 4;
  u32 left_w = 10;

  u32 right_row = 39;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_5(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 2;
  u32 right_c = 1;
  u32 right_w = 2;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_6(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 2;
  u32 right_c = 2;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_7(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_8(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_9(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_10(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_11(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_12(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_13(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 2;
  u32 right_c = 1;
  u32 right_w = 2;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_14(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_15(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_16(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_17(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = true;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_18(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_19(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_20(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 4;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_21(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_22(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_23(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_24(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 1;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_25(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 4;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_26(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 4;
  u32 left_col = 1;
  u32 left_c = 1;
  u32 left_w = 1;

  u32 right_row = 1;
  u32 right_col = 1;
  u32 right_c = 1;
  u32 right_w = 1;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_I8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_27(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_28(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 2;
  p.rshift_bits = 3;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_29(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 2;
  p.rshift_bits = 3;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 477;
  u32 left_c = 60;
  u32 left_w = 8;

  u32 right_row = 477;
  u32 right_col = 10;
  u32 right_c = 3;
  u32 right_w = 4;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_30(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_31(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 3;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_32(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 6;
  p.rshift_bits = 2;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_33(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 6;
  p.rshift_bits = 2;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_I8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_34(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 13;
  p.res_is_int8 = true;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_U8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_U8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_35(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = true;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_U8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_U8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_36(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  ml_shape_t bias_shape = right_shape;
  bias_shape.n = 2;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_U8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_U8, 1);
  p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_U8, 1);
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_37(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 10;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_U8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_U8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

static param_t param_38(bmk_ctx_t *bk_ctx)
{
  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 1;
  p.rshift_bits = 6;
  p.res_is_int8 = false;
  p.relu_enable = false;
  p.add_result = false;

  u32 left_row = 7;
  u32 left_col = 23;
  u32 left_c = 3;
  u32 left_w = 8;

  u32 right_row = 23;
  u32 right_col = 477;
  u32 right_c = 60;
  u32 right_w = 8;

  ml_shape_t left_shape;
  left_shape.n = left_row;
  left_shape.c = left_c;
  left_shape.w = left_w;
  left_shape.col = left_col;

  ml_shape_t right_shape;
  right_shape.n = right_row;
  right_shape.c = right_c;
  right_shape.w = right_w;
  right_shape.col = right_col;

  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_U8, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_I8, 1);
  p.bias = NULL;
  p.res = alloc_param_res(bk_ctx, &p);

  return p;
}

#define test_one_param(n)                               \
  do {                                                  \
    param_t p = param_##n(bk_ctx);                      \
    test_param(&ctx, bk_ctx, &p);                       \
    destroy_param(bk_ctx, &p);                          \
  } while (0)

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

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

  test_exit(&ctx);
  return 0;
}
