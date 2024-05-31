/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#ifndef CVI_QUANT_HELPER_H
#define CVI_QUANT_HELPER_H
#include <assert.h>
#include <stdint.h>
#include <cmath>
#include <limits>
#include <iostream>

static int RoundingDivideByPOT(int x, int exponent) {
  if (x == 0) {
    return 0;
  }
  if (exponent == 0) {
    return x;
  }
  assert(exponent > 0);
  const int shift_vec = -exponent;
  const int fixup = (x & shift_vec) >> 31;
  const int fixed_up_x = x + fixup;

  int nudge = 1 << (exponent - 1);
  int val = (fixed_up_x + nudge) >> exponent;

  return val;
}

static int SaturatingRoundingDoublingHighMul(int a, int b) {
  int64_t a_64(a);
  int64_t b_64(b);
  int64_t ab_64 = a_64 * b_64;
  int nudge = ab_64 >= 0 ? (1 << 30) : (1 - (1 << 30));
  int ab_x2_high32 = static_cast<int>((ab_64 + nudge) / (1ll << 31));
  return ab_x2_high32;
}

/// saturate a float to range [-128, 127]
static int8_t saturateInt8(float f) {
#if 0
  // cast
  int q = (int)f;
#elif 0
  // away_from_zero
  int q = (f >= 0) ? (int)std::ceil(f) : (int)std::floor(f);
#elif 0
  // round
  int q = (int)std::roundf(f);
#elif 0
  // trancate, (towards zero)
  int q = (f >= 0) ? (int)std::floor(f) : (int)std::ceil(f);
#elif 1
  // from caffe_int8
  int q = (int)std::floor(f + 0.5);
#else
  // looks HW is different than std::round()
  // we shall apply round only for input quant()
  int q = (int)std::round(f);
#endif
  if (q > 127)
    q = 127;
  if (q < -128)
    q = -128;

  return (int8_t)q;
}

/// Simulate HW behavior, after accumuation
/// apply multiplier, do rshift, and then saturate to INT8
/// used in BM1880v2 per-channel mode (32bit bias)
/// qdm mode
///   use GOOGLE GEMMLOWP QDM multiply and shift
///   during multiply, a factor of (1 << 31) has been devided
static int8_t applyMultiplierAndRShiftAndSaturateInt8(float v, uint32_t rshift,
                                                      uint32_t multiplier, bool qdm) {
  if (qdm) {
    int32_t q = RoundingDivideByPOT(
        SaturatingRoundingDoublingHighMul((int32_t)v, (int32_t)multiplier), rshift);
    // llvm::errs() << "v,rshift,multiplier,q = " << v << ","
    //             << rshift << "," << multiplier << "," << q << "\n";
    return saturateInt8((float)q);
  } else {
    return saturateInt8(v * multiplier / (1 << rshift));
  }
}

// reference to reference to [arxiv 1712.05877]
// This implementation comes from tensorflow
// https://github.com/tensorflow/tensorflow/blob/98ff991500a0247f8f57c60db9a206204268bc42/tensorflow/lite/kernels/internal/quantization_util.cc#L52-L90
#define Tensorflow_QuantizeMultiplier QuantizeMultiplier
static void QuantizeMultiplier(double double_multiplier, int32_t *quantized_multiplier,
                        int *shift) {
  if (double_multiplier == 0.) {
    *quantized_multiplier = 0;
    *shift = 0;
    return;
  }

  const double q = std::frexp(double_multiplier, shift);
  auto q_fixed = static_cast<int64_t>(std::round(q * (1ll << 31)));

  assert(q_fixed <= (1ll << 31));
  if (q_fixed == (1ll << 31)) {
    q_fixed /= 2;
    ++*shift;
  }

  assert(q_fixed <= std::numeric_limits<int32_t>::max());
  // A shift amount smaller than -31 would cause all bits to be shifted out
  // and thus all results would be zero. We implement that instead with
  // q_fixed==0, so as to avoid hitting issues with right-shift
  // operations with shift amounts greater than 31. Note that this happens
  // roughly when abs(double_multiplier) < 2^-31 and the present handling means
  // that we're effectively flushing tiny double_multiplier's to zero.
  // We could conceivably handle values in the range (roughly) [32, 63]
  // as 'denormals' i.e. (shift==0, q_fixed < 2^30). In that point of view
  // the present handling is just doing 'flush denormals to zero'. We could
  // reconsider and actually generate nonzero denormals if a need arises.
  if (*shift < -31) {
    *shift = 0;
    q_fixed = 0;
  }
  *quantized_multiplier = static_cast<int32_t>(q_fixed);
}

/// find RShift and Multiplier from QScale
///   QScale = Multiplier / (1 << RShift)
///   Multiplier is an integer
/// case 1: specifically multiply a int8/uint8 multplier, then rshift
///   used in layers like element_wise, pooling, concat, etc
///   qdm is false
///   a max_multiplier (127 or 255 normally) has to be provided
/// case 2: qdm mode
///   used in BM1880v2 per-channel conv mode
///   qdm is true
///   reference to [arxiv 1712.05877]
///     choose the int32 value nearest to 2^31 * M0, M0 in [0.5, 1]
///     this value is always at least 2^30 and have at least 30 bits accuracy
///   the max_multiplier argument is ignored, fixed to (1 << 31)
/// if 'uint32_t *multiplier' is present, return multipler alongside
static int8_t findRShiftAndMultiplierFromQScale(double qscale,
                                                uint32_t *multiplier = nullptr,
                                                bool qdm = false,
                                                uint32_t max_multiplier = 127) {
  if (qdm) {
#if 0
    max_multiplier = (1 << 31);
    for (uint32_t rshift = 0; rshift < 63; ++rshift) {
      if ( ((double)qscale * (1ULL << (rshift + 1))) >= (double)max_multiplier ) {
        if (multiplier) {
          *multiplier = (uint32_t)((double)qscale * (1ULL << rshift));
        }
        return rshift - 31;
      }
    }
#endif
    // this ensures if qscale is 0, both multiplier and shift will be 0
    int32_t quantized_multiplier = 0;
    int lshift = 0;
    Tensorflow_QuantizeMultiplier(qscale, &quantized_multiplier, &lshift);
    if (multiplier)
      *multiplier = quantized_multiplier;
    int rshift = -lshift;
    assert(rshift >= 0);
    if (rshift > 25) {
      std::cout << "WARNING: large rshift = " << rshift << ", qscale = " << qscale
                << "\n";
    }
    return (int8_t)rshift;
  } else {
    assert(qscale < max_multiplier);
    for (int8_t rshift = 0; rshift < 63; ++rshift) {
      if (((double)qscale * (1ULL << (rshift + 1))) >= (double)max_multiplier) {
        if (multiplier) {
          *multiplier = (uint32_t)((double)qscale * (1ULL << rshift));
        }
        return rshift;
      }
    }
    // assert(false);
    std::cout << "WARNING: failed to find rshift, qscale = " << std::to_string(qscale)
              << "\n";
    // we are here because qscale is too small, return 0 for both shift and multiplier
    if (multiplier) {
      *multiplier = 0;
    }
    return 0;
  }
}
#endif