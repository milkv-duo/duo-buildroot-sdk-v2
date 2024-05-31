#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include "test_tf_quant_util.h"

// Correctly-rounded-to-nearest division by a power-of-two.
// Also known as a rounding arithmetic right shift.
int32_t RoundingDivideByPOT(int32_t x, int exponent)
{
  const int32_t mask = (1ll << exponent) - 1;
  const int32_t remainder = x & mask;
  const int32_t threshold = (mask >> 1) + ((x < 0) ? 1 : 0);
  return ((x >> exponent) + ((remainder > threshold) ? 1 : 0));
}

int32_t SaturatingRoundingDoublingHighMul(int32_t a, int32_t b)
{
  int64_t a_64 = a;
  int64_t b_64 = b;
  int64_t ab_64 = a_64 * b_64;
  int32_t nudge = ab_64 >= 0 ? (1 << 30) : (1 - (1 << 30));
  int32_t ab_x2_high32 = (int32_t)((ab_64 + nudge) / (1ll << 31));

  return ab_x2_high32;
}

int32_t MultiplyByQuantizedMultiplier(int32_t x, int32_t quantized_multiplier,
                                      int rshift)
{
  int left_shift = rshift > 0 ? 0 : -rshift;
  int right_shift = rshift > 0 ? rshift : 0;

  int32_t x1 = SaturatingRoundingDoublingHighMul(x * (1 << left_shift),
                                                 quantized_multiplier);
  int32_t x2 = RoundingDivideByPOT(x1, right_shift);

  return x2;
}

// 1880v2: 5bit right shift
// 1822:   6bit left/right shift, 1b sign, 5bit shift
static uint8_t pack_6b_shift(int8_t rshift)
{
  // 8b -> 6b
  uint8_t sign_bit = (((uint8_t)rshift) & 0x80) >> 7; // 1bit
  uint8_t shift_val = ((uint8_t)rshift) & 0x1f; // 5bit
  uint8_t val = (sign_bit << 5) | shift_val;

  return val;
}

void pack_chl_quan_param(uint32_t channels, int has_bias, int32_t *bias,
                         uint32_t *multiplier, int8_t *rshift,
                         uint8_t *packed_data)
{
  uint8_t *ptr = packed_data;

  for (uint32_t i = 0; i < channels; i++) {
    if (has_bias) {
      uint32_t val = (uint32_t)bias[i];
      *ptr = val & 0xff;
      ptr++;
      *ptr = (val >> 8) & 0xff;
      ptr++;
      *ptr = (val >> 16) & 0xff;
      ptr++;
      *ptr = (val >> 24) & 0xff;
      ptr++;
    }

    {
      uint32_t val = multiplier[i];
      *ptr = val & 0xff;
      ptr++;
      *ptr = (val >> 8) & 0xff;
      ptr++;
      *ptr = (val >> 16) & 0xff;
      ptr++;
      *ptr = (val >> 24) & 0xff;
      ptr++;
    }

    {
      uint8_t val = pack_6b_shift(rshift[i]);
      *ptr = val;
      ptr++;
    }
  }
}

void QuantizeMultiplierSmallerThanOne(float real_multiplier,
                                      uint32_t *quantized_multiplier,
                                      int *right_shift)
{
  assert(real_multiplier > 0.f);
  assert(real_multiplier < 1.f);
  int s = 0;
  // We want to bring the real multiplier into the interval [1/2, 1).
  // We can do so by multiplying it by two, and recording how many times
  // we multiplied by two so that we can compensate that by a right
  // shift by the same amount.
  while (real_multiplier < 0.5f) {
    real_multiplier *= 2.0f;
    s++;
  }
  // Now that the real multiplier is in [1/2, 1), we convert it
  // into a fixed-point number.
  int64_t q = (int64_t)(round(real_multiplier * (1ll << 31)));
  assert(q <= (1ll << 31));
  // Handle the special case when the real multiplier was so close to 1
  // that its fixed-point approximation was undistinguishable from 1.
  // We handle this by dividing it by two, and remembering to decrement
  // the right shift amount.
  if (q == (1ll << 31)) {
    q /= 2;
    s--;
  }
  assert(s >= 0);
  assert(q <= (int64_t)LONG_MAX);
  *quantized_multiplier = (uint32_t)q;
  *right_shift = s;

#ifdef ENABLE_DEBUG_MSG
  printf(
      "    QuantizeMultiplierSmallerThanOne: %f -> multiplier %d, rshift %d\n",
      real_multiplier, *quantized_multiplier, *right_shift);
#endif
}

int8_t truncate_rshift(int8_t rshift, int8_t allow_lshift)
{
  int8_t lower_bound = 0;
  int8_t upper_bound = 0;
  const int8_t BITS = 6;

  if (rshift < 0) {
    printf("truncate_rshift rshift %d\n", rshift);
  }

  upper_bound = (1 << (BITS - 1)) - 1;
  lower_bound = allow_lshift ? (-1 * (1 << (BITS-1))) : 0;

  rshift = (rshift < lower_bound) ? lower_bound : rshift;
  rshift = (rshift > upper_bound) ? upper_bound : rshift;

  return rshift;
}
