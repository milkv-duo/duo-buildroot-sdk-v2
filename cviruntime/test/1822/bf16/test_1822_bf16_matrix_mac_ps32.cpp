#include "../1822_test_util.h"

typedef bmk1822_tiu_matrix_multiplication_param_t param_t;

typedef struct{
  u32 left_sign;
  u32 left_row ;
  u32 left_col ;
  u32 left_c ;
  u32 left_w ;
  u32 right_sign;
  u32 right_row ;
  u32 right_col ;
  u32 right_c ;
  u32 right_w ;
  u32 lshift_bits ;
  u32 rshift_bits ;
  u32 relu_enable ;
  u32 using_bias;
  u32 bias_sign;
} matrix_init_para_t;

u32 random_seed;
matrix_init_para_t matrix_para_t;

static void make_bmk_matrix_param_ps32(bmk_ctx_t *bk_ctx, param_t *p, int ps32_mode);
static param_t param_init();

void print_param(param_t *p)
{
  printf("random_seed =%d\n", random_seed);
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
    return matrix_size(p->res);
}

static u16 * alloc_left(param_t *p)
{
  u64 size = matrix_size(p->left);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = convert_fp32_bf16(i);

  return buf;
}

static u16 * alloc_right(param_t *p)
{
  u64 size = matrix_size(p->right);

  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = convert_fp32_bf16(i);

  return buf;
}
static u32 * alloc_bias(param_t *p)
{
  if (!p->bias)
    return NULL;

  u64 size = matrix_size(p->bias) / 2;

  u32 *buf = (u32 *)malloc(sizeof(u32) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = convert_fp32_hex(i);

  return buf;
}

static u16 * alloc_ps32_res(param_t *p)
{
  u64 size = res_ps32_size(p)*2;
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++)
    buf[i] = convert_fp32_bf16(i);

  return buf;
}

static inline void bf16_relu(float *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static int ps32_m2_matrix_mac_ref(
  param_t *p,
  u16 *left,
  u16 *right,
  u16 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  u32 left_c = p->left->shape.c;
  u32 left_w = p->left->shape.w;

  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  float *tmp_res = (float *)malloc(sizeof(float) * size);
  for (u32 i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
  for (u32 row = 0; row < res_row; row++) {
    for (u32 col = 0; col < res_col; col++) {
      for (u32 wi = 0; wi < left_w; wi++) {
        for (u32 ci = 0; ci < left_c; ci++) {
          if ((wi + (ci*left_w)) >= left_col)
            continue;
          u32 li = row * left_col + left_w * ci + wi;
          u32 ri = (ci* left_w + wi )* right_col + col;

          float l = convert_bf16_fp32(left[li]);
          float r = convert_bf16_fp32(right[ri]);
          tmp_res[row * res_col + col] += l * r;
        }
      }
    }
  }

  for (u64 i = 0; i < size; i++)
    res[i + bstride*0] = (convert_fp32_hex(tmp_res[i]) >> 16) & 0xFFFF;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*1] = (convert_fp32_hex(tmp_res[i]) >> 0) & 0xFFFF;

  free(tmp_res);

  return ret;
}

static int ps32_m3_matrix_mac_ref(
  param_t *p,
  u16 *left,
  u16 *right,
  u16 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  u32 left_c = p->left->shape.c;
  u32 left_w = p->left->shape.w;

  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  float *tmp_res = (float *)malloc(sizeof(float) * size);

  for (u64 i = 0; i < size; i++)
    tmp_res[i] = convert_hex_fp32((res[i + bstride*0] << 16) | res[i + bstride*1]);

  for (u32 row = 0; row < res_row; row++) {
    for (u32 col = 0; col < res_col; col++) {
      for (u32 wi = 0; wi < left_w; wi++) {
        for (u32 ci = 0; ci < left_c; ci++) {
          if ((wi + (ci*left_w)) >= left_col)
            continue;
          u32 li = row * left_col + left_w * ci + wi;
          u32 ri = (ci* left_w + wi )* right_col + col;

          float l = convert_bf16_fp32(left[li]);
          float r = convert_bf16_fp32(right[ri]);
          tmp_res[row * res_col + col] += l * r;
        }
      }
    }
  }

  for (u64 i = 0; i < size; i++)
    res[i + bstride*0] = (convert_fp32_hex(tmp_res[i]) >> 16) & 0xFFFF;
  for (u64 i = 0; i < size; i++)
    res[i + bstride*1] = (convert_fp32_hex(tmp_res[i]) >> 0) & 0xFFFF;

  free(tmp_res);

  return ret;
}

static int ps32_m1_matrix_mac_ref(
  param_t *p,
  u16 *left,
  u16 *right,
  u32 * bias,
  u16 *res)
{
  u64 size = res_ps32_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  u32 left_c = p->left->shape.c;
  u32 left_w = p->left->shape.w;

  int ret = BM_SUCCESS;
  int bstride = res_row * res_col;

  float *tmp_res = (float *)malloc(sizeof(float) * size);

  for (u64 i = 0; i < size; i++) {
    tmp_res[i] = convert_hex_fp32((res[i + bstride*0] << 16) | res[i + bstride*1]);
   }

  for (u32 row = 0; row < res_row; row++) {
    for (u32 col = 0; col < res_col; col++) {
      for (u32 wi = 0; wi < left_w; wi++) {
        for (u32 ci = 0; ci < left_c; ci++) {
          if ((wi + (ci*left_w)) >= left_col)
            continue;
          u32 li = row * left_col + left_w * ci + wi;
          u32 ri = (ci* left_w + wi )* right_col + col;

          float l = convert_bf16_fp32(left[li]);
          float r = convert_bf16_fp32(right[ri]);
          tmp_res[row * res_col + col] += l * r;
        }
      }
    }
  }

  if (p->bias && bias) {
    for (u32 row = 0; row < res_row; row++) {
      for (u32 col = 0; col < res_col; col++) {
        float b = convert_hex_fp32(bias[col]);
        tmp_res[row * res_col + col] += b;
      }
    }
  }

  if (p->relu_enable)
    bf16_relu(tmp_res, size);

  for (u64 i = 0; i < size; i++)
    res[i] = convert_fp32_bf16(tmp_res[i]);

  free(tmp_res);

  return ret;
}

static void put_bias(
    bmctx_t *ctx,
    bmk_ctx_t *bk_ctx,
    const ml_t *ml,
    u32 data[])
{
  u64 size = ml->shape.col;

  u16 *tmp = (u16 *)malloc(sizeof(u16) * size * 2);
  if(!tmp)
    return;

  for (u64 i = 0; i < size; i++) {
    tmp[i] = data[i] >> 16;
    tmp[i + size] = data[i] & 0xFFFF;
  }
  put_bf16_matrix_g2l(ctx, bk_ctx, ml, (u8*) tmp, FMT_BF16);

  free(tmp);
}


static int test_matrix_ps32_ut(bmctx_t *ctx, bmk_ctx_t *bk_ctx, param_t *p)
{
  make_bmk_matrix_param_ps32(bk_ctx, p, 2);
  u16 *left = alloc_left(p);
  u16 *right = alloc_right(p);
  u16 *ref = alloc_ps32_res(p);

  {
     bmerr_t ret = ps32_m2_matrix_mac_ref(p, left, right, ref);
     assert(ret == BM_SUCCESS);

     put_bf16_matrix_g2l(ctx, bk_ctx, p->left, (u8*) left, FMT_BF16);
     put_bf16_matrix_g2l(ctx, bk_ctx, p->right, (u8*) right, FMT_BF16);
     bmk1822_tiu_matrix_multiplication(bk_ctx, p);
     bmk1822_matrix_lmem_t ps32_res;
     ps32_res = *p->res;
     ps32_res.shape.n *= sizeof(short);
     u16 *res = (u16*) get_bf16_matrix_l2g(ctx, bk_ctx, &ps32_res, FMT_BF16);

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

    bmk1822_tiu_matrix_multiplication(bk_ctx, p);
    bmk1822_matrix_lmem_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= sizeof(short);
    u16 *res = (u16 *) get_bf16_matrix_l2g(ctx, bk_ctx, &ps32_res, FMT_BF16);

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
    u32 *bias = alloc_bias(p);

    bmerr_t ret = ps32_m1_matrix_mac_ref(p, left, right, bias, ref);
    assert(ret == BM_SUCCESS);

    if(p->bias)
      put_bias(ctx, bk_ctx, p->bias, bias);

    bmk1822_tiu_matrix_multiplication(bk_ctx, p);
    bmk1822_matrix_lmem_t ps32_res;
    ps32_res = *p->res;
    ps32_res.shape.n *= 2;

    u16 *res = (u16 *) get_bf16_matrix_l2g(ctx, bk_ctx, &ps32_res, FMT_BF16);

    int has_error = array_cmp_int8(
        "Comparing m1 results ...\n",
        (s8 *)ref, (s8 *)res ,(int)res_size(p)*2);
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
    bmk1822_lmem_free_matrix(bk_ctx, p->bias);
  bmk1822_lmem_free_matrix(bk_ctx, p->res);
  bmk1822_lmem_free_matrix(bk_ctx, p->right);
  bmk1822_lmem_free_matrix(bk_ctx, p->left);
}

static ml_t *alloc_param_res(
    bmk_ctx_t *bk_ctx, param_t *p)
{
  ml_shape_t s;
  s.n = p->left->shape.n;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;

  fmt_t fmt = FMT_BF16;
  return bmk1822_lmem_alloc_ps32_matrix(bk_ctx, s, fmt, 1);
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
    p->left  = bmk1822_lmem_alloc_matrix(bk_ctx, left_shape, FMT_BF16, 1);
    p->right = bmk1822_lmem_alloc_matrix(bk_ctx, right_shape, FMT_BF16, 1);
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
       p->bias = bmk1822_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_BF16, 1);
       assert(p->bias);
    }
  }
  //print_param(p);
}

static param_t param_init(void)
{
  param_t p;

  random_seed = clock();
  srand(random_seed);

  memset(&p, 0, sizeof(param_t));
  memset(&matrix_para_t, 0, sizeof(matrix_init_para_t));

  matrix_para_t.using_bias = rand()%2;
  matrix_para_t.relu_enable = rand()%2;

  matrix_para_t.left_row = rand()%60+1;
  matrix_para_t.left_col = rand()%40+1;
  matrix_para_t.left_w = matrix_para_t.left_col/0x10 ? rand()%8+8 : matrix_para_t.left_col;
  matrix_para_t.left_c =
    matrix_para_t.left_col%matrix_para_t.left_w?
      matrix_para_t.left_col/matrix_para_t.left_w+1 : matrix_para_t.left_col/matrix_para_t.left_w;

  matrix_para_t.right_row = matrix_para_t.left_col;
  matrix_para_t.right_col = rand()%50+1;
  matrix_para_t.right_w = rand()%16+1;
  matrix_para_t.right_c =
    matrix_para_t.right_col%matrix_para_t.right_w?
      matrix_para_t.right_col/matrix_para_t.right_w+1 : matrix_para_t.right_col/matrix_para_t.right_w;
  return p;
}

int main()
{
  bmctx_t ctx;
  bmk_ctx_t *bk_ctx;
  test_init(&ctx, &bk_ctx);
  int round_mode;
  round_mode = set_store_feround();

  int test_finished_num = 0;
  for (int i = 0; i < 30; i++) {
    printf("random_test_conv iteration: %d\n", i);
    param_t p = param_init();

    test_finished_num += test_matrix_ps32_ut(&ctx, bk_ctx, &p);
    destroy_param(bk_ctx, &p);
  }
  printf("test_finished_num: %d\n", test_finished_num);
  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
