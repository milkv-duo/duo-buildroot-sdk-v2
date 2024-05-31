#include <cvimath_internal.h>

#include <bits/stdc++.h>
#ifdef __ARM_ARCH
#include <arm_neon.h>
#endif

template <typename T>
void k_selection_sort_index(T *array, uint32_t *index, T *value, const uint32_t array_size,
                            const uint32_t k) {
  for (uint32_t i = 0; i < k; i++) {
    int largest = 0;
    for (uint32_t j = 0; j < array_size; j++) {
      if (array[j] > array[largest]) {
        largest = j;
      }
    }
    value[i] = array[largest];
    index[i] = largest;
    array[largest] = 0;
  }
}

inline uint32_t dot(uint8_t *a, uint8_t *b, uint32_t data_length) {
  uint32_t dot_result = 0;
  for (uint32_t i = 0; i < data_length; i++) {
    dot_result += ((short)a[i] * b[i]);
  }
  return dot_result;
}

inline int32_t dot_i8(int8_t *a, int8_t *b, uint32_t data_length) {
  int32_t dot_result = 0;
  for (uint32_t i = 0; i < data_length; i++) {
    dot_result += ((short)a[i] * b[i]);
  }
  return dot_result;
}

void cvm_gen_precached_i8_unit_length(int8_t *precached, float *unit_precached_arr,
                                      const uint32_t data_length, const uint32_t data_num) {
  for (uint32_t i = 0; i < data_num; i++) {
    int8_t *fb_offset = precached + i * data_length;
    unit_precached_arr[i] = dot_i8(fb_offset, fb_offset, data_length);
    unit_precached_arr[i] = sqrt(unit_precached_arr[i]);
  }
}

void cvm_gen_precached_u8_unit_length(uint8_t *precached, float *unit_precached_arr,
                                      const uint32_t data_length, const uint32_t data_num) {
  for (uint32_t i = 0; i < data_num; i++) {
    uint8_t *fb_offset = precached + i * data_length;
    unit_precached_arr[i] = dot(fb_offset, fb_offset, data_length);
    unit_precached_arr[i] = sqrt(unit_precached_arr[i]);
  }
}

void cvm_cpu_i8data_ip_match(int8_t *feature, int8_t *precached, float *unit_precached_arr,
                             uint32_t *k_index, float *k_value, float *buffer,
                             const uint32_t data_length, const uint32_t data_num,
                             const uint32_t k) {
  float unit_feature = (float)dot_i8(feature, feature, data_length);
  unit_feature = sqrt(unit_feature);
  for (uint32_t i = 0; i < data_num; i++) {
    buffer[i] = dot_i8(feature, precached + i * data_length, data_length) /
                (unit_feature * unit_precached_arr[i]);
  }
  k_selection_sort_index(buffer, k_index, k_value, data_num, k);
}

void cvm_cpu_u8data_ip_match(uint8_t *feature, uint8_t *precached, float *unit_precached_arr,
                             uint32_t *k_index, float *k_value, float *buffer,
                             const uint32_t data_length, const uint32_t data_num,
                             const uint32_t k) {
  float unit_feature = (float)dot(feature, feature, data_length);
  unit_feature = sqrt(unit_feature);
  for (uint32_t i = 0; i < data_num; i++) {
    buffer[i] = dot(feature, precached + i * data_length, data_length) /
                (unit_feature * unit_precached_arr[i]);
  }
  k_selection_sort_index(buffer, k_index, k_value, data_num, k);
}