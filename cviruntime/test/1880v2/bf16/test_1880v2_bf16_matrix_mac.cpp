#include "../1880v2_test_util.h"

typedef bmk1880v2_tiu_matrix_multiplication_param_t param_t;
int random_seed;

static u64 matrix_size(const ml_t *ml)
{

  u64 row = ml->shape.n;
  u64 col = ml->shape.col;
  return row * col;
}

static u64 res_size(param_t *p)
{
    return matrix_size(p->res);
}

static u16 * alloc_left(param_t *p)
{
  u64 size = matrix_size(p->left);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }

  return buf;
}

static u16 * alloc_right(param_t *p)
{
  u64 size = matrix_size(p->right);
  u16 *buf = (u16 *)malloc(sizeof(u16) * size);
  for (u64 i = 0; i < size; i++) {
    float val = 0.01;
    buf[i] = convert_fp32_bf16(i);
    val += 0.01;
  }
  return buf;
}

static u32 * alloc_bias(param_t *p)
{
  if (!p->bias)
    return NULL;

  u64 size = matrix_size(p->bias);
  u32 *buf = (u32 *)malloc(sizeof(u32) * size);
  for (u64 i = 0; i < size; i++) {
    buf[i] = convert_fp32_hex(i);
  }
  return buf;
}

static u32 * alloc_res(param_t *p)
{
  u64 size = res_size(p);
  u32 *buf = (u32 *)malloc(sizeof(u32) * size);
  for (u64 i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }
  return buf;
}

static inline void bf16_relu(float *buf, u64 size)
{
  for (u64 i = 0; i < size; i++)
    if (buf[i] < 0)
      buf[i] = 0;
}

static void matrix_mac_ref(
    param_t *p, u16 left[], u16 right[], u32 bias[], u32 res[])
{
  u64 size = res_size(p);
  u32 left_col = p->left->shape.col;
  u32 right_col = p->right->shape.col;
  u32 res_row = p->left->shape.n;
  u32 res_col = p->res->shape.col;
  u32 left_c = p->left->shape.c; 
  u32 left_w = p->left->shape.w; 

  float *tmp_res = (float *)malloc(sizeof(float) * size);
  if (p->add_result) {
    for (u32 i = 0; i < res_row * res_col; i++)
      tmp_res[i] = convert_bf16_fp32(res[i]);
  } else {
    for (u32 i = 0; i < res_row * res_col; i++)
      tmp_res[i] = 0;
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

  for (u64 i = 0; i < size; i++) {
    res[i] = convert_fp32_bf16(tmp_res[i]);
  }
  free(tmp_res);
}

static void put_bias(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const ml_t *ml,
    u32 data[])
{
  u64 size = ml->shape.col;

  u16 *tmp = (u16 *)malloc(sizeof(u16) * size * 2);
  if (!tmp)
    return;

  for (u64 i = 0; i < size; i++) {
    tmp[i] = (data[i] >> 16) & 0xFFFF;
    tmp[i + size] = (data[i] & 0xFFFF);
  }

  put_bf16_matrix_g2l(ctx, bk_ctx, ml, (u8*)tmp, FMT_BF16);

  free(tmp);
}

static void put_res(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    const ml_t *ml,
    u32 data[])
{
  u64 size = ml->shape.n  * ml->shape.col;

  u16 *tmp = (u16 *)malloc(sizeof(u16) * size);
  if (!tmp)
    return;

  for (u64 i = 0; i < size; i++) {
    tmp[i] = (data[i] & 0xFFFF);
  }

  put_bf16_matrix_g2l(ctx, bk_ctx, ml, (u8*)tmp, FMT_BF16);

  free(tmp);
}

static u32 * get_res(
    CVI_RT_HANDLE *ctx,
    bmk_ctx_t *bk_ctx,
    param_t *p)
{
  u64 size = res_size(p);
  u32 *res = (u32 *)malloc(sizeof(u32) * size);

  u16 *tmp = (u16 *)get_bf16_matrix_l2g(ctx, bk_ctx, p->res, FMT_BF16);
  for (u64 i = 0; i < size; i++)
    res[i] = tmp[i];

  free(tmp);
  return res;
}

static void test_param(CVI_RT_HANDLE *ctx, bmk_ctx_t *bk_ctx, param_t *p)
{
  u16 *left = alloc_left(p);
  u16 *right = alloc_right(p);
  u32 *bias = alloc_bias(p);
  u32 *ref = alloc_res(p);
  put_bf16_matrix_g2l(ctx, bk_ctx, p->left, (u8*)left, FMT_BF16);
  put_bf16_matrix_g2l(ctx, bk_ctx, p->right, (u8*)right, FMT_BF16);
  if (bias)
    put_bias(ctx, bk_ctx, p->bias, bias);
  if (p->add_result)
    put_res(ctx, bk_ctx, p->res, ref);

  bmk1880v2_tiu_matrix_multiplication(bk_ctx, p);
  u32 *res = get_res(ctx, bk_ctx, p);
  matrix_mac_ref(p, left, right, bias, ref);
  u64 size = res_size(p);
  for (u64 i = 0; i < size; i++) {
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

static void destroy_param(bmk_ctx_t *bk_ctx, param_t *p)
{
  if (p->bias)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->bias);
  if (p->res)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->res);
  if (p->right)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->right);
  if (p->left)
    bmk1880v2_lmem_free_matrix(bk_ctx, p->left);
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
  return bmk1880v2_lmem_alloc_matrix(bk_ctx, s, fmt, 1);
}

static param_t param_0(bmk_ctx_t *bk_ctx)
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

  u32 left_row = rand() % 100 +1;
  u32 left_col = rand() % 100 + 1;
  u32 left_w = rand() % (left_col/5+1) + 1; // c is generate by w, and make c is larger
  u32 left_c = left_col / left_w + (left_col % left_w ? 1: 0);

  u32 right_row = left_col;
  u32 right_col = rand() % 100 + 1;
  u32 right_w = (rand() % (right_col/5+1) + 1); // make c is larger
  u32 right_c = right_col / right_w + (right_col % right_w ? 1: 0) ;

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
  
  u32 bias = rand()%2;
  p.bias = NULL;
  p.left = bmk1880v2_lmem_alloc_matrix(bk_ctx, left_shape, FMT_BF16, 1);
  p.right = bmk1880v2_lmem_alloc_matrix(bk_ctx, right_shape, FMT_BF16, 1);
  if (!p.left || !p.right) {
    printf("retry init_matrix_param\n");
    destroy_param(bk_ctx, &p);
    goto retry;
  }

  p.res = alloc_param_res(bk_ctx, &p);
  if (bias) {
    ml_shape_t bias_shape = right_shape;
    bias_shape.n = 2;
    p.bias = bmk1880v2_lmem_alloc_matrix(bk_ctx, bias_shape, FMT_BF16, 1);
  }

  if (!p.res || (bias && !p.bias)) {
    printf("retry init_matrix_param\n");
    destroy_param(bk_ctx, &p);
    goto retry;
  }

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
  int round_mode;
  round_mode = set_store_feround();

  for (int i = 0 ; i < 30 ; i++)
    test_one_param(0);

  restore_feround(round_mode);
  test_exit(&ctx);
  return 0;
}
