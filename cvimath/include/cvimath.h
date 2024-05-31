#ifndef CVIMATH_H
#define CVIMATH_H

#include <stdint.h>

// public function
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function calculated the unit length of a precahed i8 feature array
 *
 * @param precached Prefetched feature array in 1-D. Format: feature1, feature2, ...
 * @param unit_precached_arr Output unit length.
 * @param data_length The length of the feature.
 * @param data_num The number of features.
 */
void cvm_gen_precached_i8_unit_length(int8_t *precached, float *unit_precached_arr,
                                      const uint32_t data_length, const uint32_t data_num);

/**
 * @brief This function calculated the unit length of a precahed u8 feature array
 *
 * @param precached Prefetched feature array in 1-D. Format: feature1, feature2, ...
 * @param unit_precached_arr Output unit length.
 * @param data_length The length of the feature.
 * @param data_num The number of features.
 */
void cvm_gen_precached_u8_unit_length(uint8_t *precached, float *unit_precached_arr,
                                      const uint32_t data_length, const uint32_t data_num);

/**
 * @brief Do inner product matching on i8 feature with given precached feature array.
 *
 * @param feature The input i8 feature to be compared.
 * @param precached The precached feature array in 1-D.
 * @param unit_precached_arr The unit length array of the precached.
 * @param k_index The output matching index result in order.
 * @param k_value The output matching value result in order.
 * @param buffer The buffer used by this function, same length as precached.
 * @param data_length The length of the single feature.
 * @param data_num The number of features of the feature array.
 * @param k Top k results, affects the length of k_index and k_value.
 */
void cvm_cpu_i8data_ip_match(int8_t *feature, int8_t *precached, float *unit_precached_arr,
                             uint32_t *k_index, float *k_value, float *buffer,
                             const uint32_t data_length, const uint32_t data_num, const uint32_t k);

/**
 * @brief Do inner product matching on u8 feature with given precached feature array.
 *
 * @param feature The input u8 feature to be compared.
 * @param precached The precached feature array in 1-D.
 * @param unit_precached_arr The unit length array of the precached.
 * @param k_index The output matching index result in order.
 * @param k_value The output matching value result in order.
 * @param buffer The buffer used by this function, same length as precached.
 * @param data_length The length of the single feature.
 * @param data_num The number of features of the feature array.
 * @param k Top k results, affects the length of k_index and k_value.
 */
void cvm_cpu_u8data_ip_match(uint8_t *feature, uint8_t *precached, float *unit_precached_arr,
                             uint32_t *k_index, float *k_value, float *buffer,
                             const uint32_t data_length, const uint32_t data_num, const uint32_t k);

// Legacy support for hj.
inline void __attribute__((always_inline))
cvm_gen_db_i8_unit_length(int8_t *precached, float *unit_precached_arr, const uint32_t data_length,
                          const uint32_t data_num) {
  cvm_gen_precached_i8_unit_length(precached, unit_precached_arr, data_length, data_num);
}

inline void __attribute__((always_inline))
cvm_gen_db_unit_length(uint8_t *precached, float *unit_precached_arr, const uint32_t data_length,
                       const uint32_t data_num) {
  cvm_gen_precached_u8_unit_length(precached, unit_precached_arr, data_length, data_num);
}

#ifdef __cplusplus
}
#endif

#endif  // CVIMATH_H
