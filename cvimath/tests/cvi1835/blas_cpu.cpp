#include <cvimath_internal.h>

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <cstdlib>
#include <iostream>

int main() {
  srand(time(NULL));
  const uint32_t data_length = 512;
  const uint32_t data_num = 20000;
  uint8_t *db = new uint8_t[data_num * data_length];
  float *db_unit = new float[data_num];
  uint8_t *data = new uint8_t[data_length];
  float *buffer_f = new float[data_num];
  memset(buffer_f, 0, data_num * sizeof(float));

  for (uint32_t i = 0; i < data_length; i++) {
    data[i] = rand() % 256;
  }
  for (uint32_t j = 0; j < data_num; j++) {
    for (uint32_t i = 0; i < data_length; i++) {
      db[j * data_length + i] = rand() % 256;
    }
  }
  cvm_gen_db_unit_length(db, db_unit, data_length, data_num);

  const uint32_t k = 5;
  uint32_t k_index[k] = {0};
  float k_value[k] = {0};
  struct timeval t0, t1;
  gettimeofday(&t0, NULL);
  cvm_cpu_u8data_ip_match(data, db, db_unit, k_index, k_value, buffer_f, data_length, data_num, k);
  gettimeofday(&t1, NULL);
  unsigned long elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("Searching time uint8: %lu us\n", elapsed_tpu);
  printf("Result:\n");
  for (uint32_t i = 0; i < k; i++) {
    printf("[%u] %f\n", k_index[i], k_value[i]);
  }
  printf("\n");
  gettimeofday(&t0, NULL);
  cvm_cpu_i8data_ip_match((int8_t *)data, (int8_t *)db, db_unit, k_index, k_value, buffer_f,
                          data_length, data_num, k);
  gettimeofday(&t1, NULL);
  elapsed_tpu = ((t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec);
  printf("Searching time int8: %lu us\n", elapsed_tpu);
  printf("Result:\n");
  for (uint32_t i = 0; i < k; i++) {
    printf("[%u] %f\n", k_index[i], k_value[i]);
  }
  printf("\n");

  delete[] data;
  delete[] db;
  delete[] db_unit;
  delete[] buffer_f;
  return 0;
}