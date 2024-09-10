#pragma once
#ifdef __ARM_ARCH
#include <arm_neon.h>
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "neon2sse/NEON_2_SSE.h"
#pragma clang diagnostic pop
#endif
#include <cmath>
#include <limits>

union neonfloatshort {
  uint16x8x2_t v_u16;
  float32x4x2_t v_f32;
};

/**
 * Nearest formula:
 * float a;
 * short b = trunc(a + ((a > 0) ? 0.5 : - 0.5));
 * Without if condition:
 * float a;
 * short b = trunc(a + float((uint32(a) & 0x8000000) | uint32(0.5)));
 * For any questions ask KK.
 */
__attribute__((always_inline)) inline uint32x4_t vcvtq_u32_f32_r(float32x4_t v4) {
  const float32x4_t half4 = vdupq_n_f32(0.5f);
  const uint32x4_t mask4 = vdupq_n_u32(0x80000000);

  const float32x4_t w4 = vreinterpretq_f32_u32(
      vorrq_u32(vandq_u32(vreinterpretq_u32_f32(v4), mask4), vreinterpretq_u32_f32(half4)));
  return vcvtq_u32_f32(vaddq_f32(v4, w4));
}

__attribute__((always_inline)) inline int32x4_t vcvtq_s32_f32_r(float32x4_t v4) {
  const float32x4_t half4 = vdupq_n_f32(0.5f);
  const uint32x4_t mask4 = vdupq_n_u32(0x80000000);

  const float32x4_t w4 = vreinterpretq_f32_u32(
      vorrq_u32(vandq_u32(vreinterpretq_u32_f32(v4), mask4), vreinterpretq_u32_f32(half4)));
  return vcvtq_s32_f32(vaddq_f32(v4, w4));
}

#ifndef __ARM_ARCH
#include <immintrin.h>
_NEON2SSESTORAGE float32x4_t vfmaq_f32(float32x4_t a, float32x4_t b,
                                       float32x4_t c);  // VMLA.F32 q0,q0,q0
_NEON2SSE_INLINE float32x4_t vfmaq_f32(float32x4_t a, float32x4_t b,
                                       float32x4_t c)  // VMLA.F32 q0,q0,q0
{
  return _mm_fmadd_ps(a, b, c);
}
union {
  __m128 v;
  float a[4];
} m128uF;
_NEON2SSESTORAGE float32_t vaddvq_f32(float32x4_t a);
_NEON2SSE_INLINE float32_t vaddvq_f32(float32x4_t a) {
  m128uF.v = a;
  return m128uF.a[0] + m128uF.a[1] + m128uF.a[2] + m128uF.a[3];
}
#endif

inline void neonU16FindMinMax(uint16_t *src_ptr, const uint64_t src_size, uint16_t *min,
                              uint16_t *max) {
  uint64_t neon_turn = src_size / 8;
  uint16x8_t v_u16min = vdupq_n_u16(*min);
  uint16x8_t v_u16max = vdupq_n_u16(*max);
  uint16_t *src_ptr1 = src_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    v_u16min = vminq_u16(v_u16min, v16);
    v_u16max = vmaxq_u16(v_u16max, v16);
    src_ptr1 += 8;
  }
  uint16x4_t v_u16min_low = vget_low_u16(v_u16min);
  uint16x4_t v_u16min_high = vget_high_u16(v_u16min);
  v_u16min_low = vmin_u16(v_u16min_low, v_u16min_high);
  uint16x4_t v_u16max_low = vget_low_u16(v_u16max);
  uint16x4_t v_u16max_high = vget_high_u16(v_u16max);
  v_u16max_low = vmax_u16(v_u16max_low, v_u16max_high);
  uint16_t min_arr[4];
  uint16_t max_arr[4];
  vst1_u16(min_arr, v_u16min_low);
  vst1_u16(max_arr, v_u16max_low);
  for (size_t i = 0; i < 4; i++) {
    if (min_arr[i] < *min) {
      *min = min_arr[i];
    }
  }
  for (size_t i = 0; i < 4; i++) {
    if (max_arr[i] > *max) {
      *max = max_arr[i];
    }
  }
  for (uint64_t i = neon_turn * 8; i < src_size; i++) {
    uint16_t tmp = src_ptr[i];
    if (tmp < *min) {
      *min = tmp;
    }
    if (tmp > *max) {
      *max = tmp;
    }
  }
}

inline void neonBF16FindMinMax(uint16_t *src_ptr, const uint64_t src_size, float *min, float *max) {
  uint64_t neon_turn = src_size / 8;
  float32x4_t v_fmin = vdupq_n_f32(*min);
  float32x4_t v_fmax = vdupq_n_f32(*max);
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    v_fmin = vminq_f32(v_fmin, n_float_short.v_f32.val[0]);
    v_fmin = vminq_f32(v_fmin, n_float_short.v_f32.val[1]);
    v_fmax = vmaxq_f32(v_fmax, n_float_short.v_f32.val[0]);
    v_fmax = vmaxq_f32(v_fmax, n_float_short.v_f32.val[1]);
    src_ptr1 += 8;
  }
  float32x2_t v_fmin_low = vget_low_f32(v_fmin);
  float32x2_t v_fmin_high = vget_high_f32(v_fmin);
  v_fmin_low = vmin_f32(v_fmin_low, v_fmin_high);
  float32x2_t v_fmax_low = vget_low_f32(v_fmax);
  float32x2_t v_fmax_high = vget_high_f32(v_fmax);
  v_fmax_low = vmax_f32(v_fmax_low, v_fmax_high);
  float32_t min_arr[2];
  float32_t max_arr[2];
  vst1_f32(min_arr, v_fmin_low);
  vst1_f32(max_arr, v_fmax_low);
  *min = min_arr[0] > min_arr[1] ? min_arr[1] : min_arr[0];
  *max = max_arr[0] > max_arr[1] ? max_arr[0] : max_arr[1];
  for (uint64_t i = neon_turn * 8; i < src_size; i++) {
    float tmp = convert_bf16_fp32(src_ptr[i]);
    if (tmp < *min) {
      *min = tmp;
    }
    if (tmp > *max) {
      *max = tmp;
    }
  }
}

inline void neonU162U8Normalize(uint16_t *src_ptr, uint8_t *dst_ptr, const uint64_t arr_size,
                                const uint16_t min, const uint16_t max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 255.f / (max - min);
  uint16x8_t v_u16min = vdupq_n_u16(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  uint16_t *src_ptr1 = src_ptr;
  uint8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    uint16x8_t u16sub = vsubq_u16(v16, v_u16min);
    uint32x4_t u32sub_low = vmovl_u16(vget_low_u16(u16sub));
    uint32x4_t u32sub_high = vmovl_u16(vget_high_u16(u16sub));
    float32x4_t fsub_low = vcvtq_f32_u32(u32sub_low);
    float32x4_t fsub_high = vcvtq_f32_u32(u32sub_high);
    float32x4_t fmul_low = vmulq_f32(fsub_low, v_fmultiplier);
    float32x4_t fmul_high = vmulq_f32(fsub_high, v_fmultiplier);
    uint32x4_t u32mul_low = vcvtq_u32_f32_r(fmul_low);
    uint32x4_t u32mul_high = vcvtq_u32_f32_r(fmul_high);
    uint16x8_t u16_mul = vcombine_u16(vqmovn_u32(u32mul_low), vqmovn_u32(u32mul_high));
    vst1_u8(dst_ptr1, vqmovn_u16(u16_mul));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    dst_ptr[i] = std::round((float)(multiplier * (src_ptr[i] - min)));
  }
}

inline void neonU162S8Normalize(uint16_t *src_ptr, int8_t *dst_ptr, const uint64_t arr_size,
                                const uint16_t min, const uint16_t max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 255.f / (max - min);
  uint16x8_t v_u16min = vdupq_n_u16(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  float32x4_t v_foffset = vdupq_n_f32(-128.f);
  uint16_t *src_ptr1 = src_ptr;
  int8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    uint16x8_t u16sub = vsubq_u16(v16, v_u16min);
    uint32x4_t u32sub_low = vmovl_u16(vget_low_u16(u16sub));
    uint32x4_t u32sub_high = vmovl_u16(vget_high_u16(u16sub));
    float32x4_t fsub_low = vcvtq_f32_u32(u32sub_low);
    float32x4_t fsub_high = vcvtq_f32_u32(u32sub_high);
    float32x4_t fmul_low = vfmaq_f32(v_foffset, fsub_low, v_fmultiplier);
    float32x4_t fmul_high = vfmaq_f32(v_foffset, fsub_high, v_fmultiplier);
    int32x4_t s32mul_low = vcvtq_s32_f32_r(fmul_low);
    int32x4_t s32mul_high = vcvtq_s32_f32_r(fmul_high);
    int16x8_t s16_mul = vcombine_s16(vqmovn_s32(s32mul_low), vqmovn_s32(s32mul_high));
    vst1_s8(dst_ptr1, vqmovn_s16(s16_mul));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    dst_ptr[i] = std::round((float)(multiplier * (src_ptr[i] - min)) - 128);
  }
}

inline void neonBF162U8Normalize(uint16_t *src_ptr, uint8_t *dst_ptr, const uint64_t arr_size,
                                 const float min, const float max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 255.f / (max - min);
  float32x4_t v_fmin = vdupq_n_f32(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  uint8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    float32x4_t fsub_0 = vsubq_f32(n_float_short.v_f32.val[0], v_fmin);
    float32x4_t fsub_1 = vsubq_f32(n_float_short.v_f32.val[1], v_fmin);
    float32x4_t fmul_0 = vmulq_f32(fsub_0, v_fmultiplier);
    float32x4_t fmul_1 = vmulq_f32(fsub_1, v_fmultiplier);
    uint32x4_t u32mul_0 = vcvtq_u32_f32_r(fmul_0);
    uint32x4_t u32mul_1 = vcvtq_u32_f32_r(fmul_1);
    uint16x8_t u16_mul = vcombine_u16(vqmovn_u32(u32mul_0), vqmovn_u32(u32mul_1));
    vst1_u8(dst_ptr1, vqmovn_u16(u16_mul));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    float tmp = convert_bf16_fp32(src_ptr[i]);
    dst_ptr[i] = std::round((float)(multiplier * (tmp - min)));
  }
}

inline void neonBF162I8Normalize(uint16_t *src_ptr, int8_t *dst_ptr, const uint64_t arr_size,
                                 const float min, const float max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 255.f / (max - min);
  float32x4_t v_fmin = vdupq_n_f32(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  float32x4_t v_foffset = vdupq_n_f32(-128.f);
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  int8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    float32x4_t fsub_0 = vsubq_f32(n_float_short.v_f32.val[0], v_fmin);
    float32x4_t fsub_1 = vsubq_f32(n_float_short.v_f32.val[1], v_fmin);
    float32x4_t fmul_0 = vfmaq_f32(v_foffset, fsub_0, v_fmultiplier);
    float32x4_t fmul_1 = vfmaq_f32(v_foffset, fsub_1, v_fmultiplier);
    int32x4_t s32mul_0 = vcvtq_s32_f32_r(fmul_0);
    int32x4_t s32mul_1 = vcvtq_s32_f32_r(fmul_1);
    int16x8_t s16_mul = vcombine_s16(vqmovn_s32(s32mul_0), vqmovn_s32(s32mul_1));
    vst1_s8(dst_ptr1, vqmovn_s16(s16_mul));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    float tmp = convert_bf16_fp32(src_ptr[i]);
    dst_ptr[i] = std::round((float)(multiplier * (tmp - min)) - 128);
  }
}

inline void neonBF162U16Normalize(uint16_t *src_ptr, uint16_t *dst_ptr, const uint64_t arr_size,
                                  const float min, const float max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 65535.f / (max - min);
  float32x4_t v_fmin = vdupq_n_f32(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  uint16_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    float32x4_t fsub_0 = vsubq_f32(n_float_short.v_f32.val[0], v_fmin);
    float32x4_t fsub_1 = vsubq_f32(n_float_short.v_f32.val[1], v_fmin);
    float32x4_t fmul_0 = vmulq_f32(fsub_0, v_fmultiplier);
    float32x4_t fmul_1 = vmulq_f32(fsub_1, v_fmultiplier);
    uint32x4_t u32mul_0 = vcvtq_u32_f32_r(fmul_0);
    uint32x4_t u32mul_1 = vcvtq_u32_f32_r(fmul_1);
    uint16x8_t u16_mul = vcombine_u16(vqmovn_u32(u32mul_0), vqmovn_u32(u32mul_1));
    vst1q_u16(dst_ptr1, u16_mul);
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    float tmp = convert_bf16_fp32(src_ptr[i]);
    dst_ptr[i] = std::round((float)(multiplier * (tmp - min)));
  }
}

inline void neonBF162S16Normalize(uint16_t *src_ptr, int16_t *dst_ptr, const uint64_t arr_size,
                                  const float min, const float max) {
  uint64_t neon_turn = arr_size / 8;
  float multiplier = 65535.f / (max - min);
  float32x4_t v_fmin = vdupq_n_f32(min);
  float32x4_t v_fmultiplier = vdupq_n_f32(multiplier);
  float32x4_t v_foffset = vdupq_n_f32(-32768.f);
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  int16_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    float32x4_t fsub_0 = vsubq_f32(n_float_short.v_f32.val[0], v_fmin);
    float32x4_t fsub_1 = vsubq_f32(n_float_short.v_f32.val[1], v_fmin);
    float32x4_t fmul_0 = vfmaq_f32(v_foffset, fsub_0, v_fmultiplier);
    float32x4_t fmul_1 = vfmaq_f32(v_foffset, fsub_1, v_fmultiplier);
    int32x4_t s32mul_0 = vcvtq_s32_f32_r(fmul_0);
    int32x4_t s32mul_1 = vcvtq_s32_f32_r(fmul_1);
    int16x8_t s16_mul = vcombine_s16(vqmovn_s32(s32mul_0), vqmovn_s32(s32mul_1));
    vst1q_s16(dst_ptr1, s16_mul);
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    float tmp = convert_bf16_fp32(src_ptr[i]);
    dst_ptr[i] = std::round((float)(multiplier * (tmp - min))) - 32768;
  }
}

inline void neonBF162U16(uint16_t *src_ptr, uint16_t *dst_ptr, const uint64_t arr_size) {
  uint64_t neon_turn = arr_size / 8;
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  uint16_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    uint32x4_t u32_0 = vcvtq_u32_f32_r(n_float_short.v_f32.val[0]);
    uint32x4_t u32_1 = vcvtq_u32_f32_r(n_float_short.v_f32.val[1]);
    uint16x8_t u16_res = vcombine_u16(vqmovn_u32(u32_0), vqmovn_u32(u32_1));
    vst1q_u16(dst_ptr1, u16_res);
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    int val = std::round(convert_bf16_fp32(src_ptr[i]));
    if (val > 65535) val = 65535;
    if (val < 0) val = 0;
    dst_ptr[i] = val;
  }
}

inline void neonBF162S16(uint16_t *src_ptr, int16_t *dst_ptr, const uint64_t arr_size) {
  uint64_t neon_turn = arr_size / 8;
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  int16_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    int32x4_t s32_0 = vcvtq_s32_f32_r(n_float_short.v_f32.val[0]);
    int32x4_t s32_1 = vcvtq_s32_f32_r(n_float_short.v_f32.val[1]);
    int16x8_t s16_res = vcombine_s16(vqmovn_s32(s32_0), vqmovn_s32(s32_1));
    vst1q_s16(dst_ptr1, s16_res);
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    int val = std::round(convert_bf16_fp32(src_ptr[i]));
    if (val > 32767) val = 32767;
    if (val < -32768) val = -32768;
    dst_ptr[i] = val;
  }
}

// Broken due to unequal stride. Temporarily disable.
inline void neonBF162F32(uint16_t *src_ptr, float *dst_ptr, const uint64_t arr_size) {
  uint64_t neon_turn = arr_size / 8;
  uint16x8_t zeros = vdupq_n_u16(0);
  uint16_t *src_ptr1 = src_ptr;
  float *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    neonfloatshort n_float_short;
    n_float_short.v_u16 = vzipq_u16(zeros, v16);
    vst1q_f32(dst_ptr1, n_float_short.v_f32.val[0]);
    dst_ptr1 += 4;
    vst1q_f32(dst_ptr1, n_float_short.v_f32.val[1]);
    dst_ptr1 += 4;
    src_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    dst_ptr[i] = convert_bf16_fp32(src_ptr[i]);
  }
}

inline void neonU162U8Threshold(uint16_t *src_ptr, uint8_t *dst_ptr, const uint64_t arr_size,
                                const uint16_t threshold, const uint8_t min, const uint8_t max) {
  uint64_t neon_turn = arr_size / 8;
  uint16x8_t vu16thresh = vdupq_n_u16(threshold);
  uint16x8_t v_u16min = vdupq_n_u16(min);
  uint16x8_t v_u16max = vdupq_n_u16(max);
  uint16_t *src_ptr1 = src_ptr;
  uint8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t v16 = vld1q_u16(src_ptr1);
    uint16x8_t mask = vcltq_u16(v16, vu16thresh);
    v16 = vbslq_u16(mask, v_u16min, v_u16max);
    vst1_u8(dst_ptr1, vqmovn_u16(v16));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    uint16_t val = src_ptr[i] >= threshold ? max : min;
    if (val > 255) val = 255;
    dst_ptr[i] = val;
  }
}

inline void neonS162S8Threshold(int16_t *src_ptr, int8_t *dst_ptr, const uint64_t arr_size,
                                const int16_t threshold, const int8_t min, const int8_t max) {
  uint64_t neon_turn = arr_size / 8;
  int16x8_t vs16thresh = vdupq_n_s16(threshold);
  int16x8_t v_s16min = vdupq_n_s16(min);
  int16x8_t v_s16max = vdupq_n_s16(max);
  int16_t *src_ptr1 = src_ptr;
  int8_t *dst_ptr1 = dst_ptr;
  for (uint64_t i = 0; i < neon_turn; i++) {
    int16x8_t v16 = vld1q_s16(src_ptr1);
    uint16x8_t mask = vcltq_s16(v16, vs16thresh);
    v16 = vbslq_s16(mask, v_s16min, v_s16max);
    vst1_s8(dst_ptr1, vqmovn_s16(v16));
    src_ptr1 += 8;
    dst_ptr1 += 8;
  }
  for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
    int16_t val = src_ptr[i] >= threshold ? max : min;
    if (val > 127) val = 127;
    if (val < -128) val = -128;
    dst_ptr[i] = val;
  }
}

inline void neonU8Accumulate(uint8_t *src_ptr, const uint64_t arr_size, uint64_t *result) {
  uint64_t neon_turn = arr_size / 16;
  uint64_t rest = arr_size - (neon_turn * 16);
  *result = 0;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint8x16_t value = vld1q_u8(src_ptr);
    uint64x2_t sum = vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(value)));
    *result += (vgetq_lane_u64(sum, 0) + vgetq_lane_u64(sum, 1));
    src_ptr += 16;
  }

  for (uint64_t i = 0; i < rest; i++) {
    *result += *src_ptr;
    src_ptr++;
  }
}

inline void neonU162U8ThresholdLH(uint16_t *src_ptr, uint8_t *dst_ptr, const uint64_t arr_size,
                                  const uint16_t threshold_low, const uint16_t threshold_high,
                                  const uint8_t min, const uint8_t mid, const uint8_t max,
                                  bool is_mmm = false) {
  uint64_t neon_turn = arr_size / 8;
  uint16x8_t vu16threshLow = vdupq_n_u16(threshold_low);
  uint16x8_t vu16threshHigh = vdupq_n_u16(threshold_high);
  uint16x8_t v_u16min = vdupq_n_u16(min);
  uint16x8_t v_u16mid = vdupq_n_u16(mid);
  uint16x8_t v_u16max = vdupq_n_u16(max);
  uint16_t *src_ptr1 = src_ptr;
  uint8_t *dst_ptr1 = dst_ptr;
  if (is_mmm) {
    for (uint64_t i = 0; i < neon_turn; i++) {
      uint16x8_t v16 = vld1q_u16(src_ptr1);
      uint16x8_t mask_low = vcltq_u16(v16, vu16threshLow);
      uint16x8_t mask_high = vcgeq_u16(v16, vu16threshHigh);
      v16 = vbslq_u16(mask_low, v_u16min, v_u16mid);
      v16 = vbslq_u16(mask_high, v_u16max, v16);
      vst1_u8(dst_ptr1, vqmovn_u16(v16));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      uint16_t val = src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : mid);
      if (val > 255) val = 255;
      dst_ptr[i] = val;
    }
  } else {
    for (uint64_t i = 0; i < neon_turn; i++) {
      uint16x8_t v16 = vld1q_u16(src_ptr1);
      uint16x8_t mask_low = vcltq_u16(v16, vu16threshLow);
      uint16x8_t mask_high = vcgeq_u16(v16, vu16threshHigh);
      v16 = vbslq_u16(mask_low, v_u16min, v16);
      v16 = vbslq_u16(mask_high, v_u16max, v16);
      vst1_u8(dst_ptr1, vqmovn_u16(v16));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      uint16_t val =
          src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : src_ptr[i]);
      if (val > 255) val = 255;
      dst_ptr[i] = val;
    }
  }
}

inline void neonS162S8ThresholdLH(int16_t *src_ptr, int8_t *dst_ptr, const uint64_t arr_size,
                                  const int16_t threshold_low, const int16_t threshold_high,
                                  const int8_t min, const int8_t mid, const int8_t max,
                                  bool is_mmm = false) {
  uint64_t neon_turn = arr_size / 8;
  int16x8_t vs16threshLow = vdupq_n_s16(threshold_low);
  int16x8_t vs16threshHigh = vdupq_n_s16(threshold_high);
  int16x8_t v_s16min = vdupq_n_s16(min);
  int16x8_t v_s16mid = vdupq_n_s16(mid);
  int16x8_t v_s16max = vdupq_n_s16(max);
  int16_t *src_ptr1 = src_ptr;
  int8_t *dst_ptr1 = dst_ptr;
  if (is_mmm) {
    for (uint64_t i = 0; i < neon_turn; i++) {
      int16x8_t v16 = vld1q_s16(src_ptr1);
      uint16x8_t mask_low = vcltq_s16(v16, vs16threshLow);
      uint16x8_t mask_high = vcgeq_s16(v16, vs16threshHigh);
      v16 = vbslq_s16(mask_low, v_s16min, v_s16mid);
      v16 = vbslq_s16(mask_high, v_s16max, v16);
      vst1_s8(dst_ptr1, vqmovn_s16(v16));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      int16_t val = src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : mid);
      if (val > 127) val = 127;
      if (val < -128) val = -128;
      dst_ptr[i] = val;
    }
  } else {
    for (uint64_t i = 0; i < neon_turn; i++) {
      int16x8_t v16 = vld1q_s16(src_ptr1);
      uint16x8_t mask_low = vcltq_s16(v16, vs16threshLow);
      uint16x8_t mask_high = vcgeq_s16(v16, vs16threshHigh);
      v16 = vbslq_s16(mask_low, v_s16min, v16);
      v16 = vbslq_s16(mask_high, v_s16max, v16);
      vst1_s8(dst_ptr1, vqmovn_s16(v16));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      int16_t val =
          src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : src_ptr[i]);
      if (val > 127) val = 127;
      if (val < -128) val = -128;
      dst_ptr[i] = val;
    }
  }
}

inline void neonU16SeperateU8(uint16_t *src_ptr, uint8_t *dst1_ptr, uint8_t *dst2_ptr,
                              const uint64_t arr_size) {
  uint64_t neon_turn = arr_size / 8;
  for (uint64_t i = 0; i < neon_turn; i++) {
    uint16x8_t u16_data = vld1q_u16(src_ptr);
    uint8x8_t u8_table_index = vqshrn_n_u16(u16_data, 8);
    uint8x8_t u8_lookup_index = vmovn_u16(u16_data);

    vst1_u8(dst1_ptr, u8_table_index);
    vst1_u8(dst2_ptr, u8_lookup_index);

    src_ptr += 8;
    dst1_ptr += 8;
    dst2_ptr += 8;
  }
}

inline void neonS162U8ThresholdLH(int16_t *src_ptr, uint8_t *dst_ptr, const uint64_t arr_size,
                                  const int16_t threshold_low, const int16_t threshold_high,
                                  const uint8_t min, const uint8_t mid, const uint8_t max,
                                  bool is_mmm = false) {
  uint64_t neon_turn = arr_size / 8;
  int16x8_t vs16threshLow = vdupq_n_s16(threshold_low);
  int16x8_t vs16threshHigh = vdupq_n_s16(threshold_high);
  int16x8_t v_s16min = vdupq_n_s16(min);
  int16x8_t v_s16mid = vdupq_n_s16(mid);
  int16x8_t v_s16max = vdupq_n_s16(max);
  int16_t *src_ptr1 = src_ptr;
  uint8_t *dst_ptr1 = dst_ptr;
  if (is_mmm) {
    for (uint64_t i = 0; i < neon_turn; i++) {
      int16x8_t v16 = vld1q_s16(src_ptr1);
      uint16x8_t mask_low = vcltq_s16(v16, vs16threshLow);
      uint16x8_t mask_high = vcgeq_s16(v16, vs16threshHigh);
      v16 = vbslq_s16(mask_low, v_s16min, v_s16mid);
      v16 = vbslq_s16(mask_high, v_s16max, v16);
      vst1_u8(dst_ptr1, vqmovn_u16(vreinterpretq_u16_s16(v16)));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      int16_t val = src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : mid);
      if (val > 255) val = 255;
      if (val < 0) val = 0;
      dst_ptr[i] = val;
    }
  } else {
    for (uint64_t i = 0; i < neon_turn; i++) {
      int16x8_t v16 = vld1q_s16(src_ptr1);
      uint16x8_t mask_low = vcltq_s16(v16, vs16threshLow);
      uint16x8_t mask_high = vcgeq_s16(v16, vs16threshHigh);
      v16 = vbslq_s16(mask_low, v_s16min, v16);
      v16 = vbslq_s16(mask_high, v_s16max, v16);
      vst1_u8(dst_ptr1, vqmovn_u16(vreinterpretq_u16_s16(v16)));
      src_ptr1 += 8;
      dst_ptr1 += 8;
    }
    for (uint64_t i = neon_turn * 8; i < arr_size; i++) {
      int16_t val =
          src_ptr[i] >= threshold_high ? max : (src_ptr[i] < threshold_low ? min : src_ptr[i]);
      if (val > 255) val = 255;
      if (val < 0) val = 0;
      dst_ptr[i] = val;
    }
  }
}