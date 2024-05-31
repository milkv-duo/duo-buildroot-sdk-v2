#include <cvimath_internal.h>
#include <cviruntime.h>
#include <cviruntime_context.h>

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <cmath>
#include <cstdlib>
#include <iostream>

void i8data_ip_match(CVI_RT_HANDLE ctx, cvk_context_t *cvk_ctx, uint64_t a_gaddr, int8_t *a_vaddr,
                     uint64_t db_gaddr, float *unit_db_arr, uint32_t *k_index, float *k_value,
                     uint64_t buffer_gemm_gaddr, uint8_t *buffer_gemm_vaddr, uint32_t *buffer_i32,
                     float *buffer_f, CVI_RT_MEM gemm_device, const uint32_t data_length,
                     const uint32_t data_num, const uint32_t k) {
  size_t *slice_num =
      cvm_gemm(cvk_ctx, a_gaddr, db_gaddr, buffer_gemm_gaddr, 1, data_length, data_num, CVK_FMT_I8);
  CVI_RT_Submit(cvk_ctx);
  CVI_RT_MemInvld(ctx, gemm_device);
  cvm_combin_gemm_i8(slice_num, buffer_gemm_vaddr, buffer_i32, 1, data_num);
  free(slice_num);
  // Get a length
  int32_t dot_result = 0;
  for (uint32_t i = 0; i < data_length; i++) {
    dot_result += ((short)a_vaddr[i] * a_vaddr[i]);
  }
  float unit_a = sqrt(dot_result);
  // Get a length end

  for (uint32_t i = 0; i < data_num; i++) {
    buffer_f[i] = ((int32_t *)buffer_i32)[i] / (unit_a * unit_db_arr[i]);
  }
  // Get k result
  for (uint32_t i = 0; i < k; i++) {
    int largest = 0;
    for (uint32_t j = 0; j < data_num; j++) {
      if (buffer_f[j] > buffer_f[largest]) {
        largest = j;
      }
    }
    k_value[i] = buffer_f[largest];
    k_index[i] = largest;
    buffer_f[largest] = 0;
  }
}

int main() {
  CVI_RT_HANDLE ctx;
  CVI_RT_Init(&ctx);
  cvk_context_t *bk_ctx = (cvk_context_t *)CVI_RT_RegisterKernel(ctx, 100000);
  printf("123\n");

  const uint32_t data_length = 512;
  const uint32_t data_num = 1000;
  // Allocate memory
  CVI_RT_MEM bmmem_a = CVI_RT_MemAlloc(ctx, data_length);
  CVI_RT_MEM bmmem_db = CVI_RT_MemAlloc(ctx, data_length * data_num);
  CVI_RT_MEM bmmem_c = CVI_RT_MemAlloc(ctx, data_num * sizeof(uint32_t));

  uint64_t gaddr_a = CVI_RT_MemGetPAddr(bmmem_a);
  uint64_t gaddr_db = CVI_RT_MemGetPAddr(bmmem_db);
  uint64_t gaddr_c = CVI_RT_MemGetPAddr(bmmem_c);

  uint8_t *vaddr_a = CVI_RT_MemGetVAddr(bmmem_a);
  uint8_t *vaddr_db = CVI_RT_MemGetVAddr(bmmem_db);
  uint8_t *vaddr_c = CVI_RT_MemGetVAddr(bmmem_c);

  int8_t *db_raw = new int8_t[data_length * data_num];
  float *db_unit = new float[data_num];
  uint32_t *buffer = new uint32_t[data_num];
  float *buffer_f = new float[data_num];

  // Generate data
  srand(time(NULL));
  for (uint32_t i = 0; i < data_length; i++) {
    ((int8_t *)vaddr_a)[i] = rand() % 10 - 10;
  }
  for (uint32_t j = 0; j < data_num; j++) {
    for (uint32_t i = 0; i < data_length; i++) {
      ((int8_t *)db_raw)[j * data_length + i] = rand() % 10 - 10;
    }
  }

  // Pass db feature to ion
  for (uint32_t n = 0; n < data_num * data_length; n++) {
    int i = n / data_num;
    int j = n % data_num;
    ((int8_t *)vaddr_db)[n] = db_raw[data_length * j + i];
  }

  // Calculate unit length for db feature
  cvm_gen_precached_i8_unit_length((int8_t *)db_raw, db_unit, data_length, data_num);
  CVI_RT_MemFlush(ctx, bmmem_a);
  CVI_RT_MemFlush(ctx, bmmem_db);

  const uint32_t k = 5;
  uint32_t k_index[k] = {0};
  float k_value[k] = {0};
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  i8data_ip_match(ctx, bk_ctx, gaddr_a, (int8_t *)vaddr_a, gaddr_db, db_unit, k_index, k_value,
                  gaddr_c, vaddr_c, buffer, buffer_f, bmmem_c, data_length, data_num, k);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("Searching time tpu int8: %lu us\n", elapsed_tpu);
  printf("Result:\n");
  for (uint32_t i = 0; i < k; i++) {
    printf("[%u] %f\n", k_index[i], k_value[i]);
  }
  printf("\n");

  gettimeofday(&t0, NULL);
  cvm_cpu_i8data_ip_match((int8_t *)vaddr_a, (int8_t *)db_raw, db_unit, k_index, k_value, buffer_f,
                          data_length, data_num, k);
  gettimeofday(&t1, NULL);
  elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("Searching time int8: %lu us\n", elapsed_tpu);
  printf("Result:\n");
  for (uint32_t i = 0; i < k; i++) {
    printf("[%u] %f\n", k_index[i], k_value[i]);
  }
  printf("\n");

  delete[] db_unit;
  delete[] buffer;
  delete[] buffer_f;
  CVI_RT_MemFree(ctx, bmmem_a);
  CVI_RT_MemFree(ctx, bmmem_db);
  CVI_RT_MemFree(ctx, bmmem_c);
  CVI_RT_UnRegisterKernel(bk_ctx);
  CVI_RT_DeInit(ctx);
  return 0;
}
