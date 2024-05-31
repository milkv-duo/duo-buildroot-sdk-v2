#include "1880v2_test_util.h"

typedef bmk1880v2_tiu_matrix_multiplication_param_t param_t;

typedef struct{
  fmt_t left_sign;
  u32 left_row ;
  u32 left_col ;
  u32 left_c ;
  u32 left_w ;
  fmt_t right_sign;
  u32 right_row ;
  u32 right_col ;
  u32 right_c ;
  u32 right_w ;
  u32 lshift_bits ;
  u32 rshift_bits ;
  u32 relu_enable ;
  u32 using_bias;
  fmt_t bias_sign;
} matrix_init_para_t;

matrix_init_para_t matrix_para_t;

static void make_bmk_matrix_param_ps32(bmk_ctx_t *bk_ctx, param_t *p, int ps32_mode);
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


static u64 matrix_size(const ml_t *ml)
{
  u64 row = ml->shape.n;
  u64 col = ml->shape.col;
  return row * col;
}

static u64 res_ps32_size(param_t *p)
{
    return matrix_size(p->res);
}

static u64 res_size(param_t *p)
{
  if (p->res_is_int8 && !p->add_result)
    return matrix_size(p->res);
  else
    return matrix_size(p->res) *2 ;
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

static u8 * alloc_ps32_res(param_t *p)
{
  u64 size = res_ps32_size(p)*4;
  u8 *buf = (u8 *)malloc(sizeof(u8) * size);
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

static int ps32_m2_matrix_mac_ref(
  param_t *p,
  u8 *left,
  u8 *right,
  u8 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == FMT_I8);
  int right_sign = (p->right->fmt == FMT_I8);
  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  s32 *tmp_res = (s32 *)malloc(sizeof(s32) * size);
  for (u32 i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
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

  for (u64 i = 0; i < size; i++)
    res[i + bstride*0] = tmp_res[i]>>0;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*2] = tmp_res[i]>>16;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*3] = tmp_res[i]>>24;

  free(tmp_res);

  return ret;
}

static int ps32_m3_matrix_mac_ref(
  param_t *p,
  u8 *left,
  u8 *right,
  u8 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == FMT_I8);
  int right_sign = (p->right->fmt == FMT_I8);
  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  u32 *tmp_res = (u32 *)malloc(sizeof(u32) * size);

  for (u64 i = 0; i < size; i++)
    tmp_res[i] = res[i + bstride*0];
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*1]<<8;
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*2]<<16;
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*3]<<24;

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

  for (u64 i = 0; i < size; i++)
    res[i + bstride*0] = tmp_res[i]>>0;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*2] = tmp_res[i]>>16;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*3] = tmp_res[i]>>24;

  free(tmp_res);

  return ret;
}

static int ps32_m1_matrix_mac_ref(
  param_t *p,
  u8 *left,
  u8 *right,
  u16 * bias,
  u8 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  int left_sign = (p->left->fmt == FMT_I8);
  int right_sign = (p->right->fmt == FMT_I8);
  int res_sign = (p->res->fmt == FMT_I8);
  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  s32 *tmp_res = (s32 *)malloc(sizeof(s32) * size);

  for (u64 i = 0; i < size; i++)
    tmp_res[i] = res[i + bstride*0];
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*1]<<8;
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*2]<<16;
  for (u64 i = 0; i < size; i++)
    tmp_res[i] |= res[i + bstride*3]<<24;

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
    res[i + bstride*0] = tmp_res[i]>>0;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*1] = tmp_res[i]>>8;

  free(tmp_res);

  return ret;
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


static int test_matrix_ps32_ut(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, param_t *p)
{
  make_bmk_matrix_param_ps32(bk_ctx, p, 2);
  u8 *left = alloc_left(p);
  u8 *right = alloc_right(p);
  u8 *ref = alloc_ps32_res(p);

  {
     bmerr_t ret = ps32_m2_matrix_mac_ref(p, left, right, ref);
     assert(ret == BM_SUCCESS);

     put_matrix_g2l(ctx, bk_ctx, p->left, left);
     put_matrix_g2l(ctx, bk_ctx, p->right, right);
     bmk1880v2_tiu_matrix_multiplication(bk_ctx, p);
     bmk1880v2_matrix_lmem_t ps32_res;
     ps32_res = *p->res;
     ps32_res.shape.n *= sizeof(int);
     u8 *res = get_matrix_l2g(ctx, bk_ctx, &ps32_res);

     int has_error = array_cmp_int8(
         "Comparing begin_mode results ...\n",
         (s8 *)ref, (s8 *)res ,(int)res_ps32_size(p)*sizeof(int));
     if (has_error) {
       printf("Comparison M2 FAILED\n");
       print_param(p);
       exit(-1);
     }else
       printf("Comparison M2 PASS\n");
     free(res);
  }

  {
    make_bmk_matrix_param_ps32(bk_ctx, p, 3);

    bmerr_t ret = ps32_m3_matrix_mac_ref(p, left, right, ref);
    assert(ret == BM_SUCCESS);

    bmk1880v2_tiu_matrix_multiplication(bk_ctx, p);
    bmk1880v2_matrix_lmem_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= sizeof(int);
    u8 *res = get_matrix_l2g(ctx, bk_ctx, &ps32_res);

    int has_error = array_cmp_int8(
        "Comparing m3 results ...\n",
        (s8 *)ref, (s8 *)res ,(int)res_ps32_size(p)*sizeof(int));
    if (has_error) {
      printf("Comparison M3 FAILED\n");
      print_param(p);
      exit(-1);
    }else
      printf("Comparison M3 PASS\n");

    free(res);
  }
  {
    make_bmk_matrix_param_ps32(bk_ctx, p, 1);
    u16 *bias = alloc_bias(p);

    bmerr_t ret = ps32_m1_matrix_mac_ref(p, left, right, bias, ref);
    assert(ret == BM_SUCCESS);

    if(p->bias)
      put_bias(ctx, bk_ctx, p->bias, bias);

    bmk1880v2_tiu_matrix_multiplication(bk_ctx, p);
    bmk1880v2_matrix_lmem_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= 2;

    u8 *res = get_matrix_l2g(ctx, bk_ctx, &ps32_res);
    int has_error = array_cmp_int8(
        "Comparing m1 results ...\n",
        (s8 *)ref, (s8 *)res ,(int)res_size(p));
    if (has_error) {
      printf("Comparison M1 FAILED\n");
      print_param(p);
      exit(-1);
    }else
      printf("Comparison M1 PASS\n");

    free(res);
    free(bias);
  }
    free(left);
    free(right);
    free(ref);
  return 1;
}

static void destroy_param(bmk_ctx_t *bk_ctx, param_t *p)
{
  if (p->bias)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->bias);
  bmk1880v2_lmem_free_matrix(bk_ctx, p->res);
  bmk1880v2_lmem_free_matrix(bk_ctx, p->right);
  bmk1880v2_lmem_free_matrix(bk_ctx, p->left);
}

static fmt_t modify_res_fmt()
{
  // Note:
  //   From 7/29/2019 update H/W relu design,
  //   res0_sign can be assigned and is not affected by relu.
  //   Kernel will use result data format to set res0_sign.
  fmt_t fmt = FMT_U8;
  if (matrix_para_t.left_sign == FMT_I8)
    fmt = FMT_I8;
  if (matrix_para_t.right_sign == FMT_I8)
    fmt = FMT_I8;
  if (matrix_para_t.using_bias)
    if (matrix_para_t.bias_sign == FMT_I8)
      fmt = FMT_I8;

  return fmt;
}

static ml_t *alloc_param_res(
    bmk_ctx_t *bk_ctx, param_t *p)
{
  ml_shape_t s;
  s.n = p->left->shape.n;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;

  fmt_t fmt = FMT_U8;
  fmt = modify_res_fmt();
  return bmk1880v2_lmem_alloc_ps32_matrix(bk_ctx, s, fmt, 1);
}


static void make_bmk_matrix_param_ps32(bmk_ctx_t *bk_ctx, param_t *p, int ps32_mode)
{

  ml_shape_t left_shape;
  ml_shape_t right_shape;

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
    p->left  = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape,  matrix_para_t.left_sign , 1);
    p->right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, matrix_para_t.right_sign, 1);
    p->bias = NULL;
    p->res = alloc_param_res(bk_ctx, p);
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

       ml_shape_t bias_shape = right_shape;
       bias_shape.n = 2;
       p->bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, matrix_para_t.bias_sign, 1);
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
  matrix_para_t.right_sign = rand()%2? FMT_I8 : FMT_U8;
  matrix_para_t.left_sign = rand()%2? FMT_I8 : FMT_U8;

  if(matrix_para_t.using_bias)
    matrix_para_t.bias_sign = rand()%2? FMT_I8 : FMT_U8;

  if(matrix_para_t.right_sign != FMT_I8 && matrix_para_t.left_sign != FMT_I8)
    matrix_para_t.relu_enable=0;

  matrix_para_t.left_row = rand()%60+1;
  matrix_para_t.left_col = rand()%40+1;
  matrix_para_t.left_w = matrix_para_t.left_col/0x10 ? rand()%8+8 : matrix_para_t.left_col;
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

int main()
{
  CVI_RT_HANDLE ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);

  int test_finished_num = 0;
  for (int i = 0; i < 20; i++) {
    printf("random_test_conv iteration: %d\n", i);
    param_t p = param_init();

    test_finished_num += test_matrix_ps32_ut(&ctx, bk_ctx, &p);
    destroy_param(bk_ctx, &p);
  }
  printf("test_finished_num: %d\n", test_finished_num);

  test_exit(&ctx);
  return 0;
}
