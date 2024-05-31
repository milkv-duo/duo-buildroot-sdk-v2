#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>

#include "test_cvikernel_util.h"
#include "test_native_ref.h"

typedef cvk_tiu_matrix_multiplication_param_t param_t;

typedef struct{
  cvk_fmt_t left_sign;
  uint32_t left_row ;
  uint32_t left_col ;
  uint32_t left_c ;
  uint32_t left_w ;
  cvk_fmt_t right_sign;
  uint32_t right_row ;
  uint32_t right_col ;
  uint32_t right_c ;
  uint32_t right_w ;
  uint32_t lshift_bits ;
  uint32_t rshift_bits ;
  uint32_t relu_enable ;
  uint32_t using_bias;
  cvk_fmt_t bias_sign;
} matrix_init_para_t;

matrix_init_para_t matrix_para_t;

static void make_bmk_matrix_param_ps32(cvk_context_t *cvk_ctx, param_t *p, int ps32_mode);
static param_t param_init();

void print_param(param_t *p)
{
  printf("ps32_mode =%d\n",p->ps32_mode);
  printf("left_shape.n =%d\n",p->left->shape.n);
  printf("left_shape.col =%d\n",p->left->shape.col);
  printf("left_shape.c =%d\n",p->left->shape.c);
  printf("left_shape.w =%d\n",p->left->shape.w);
  printf("left_fmt =%d\n",p->left->fmt);
  printf("right_shape.n =%d\n",p->right->shape.n);
  printf("right_shape.col =%d\n",p->right->shape.col);
  printf("right_shape.c =%d\n",p->right->shape.c);
  printf("right_shape.w =%d\n",p->right->shape.w);
  printf("right_fmt =%d\n",p->right->fmt);
  if(p->bias)
  {
    printf("bias_shape.n =%d\n",p->bias->shape.n);
    printf("bias_shape.col =%d\n",p->bias->shape.col);
    printf("bias_shape.c =%d\n",p->bias->shape.c);
    printf("bias_shape.w =%d\n",p->bias->shape.w);
    printf("bias_fmt =%d\n",p->bias->fmt);
  }
  printf("result_shape.n =%d\n",p->res->shape.n);
  printf("result_shape.col =%d\n",p->res->shape.col);
  printf("result_shape.c =%d\n",p->res->shape.c);
  printf("result_shape.w =%d\n",p->res->shape.w);
  printf("result_fmt =%d\n",p->res->fmt);
  printf("relu_enable=%d\n",p->relu_enable);
  printf("rshift_bits=%d\n",p->rshift_bits);
}


static uint64_t matrix_size(const cvk_ml_t *ml)
{
  uint64_t row = ml->shape.n;
  uint64_t col = ml->shape.col;
  return row * col;
}

static uint64_t res_ps32_size(param_t *p)
{
    return matrix_size(p->res);
}

static uint64_t res_size(param_t *p)
{
  if (p->res_is_int8 && !p->add_result)
    return matrix_size(p->res);
  else
    return matrix_size(p->res) *2 ;
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

static uint8_t * alloc_ps32_res(param_t *p)
{
  uint64_t size = res_ps32_size(p)*4;
  uint8_t *buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
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

static int ps32_m2_matrix_mac_ref(
  param_t *p,
  uint8_t *left,
  uint8_t *right,
  uint8_t *res)
{
  uint64_t size = res_ps32_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == CVK_FMT_I8);
  int right_sign = (p->right->fmt == CVK_FMT_I8);
  int ret = 0;
  int bstride = res_row * res_col;

  int32_t *tmp_res = (int32_t *)malloc(sizeof(int32_t) * size);
  if (!tmp_res)
    return -1;

  for (uint32_t i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
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

  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*0] = tmp_res[i]>>0;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*2] = tmp_res[i]>>16;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*3] = tmp_res[i]>>24;

  free(tmp_res);

  return ret;
}

static int ps32_m3_matrix_mac_ref(
  param_t *p,
  uint8_t *left,
  uint8_t *right,
  uint8_t *res)
{
  uint64_t size = res_ps32_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == CVK_FMT_I8);
  int right_sign = (p->right->fmt == CVK_FMT_I8);
  int ret = 0;
  int bstride = res_row * res_col;

  uint32_t *tmp_res = (uint32_t *)malloc(sizeof(uint32_t) * size);
  if (!tmp_res)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] = res[i + bstride*0];
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*1]<<8;
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*2]<<16;
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*3]<<24;

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

  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*0] = tmp_res[i]>>0;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*2] = tmp_res[i]>>16;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*3] = tmp_res[i]>>24;

  free(tmp_res);

  return ret;
}

static int ps32_m1_matrix_mac_ref(
  param_t *p,
  uint8_t *left,
  uint8_t *right,
  uint16_t * bias,
  uint8_t *res)
{
  uint64_t size = res_ps32_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == CVK_FMT_I8);
  int right_sign = (p->right->fmt == CVK_FMT_I8);
  int res_sign = (p->res->fmt == CVK_FMT_I8);
  int ret = 0;
  int bstride = res_row * res_col;

  int32_t *tmp_res = (int32_t *)malloc(sizeof(int32_t) * size);
  if (!tmp_res)
    return -1;

  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] = res[i + bstride*0];
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*1]<<8;
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*2]<<16;
  for (uint64_t i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*3]<<24;

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
    res[i + bstride*0] = tmp_res[i]>>0;
  for (uint64_t i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;

  free(tmp_res);

  return ret;
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


static int test_matrix_ps32_ut(CVI_RT_HANDLE rt_handle, cvk_context_t *cvk_ctx, param_t *p)
{
  int ret = 0;
  make_bmk_matrix_param_ps32(cvk_ctx, p, 2);
  uint8_t *left = alloc_left(p);
  uint8_t *right = alloc_right(p);
  uint8_t *ref = alloc_ps32_res(p);
  if (!left || !right || !ref) {
    ret = -1;
    goto fail_exit;
  }

  {
    ret = ps32_m2_matrix_mac_ref(p, left, right, ref);
    if (ret)
      goto fail_exit;

     matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->left, left);
     matrix_copy_s2d_g2l(rt_handle, cvk_ctx, p->right, right);
     cvk_ctx->ops->tiu_matrix_multiplication(cvk_ctx, p);
     cvk_ml_t ps32_res;
     ps32_res = *p->res;
     ps32_res.shape.n *= sizeof(int);
     uint8_t *res = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, &ps32_res);

     ret = array_cmp_int8(
         "Comparing begin_mode results ...\n",
         (int8_t *)ref, (int8_t *)res ,(int)res_ps32_size(p)*sizeof(int));
     if (ret) {
       printf("Comparison M2 FAILED\n");
       print_param(p);
     }else
       printf("Comparison M2 PASS\n");
     free(res);
  }

  {
    make_bmk_matrix_param_ps32(cvk_ctx, p, 3);

    ret = ps32_m3_matrix_mac_ref(p, left, right, ref);
    if (ret)
      goto fail_exit;

    cvk_ctx->ops->tiu_matrix_multiplication(cvk_ctx, p);
    cvk_ml_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= sizeof(int);
    uint8_t *res = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, &ps32_res);

    ret = array_cmp_int8(
        "Comparing m3 results ...\n",
        (int8_t *)ref, (int8_t *)res ,(int)res_ps32_size(p)*sizeof(int));
    if (ret) {
      printf("Comparison M3 FAILED\n");
      print_param(p);
    }else
      printf("Comparison M3 PASS\n");

    free(res);
  }
  {
    make_bmk_matrix_param_ps32(cvk_ctx, p, 1);
    uint16_t *bias = alloc_bias(p);

    ret = ps32_m1_matrix_mac_ref(p, left, right, bias, ref);
    if (ret)
      goto fail_exit;

    if(p->bias)
      put_bias(rt_handle, cvk_ctx, p->bias, bias);

    cvk_ctx->ops->tiu_matrix_multiplication(cvk_ctx, p);
    cvk_ml_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= 2;

    uint8_t *res = matrix_copy_l2g_d2s(rt_handle, cvk_ctx, &ps32_res);
    ret = array_cmp_int8(
        "Comparing m1 results ...\n",
        (int8_t *)ref, (int8_t *)res ,(int)res_size(p));
    if (ret) {
      printf("Comparison M1 FAILED\n");
      print_param(p);
    }else
      printf("Comparison M1 PASS\n");

    free(res);
    free(bias);
  }

fail_exit:
  free(left);
  free(right);
  free(ref);

  return ret;
}

static void destroy_param(cvk_context_t *cvk_ctx, param_t *p)
{
  if (p->bias)
    cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->bias);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->res);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->right);
  cvk_ctx->ops->lmem_free_matrix(cvk_ctx, p->left);
}

static cvk_fmt_t modify_res_fmt()
{
  cvk_fmt_t fmt = CVK_FMT_U8;
  if (matrix_para_t.left_sign == CVK_FMT_I8)
    fmt = CVK_FMT_I8;
  if (matrix_para_t.right_sign == CVK_FMT_I8)
    fmt = CVK_FMT_I8;
  if (matrix_para_t.using_bias)
    if (matrix_para_t.bias_sign == CVK_FMT_I8)
      fmt = CVK_FMT_I8;

//  if (matrix_para_t.relu_enable)
//    fmt = CVK_FMT_U8;

  return fmt;
}

static cvk_ml_t *alloc_param_res(
    cvk_context_t *cvk_ctx, param_t *p)
{
  cvk_ml_shape_t s;
  s.n = p->left->shape.n;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;

  cvk_fmt_t fmt = CVK_FMT_U8;
  fmt = modify_res_fmt();
  return cvk_ctx->ops->lmem_alloc_ps32_matrix(cvk_ctx, s, fmt, 1);
}


static void make_bmk_matrix_param_ps32(cvk_context_t *cvk_ctx, param_t *p, int ps32_mode)
{

  cvk_ml_shape_t left_shape;
  cvk_ml_shape_t right_shape;

  p->ps32_mode = ps32_mode;
  p->relu_enable = 0;
  p->lshift_bits = 0;
  p->rshift_bits = 0;

  if(ps32_mode==2)
  {
    left_shape.n = matrix_para_t.left_row;
    left_shape.c = matrix_para_t.left_c;
    left_shape.w = matrix_para_t.left_w;
    left_shape.col = matrix_para_t.left_col;

    right_shape.n = matrix_para_t.right_row;
    right_shape.c = matrix_para_t.right_c;
    right_shape.w = matrix_para_t.right_w;
    right_shape.col = matrix_para_t.right_col;
    p->left  = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, left_shape,  matrix_para_t.left_sign , 1);
    p->right = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, right_shape, matrix_para_t.right_sign, 1);
    p->bias = NULL;
    p->res = alloc_param_res(cvk_ctx, p);
  }else if(ps32_mode==3)
  {

  }else if(ps32_mode==1)
  {
     p->relu_enable = matrix_para_t.relu_enable;
     p->rshift_bits = matrix_para_t.rshift_bits;
     if(matrix_para_t.using_bias)
     {
       right_shape.n = matrix_para_t.right_row;
       right_shape.c = matrix_para_t.right_c;
       right_shape.w = matrix_para_t.right_w;
       right_shape.col = matrix_para_t.right_col;

       cvk_ml_shape_t bias_shape = right_shape;
       bias_shape.n = 2;
       p->bias = cvk_ctx->ops->lmem_alloc_matrix(cvk_ctx, bias_shape, matrix_para_t.bias_sign, 1);
       assert(p->bias);
    }
  }

}
static param_t param_init(void)
{
  param_t p;

  //srand(clock());

  memset(&p, 0, sizeof(param_t));
  memset(&matrix_para_t, 0, sizeof(matrix_init_para_t));

  matrix_para_t.rshift_bits = rand()%4+2;
  matrix_para_t.using_bias = rand()%2;
  matrix_para_t.relu_enable = rand()%2;
  matrix_para_t.right_sign = rand()%2? CVK_FMT_I8 : CVK_FMT_U8;
  matrix_para_t.left_sign = rand()%2? CVK_FMT_I8 : CVK_FMT_U8;

  if(matrix_para_t.using_bias)
    matrix_para_t.bias_sign = rand()%2? CVK_FMT_I8 : CVK_FMT_U8;

  if(matrix_para_t.right_sign != CVK_FMT_I8 && matrix_para_t.left_sign != CVK_FMT_I8)
    matrix_para_t.relu_enable=0;

  matrix_para_t.left_row = rand()%60+1;
  matrix_para_t.left_col = rand()%40+1;
  matrix_para_t.left_w = matrix_para_t.left_col/0x10 ? ((uint32_t)rand())%8+8 : matrix_para_t.left_col;
  //matrix_para_t.left_w = rand()%16+1;
  matrix_para_t.left_c =
    matrix_para_t.left_col%matrix_para_t.left_w?
      matrix_para_t.left_col/matrix_para_t.left_w+1 : matrix_para_t.left_col/matrix_para_t.left_w;

  matrix_para_t.right_row = matrix_para_t.left_col;
  matrix_para_t.right_col = rand()%50+1;
  //matrix_para_t.right_w = 16;
  matrix_para_t.right_w = rand()%16+1;
  matrix_para_t.right_c =
    matrix_para_t.right_col%matrix_para_t.right_w?
      matrix_para_t.right_col/matrix_para_t.right_w+1 : matrix_para_t.right_col/matrix_para_t.right_w;

  return p;
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

  for (int i = 0; i < 20; i++) {
    printf("random_test_conv iteration: %d\n", i);
    param_t p = param_init();

    ret |= test_matrix_ps32_ut(rt_handle, cvk_ctx, &p);
    destroy_param(cvk_ctx, &p);
  }

  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_DeInit(rt_handle);

  return ret;
}
