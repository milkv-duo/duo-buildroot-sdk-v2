#include <cvimath_internal.h>
#include <sys/time.h>
#include <test_cvikernel_util.h>
#include <time.h>  // clock

typedef cvk_tiu_matrix_multiplication_param_t param_t;
int random_seed;

static uint64_t matrix_size(const cvk_ml_t *ml) {
  uint64_t row = ml->shape.n;
  uint64_t col = ml->shape.col;
  return row * col;
}

static uint64_t res_size(param_t *p) { return matrix_size(p->res); }

static uint16_t *alloc_left(param_t *p) {
  uint64_t size = matrix_size(p->left);
  uint16_t *buf = new uint16_t[size];
  for (uint64_t i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }

  return buf;
}

static uint16_t *alloc_right(param_t *p) {
  uint64_t size = matrix_size(p->right);
  uint16_t *buf = new uint16_t[size];
  for (uint64_t i = 0; i < size; i++) {
    float val = 0.01;
    buf[i] = convert_fp32_bf16(i);
    val += 0.01;
  }
  return buf;
}

static uint32_t *alloc_bias(param_t *p) {
  if (!p->bias) return NULL;

  uint64_t size = matrix_size(p->bias);
  uint32_t *buf = new uint32_t[size];
  for (uint64_t i = 0; i < size; i++) {
    buf[i] = convert_fp32_hex(i);
  }
  return buf;
}

static uint32_t *alloc_res(param_t *p) {
  uint64_t size = res_size(p);
  uint32_t *buf = new uint32_t[size];
  for (uint64_t i = 0; i < size; i++) {
    buf[i] = convert_fp32_bf16(i);
  }
  return buf;
}

static inline void cvm_relu(float *buf, uint64_t size) {
  for (uint64_t i = 0; i < size; i++)
    if (buf[i] < 0) buf[i] = 0;
}

static void matrix_mac_ref(param_t *p, uint16_t left[], uint16_t right[], uint32_t bias[],
                           uint32_t res[]) {
  uint64_t size = res_size(p);
  uint32_t left_col = p->left->shape.col;
  uint32_t right_col = p->right->shape.col;
  uint32_t res_row = p->left->shape.n;
  uint32_t res_col = p->res->shape.col;
  uint32_t left_c = p->left->shape.c;
  uint32_t left_w = p->left->shape.w;

  float *tmp_res = new float[size];
  if (p->add_result) {
    for (uint32_t i = 0; i < res_row * res_col; i++) tmp_res[i] = convert_bf16_fp32(res[i]);
  } else {
    for (uint32_t i = 0; i < res_row * res_col; i++) tmp_res[i] = 0;
  }
  for (uint32_t row = 0; row < res_row; row++) {
    for (uint32_t col = 0; col < res_col; col++) {
      for (uint32_t wi = 0; wi < left_w; wi++) {
        for (uint32_t ci = 0; ci < left_c; ci++) {
          if ((wi + (ci * left_w)) >= left_col) continue;
          uint32_t li = row * left_col + left_w * ci + wi;
          uint32_t ri = (ci * left_w + wi) * right_col + col;

          float l = convert_bf16_fp32(left[li]);
          float r = convert_bf16_fp32(right[ri]);
          tmp_res[row * res_col + col] += l * r;
        }
      }
    }
  }

  if (p->bias) {
    for (uint32_t row = 0; row < res_row; row++) {
      for (uint32_t col = 0; col < res_col; col++) {
        float b = convert_hex_fp32(bias[col]);
        tmp_res[row * res_col + col] += b;
      }
    }
  }

  if (p->relu_enable) cvm_relu(tmp_res, size);

  for (uint64_t i = 0; i < size; i++) {
    res[i] = convert_fp32_bf16(tmp_res[i]);
  }
  delete[] tmp_res;
}

static void put_bias(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx, const cvk_ml_t *ml,
                     uint32_t data[]) {
  uint64_t size = ml->shape.col;

  uint16_t *tmp = new uint16_t[size * 2];
  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = (data[i] >> 16) & 0xFFFF;
    tmp[i + size] = (data[i] & 0xFFFF);
  }

  test_put_matrix_g2l_comp(ctx, bk_ctx, ml, (uint8_t *)tmp);

  delete[] tmp;
}

static void put_res(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx, const cvk_ml_t *ml,
                    uint32_t data[]) {
  uint64_t size = ml->shape.n * ml->shape.col;

  uint16_t *tmp = new uint16_t[size];
  for (uint64_t i = 0; i < size; i++) {
    tmp[i] = (data[i] & 0xFFFF);
  }

  test_put_matrix_g2l_comp(ctx, bk_ctx, ml, (uint8_t *)tmp);

  delete[] tmp;
}

static uint32_t *get_res(CVI_RT_HANDLE *ctx, cvk_mg_t *mg, param_t *p) {
  uint64_t size = res_size(p);
  uint32_t *res = new uint32_t[size];

  uint16_t *tmp = (uint16_t *)test_get_mg_mem_comp(ctx, mg);
  for (uint64_t i = 0; i < size; i++) res[i] = tmp[i];

  delete[] tmp;
  return res;
}

static inline cvk_mg_t *put_bf16_matrix_g(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx,
                                          const cvk_ml_t *ml, uint8_t data[],
                                          cvk_fmt_t mg_data_format) {
  cvk_mg_shape_t s;
  s.row = ml->shape.n;
  s.col = ml->shape.col;
  cvk_mg_t *mg = test_alloc_mg_mem_comp(ctx, s, mg_data_format);

  test_put_mg_mem_comp(ctx, mg, data);
  test_submit_comp(ctx, bk_ctx);

  return mg;
}

static void test_param(CVI_RT_HANDLE *ctx, cvk_context_t *bk_ctx, param_t *p) {
  uint16_t *left = alloc_left(p);
  uint16_t *right = alloc_right(p);
  uint32_t *bias = alloc_bias(p);
  uint32_t *ref = alloc_res(p);

  cvk_mg_t *left_mg = put_bf16_matrix_g(ctx, bk_ctx, p->left, (uint8_t *)left, CVK_FMT_BF16);
  cvk_mg_t *right_mg = put_bf16_matrix_g(ctx, bk_ctx, p->right, (uint8_t *)right, CVK_FMT_BF16);
  cvk_mg_shape_t s;
  s.row = p->res->shape.n;
  s.col = p->res->shape.col;
  cvk_mg_t *result_mg = test_alloc_mg_mem_comp(ctx, s, CVK_FMT_BF16);

  if (bias) put_bias(ctx, bk_ctx, p->bias, bias);
  if (p->add_result) put_res(ctx, bk_ctx, p->res, ref);

  printf("start\n");
  size_t *slice_num =
      cvm_gemm(bk_ctx, left_mg->start_address, right_mg->start_address, result_mg->start_address,
               p->left->shape.n, p->left->shape.col, p->res->shape.col, CVK_FMT_BF16);
  free(slice_num);  // no need use in bf16
  test_submit_comp(ctx, bk_ctx);

  uint32_t *res = get_res(ctx, result_mg, p);
  matrix_mac_ref(p, left, right, bias, ref);

  uint64_t size = res_size(p);
  for (uint64_t i = 0; i < size; i++) {
    if (res[i] != ref[i]) {
      uint16_t _res = res[i] & 0xffff;
      uint16_t _ref = ref[i] & 0xffff;
      fprintf(stderr, "comparing failed at out[%lu], got %f(0x%x), exp %f(0x%x)\n", i,
              convert_bf16_fp32(_res), res[i], convert_bf16_fp32(_ref), ref[i]);
      fprintf(stderr, "random_seed=%d\n", random_seed);
      exit(-1);
    }
  }

  test_free_mg_mem_comp(ctx, left_mg);
  test_free_mg_mem_comp(ctx, right_mg);
  test_free_mg_mem_comp(ctx, result_mg);

  delete[] left;
  delete[] right;
  delete[] bias;
  delete[] res;
}

static void destroy_param(cvk_context_t *bk_ctx, param_t *p) {
  if (p->bias) bk_ctx->ops->lmem_free_matrix(bk_ctx, p->bias);
  if (p->res) bk_ctx->ops->lmem_free_matrix(bk_ctx, p->res);
  if (p->right) bk_ctx->ops->lmem_free_matrix(bk_ctx, p->right);
  if (p->left) bk_ctx->ops->lmem_free_matrix(bk_ctx, p->left);
}

static cvk_ml_t *alloc_param_res(cvk_context_t *bk_ctx, param_t *p) {
  cvk_ml_shape_t s;

  s.n = p->left->shape.n;
  s.c = p->right->shape.c;
  s.w = p->right->shape.w;
  s.col = p->right->shape.col;
  cvk_fmt_t fmt = CVK_FMT_BF16;
  cvk_ml_shape_t fake;
  fake.n = 1;
  fake.c = 1;
  fake.w = 1;
  fake.col = 1;
  cvk_ml_t *t = bk_ctx->ops->lmem_alloc_matrix(bk_ctx, fake, fmt, 1);
  t->shape = s;
  return t;
}

static param_t param_0(cvk_context_t *bk_ctx) {
retry:
  random_seed = clock();
  srand(random_seed);

  param_t p;
  memset(&p, 0, sizeof(p));
  p.lshift_bits = 0;
  p.rshift_bits = 0;
  p.res_is_int8 = true;
  p.relu_enable = rand() % 2;
  p.relu_enable = 0;
  p.add_result = 0; /*bf16 HW does not support add_result*/
  p.ps32_mode = 0;

  uint32_t left_row = rand() % 100 + 1;
  uint32_t left_col = rand() % 100 + 1;
  left_row = 1024;
  left_col = 1024;
  uint32_t left_w = rand() % (left_col / 5 + 1) + 1;  // c is generate by w, and make c is larger
  uint32_t left_c = left_col / left_w + (left_col % left_w ? 1 : 0);

  uint32_t right_row = left_col;
  uint32_t right_col = rand() % 100 + 1;
  right_col = 1024;
  uint32_t right_w = (rand() % (right_col / 5 + 1) + 1);  // make c is larger
  uint32_t right_c = right_col / right_w + (right_col % right_w ? 1 : 0);

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

  uint32_t bias = rand() % 2;
  bias = 0;
  p.bias = NULL;

  cvk_ml_shape_t fake;
  fake.n = 1;
  fake.c = 1;
  fake.w = 1;
  fake.col = 1;

  cvk_ml_t *t = bk_ctx->ops->lmem_alloc_matrix(bk_ctx, fake, CVK_FMT_BF16, 1);
  t->shape = left_shape;
  p.left = t;

  t = bk_ctx->ops->lmem_alloc_matrix(bk_ctx, fake, CVK_FMT_BF16, 1);
  t->shape = right_shape;
  p.right = t;
  if (!p.left || !p.right) {
    printf("retry init_matrix_param\n");
    destroy_param(bk_ctx, &p);
    goto retry;
  }

  p.res = alloc_param_res(bk_ctx, &p);
  if (bias) {
    cvk_ml_shape_t bias_shape = right_shape;
    bias_shape.n = 2;
    p.bias = bk_ctx->ops->lmem_alloc_matrix(bk_ctx, bias_shape, CVK_FMT_BF16, 1);
  }

  if (!p.res || (bias && !p.bias)) {
    printf("retry init_matrix_param\n");
    destroy_param(bk_ctx, &p);
    goto retry;
  }

  return p;
}

// gemm test function
//#define USE_CBLAS_VERITY (1)

#ifdef USE_CBLAS_VERITY
#include <cblas.h>
#endif /* ifdef USE_CBLAS_VERITY */

// comes from
// https://stackoverflow.com/questions/47023651/multiplying-matrices-in-one-dimensional-arrays
void multiply(uint16_t *a, int row1, int col1, uint16_t *b, int row2, int col2, uint16_t *d) {
  assert(col1 == row2);
  // silence error=unused-but-set-parameter warning
  (void)row2;

  for (int i = 0; i < row1; i++) {
    for (int j = 0; j < col2; j++) {
      float sum = 0;
      for (int k = 0; k < col1; k++) {
        float _a = convert_bf16_fp32(a[i * col1 + k]);
        float _b = convert_bf16_fp32(b[k * col2 + j]);
        sum = sum + _a * _b;
      }
      d[i * col2 + j] = convert_fp32_bf16(sum);
    }
  }

#if 0
    for (int i = 0; i < size; i++) {
        if (i % col2 == 0) {
            printf("\n");
        }
        printf("%f ", convert_bf16_fp32(d[i]));
    }
#endif
}

#ifdef USE_CBLAS_VERITY
#else
static void multiply_i32(uint8_t *a, int row1, int col1, uint8_t *b, int row2, int col2,
                         uint32_t *d, cvk_fmt_t fmt) {
  assert(col1 == row2);
  // silence error=unused-but-set-parameter warning
  (void)row2;

  for (int i = 0; i < row1; i++) {
    for (int j = 0; j < col2; j++) {
      int sum = 0;
      for (int k = 0; k < col1; k++) {
        int _a = fmt == CVK_FMT_I8 ? (int8_t)(a[i * col1 + k]) : (a[i * col1 + k]);
        int _b = fmt == CVK_FMT_I8 ? (int8_t)(b[k * col2 + j]) : (b[k * col2 + j]);
        // printf("sum = sum + _a * _b = %d = %d + %d * %d\n", sum + _a * _b, sum, _a, _b);
        sum = sum + _a * _b;
      }
      // printf("out [%d] is %d\n", i * col2 + j, sum);
      d[i * col2 + j] = (sum);
    }
  }

#if 0
    for (int i = 0; i < size; i++) {
        if (i % col2 == 0) {
            printf("\n");
        }
        printf("%f ", convert_bf16_fp32(d[i]));
    }
#endif
}
#endif /* ifdef USE_CBLAS_VERITY */

int array_cmp_int16(const char *const info, const uint16_t *p_exp, const uint16_t *p_got,
                    int count) {
  int idx;
  for (idx = 0; idx < count; idx++) {
    if (p_exp[idx] != p_got[idx]) {
      printf("%s error at index %d exp %d(%f,0x%x) got %d(%f,0x%x)\n", info, idx, p_exp[idx],
             convert_bf16_fp32(p_exp[idx]), p_exp[idx], p_got[idx], convert_bf16_fp32(p_got[idx]),
             p_got[idx]);
      return -1;
    }
  }
  return 0;
}

int array_cmp_int32(const char *const info, const uint32_t *p_exp, const uint32_t *p_got,
                    int count) {
  int idx;
  for (idx = 0; idx < count; idx++) {
    if (p_exp[idx] != p_got[idx]) {
      printf("%s error at index %d exp %d got %d\n", info, idx, p_exp[idx], p_got[idx]);
      return -1;
    }
  }
  return 0;
}

static void assign_bf16_values_to_matrix(uint16_t *matrix, size_t size) {
  float t;
  for (size_t i = 0; i < size; i++) {
    float f;
#if 1
    if (i % 2 == 0) t = i % 8;
    if (i % 2 == 1) t = -1 * (i % 8);
    f = t;
#else
    t = i * (i % 2 ? -1 : 1);
    f = t * 0.01 + size * 0.01;
#endif
    matrix[i] = convert_fp32_bf16(f);
    // printf("f[%lu] is %f(0x%x)\n", i, f, matrix[i]);
  }
}

static void uint16_to_float(float *float_data, uint16_t *bf16_data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    float_data[i] = convert_bf16_fp32(bf16_data[i]);
  }
}

static void uint8_to_float(float *float_data, uint8_t *i8_data, size_t size, cvk_fmt_t fmt) {
  for (size_t i = 0; i < size; i++) {
    int input = (i8_data[i]);
    if (fmt == CVK_FMT_I8) {
      input = (int8_t)(i8_data[i]);
    }
    float_data[i] = (float)input;
  }
}

static void assign_i8_values_to_matrix(uint8_t *matrix, size_t size) {
  for (size_t i = 0; i < size; i++) {
    matrix[i] = i + 20;
  }
}

#ifdef USE_CBLAS_VERITY
static void float_to_int16(uint16_t *int16_data, float *float_data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    int16_data[i] = convert_fp32_bf16(float_data[i]);
  }
}

static void float_to_int32(uint32_t *int32_data, float *float_data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    int32_data[i] = (uint32_t)float_data[i];
  }
}
#endif

// int8
static int _test_bmblas_gemm_bm1880v2(size_t M, size_t N, size_t K, cvk_fmt_t fmt) {
  long elapsed;
  struct timeval t0, t1;
  int ret = 0;

  uint8_t *i8_A = new uint8_t[M * K];
  uint8_t *i8_B = new uint8_t[N * K];
  uint8_t *i8_C = new uint8_t[4 * M * N];  // 32 bit output
  uint32_t *i32bit_ref = new uint32_t[M * N];

  assign_i8_values_to_matrix(i8_A, M * K);
  assign_i8_values_to_matrix(i8_B, N * K);

  float *float_A = new float[M * K];
  float *float_B = new float[N * K];
  float *float_C_ref = new float[M * N];
  uint8_to_float(float_A, i8_A, M * K, fmt);
  uint8_to_float(float_B, i8_B, N * K, fmt);

#if 0
  printf("\nA:");
  for (int i = 0; i < M; i++) {
      printf("\n");
  for (int j = 0; j < K; j++) {
      printf("%e(0x%x) ", float_A[i * K + j], i8_A[i * K + j]);
  }
  }
  printf("\nB:");
  for (int i = 0; i < K; i++) {
      printf("\n");
  for (int j = 0; j < N; j++) {
      printf("%e(0x%x) ", float_B[i * N + j], i8_B[i * N + j]);
  }
  }
  printf("\nR:");
  for (int i = 0; i < M; i++) {
      printf("\n");
  for (int j = 0; j < N; j++) {
      printf("%e ", convert_i8_fp32(i32bit_ref[i * N + j]));
  }
  }
#endif
  gettimeofday(&t0, NULL);

#ifdef USE_CBLAS_VERITY
  float alpha = 0;
  float beta = 0;

  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, alpha, float_A, K, float_B, N,
              beta, float_C_ref, N);
  float_to_int32(i32bit_ref, float_C_ref, M * N);
#else  /* ! ifdef USE_CBLAS_VERITY */
  multiply_i32(i8_A, M, K, i8_B, K, N, i32bit_ref, fmt);
#endif /* ifdef USE_CBLAS_VERITY */

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
#ifdef USE_CBLAS_VERITY
  printf("cblas GEMM takes %ld us\n", elapsed);
#else  /* ! ifdef USE_CBLAS_VERITY */
  printf("CPU GEMM takes %ld us\n", elapsed);
#endif /* ifdef USE_CBLAS_VERITY */

  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  test_init(&ctx, &bk_ctx);

  // alloc device memory
  cvk_mg_shape_t s_a = {(uint32_t)M, (uint32_t)K};
  cvk_mg_shape_t s_b = {(uint32_t)K, (uint32_t)N};
  cvk_mg_shape_t s_r = {2 * (uint32_t)M, 2 * (uint32_t)N};

  size_t s_size_a = mg_shape_size(&s_a) * bytesize_of_fmt(fmt);
  size_t s_size_b = mg_shape_size(&s_b) * bytesize_of_fmt(fmt);
  size_t s_size_r = mg_shape_size(&s_r) * bytesize_of_fmt(fmt);

  CVI_RT_MEM devmem_a = CVI_RT_MemAlloc(ctx, s_size_a);
  CVI_RT_MEM devmem_b = CVI_RT_MemAlloc(ctx, s_size_b);
  CVI_RT_MEM devmem_r = CVI_RT_MemAlloc(ctx, s_size_r);

  gaddr_t gaddr_a = CVI_RT_MemGetPAddr(devmem_a);
  gaddr_t gaddr_b = CVI_RT_MemGetPAddr(devmem_b);
  gaddr_t gaddr_r = CVI_RT_MemGetPAddr(devmem_r);

  // copy to device memory
  CVI_RT_MemCopyS2D(ctx, devmem_a, (uint8_t *)i8_A);
  CVI_RT_MemCopyS2D(ctx, devmem_b, (uint8_t *)i8_B);
  CVI_RT_MemCopyS2D(ctx, devmem_r, (uint8_t *)i8_C);

  // do computation with bmkernel
  // bmruntime_bmkernel_create(ctx, (void**)&bk_ctx);

  // printf("gaddr_a/gaddr_b/gaddr_r at %zx %zx %zx\n", gaddr_a, gaddr_b, gaddr_r);
  size_t *slice_num = cvm_gemm((cvk_context_t *)bk_ctx, gaddr_a, gaddr_b, gaddr_r, M, K, N, fmt);

  gettimeofday(&t0, NULL);
  test_submit_comp(&ctx, bk_ctx);
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("TPU GEMM takes %ld us\n", elapsed);

  CVI_RT_MemCopyD2S(ctx, (uint8_t *)i8_C, devmem_r);

  CVI_RT_MemFree(ctx, devmem_a);
  CVI_RT_MemFree(ctx, devmem_b);
  CVI_RT_MemFree(ctx, devmem_r);

  test_exit(&ctx, bk_ctx);

  uint32_t *i32_C = new uint32_t[M * N];  // 32 bit output with stirded

  cvm_combin_gemm_i8(slice_num, i8_C, i32_C, M, N);

  free(slice_num);

  int cmp_res = array_cmp_int32("gemm", i32bit_ref, i32_C, M * N);
  if (cmp_res != 0) {
    ret = -1;
    printf("Comparison failed for cblas_sgemm and bmblas_gemm!\n");
#if 0
    printf("\nref/cmd is:");
    for (int i = 0; i < M; i++) {
      printf(">\n");
      for (int j = 0; j < N; j++) {
        printf("%f(0x%x)/%f(0x%x) ",
            convert_i8_fp32(i32bit_ref[i * N + j]), i32bit_ref[i * N + j],
            convert_i8_fp32(i8_C[i * N + j]), i8_C[i * N + j]
            );
      }
    }
#endif
  } else {
    // printf("Comparison done for cblas_sgemm and bmblas_gemm!\n\n");
  }

  delete[] float_A;
  delete[] float_B;
  delete[] float_C_ref;
  delete[] i8_A;
  delete[] i8_B;
  delete[] i8_C;
  delete[] i32bit_ref;
  delete[] i32_C;
  return ret;
}

int test_bmblas_gemm_bm1880v2(size_t M, size_t N, size_t K, cvk_fmt_t fmt) {
  printf("%s: M=%zu, N=%zu, K=%zu, fmt_sz: %d\n", __func__, M, N, K, cvm_bytesize_of_fmt(fmt));

  // FIXME: not duplicate
  if (fmt != CVK_FMT_BF16) {
    return _test_bmblas_gemm_bm1880v2(M, N, K, fmt);
  }

  long elapsed;
  struct timeval t0, t1;
  int ret = 0;

  uint16_t *bf16_A = new uint16_t[M * K];
  uint16_t *bf16_B = new uint16_t[N * K];
  uint16_t *bf16_C = new uint16_t[2 * M * N];
  uint16_t *int16_C_ref = new uint16_t[M * N];

  assign_bf16_values_to_matrix(bf16_A, M * K);
  assign_bf16_values_to_matrix(bf16_B, N * K);

  float *float_A = new float[M * K];
  float *float_B = new float[N * K];
  float *float_C_ref = new float[M * N];
  uint16_to_float(float_A, bf16_A, M * K);
  uint16_to_float(float_B, bf16_B, N * K);

#if 0
  printf("\nA:");
  for (int i = 0; i < M; i++) {
      printf("\n");
  for (int j = 0; j < K; j++) {
      printf("%e(0x%x) ", float_A[i * K + j], bf16_A[i * K + j]);
  }
  }
  printf("\nB:");
  for (int i = 0; i < K; i++) {
      printf("\n");
  for (int j = 0; j < N; j++) {
      printf("%e(0x%x) ", float_B[i * N + j], bf16_B[i * N + j]);
  }
  }
  printf("\nR:");
  for (int i = 0; i < M; i++) {
      printf("\n");
  for (int j = 0; j < N; j++) {
      printf("%e ", convert_bf16_fp32(int16_C_ref[i * N + j]));
  }
  }
#endif
  gettimeofday(&t0, NULL);

#ifdef USE_CBLAS_VERITY
  float alpha = 0;
  float beta = 0;
  cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, alpha, float_A, K, float_B, N,
              beta, float_C_ref, N);
  float_to_int16(int16_C_ref, float_C_ref, M * N);
#else  /* ! ifdef USE_CBLAS_VERITY */
  multiply(bf16_A, M, K, bf16_B, K, N, int16_C_ref);
#endif /* ifdef USE_CBLAS_VERITY */

  delete[] float_A;
  delete[] float_B;
  delete[] float_C_ref;

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
#ifdef USE_CBLAS_VERITY
  printf("cblas GEMM takes %ld us\n", elapsed);
#else
  printf("CPU GEMM takes %ld us\n", elapsed);
#endif

  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  test_init(&ctx, &bk_ctx);

  // alloc device memory
  cvk_mg_shape_t s_a = {(uint32_t)M, (uint32_t)K};
  cvk_mg_shape_t s_b = {(uint32_t)K, (uint32_t)N};
  cvk_mg_shape_t s_r = {(uint32_t)M, (uint32_t)N};

  size_t s_size_a = mg_shape_size(&s_a) * bytesize_of_fmt(fmt);
  size_t s_size_b = mg_shape_size(&s_b) * bytesize_of_fmt(fmt);
  size_t s_size_r = mg_shape_size(&s_r) * bytesize_of_fmt(fmt) * bytesize_of_fmt(fmt);

  CVI_RT_MEM devmem_a = CVI_RT_MemAlloc(ctx, s_size_a);
  CVI_RT_MEM devmem_b = CVI_RT_MemAlloc(ctx, s_size_b);
  CVI_RT_MEM devmem_r = CVI_RT_MemAlloc(ctx, s_size_r);

  gaddr_t gaddr_a = CVI_RT_MemGetPAddr(devmem_a);
  gaddr_t gaddr_b = CVI_RT_MemGetPAddr(devmem_b);
  gaddr_t gaddr_r = CVI_RT_MemGetPAddr(devmem_r);

  // copy to device memory
  CVI_RT_MemCopyS2D(ctx, devmem_a, (uint8_t *)bf16_A);
  CVI_RT_MemCopyS2D(ctx, devmem_b, (uint8_t *)bf16_B);
  CVI_RT_MemCopyS2D(ctx, devmem_r, (uint8_t *)bf16_C);
  // do computation with bmkernel
  // bmruntime_bmkernel_create(ctx, (void**)&bk_ctx);

  size_t *slice_num =
      cvm_gemm((cvk_context_t *)bk_ctx, gaddr_a, gaddr_b, gaddr_r, M, K, N, CVK_FMT_BF16);
  free(slice_num);  // no use slice_num infomation in BF16

  gettimeofday(&t0, NULL);
  test_submit_comp(&ctx, bk_ctx);
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("TPU GEMM takes %ld us\n", elapsed);

  CVI_RT_MemCopyD2S(ctx, (uint8_t *)bf16_C, devmem_r);

  // bmruntime_bmkernel_destroy(ctx);

  CVI_RT_MemFree(ctx, devmem_a);
  CVI_RT_MemFree(ctx, devmem_b);
  CVI_RT_MemFree(ctx, devmem_r);

  test_exit(&ctx, bk_ctx);

  int cmp_res = array_cmp_int16("gemm", int16_C_ref, bf16_C, M * N);
  if (cmp_res != 0) {
    ret = -1;
    printf("Comparison failed for cblas_sgemm and bmblas_gemm!\n");
#if 0
    printf("\nref/cmd is:");
    for (int i = 0; i < M; i++) {
      printf(">\n");
      for (int j = 0; j < N; j++) {
        printf("%f(0x%x)/%f(0x%x) ",
            convert_bf16_fp32(int16_C_ref[i * N + j]), int16_C_ref[i * N + j],
            convert_bf16_fp32(bf16_C[i * N + j]), bf16_C[i * N + j]
            );
      }
    }
#endif
  } else {
    // printf("Comparison done for cblas_sgemm and bmblas_gemm!\n\n");
  }

  delete[] bf16_A;
  delete[] bf16_B;
  delete[] bf16_C;
  delete[] int16_C_ref;
  return ret;
}

#define test_one_param(n)          \
  do {                             \
    param_t p = param_##n(bk_ctx); \
    test_param(&ctx, bk_ctx, &p);  \
    destroy_param(bk_ctx, &p);     \
  } while (0)

int main() {
  int round_mode;
  round_mode = set_store_feround();
  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  test_init(&ctx, &bk_ctx);

  // int8 example
  if (0 != test_bmblas_gemm_bm1880v2(1, 100, 512, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(2, 100, 512, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(4, 100, 512, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(8, 100, 512, CVK_FMT_I8)) exit(-1);

  if (0 != test_bmblas_gemm_bm1880v2(1, 20000, 512, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(10, 200, 10, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(1, 200, 500, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(1, 20, 50, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(2, 10, 100, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(2, 1000, 5, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(20, 5, 5, CVK_FMT_I8)) exit(-1);
  if (0 != test_bmblas_gemm_bm1880v2(2, 5, 5, CVK_FMT_I8)) exit(-1);
  cvk_fmt_t fmts[2] = {CVK_FMT_BF16, CVK_FMT_I8};
  // cvk_fmt_t fmts[1] = {CVK_FMT_BF16};
  int fmts_sz = sizeof(fmts) / sizeof(fmts[0]);

  for (int i = 0; i < fmts_sz; i++) {
    cvk_fmt_t fmt = fmts[i];
    if (0) {
      // backend implement
      for (int i = 0; i < 30; i++) test_one_param(0);

    } else {
      // gemm, plz refer bmtap2/libbmblas
      int M = 10000;
      int N = 10000;
      int K = 1024;
      M = 2000;
      N = 2000;
      int m, k, n;

      if (0) {
        for (m = 1; m <= M; m *= 10) {
          for (n = 1; n <= N; n += 200) {
            for (k = 1; k <= K; k *= 2) {
              if (0 != test_bmblas_gemm_bm1880v2(m, n, k, fmt)) {
                exit(-1);
              }
            }
          }
        }
      }

      if (1) {
        if (0 != test_bmblas_gemm_bm1880v2(1, 500, 512, fmt)) exit(-1);
        if (0 != test_bmblas_gemm_bm1880v2(1, 750, 512, fmt)) exit(-1);
        if (0 != test_bmblas_gemm_bm1880v2(1, 100, 512, fmt)) exit(-1);
        if (0 != test_bmblas_gemm_bm1880v2(2, 100, 512, fmt)) exit(-1);
        if (0 != test_bmblas_gemm_bm1880v2(4, 100, 512, fmt)) exit(-1);
        if (0 != test_bmblas_gemm_bm1880v2(8, 100, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(1, 50000, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(1, 75000, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(1, 10000, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(2, 10000, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(4, 10000, 512, fmt)) exit(-1);
        // if (0 != test_bmblas_gemm_bm1880v2(8, 10000, 512, fmt)) exit(-1);
      }

      printf("Comparison done for cblas_sgemm and bmblas_gemm!\n\n");
    }
  }

  test_exit(&ctx, bk_ctx);
  restore_feround(round_mode);
  return 0;
}
