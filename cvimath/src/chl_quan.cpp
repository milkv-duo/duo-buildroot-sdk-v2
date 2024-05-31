#include <cvimath_internal.h>

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <iostream>

void cvm_get_chl_quan(float real_multiplier, uint32_t *quantized_multiplier, int *right_shift) {
  if (real_multiplier <= 0.f || real_multiplier > 1.f) {
    std::cerr << "Multiplier should be bigger than 0, smaller or euqal to 1." << std::endl;
    *quantized_multiplier = 0;
    *right_shift = 0;
    return;
  } else if (real_multiplier == 1.f) {
    *quantized_multiplier = (uint32_t)(1ll << 31) - 1;
    *right_shift = 0;
  } else {
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
    int64_t q = static_cast<int64_t>(round(real_multiplier * (1ll << 31)));
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
  }
}

inline void cvm_pack_per_chan_cal_data(uint32_t channels, bool has_bias, int32_t *bias,
                                       uint32_t *multiplier, int8_t *shift, uint8_t *packed_data) {
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
      uint8_t val = shift[i];
      *ptr = val;
      ptr++;
    }
  }
}

void cvm_fill_chl_quan_data(const uint32_t c, const uint32_t quantized_multiplier,
                            const int right_shift, uint8_t *cal_data, int32_t *bias_data,
                            bool has_bias) {
  // Create tl_multiplier
  uint32_t *multiplier_data = new uint32_t[c];
  int8_t *shift_data = new int8_t[c];
  for (unsigned int i = 0; i < c; ++i) {
    // multipliers typically range in [2^30 ; 2^31 - 1].
    // Values in [0, 2^30 - 1] are normally unused, but harmless.
    // Thus a good way to randomize multipliers is to subtract from them
    // a random value smaller than 2^30 but still significant compared to it.
    multiplier_data[i] = quantized_multiplier;

    // Our H/W only supports right shift
    shift_data[i] = right_shift > 0 ? right_shift : 0;

#ifdef ENABLE_DEBUG_MSG
    printf("      [oc=%d] multiplier_data %d, shift_data %d\n", i, p_param->multiplier_data[i],
           p_param->shift_data[i]);
#endif
  }

  cvm_pack_per_chan_cal_data(c, has_bias, bias_data, multiplier_data, shift_data, cal_data);
  delete[] multiplier_data;
  delete[] shift_data;
}

uint8_t *cvm_get_chl_quan_data(const uint32_t c, const uint32_t quantized_multiplier,
                               const int &right_shift, int32_t *bias_data, bool has_bias) {
  const int per_chan_cal_data_size =
      has_bias ? CVK_MULTIPLIER_BIAS_PACKED_DATA_SIZE : CVK_MULTIPLIER_ONLY_PACKED_DATA_SIZE;
  const int cal_data_size = c * per_chan_cal_data_size;
  uint8_t *cal_data = (uint8_t *)malloc(cal_data_size);
  cvm_fill_chl_quan_data(c, quantized_multiplier, right_shift, cal_data, bias_data, has_bias);
  return cal_data;
}
