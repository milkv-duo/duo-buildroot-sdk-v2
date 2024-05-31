#ifndef TEST_TF_QUANT_UTIL_H
#define TEST_TF_QUANT_UTIL_H

#include <stdint.h>

#define MAX(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
      _a > _b ? _a : _b; })

#define MIN(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
      _a > _b ? _b : _a; })

#ifdef __cplusplus
extern "C" {
#endif

int32_t RoundingDivideByPOT(int32_t x, int exponent);
int32_t SaturatingRoundingDoublingHighMul(int32_t a, int32_t b);
int32_t MultiplyByQuantizedMultiplier(int32_t x, int32_t quantized_multiplier,
                                      int rshift);
void QuantizeMultiplierSmallerThanOne(float real_multiplier,
                                      uint32_t *quantized_multiplier,
                                      int *right_shift);

void pack_chl_quan_param(uint32_t channels, int has_bias, int32_t *bias,
                         uint32_t *multiplier, int8_t *rshift,
                         uint8_t *packed_data);

// 1880v2: 5bit right shift, [0, 31]
// 1822:   1bit sign, 5b shift, [-32, 31]
int8_t truncate_rshift(int8_t rshift, int8_t allow_lshift);

#ifdef __cplusplus
}
#endif

#endif // TEST_TF_QUANT_UTIL_H
