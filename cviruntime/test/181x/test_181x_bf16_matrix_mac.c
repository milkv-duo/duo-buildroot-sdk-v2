#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"

typedef cvk_tiu_matrix_multiplication_param_t param_t;
int random_seed;

static uint64_t matrix_size(const cvk_ml_t *ml)
{

  uint64_t row = ml->shape.n;
  uint64_t col = ml->shape.col;
  return row * col;
}

static uint64_t res_size(param_t *p)
{
    return matrix_size(p->res);
}

static uint16_t * alloc_left(param_t *p)
{
  uint64_t size = matrix_size(p->left);
  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++) {
    buf[i] = cvk_convert_fp32_bf16(i);
  }

  return buf;
}

static uint16_t * alloc_right(param_t *p)
{
  uint64_t size = matrix_size(p->right);
  uint16_t *buf = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++) {
    float val = 0.01;
    buf[i] = cvk_convert_fp32_bf16(i);
    val += 0.01;
  }
  return buf;
}

static uint32_t * alloc_bias(param_t *p)
{
  if (!p->bias)
    return NULL;

  uint64_t size = matrix_size(p->bias);
  uint32_t *buf = (uint32_t *)malloc(sizeof(uint32_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++) {
    buf[i] = cvk_convert_fp32_hex(i);
  }
  return buf;
}

static uint32_t * alloc_res(param_t *p)
{
  uint64_t size = res_size(p);
  uint32_t *buf = (uint32_t *)malloc(sizeof(uint32_t) * size);
  if (!buf)
    return NULL;

  for (uint64_t i = 0; i < size; i++) {
    buf[i] = cvk_convert_fp32_bf16(i);
  }
  return buf;
}

static inline void bf16_relu(float *buf, uint64_t size)
{
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static void matrix_mac_ref(
    param_t *p, uint16_t left[], uint16_t right[], uint32_t bias[], uint32_t res[])
{
  uint64_t size = res_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  uint32_t left_c = p->left->shape.c;
  uint32_t left_w = p->left->shape.w;

  float *tmp_res = (float *)malloc(sizeof(float) * size);
  if (!tmp_res)
    return;

  if (p->add_result) {
    for (uint32_t i = 0; i < res_row * res_col; i++)
      tmp_res[i] = cvk_convert_bf16_fp32(res[i]);
  } else {
    for (uint32_t i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
  }
  for (uint32_t row = 0; row < res_row; row++) {
    for (uint32_t col = 0; col < res_col; col++) {
      for (uint32_t wi = 0; wi < left_w; wi++) {
        for (uint32_t ci = 0; ci < left_c; ci++) {
          if ((wi + (ci*left_w)) >= left_col)
            continue;
          uint32_t li = row * left_col + left_w * ci + wi;
          uint32_t ri = (ci* left_w + wi )* right_col + col;

          float l = cvk_convert_bf16_fp32(left[li]);
          float r = cvk_convert_bf16_fp32(right[ri]);
          tmp_res[row * res_col + col] += l * r;
        }
      }
    }
  }

  if (p->bias && bias) {
    for (uint32_t row = 0; row < res_row; row++) {
      for (uint32_t col = 0; col < res_col; col++) {
        float b = cvk_convert_hex_fp32(bias[col]);
        tmp_res[row * res_col + col] += b;
      }
    }
  }

  if (p->relu_enable)
    bf16_relu(tmp_res, size);

  for (uint64_t i = 0; i < size; i++) {
    res[i] = cvk_convert_fp32_bf16(tmp_res[i]);
  }
  free(tmp_res);
}

static void put_bias(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint32_t data[])
{
  uint64_t size = ml->shape.col;

  uint16_t *tmp = (uint16_t *)malloc(sizeof(uint16_t) * size * 2);
  if (!tmp)
    return;

  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = (data[i] >> 16) & 0xFFFF;
    tmp[i + size] = (data[i] & 0xFFFF);
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, ml, (uint8_t*)tmp);

  free(tmp);
}

static void put_res(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    const cvk_ml_t *ml,
    uint32_t data[])
{
  uint64_t size = ml->shape.n  * ml->shape.col;

  uint16_t *tmp = (uint16_t *)malloc(sizeof(uint16_t) * size);
  if (!tmp)
    return;

  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = (data[i] & 0xFFFF);
  }

  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, ml, (uint8_t*)tmp);

  free(tmp);
}

static uint32_t * get_res(
    CVI_RT_HANDLE rt_handle,
    cvk_context_t *cvk_ctx,
    param_t *p)
{
  uint64_t size = res_size(p);
  uint32_t *res = (uint32_t *)malloc(sizeof(uint32_t) * size);
  if (!res)
    return NULL;

  uint16_t *tmp = (uint16_t *)matrix_copy_l2g_d2s(rt_handle, cvk_ctx, p->res);
  if (tmp) {
    for (uint64_t i = 0; i < size; i++)
      res[i] = tmp[i];

    free(tmp);
  } else {
    free(res);
    res = NULL;
  }

  return res;
}

static void test_param(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  uint16_t *left = alloc_left(p);
  uint16_t *right = alloc_right(p);
  uint32_t *bias = alloc_bias(p);
  uint32_t *ref = alloc_res(p);
  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->left, (uint8_t*)left);
  matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->right, (uint8_t*)right);
  if (bias)
    put_bias(rt_handle, cvk_ctx, p->bias, bias);
  if (p->add_result)
    put_res(rt_handle, cvk_ctx, p->res, ref);

  cvk_ctx->ops->tiu_matrix_multiplication(cvk_ctx, p);
  uint32_t *res = get_res(rt_handle, cvk_ctx, p);
  matrix_mac_ref(p, left, right, bias, ref);
  uint64_t size = res_size(p);
  for (uint64_t i = 0; i < size; i++) {
    if (res[i] != ref[i]) {
      fprintf(stderr, "comparing failed at out[%" PRIu64 "], got %x, exp %x\n",
              i, res[i], ref[i]);
      fprintf(stderr, "random_seed=%d\n", random_seed);
      exit(-1);
    }
  }
  free(left);
  free(right);
  free(bias);
  free(ref);
  free(res);
}

static void destroy_param(cvk_context_t *cvk_ctx, param_t *p)
{
  if (p->bias)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->bias);
  if (p->res)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->res);
  if (p->right)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->right);
  if (p->left)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->left);
}

static cvk_ml_t *alloc_param_res(
    cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ml_shape_t s;

  s.n = p->left->shape.n;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;
  cvk_fmt_t fmt = CVK_FMT_BF16;
  return cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, s, fmt, 1);
}

static param_t param_0(cvk_context_t *cvk_ctx)
{

retry:
  random_seed = clock();
  srand(random_seed);

  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = rand()%2;
  p.add_result = 0; /*bf16 HW does not support add_result*/
  p.ps32_mode = 0;

  uint32_t left_row = rand() % 100 +1;
  uint32_t left_col = rand() % 100 + 1;
  uint32_t left_w = rand() % (left_col/5+1) + 1; // c is generate by w, and make c is larger
  uint32_t left_c = left_col / left_w + (left_col % left_w ? 1: 0);

  uint32_t right_row = left_col;
  uint32_t right_col = rand() % 100 + 1;
  uint32_t right_w = (rand() % (right_col/5+1) + 1); // make c is larger
  uint32_t right_c = right_col / right_w + (right_col % right_w ? 1: 0) ;

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

  uint32_t bias = rand()%2;
  p.bias = NULL;
  p.left = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape, CVK_FMT_BF16, 1);
  p.right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, CVK_FMT_BF16, 1);
  if (!p.left || !p.right) {
    printf("retry init_matrix_param\n");
    destroy_param(cvk_ctx, &p);
    goto retry;
  }

  p.res = alloc_param_res(cvk_ctx, &p);
  if (bias) {
    cvk_ml_shape_t bias_shape = right_shape;
    bias_shape.n = 2;
    p.bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, CVK_FMT_BF16, 1);
  }

  if (!p.res || (bias && !p.bias)) {
    printf("retry init_matrix_param\n");
    destroy_param(cvk_ctx, &p);
    goto retry;
  }

  return p;
}


#define test_one_param(n)                               \
  do {                                                  \
    param_t p = param_##n(cvk_ctx);                      \
    test_param(rt_handle, cvk_ctx, &p);                       \
    destroy_param(cvk_ctx, &p);                          \
  } while (0)

int main(int argc, char **argv)
{
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

  for (int i = 0 ; i < 30 ; i++)
    test_one_param(0);

  cvk_restore_feround(round_mode);

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return 0;
}
