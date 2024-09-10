#include "neon_utils.hpp"
#include <iostream>

namespace cvitdl {

using cv::Mat;
using std::string;
using std::vector;

#ifdef __ARM_ARCH
#ifdef USE_NEON_F2S_NEAREST
#define NEON_ROUNDING_VALUE 0.5
/* vcvtq_s32_f32 "rounds" numbers, note that OpenCV convertTo uses nearest
 * Nearest formula:
 * float a;
 * short b = trunc(a + ((a > 0) ? 0.5 : - 0.5));
 * Without if condition:
 * float a;
 * short b = trunc(a + float((uint32(a) & 0x8000000) | uint32(0.5)));
 * For any questions ask KK.
 */
__attribute__((always_inline)) inline int32x4_t vcvtq_s32_f32_r(float32x4_t v4) {
  const float32x4_t half4 = vdupq_n_f32(0.5f);
  const uint32x4_t mask4 = vdupq_n_u32(0x80000000);

  const float32x4_t w4 = vreinterpretq_f32_u32(
      vorrq_u32(vandq_u32(vreinterpretq_u32_f32(v4), mask4), vreinterpretq_u32_f32(half4)));
  return vcvtq_s32_f32(vaddq_f32(v4, w4));
}
#else
#define NEON_ROUNDING_VALUE 0
__attribute__((always_inline)) inline int32x4_t vcvtq_s32_f32_r(float32x4_t v4) {
  return vcvtq_s32_f32(v4);
}
#endif
#endif
// MACRO Tokens :P
#define GENERATE_NEON_SPLITBYCHANNEL(Var, srcptr, sum, mul, ptr)           \
  uint16x8_t Var##u16 = vmovl_u8(srcptr);                                  \
  uint32x4_t Var##low = vmovl_u16(vget_low_u16(Var##u16));                 \
  uint32x4_t Var##high = vmovl_u16(vget_high_u16(Var##u16));               \
  float32x4_t Var##f32_low = vcvtq_f32_u32(Var##low);                      \
  float32x4_t Var##f32_high = vcvtq_f32_u32(Var##high);                    \
  float32x4_t Var##0 = vfmaq_f32(sum, Var##f32_low, mul);                  \
  float32x4_t Var##1 = vfmaq_f32(sum, Var##f32_high, mul);                 \
  int32x4_t Var##l = vcvtq_s32_f32_r(Var##0);                              \
  int32x4_t Var##h = vcvtq_s32_f32_r(Var##1);                              \
  int16x8_t Var##s = vcombine_s16(vqmovn_s32(Var##l), vqmovn_s32(Var##h)); \
  vst1_s8(ptr, vqmovn_s16(Var##s));

void NormalizeToU8(cv::Mat *src_channels, const std::vector<float> &mean,
                   const std::vector<float> &scales, std::vector<cv::Mat> &channels) {
  for (size_t c = 0; c < 3; c++) {
    src_channels[c].convertTo(src_channels[c], CV_32F);
    src_channels[c].convertTo(channels[c], CV_8SC1, scales[c], mean[c] * scales[c] + 0.5);
  }
}

// NEON instructions only supports channels from 1 to 4
void NormalizeAndSplitToU8(cv::Mat &prepared, const std::vector<float> &mean,
                           const std::vector<float> &scales, std::vector<cv::Mat> &channels) {
#ifndef __ARM_ARCH
  vector<cv::Mat> tmpchannels;
  if (prepared.type() != CV_32F) prepared.convertTo(prepared, CV_32F);
  cv::split(prepared, tmpchannels);
  for (size_t c = 0; c < tmpchannels.size(); c++) {
    tmpchannels[c].convertTo(channels[c], CV_8SC1, scales[c], mean[c] * scales[c] + 0.5);
  }
#else
  const int &width = prepared.cols;
  const int &height = prepared.rows;
  bool is_padded =
      ((size_t)prepared.step == (size_t)(prepared.cols * prepared.channels())) ? false : true;
  if (prepared.channels() == 1) {
    unsigned char *pSrc = (unsigned char *)prepared.data;
    int8_t *pG = (int8_t *)channels[0].data;

    float mG = mean[0] * scales[0] + NEON_ROUNDING_VALUE;

    float32x4_t sumG = vdupq_n_f32(mG);
    float32x4_t mulG = vdupq_n_f32(scales[0]);

    int total_pixel = width * height;
    int i_limit = (int)(total_pixel / 8) * 8;

    int i = 0;        // Index need to be saved in later use.
    int counter = 0;  // Counter for counting padded source image index.
    int offset = prepared.step - width;
    for (i = 0; i < i_limit;) {
      uint8x8_t src = vld1_u8(pSrc);
      GENERATE_NEON_SPLITBYCHANNEL(G, src, sumG, mulG, pG);

      // TODO: FIXME: Need optimize

      int aa = width - counter;
      if (aa <= 8 && is_padded) {
        counter = 0;
        pG += aa;
        i += aa;
        pSrc += aa + offset;
      } else {
        counter += 8;
        pG += 8;
        i += 8;
        pSrc += 8;
      }
    }
    for (; i < total_pixel; i++) {
      *pG = cv::saturate_cast<char>((float)pSrc[0] * scales[0] + mG);
      pG++;
      pSrc += 1;
    }
  } else if (prepared.channels() == 2) {
    unsigned char *pSrc = (unsigned char *)prepared.data;
    int8_t *pG = (int8_t *)channels[0].data;
    int8_t *pB = (int8_t *)channels[1].data;

    float mG = mean[0] * scales[0] + NEON_ROUNDING_VALUE;
    float mB = mean[1] * scales[1] + NEON_ROUNDING_VALUE;

    float32x4_t sumG = vdupq_n_f32(mG);
    float32x4_t mulG = vdupq_n_f32(scales[0]);

    float32x4_t sumB = vdupq_n_f32(mB);
    float32x4_t mulB = vdupq_n_f32(scales[1]);

    int total_pixel = width * height;
    int i_limit = (int)(total_pixel / 8) * 8;

    int i = 0;        // Index need to be saved in later use.
    int counter = 0;  // Counter for counting padded source image index.
    int offset = prepared.step - width * 2;
    for (i = 0; i < i_limit;) {
      uint8x8x2_t src = vld2_u8(pSrc);
      GENERATE_NEON_SPLITBYCHANNEL(G, src.val[0], sumG, mulG, pG);
      GENERATE_NEON_SPLITBYCHANNEL(B, src.val[1], sumB, mulB, pB);

      // TODO: FIXME: Need optimize

      int aa = width - counter;
      if (aa <= 8 && is_padded) {
        counter = 0;
        pG += aa;
        pB += aa;
        i += aa;
        pSrc += aa * 2 + offset;
      } else {
        counter += 8;
        pG += 8;
        pB += 8;
        i += 8;
        pSrc += 16;
      }
    }
    for (; i < total_pixel; i++) {
      *pG = cv::saturate_cast<char>((float)pSrc[0] * scales[0] + mG);
      *pB = cv::saturate_cast<char>((float)pSrc[1] * scales[1] + mB);
      pG++;
      pB++;
      pSrc += 2;
    }
  } else if (prepared.channels() == 3) {
    unsigned char *pSrc = (unsigned char *)prepared.data;
    int8_t *pR = (int8_t *)channels[0].data;
    int8_t *pG = (int8_t *)channels[1].data;
    int8_t *pB = (int8_t *)channels[2].data;

    float mR = mean[1] * scales[0] + NEON_ROUNDING_VALUE;
    float mG = mean[1] * scales[1] + NEON_ROUNDING_VALUE;
    float mB = mean[2] * scales[2] + NEON_ROUNDING_VALUE;

    float32x4_t sumR = vdupq_n_f32(mR);
    float32x4_t mulR = vdupq_n_f32(scales[0]);

    float32x4_t sumG = vdupq_n_f32(mG);
    float32x4_t mulG = vdupq_n_f32(scales[1]);

    float32x4_t sumB = vdupq_n_f32(mB);
    float32x4_t mulB = vdupq_n_f32(scales[2]);

    int total_pixel = width * height;
    int i_limit = (int)(total_pixel / 8) * 8;

    int i = 0;        // Index need to be saved in later use.
    int counter = 0;  // Counter for counting padded source image index.
    int offset = prepared.step - width * 3;
    for (i = 0; i < i_limit;) {
      uint8x8x3_t src = vld3_u8(pSrc);
      GENERATE_NEON_SPLITBYCHANNEL(R, src.val[0], sumR, mulR, pR);
      GENERATE_NEON_SPLITBYCHANNEL(G, src.val[1], sumG, mulG, pG);
      GENERATE_NEON_SPLITBYCHANNEL(B, src.val[2], sumB, mulB, pB);

      // TODO: FIXME: Need optimize

      int aa = width - counter;
      if (aa <= 8 && is_padded) {
        counter = 0;
        pR += aa;
        pG += aa;
        pB += aa;
        i += aa;
        pSrc += aa * 3 + offset;
      } else {
        counter += 8;
        pR += 8;
        pG += 8;
        pB += 8;
        i += 8;
        pSrc += 24;
      }
    }
    for (; i < total_pixel; i++) {
      *pR = cv::saturate_cast<char>((float)pSrc[0] * scales[0] + mR);
      *pG = cv::saturate_cast<char>((float)pSrc[1] * scales[1] + mG);
      *pB = cv::saturate_cast<char>((float)pSrc[2] * scales[2] + mB);
      pR++;
      pG++;
      pB++;
      pSrc += 3;
    }
  } else if (prepared.channels() == 4) {
    unsigned char *pSrc = (unsigned char *)prepared.data;
    int8_t *pA = (int8_t *)channels[0].data;
    int8_t *pR = (int8_t *)channels[1].data;
    int8_t *pG = (int8_t *)channels[2].data;
    int8_t *pB = (int8_t *)channels[3].data;

    float mA = mean[0] * scales[0] + NEON_ROUNDING_VALUE;
    float mR = mean[1] * scales[1] + NEON_ROUNDING_VALUE;
    float mG = mean[2] * scales[2] + NEON_ROUNDING_VALUE;
    float mB = mean[3] * scales[3] + NEON_ROUNDING_VALUE;

    float32x4_t sumA = vdupq_n_f32(mA);
    float32x4_t mulA = vdupq_n_f32(scales[0]);

    float32x4_t sumR = vdupq_n_f32(mR);
    float32x4_t mulR = vdupq_n_f32(scales[1]);

    float32x4_t sumG = vdupq_n_f32(mG);
    float32x4_t mulG = vdupq_n_f32(scales[2]);

    float32x4_t sumB = vdupq_n_f32(mB);
    float32x4_t mulB = vdupq_n_f32(scales[3]);

    int total_pixel = width * height;
    int i_limit = (int)(total_pixel / 8) * 8;

    int i = 0;        // Index need to be saved in later use.
    int counter = 0;  // Counter for counting padded source image index.
    int offset = prepared.step - width * 4;
    for (i = 0; i < i_limit;) {
      uint8x8x4_t src = vld4_u8(pSrc);
      GENERATE_NEON_SPLITBYCHANNEL(A, src.val[0], sumA, mulA, pA);
      GENERATE_NEON_SPLITBYCHANNEL(R, src.val[1], sumR, mulR, pR);
      GENERATE_NEON_SPLITBYCHANNEL(G, src.val[2], sumG, mulG, pG);
      GENERATE_NEON_SPLITBYCHANNEL(B, src.val[3], sumB, mulB, pB);

      // TODO: FIXME: Need optimize
      int aa = width - counter;
      if (aa <= 8 && is_padded) {
        counter = 0;
        pA += aa;
        pR += aa;
        pG += aa;
        pB += aa;
        i += aa;
        pSrc += aa * 4 + offset;
      } else {
        counter += 8;
        pA += 8;
        pR += 8;
        pG += 8;
        pB += 8;
        i += 8;
        pSrc += 32;
      }
    }
    for (; i < total_pixel; i++) {
      *pA = cv::saturate_cast<char>((float)pSrc[0] * scales[0] + mA);
      *pR = cv::saturate_cast<char>((float)pSrc[1] * scales[1] + mR);
      *pG = cv::saturate_cast<char>((float)pSrc[2] * scales[2] + mG);
      *pB = cv::saturate_cast<char>((float)pSrc[3] * scales[3] + mB);
      pA++;
      pR++;
      pG++;
      pB++;
      pSrc += 4;
    }
  } else {
    std::cout << "Currently the Network SDK does not support " << prepared.channels()
              << " channels input with NEON." << std::endl;
    exit(-1);
  }
#endif
}

void AverageAndSplitToF32(cv::Mat &prepared, vector<cv::Mat> &channels, float MeanR, float MeanG,
                          float MeanB, int width, int height) {
#ifndef __ARM_ARCH
  prepared.convertTo(prepared, CV_32FC3);

  cv::split(prepared, channels);
  channels[0].convertTo(channels[0], CV_32FC1, 1.0, MeanR);
  channels[1].convertTo(channels[1], CV_32FC1, 1.0, MeanG);
  channels[2].convertTo(channels[2], CV_32FC1, 1.0, MeanB);

#else
  unsigned char *pSrc = (unsigned char *)prepared.data;
  float *pR = (float *)channels[0].data;
  float *pG = (float *)channels[1].data;
  float *pB = (float *)channels[2].data;

  float32x4_t sumR = vdupq_n_f32(MeanR);
  float32x4_t sumG = vdupq_n_f32(MeanG);
  float32x4_t sumB = vdupq_n_f32(MeanB);

  for (int i = 0; i < width * height; i += 8) {
    uint8x8x3_t src = vld3_u8(pSrc);

    uint16x8_t Ru16 = vmovl_u8(src.val[0]);
    uint16x8_t Gu16 = vmovl_u8(src.val[1]);
    uint16x8_t Bu16 = vmovl_u8(src.val[2]);

    uint32x4_t Rlow = vmovl_u16(vget_low_u16(Ru16));
    uint32x4_t Rhigh = vmovl_u16(vget_high_u16(Ru16));

    uint32x4_t Glow = vmovl_u16(vget_low_u16(Gu16));
    uint32x4_t Ghigh = vmovl_u16(vget_high_u16(Gu16));

    uint32x4_t Blow = vmovl_u16(vget_low_u16(Bu16));
    uint32x4_t Bhigh = vmovl_u16(vget_high_u16(Bu16));

    float32x4_t Rf32_low = vcvtq_f32_u32(Rlow);
    float32x4_t Rf32_high = vcvtq_f32_u32(Rhigh);

    float32x4_t Gf32_low = vcvtq_f32_u32(Glow);
    float32x4_t Gf32_high = vcvtq_f32_u32(Ghigh);

    float32x4_t Bf32_low = vcvtq_f32_u32(Blow);
    float32x4_t Bf32_high = vcvtq_f32_u32(Bhigh);

    float32x4_t R0 = vaddq_f32(Rf32_low, sumR);
    float32x4_t R1 = vaddq_f32(Rf32_high, sumR);
    vst1q_f32(pR, R0);
    vst1q_f32(pR + 4, R1);

    float32x4_t G0 = vaddq_f32(Gf32_low, sumG);
    float32x4_t G1 = vaddq_f32(Gf32_high, sumG);
    vst1q_f32(pG, G0);
    vst1q_f32(pG + 4, G1);

    float32x4_t B0 = vaddq_f32(Bf32_low, sumB);
    float32x4_t B1 = vaddq_f32(Bf32_high, sumB);
    vst1q_f32(pB, B0);
    vst1q_f32(pB + 4, B1);

    pSrc += 24;
    pR += 8;
    pG += 8;
    pB += 8;
  }
#endif
}

void NormalizeAndSplitToF32(cv::Mat &prepared, vector<cv::Mat> &channels, float MeanR, float alphaR,
                            float MeanG, float alphaG, float MeanB, float alphaB, int width,
                            int height) {
#ifndef __ARM_ARCH
  cv::Mat crop_float;
  prepared.convertTo(crop_float, CV_32FC3, 0.0078125, -127.5 * 0.0078125);
  cv::split(crop_float, channels);
#else
  float mR = MeanR * alphaR;
  float mG = MeanG * alphaG;
  float mB = MeanB * alphaB;

  float32x4_t sumR = vdupq_n_f32(mR);
  float32x4_t mulR = vdupq_n_f32(alphaR);

  float32x4_t sumG = vdupq_n_f32(mG);
  float32x4_t mulG = vdupq_n_f32(alphaG);

  float32x4_t sumB = vdupq_n_f32(mB);
  float32x4_t mulB = vdupq_n_f32(alphaB);

  unsigned char *pSrc = (unsigned char *)prepared.data;
  float *pR = (float *)channels[0].data;
  float *pG = (float *)channels[1].data;
  float *pB = (float *)channels[2].data;

  for (int i = 0; i < width * height; i += 8) {
    uint8x8x3_t src = vld3_u8(pSrc);

    uint16x8_t Ru16 = vmovl_u8(src.val[0]);
    uint16x8_t Gu16 = vmovl_u8(src.val[1]);
    uint16x8_t Bu16 = vmovl_u8(src.val[2]);

    uint32x4_t Rlow = vmovl_u16(vget_low_u16(Ru16));
    uint32x4_t Rhigh = vmovl_u16(vget_high_u16(Ru16));

    uint32x4_t Glow = vmovl_u16(vget_low_u16(Gu16));
    uint32x4_t Ghigh = vmovl_u16(vget_high_u16(Gu16));

    uint32x4_t Blow = vmovl_u16(vget_low_u16(Bu16));
    uint32x4_t Bhigh = vmovl_u16(vget_high_u16(Bu16));

    float32x4_t Rf32_low = vcvtq_f32_u32(Rlow);
    float32x4_t Rf32_high = vcvtq_f32_u32(Rhigh);

    float32x4_t Gf32_low = vcvtq_f32_u32(Glow);
    float32x4_t Gf32_high = vcvtq_f32_u32(Ghigh);

    float32x4_t Bf32_low = vcvtq_f32_u32(Blow);
    float32x4_t Bf32_high = vcvtq_f32_u32(Bhigh);

    float32x4_t R0 = vfmaq_f32(sumR, Rf32_low, mulR);
    float32x4_t R1 = vfmaq_f32(sumR, Rf32_high, mulR);
    vst1q_f32(pR, R0);
    vst1q_f32(pR + 4, R1);

    float32x4_t G0 = vfmaq_f32(sumG, Gf32_low, mulG);
    float32x4_t G1 = vfmaq_f32(sumG, Gf32_high, mulG);
    vst1q_f32(pG, G0);
    vst1q_f32(pG + 4, G1);

    float32x4_t B0 = vfmaq_f32(sumB, Bf32_low, mulB);
    float32x4_t B1 = vfmaq_f32(sumB, Bf32_high, mulB);
    vst1q_f32(pB, B0);
    vst1q_f32(pB + 4, B1);

    pSrc += 24;
    pR += 8;
    pG += 8;
    pB += 8;
  }
#endif
}

}  // namespace cvitdl
