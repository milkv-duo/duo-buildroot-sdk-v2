// \file sample for gemm(general matrix multiply)

// header include
#include <assert.h>
#include <cvimath_internal.h>     // math
#include <test_cvikernel_util.h>  // kerenl

#include <sys/time.h>  // int gettimeofday
#include <time.h>      /* clock_t, clock, CLOCKS_PER_SEC */

typedef cvk_tiu_matrix_multiplication_param_t param_t;

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
}

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
        sum = sum + _a * _b;
      }
      d[i * col2 + j] = (sum);
    }
  }
}

// compare with uint16_t type
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

static int array_cmp_int32(const char *const info, const uint32_t *p_exp, const uint32_t *p_got,
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

static cvk_mg_t *_test_put_matrix_g(CVI_RT_HANDLE *rt_ctx, size_t row, size_t col,
                                    cvk_fmt_t mg_data_format, uint8_t data[]) {
  cvk_mg_shape_t s;
  s.row = row;
  s.col = col;
  return test_put_matrix_g(rt_ctx, s, mg_data_format, data);
}

static void assign_bf16_values_to_matrix(uint16_t *matrix, size_t size) {
  float t;
  for (size_t i = 0; i < size; i++) {
    float f;
#if 1
    // simple pattern
    if (i % 2 == 0) t = i % 8;
    if (i % 2 == 1) t = -1 * (i % 8);
    f = t;
#else
    t = i * (i % 2 ? -1 : 1);
    f = t * 0.01 + size * 0.01;
#endif
    matrix[i] = convert_fp32_bf16(f);
  }
}

static void assign_i8_values_to_matrix(uint8_t *matrix, size_t size) {
  for (size_t i = 0; i < size; i++) {
    matrix[i] = i + 20;
  }
}

static int test_gemm_bf16(size_t M, size_t N, size_t K) {
  long elapsed;
  struct timeval t0, t1;
  int ret = 0;

  // alloc test data in host
  uint16_t *bf16_A = new uint16_t[M * K];
  uint16_t *bf16_B = new uint16_t[N * K];
  uint16_t *bf16_R = new uint16_t[2 * M * N];
  uint16_t *int16_C_ref = new uint16_t[M * N];

  // assign data
  assign_bf16_values_to_matrix(bf16_A, M * K);
  assign_bf16_values_to_matrix(bf16_B, N * K);

  gettimeofday(&t0, NULL);

  multiply(bf16_A, M, K, bf16_B, K, N, int16_C_ref);

  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("CPU GEMM takes %ld us\n", elapsed);

  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  // init runtime / kerenl structure
  test_init(&ctx, &bk_ctx);

  // alloc device memory and put data to device
  cvk_mg_t *mg_A = _test_put_matrix_g(&ctx, M, K, CVK_FMT_BF16, (uint8_t *)bf16_A);
  cvk_mg_t *mg_B = _test_put_matrix_g(&ctx, K, N, CVK_FMT_BF16, (uint8_t *)bf16_B);
  cvk_mg_t *mg_R = _test_put_matrix_g(&ctx, M * 2, N, CVK_FMT_BF16, (uint8_t *)bf16_R);

  // get device address for gemm
  gaddr_t gaddr_a = mg_A->start_address;
  gaddr_t gaddr_b = mg_B->start_address;
  gaddr_t gaddr_r = mg_R->start_address;

  // prepare gemm descriptor
  size_t *slice_num =
      cvm_gemm((cvk_context_t *)bk_ctx, gaddr_a, gaddr_b, gaddr_r, M, K, N, CVK_FMT_BF16);

  // submit descriptor
  gettimeofday(&t0, NULL);
  test_submit_comp(&ctx, bk_ctx);
  gettimeofday(&t1, NULL);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;

  printf("GEMM takes %ld us\n", elapsed);

  // get result from device to host
  uint16_t *bf16_ref = (uint16_t *)test_get_mg_mem_comp(&ctx, mg_R);

  // compare, exit once compare fail in
  int cmp_res = array_cmp_int16("gemm", int16_C_ref, bf16_ref, M * N);
  if (cmp_res != 0) {
    ret = -1;
    printf("Comparison failed for cblas_sgemm and bmblas_gemm!\n");
  }

  // free device resource
  test_free_mg_mem_comp(&ctx, mg_A);
  test_free_mg_mem_comp(&ctx, mg_B);
  test_free_mg_mem_comp(&ctx, mg_R);

  // de-init runtime / kerenl structure
  test_exit(&ctx, bk_ctx);

  // free resource from host
  delete[] bf16_A;
  delete[] bf16_B;
  delete[] bf16_R;
  delete[] int16_C_ref;
  free(bf16_ref);
  free(slice_num);

  return ret;
}

static int test_gemm_i8(size_t M, size_t N, size_t K, cvk_fmt_t fmt) {
  long elapsed;
  struct timeval t0, t1;
  int ret = 0;

  // 4 means 32bit takes 4 times size of uint8_t
  int uint32_per_uint8 = sizeof(uint32_t) / sizeof(uint8_t);

  // alloc test data in host
  uint8_t *i8_A = new uint8_t[M * K];
  uint8_t *i8_B = new uint8_t[N * K];
  uint8_t *i8_R = new uint8_t[uint32_per_uint8 * M * N];
  uint32_t *int32_C_ref = new uint32_t[M * N];

  // assign data
  assign_i8_values_to_matrix(i8_A, M * K);
  assign_i8_values_to_matrix(i8_B, N * K);

  // measure cpu time
  gettimeofday(&t0, NULL);

  multiply_i32(i8_A, M, K, i8_B, K, N, int32_C_ref, fmt);

  gettimeofday(&t1, NULL);

  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("CPU GEMM takes %ld us\n", elapsed);

  // alloc runtime
  CVI_RT_HANDLE ctx;
  cvk_context_t *bk_ctx;

  // init runtime / kerenl structure
  test_init(&ctx, &bk_ctx);

  // alloc device memory and put data to device
  cvk_mg_t *mg_A = _test_put_matrix_g(&ctx, M, K, CVK_FMT_I8, (uint8_t *)i8_A);
  cvk_mg_t *mg_B = _test_put_matrix_g(&ctx, K, N, CVK_FMT_I8, (uint8_t *)i8_B);
  cvk_mg_t *mg_R = _test_put_matrix_g(&ctx, M * uint32_per_uint8, N, CVK_FMT_I8, (uint8_t *)i8_R);

  // get device address for gemm
  gaddr_t gaddr_a = mg_A->start_address;
  gaddr_t gaddr_b = mg_B->start_address;
  gaddr_t gaddr_r = mg_R->start_address;

  // prepare gemm descriptor
  size_t *slice_num = cvm_gemm((cvk_context_t *)bk_ctx, gaddr_a, gaddr_b, gaddr_r, M, K, N, fmt);

  gettimeofday(&t0, NULL);

  // submit descriptor
  test_submit_comp(&ctx, bk_ctx);

  gettimeofday(&t1, NULL);

  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  printf("GEMM takes %ld us\n", elapsed);

  // get result from device to host
  uint8_t *i8_R_host = (uint8_t *)test_get_mg_mem_comp(&ctx, mg_R);

  // for re-combine
  uint32_t *i32_C = new uint32_t[M * N];

  if (fmt == CVK_FMT_I8) {
    cvm_combin_gemm_i8(slice_num, i8_R_host, i32_C, M, N);
  }

  free(slice_num);

  // compare, exit once compare fail in
  int cmp_res = array_cmp_int32("gemm", int32_C_ref, i32_C, M * N);
  if (cmp_res != 0) {
    ret = -1;
    printf("Comparison failed for cblas_sgemm and bmblas_gemm!\n");
  }

  // free device resource
  test_free_mg_mem_comp(&ctx, mg_A);
  test_free_mg_mem_comp(&ctx, mg_B);
  test_free_mg_mem_comp(&ctx, mg_R);

  // de-init runtime / kerenl structure
  test_exit(&ctx, bk_ctx);

  // free resource from host
  delete[] i8_A;
  delete[] i8_B;
  delete[] i8_R;
  delete[] int32_C_ref;
  delete[] i32_C;
  free(i8_R_host);

  return ret;
}

static int test_gemm(size_t M, size_t N, size_t K, cvk_fmt_t fmt) {
  printf("%s: M=%zu, N=%zu, K=%zu\n", __func__, M, N, K);
  if (fmt == CVK_FMT_BF16) {
    return test_gemm_bf16(M, N, K);
  } else {
    return test_gemm_i8(M, N, K, fmt);
  }
}

int main() {
  int round_mode;
  // align backend rounding
  round_mode = set_store_feround();

  if (0 != test_gemm(3, 500, 512, CVK_FMT_BF16)) exit(-1);
  if (0 != test_gemm(1, 20000, 512, CVK_FMT_I8)) exit(-1);

  // heavy test
  // if (0 != test_gemm(300, 500, 512, CVK_FMT_BF16)) exit(-1);

  printf("Comparison done for cpu gemm and tpu gemm!\n\n");

  // restore rounding
  restore_feround(round_mode);

  return 0;
}
