#include "imgwarp.hpp"

// #include "arm_neon.h"

#define EXT_FUNCTION 0

// #define DEBUG_IMGWARP

/************** interpolation formulas and tables ***************/
// L131
const int INTER_RESIZE_COEF_BITS = 11;
const int INTER_RESIZE_COEF_SCALE = 1 << INTER_RESIZE_COEF_BITS;

const int INTER_REMAP_COEF_BITS = 15;
const int INTER_REMAP_COEF_SCALE = 1 << INTER_REMAP_COEF_BITS;

static uchar NNDeltaTab_i[INTER_TAB_SIZE2][2];

static float BilinearTab_f[INTER_TAB_SIZE2][2][2];
static short BilinearTab_i[INTER_TAB_SIZE2][2][2];

#if 1
static short BilinearTab_iC4_buf[INTER_TAB_SIZE2 + 2][2][8];
static short (*BilinearTab_iC4)[2][8] = (short (*)[2][8])cv::alignPtr(BilinearTab_iC4_buf, 16);
#endif

#if EXT_FUNCTION
static float BicubicTab_f[INTER_TAB_SIZE2][4][4];
static short BicubicTab_i[INTER_TAB_SIZE2][4][4];

static float Lanczos4Tab_f[INTER_TAB_SIZE2][8][8];
static short Lanczos4Tab_i[INTER_TAB_SIZE2][8][8];
#endif

// L153
static inline void interpolateLinear(float x, float* coeffs) {
  coeffs[0] = 1.f - x;
  coeffs[1] = x;
}

#if EXT_FUNCTION
// L159
static inline void interpolateCubic(float x, float* coeffs) {
  const float A = -0.75f;

  coeffs[0] = ((A * (x + 1) - 5 * A) * (x + 1) + 8 * A) * (x + 1) - 4 * A;
  coeffs[1] = ((A + 2) * x - (A + 3)) * x * x + 1;
  coeffs[2] = ((A + 2) * (1 - x) - (A + 3)) * (1 - x) * (1 - x) + 1;
  coeffs[3] = 1.f - coeffs[0] - coeffs[1] - coeffs[2];
}

// L169
static inline void interpolateLanczos4(float x, float* coeffs) {
  static const double s45 = 0.70710678118654752440084436210485;
  static const double cs[][2] = {{1, 0},  {-s45, -s45}, {0, 1},  {s45, -s45},
                                 {-1, 0}, {s45, s45},   {0, -1}, {-s45, s45}};

  if (x < FLT_EPSILON) {
    for (int i = 0; i < 8; i++) coeffs[i] = 0;
    coeffs[3] = 1;
    return;
  }

  float sum = 0;
  double y0 = -(x + 3) * CV_PI * 0.25, s0 = sin(y0), c0 = cos(y0);
  for (int i = 0; i < 8; i++) {
    double y = -(x + 3 - i) * CV_PI * 0.25;
    coeffs[i] = (float)((cs[i][0] * s0 + cs[i][1] * c0) / (y * y));
    sum += coeffs[i];
  }

  sum = 1.f / sum;
  for (int i = 0; i < 8; i++) coeffs[i] *= sum;
}
#endif

// L197
static void initInterTab1D(int method, float* tab, int tabsz) {
  float scale = 1.f / tabsz;
  if (method == INTER_LINEAR) {
    for (int i = 0; i < tabsz; i++, tab += 2) interpolateLinear(i * scale, tab);
  } else if (method == INTER_CUBIC) {
    CV_Assert(0);
  } else if (method == INTER_LANCZOS4) {
    CV_Assert(0);
  } else
    CV_Error(CV_StsBadArg, "Unknown interpolation method");
}

// L220
static const void* initInterTab2D(int method, bool fixpt) {
  static bool inittab[INTER_MAX + 1] = {false};
  float* tab = 0;
  short* itab = 0;
  int ksize = 0;
  if (method == INTER_LINEAR) {
    tab = BilinearTab_f[0][0], itab = BilinearTab_i[0][0], ksize = 2;
  } else if (method == INTER_CUBIC) {
    CV_Assert(0);
  } else if (method == INTER_LANCZOS4) {
    CV_Assert(0);
  } else
    CV_Error(CV_StsBadArg, "Unknown/unsupported interpolation type");

  if (!inittab[method]) {
    cv::AutoBuffer<float> _tab(8 * INTER_TAB_SIZE);
    int i, j, k1, k2;
    initInterTab1D(method, _tab, INTER_TAB_SIZE);
    for (i = 0; i < INTER_TAB_SIZE; i++)
      for (j = 0; j < INTER_TAB_SIZE; j++, tab += ksize * ksize, itab += ksize * ksize) {
        int isum = 0;
        NNDeltaTab_i[i * INTER_TAB_SIZE + j][0] = j < INTER_TAB_SIZE / 2;
        NNDeltaTab_i[i * INTER_TAB_SIZE + j][1] = i < INTER_TAB_SIZE / 2;

        for (k1 = 0; k1 < ksize; k1++) {
          float vy = _tab[i * ksize + k1];
          for (k2 = 0; k2 < ksize; k2++) {
            float v = vy * _tab[j * ksize + k2];
            tab[k1 * ksize + k2] = v;
            isum += itab[k1 * ksize + k2] = cv::saturate_cast<short>(v * INTER_REMAP_COEF_SCALE);
          }
        }

        if (isum != INTER_REMAP_COEF_SCALE) {
          int diff = isum - INTER_REMAP_COEF_SCALE;
          int ksize2 = ksize / 2, Mk1 = ksize2, Mk2 = ksize2, mk1 = ksize2, mk2 = ksize2;
          for (k1 = ksize2; k1 < ksize2 + 2; k1++)
            for (k2 = ksize2; k2 < ksize2 + 2; k2++) {
              if (itab[k1 * ksize + k2] < itab[mk1 * ksize + mk2])
                mk1 = k1, mk2 = k2;
              else if (itab[k1 * ksize + k2] > itab[Mk1 * ksize + Mk2])
                Mk1 = k1, Mk2 = k2;
            }
          if (diff < 0)
            itab[Mk1 * ksize + Mk2] = (short)(itab[Mk1 * ksize + Mk2] - diff);
          else
            itab[mk1 * ksize + mk2] = (short)(itab[mk1 * ksize + mk2] - diff);
        }
      }
    tab -= INTER_TAB_SIZE2 * ksize * ksize;
    itab -= INTER_TAB_SIZE2 * ksize * ksize;
#if CV_NEON
    if (method == INTER_LINEAR) {
      for (i = 0; i < INTER_TAB_SIZE2; i++)
        for (j = 0; j < 4; j++) {
          BilinearTab_iC4[i][0][j * 2] = BilinearTab_i[i][0][0];
          BilinearTab_iC4[i][0][j * 2 + 1] = BilinearTab_i[i][0][1];
          BilinearTab_iC4[i][1][j * 2] = BilinearTab_i[i][1][0];
          BilinearTab_iC4[i][1][j * 2 + 1] = BilinearTab_i[i][1][1];
        }
    }
#endif
    inittab[method] = true;
  }
  return fixpt ? (const void*)itab : (const void*)tab;
}

// L310
template <typename ST, typename DT>
struct Cast {
  typedef ST type1;
  typedef DT rtype;

  DT operator()(ST val) const { return cv::saturate_cast<DT>(val); }
};

template <typename ST, typename DT, int bits>
struct FixedPtCast {
  typedef ST type1;
  typedef DT rtype;
  enum { SHIFT = bits, DELTA = 1 << (bits - 1) };

  DT operator()(ST val) const { return cv::saturate_cast<DT>((val + DELTA) >> SHIFT); }
};

/****************************************************************************************\
*                                         Resize                                         *
\****************************************************************************************/

// L450
struct VResizeNoVec {
  int operator()(const uchar**, uchar*, const uchar*, int) const { return 0; }
};

struct HResizeNoVec {
  int operator()(const uchar**, uchar**, int, const int*, const uchar*, int, int, int, int,
                 int) const {
    return 0;
  }
};

#if CV_NEON
// L1039
struct VResizeLinearVec_32s8u {
  int operator()(const uchar** _src, uchar* dst, const uchar* _beta, int width) const {
    const int **src = (const int**)_src, *S0 = src[0], *S1 = src[1];
    const short* beta = (const short*)_beta;
    int x = 0;
    int16x8_t v_b0 = vdupq_n_s16(beta[0]), v_b1 = vdupq_n_s16(beta[1]), v_delta = vdupq_n_s16(2);

    for (; x <= width - 16; x += 16) {
      int32x4_t v_src00 = vshrq_n_s32(vld1q_s32(S0 + x), 4),
                v_src10 = vshrq_n_s32(vld1q_s32(S1 + x), 4);
      int32x4_t v_src01 = vshrq_n_s32(vld1q_s32(S0 + x + 4), 4),
                v_src11 = vshrq_n_s32(vld1q_s32(S1 + x + 4), 4);

      int16x8_t v_src0 = vcombine_s16(vmovn_s32(v_src00), vmovn_s32(v_src01));
      int16x8_t v_src1 = vcombine_s16(vmovn_s32(v_src10), vmovn_s32(v_src11));

      int16x8_t v_dst0 = vaddq_s16(vshrq_n_s16(vqdmulhq_s16(v_src0, v_b0), 1),
                                   vshrq_n_s16(vqdmulhq_s16(v_src1, v_b1), 1));
      v_dst0 = vshrq_n_s16(vaddq_s16(v_dst0, v_delta), 2);

      v_src00 = vshrq_n_s32(vld1q_s32(S0 + x + 8), 4);
      v_src10 = vshrq_n_s32(vld1q_s32(S1 + x + 8), 4);
      v_src01 = vshrq_n_s32(vld1q_s32(S0 + x + 12), 4);
      v_src11 = vshrq_n_s32(vld1q_s32(S1 + x + 12), 4);

      v_src0 = vcombine_s16(vmovn_s32(v_src00), vmovn_s32(v_src01));
      v_src1 = vcombine_s16(vmovn_s32(v_src10), vmovn_s32(v_src11));

      int16x8_t v_dst1 = vaddq_s16(vshrq_n_s16(vqdmulhq_s16(v_src0, v_b0), 1),
                                   vshrq_n_s16(vqdmulhq_s16(v_src1, v_b1), 1));
      v_dst1 = vshrq_n_s16(vaddq_s16(v_dst1, v_delta), 2);

      vst1q_u8(dst + x, vcombine_u8(vqmovun_s16(v_dst0), vqmovun_s16(v_dst1)));
    }

    return x;
  }
};

struct VResizeLinearVec_32f16u {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1];
    ushort* dst = (ushort*)_dst;
    int x = 0;

    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_src00 = vld1q_f32(S0 + x), v_src01 = vld1q_f32(S0 + x + 4);
      float32x4_t v_src10 = vld1q_f32(S1 + x), v_src11 = vld1q_f32(S1 + x + 4);

      float32x4_t v_dst0 = vmlaq_f32(vmulq_f32(v_src00, v_b0), v_src10, v_b1);
      float32x4_t v_dst1 = vmlaq_f32(vmulq_f32(v_src01, v_b0), v_src11, v_b1);

      vst1q_u16(dst + x, vcombine_u16(vqmovn_u32(cv_vrndq_u32_f32(v_dst0)),
                                      vqmovn_u32(cv_vrndq_u32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeLinearVec_32f16s {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1];
    short* dst = (short*)_dst;
    int x = 0;

    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_src00 = vld1q_f32(S0 + x), v_src01 = vld1q_f32(S0 + x + 4);
      float32x4_t v_src10 = vld1q_f32(S1 + x), v_src11 = vld1q_f32(S1 + x + 4);

      float32x4_t v_dst0 = vmlaq_f32(vmulq_f32(v_src00, v_b0), v_src10, v_b1);
      float32x4_t v_dst1 = vmlaq_f32(vmulq_f32(v_src01, v_b0), v_src11, v_b1);

      vst1q_s16(dst + x, vcombine_s16(vqmovn_s32(cv_vrndq_s32_f32(v_dst0)),
                                      vqmovn_s32(cv_vrndq_s32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeLinearVec_32f {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1];
    float* dst = (float*)_dst;
    int x = 0;

    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_src00 = vld1q_f32(S0 + x), v_src01 = vld1q_f32(S0 + x + 4);
      float32x4_t v_src10 = vld1q_f32(S1 + x), v_src11 = vld1q_f32(S1 + x + 4);

      vst1q_f32(dst + x, vmlaq_f32(vmulq_f32(v_src00, v_b0), v_src10, v_b1));
      vst1q_f32(dst + x + 4, vmlaq_f32(vmulq_f32(v_src01, v_b0), v_src11, v_b1));
    }

    return x;
  }
};

typedef VResizeNoVec VResizeCubicVec_32s8u;

struct VResizeCubicVec_32f16u {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3];
    ushort* dst = (ushort*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1, vld1q_f32(S1 + x)), v_b2,
                    vld1q_f32(S2 + x)),
          v_b3, vld1q_f32(S3 + x));
      float32x4_t v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x + 4)), v_b1, vld1q_f32(S1 + x + 4)),
                    v_b2, vld1q_f32(S2 + x + 4)),
          v_b3, vld1q_f32(S3 + x + 4));

      vst1q_u16(dst + x, vcombine_u16(vqmovn_u32(cv_vrndq_u32_f32(v_dst0)),
                                      vqmovn_u32(cv_vrndq_u32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeCubicVec_32f16s {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3];
    short* dst = (short*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1, vld1q_f32(S1 + x)), v_b2,
                    vld1q_f32(S2 + x)),
          v_b3, vld1q_f32(S3 + x));
      float32x4_t v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x + 4)), v_b1, vld1q_f32(S1 + x + 4)),
                    v_b2, vld1q_f32(S2 + x + 4)),
          v_b3, vld1q_f32(S3 + x + 4));

      vst1q_s16(dst + x, vcombine_s16(vqmovn_s32(cv_vrndq_s32_f32(v_dst0)),
                                      vqmovn_s32(cv_vrndq_s32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeCubicVec_32f {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3];
    float* dst = (float*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]);

    for (; x <= width - 8; x += 8) {
      vst1q_f32(dst + x, vmlaq_f32(vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1,
                                                       vld1q_f32(S1 + x)),
                                             v_b2, vld1q_f32(S2 + x)),
                                   v_b3, vld1q_f32(S3 + x)));
      vst1q_f32(dst + x + 4, vmlaq_f32(vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x + 4)),
                                                           v_b1, vld1q_f32(S1 + x + 4)),
                                                 v_b2, vld1q_f32(S2 + x + 4)),
                                       v_b3, vld1q_f32(S3 + x + 4)));
    }

    return x;
  }
};

struct VResizeLanczos4Vec_32f16u {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3], *S4 = src[4], *S5 = src[5],
                *S6 = src[6], *S7 = src[7];
    ushort* dst = (ushort*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]),
                v_b4 = vdupq_n_f32(beta[4]), v_b5 = vdupq_n_f32(beta[5]),
                v_b6 = vdupq_n_f32(beta[6]), v_b7 = vdupq_n_f32(beta[7]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1, vld1q_f32(S1 + x)), v_b2,
                    vld1q_f32(S2 + x)),
          v_b3, vld1q_f32(S3 + x));
      float32x4_t v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b4, vld1q_f32(S4 + x)), v_b5, vld1q_f32(S5 + x)), v_b6,
                    vld1q_f32(S6 + x)),
          v_b7, vld1q_f32(S7 + x));
      float32x4_t v_dst = vaddq_f32(v_dst0, v_dst1);

      v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x + 4)), v_b1, vld1q_f32(S1 + x + 4)),
                    v_b2, vld1q_f32(S2 + x + 4)),
          v_b3, vld1q_f32(S3 + x + 4));
      v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b4, vld1q_f32(S4 + x + 4)), v_b5, vld1q_f32(S5 + x + 4)),
                    v_b6, vld1q_f32(S6 + x + 4)),
          v_b7, vld1q_f32(S7 + x + 4));
      v_dst1 = vaddq_f32(v_dst0, v_dst1);

      vst1q_u16(dst + x, vcombine_u16(vqmovn_u32(cv_vrndq_u32_f32(v_dst)),
                                      vqmovn_u32(cv_vrndq_u32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeLanczos4Vec_32f16s {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3], *S4 = src[4], *S5 = src[5],
                *S6 = src[6], *S7 = src[7];
    short* dst = (short*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]),
                v_b4 = vdupq_n_f32(beta[4]), v_b5 = vdupq_n_f32(beta[5]),
                v_b6 = vdupq_n_f32(beta[6]), v_b7 = vdupq_n_f32(beta[7]);

    for (; x <= width - 8; x += 8) {
      float32x4_t v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1, vld1q_f32(S1 + x)), v_b2,
                    vld1q_f32(S2 + x)),
          v_b3, vld1q_f32(S3 + x));
      float32x4_t v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b4, vld1q_f32(S4 + x)), v_b5, vld1q_f32(S5 + x)), v_b6,
                    vld1q_f32(S6 + x)),
          v_b7, vld1q_f32(S7 + x));
      float32x4_t v_dst = vaddq_f32(v_dst0, v_dst1);

      v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x + 4)), v_b1, vld1q_f32(S1 + x + 4)),
                    v_b2, vld1q_f32(S2 + x + 4)),
          v_b3, vld1q_f32(S3 + x + 4));
      v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b4, vld1q_f32(S4 + x + 4)), v_b5, vld1q_f32(S5 + x + 4)),
                    v_b6, vld1q_f32(S6 + x + 4)),
          v_b7, vld1q_f32(S7 + x + 4));
      v_dst1 = vaddq_f32(v_dst0, v_dst1);

      vst1q_s16(dst + x, vcombine_s16(vqmovn_s32(cv_vrndq_s32_f32(v_dst)),
                                      vqmovn_s32(cv_vrndq_s32_f32(v_dst1))));
    }

    return x;
  }
};

struct VResizeLanczos4Vec_32f {
  int operator()(const uchar** _src, uchar* _dst, const uchar* _beta, int width) const {
    const float** src = (const float**)_src;
    const float* beta = (const float*)_beta;
    const float *S0 = src[0], *S1 = src[1], *S2 = src[2], *S3 = src[3], *S4 = src[4], *S5 = src[5],
                *S6 = src[6], *S7 = src[7];
    float* dst = (float*)_dst;
    int x = 0;
    float32x4_t v_b0 = vdupq_n_f32(beta[0]), v_b1 = vdupq_n_f32(beta[1]),
                v_b2 = vdupq_n_f32(beta[2]), v_b3 = vdupq_n_f32(beta[3]),
                v_b4 = vdupq_n_f32(beta[4]), v_b5 = vdupq_n_f32(beta[5]),
                v_b6 = vdupq_n_f32(beta[6]), v_b7 = vdupq_n_f32(beta[7]);

    for (; x <= width - 4; x += 4) {
      float32x4_t v_dst0 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b0, vld1q_f32(S0 + x)), v_b1, vld1q_f32(S1 + x)), v_b2,
                    vld1q_f32(S2 + x)),
          v_b3, vld1q_f32(S3 + x));
      float32x4_t v_dst1 = vmlaq_f32(
          vmlaq_f32(vmlaq_f32(vmulq_f32(v_b4, vld1q_f32(S4 + x)), v_b5, vld1q_f32(S5 + x)), v_b6,
                    vld1q_f32(S6 + x)),
          v_b7, vld1q_f32(S7 + x));
      vst1q_f32(dst + x, vaddq_f32(v_dst0, v_dst1));
    }

    return x;
  }
};
#else
// L1376
typedef VResizeNoVec VResizeLinearVec_32s8u;
typedef VResizeNoVec VResizeLinearVec_32f16u;
typedef VResizeNoVec VResizeLinearVec_32f16s;
typedef VResizeNoVec VResizeLinearVec_32f;

typedef VResizeNoVec VResizeCubicVec_32s8u;
typedef VResizeNoVec VResizeCubicVec_32f16u;
typedef VResizeNoVec VResizeCubicVec_32f16s;
typedef VResizeNoVec VResizeCubicVec_32f;

typedef VResizeNoVec VResizeLanczos4Vec_32f16u;
typedef VResizeNoVec VResizeLanczos4Vec_32f16s;
typedef VResizeNoVec VResizeLanczos4Vec_32f;
#endif

typedef HResizeNoVec HResizeLinearVec_8u32s;
typedef HResizeNoVec HResizeLinearVec_16u32f;
typedef HResizeNoVec HResizeLinearVec_16s32f;
typedef HResizeNoVec HResizeLinearVec_32f;
typedef HResizeNoVec HResizeLinearVec_64f;

// L1399
template <typename T, typename WT, typename AT, int ONE, class VecOp>
struct HResizeLinear {
  typedef T value_type;
  typedef WT buf_type;
  typedef AT alpha_type;

  void operator()(const T** src, WT** dst, int count, const int* xofs, const AT* alpha, int swidth,
                  int dwidth, int cn, int xmin, int xmax) const {
    int dx, k;
    VecOp vecOp;

    int dx0 = vecOp((const uchar**)src, (uchar**)dst, count, xofs, (const uchar*)alpha, swidth,
                    dwidth, cn, xmin, xmax);

    for (k = 0; k <= count - 2; k++) {
      const T *S0 = src[k], *S1 = src[k + 1];
      WT *D0 = dst[k], *D1 = dst[k + 1];
      for (dx = dx0; dx < xmax; dx++) {
        int sx = xofs[dx];
        WT a0 = alpha[dx * 2], a1 = alpha[dx * 2 + 1];
        WT t0 = S0[sx] * a0 + S0[sx + cn] * a1;
        WT t1 = S1[sx] * a0 + S1[sx + cn] * a1;
        D0[dx] = t0;
        D1[dx] = t1;
      }

      for (; dx < dwidth; dx++) {
        int sx = xofs[dx];
        D0[dx] = WT(S0[sx] * ONE);
        D1[dx] = WT(S1[sx] * ONE);
      }
    }

    for (; k < count; k++) {
      const T* S = src[k];
      WT* D = dst[k];
      for (dx = 0; dx < xmax; dx++) {
        int sx = xofs[dx];
        D[dx] = S[sx] * alpha[dx * 2] + S[sx + cn] * alpha[dx * 2 + 1];
      }

      for (; dx < dwidth; dx++) D[dx] = WT(S[xofs[dx]] * ONE);
    }
  }
};

// L1453
template <typename T, typename WT, typename AT, class CastOp, class VecOp>
struct VResizeLinear {
  typedef T value_type;
  typedef WT buf_type;
  typedef AT alpha_type;

  void operator()(const WT** src, T* dst, const AT* beta, int width) const {
    WT b0 = beta[0], b1 = beta[1];
    const WT *S0 = src[0], *S1 = src[1];
    CastOp castOp;
    VecOp vecOp;

    int x = vecOp((const uchar**)src, (uchar*)dst, (const uchar*)beta, width);
#if CV_ENABLE_UNROLLED
    for (; x <= width - 4; x += 4) {
      WT t0, t1;
      t0 = S0[x] * b0 + S1[x] * b1;
      t1 = S0[x + 1] * b0 + S1[x + 1] * b1;
      dst[x] = castOp(t0);
      dst[x + 1] = castOp(t1);
      t0 = S0[x + 2] * b0 + S1[x + 2] * b1;
      t1 = S0[x + 3] * b0 + S1[x + 3] * b1;
      dst[x + 2] = castOp(t0);
      dst[x + 3] = castOp(t1);
    }
#endif
    for (; x < width; x++) dst[x] = castOp(S0[x] * b0 + S1[x] * b1);
  }
};

// L1680
static inline int clip(int x, int a, int b) { return x >= a ? (x < b ? x : b - 1) : a; }

static const int MAX_ESIZE = 16;

// L1688
template <typename HResize, typename VResize>
class resizeGeneric_Invoker : public cv::ParallelLoopBody {
 public:
  typedef typename HResize::value_type T;
  typedef typename HResize::buf_type WT;
  typedef typename HResize::alpha_type AT;

  resizeGeneric_Invoker(const cv::Mat& _src, cv::Mat& _dst, const int* _xofs, const int* _yofs,
                        const AT* _alpha, const AT* __beta, const cv::Size& _ssize,
                        const cv::Size& _dsize, int _ksize, int _xmin, int _xmax)
      : ParallelLoopBody(),
        src(_src),
        dst(_dst),
        xofs(_xofs),
        yofs(_yofs),
        alpha(_alpha),
        _beta(__beta),
        ssize(_ssize),
        dsize(_dsize),
        ksize(_ksize),
        xmin(_xmin),
        xmax(_xmax) {
    CV_Assert(ksize <= MAX_ESIZE);
  }

#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
  virtual void operator()(const cv::Range& range) const {
    // printf("cvitdl::resizeGeneric_Invoker op()\n");
    int dy, cn = src.channels();
    HResize hresize;
    VResize vresize;

    int bufstep = (int)cv::alignSize(dsize.width, 16);
    cv::AutoBuffer<WT> _buffer(bufstep * ksize);
    const T* srows[MAX_ESIZE] = {0};
    WT* rows[MAX_ESIZE] = {0};
    int prev_sy[MAX_ESIZE];

    for (int k = 0; k < ksize; k++) {
      prev_sy[k] = -1;
      rows[k] = (WT*)_buffer + bufstep * k;
    }

    const AT* beta = _beta + ksize * range.start;

    for (dy = range.start; dy < range.end; dy++, beta += ksize) {
      int sy0 = yofs[dy], k0 = ksize, k1 = 0, ksize2 = ksize / 2;

      for (int k = 0; k < ksize; k++) {
        int sy = clip(sy0 - ksize2 + 1 + k, 0, ssize.height);
        for (k1 = std::max(k1, k); k1 < ksize; k1++) {
          if (sy == prev_sy[k1])  // if the sy-th row has been computed already, reuse it.
          {
            if (k1 > k) memcpy(rows[k], rows[k1], bufstep * sizeof(rows[0][0]));
            break;
          }
        }
        if (k1 == ksize) k0 = std::min(k0, k);  // remember the first row that needs to be computed
        srows[k] = src.template ptr<T>(sy);
        prev_sy[k] = sy;
      }

      if (k0 < ksize)
        hresize((const T**)(srows + k0), (WT**)(rows + k0), ksize - k0, xofs, (const AT*)(alpha),
                ssize.width, dsize.width, cn, xmin, xmax);
      vresize((const WT**)rows, (T*)(dst.data + dst.step * dy), beta, dsize.width);
    }
  }
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 8)
#pragma GCC diagnostic pop
#endif

 private:
  cv::Mat src;
  cv::Mat dst;
  const int *xofs, *yofs;
  const AT *alpha, *_beta;
  cv::Size ssize, dsize;
  const int ksize, xmin, xmax;

  resizeGeneric_Invoker& operator=(const resizeGeneric_Invoker&);
};

// L1774
template <class HResize, class VResize>
static void resizeGeneric_(const cv::Mat& src, cv::Mat& dst, const int* xofs, const void* _alpha,
                           const int* yofs, const void* _beta, int xmin, int xmax, int ksize) {
  // printf("cvitdl::resizeGeneric_\n");
  typedef typename HResize::alpha_type AT;

  const AT* beta = (const AT*)_beta;
  cv::Size ssize = src.size(), dsize = dst.size();
  int cn = src.channels();
  ssize.width *= cn;
  dsize.width *= cn;
  xmin *= cn;
  xmax *= cn;
  // image resize is a separable operation. In case of not too strong

  cv::Range range(0, dsize.height);
  resizeGeneric_Invoker<HResize, VResize> invoker(src, dst, xofs, yofs, (const AT*)_alpha, beta,
                                                  ssize, dsize, ksize, xmin, xmax);
  cv::parallel_for_(range, invoker, dst.total() / (double)(1 << 16));
}

// L2662
typedef void (*ResizeFunc)(const cv::Mat& src, cv::Mat& dst, const int* xofs, const void* alpha,
                           const int* yofs, const void* beta, int xmin, int xmax, int ksize);

//==================================================================================================

// L3129
void _resize(int src_type, const uchar* src_data, size_t src_step, int src_width, int src_height,
             uchar* dst_data, size_t dst_step, int dst_width, int dst_height, double inv_scale_x,
             double inv_scale_y, int interpolation) {
  // printf("cvitdl::resize(2)\n");
  /* NOTE: interpolation = 1 */

  CV_Assert((dst_width * dst_height > 0) || (inv_scale_x > 0 && inv_scale_y > 0));
  if (inv_scale_x < DBL_EPSILON || inv_scale_y < DBL_EPSILON) {
    inv_scale_x = static_cast<double>(dst_width) / src_width;
    inv_scale_y = static_cast<double>(dst_height) / src_height;
  }

  static ResizeFunc linear_tab[] = {
      resizeGeneric_<
          HResizeLinear<uchar, int, short, INTER_RESIZE_COEF_SCALE, HResizeLinearVec_8u32s>,
          VResizeLinear<uchar, int, short, FixedPtCast<int, uchar, INTER_RESIZE_COEF_BITS * 2>,
                        VResizeLinearVec_32s8u> >,
      0,
      0,
      0,
      0,
      0,
      0,
      0};

#if 0
    static ResizeFunc cubic_tab[] = {};

    static ResizeFunc lanczos4_tab[] = {};

    static ResizeAreaFastFunc areafast_tab[] = {};

    static ResizeAreaFunc area_tab[] = {};
#endif

  int depth = CV_MAT_DEPTH(src_type), cn = CV_MAT_CN(src_type); /* NOTE: depth = 0, cn = 3 */
  double scale_x = 1. / inv_scale_x, scale_y = 1. / inv_scale_y;

  int iscale_x = cv::saturate_cast<int>(scale_x);
  int iscale_y = cv::saturate_cast<int>(scale_y);

  bool is_area_fast =
      std::abs(scale_x - iscale_x) < DBL_EPSILON && std::abs(scale_y - iscale_y) < DBL_EPSILON;

  cv::Size dsize = cv::Size(cv::saturate_cast<int>(src_width * inv_scale_x),
                            cv::saturate_cast<int>(src_height * inv_scale_y));
  CV_Assert(dsize.area() > 0);

  cv::Mat src(cv::Size(src_width, src_height), src_type, const_cast<uchar*>(src_data), src_step);
  cv::Mat dst(dsize, src_type, dst_data, dst_step);

  if (interpolation == INTER_NEAREST) {
    CV_Assert(0);
  }

  int k, sx, sy, dx, dy;
  {
    // in case of scale_x && scale_y is equal to 2
    // INTER_AREA (fast) also is equal to INTER_LINEAR
    if (interpolation == INTER_LINEAR && is_area_fast && iscale_x == 2 && iscale_y == 2)
      interpolation = INTER_AREA;

    // true "area" interpolation is only implemented for the case (scale_x <= 1 && scale_y <= 1).
    // In other cases it is emulated using some variant of bilinear interpolation
    if (interpolation == INTER_AREA && scale_x >= 1 && scale_y >= 1) {
      CV_Assert(0);
    }
  }

  int xmin = 0, xmax = dsize.width, width = dsize.width * cn;
  bool area_mode = interpolation == INTER_AREA;
  bool fixpt = depth == CV_8U;
  float fx, fy;
  ResizeFunc func = 0;
  int ksize = 0, ksize2;
  if (interpolation == INTER_CUBIC) {
    CV_Assert(0);
  } else if (interpolation == INTER_LANCZOS4) {
    CV_Assert(0);
  } else if (interpolation == INTER_LINEAR || interpolation == INTER_AREA) {
    ksize = 2, func = linear_tab[depth];
  } else
    CV_Error(CV_StsBadArg, "Unknown interpolation method");
  ksize2 = ksize / 2;

  CV_Assert(func != 0);

  cv::AutoBuffer<uchar> _buffer((width + dsize.height) * (sizeof(int) + sizeof(float) * ksize));
  int* xofs = (int*)(uchar*)_buffer;
  int* yofs = xofs + width;
  float* alpha = (float*)(yofs + dsize.height);
  short* ialpha = (short*)alpha;
  float* beta = alpha + width * ksize;
  short* ibeta = ialpha + width * ksize;
  float cbuf[MAX_ESIZE];

  for (dx = 0; dx < dsize.width; dx++) {
    if (!area_mode) {
      fx = (float)((dx + 0.5) * scale_x - 0.5);
      sx = cvFloor(fx);
      fx -= sx;
    } else {
      sx = cvFloor(dx * scale_x);
      fx = (float)((dx + 1) - (sx + 1) * inv_scale_x);
      fx = fx <= 0 ? 0.f : fx - cvFloor(fx);
    }

    if (sx < ksize2 - 1) {
      xmin = dx + 1;
      if (sx < 0 && (interpolation != INTER_CUBIC && interpolation != INTER_LANCZOS4))
        fx = 0, sx = 0;
    }

    if (sx + ksize2 >= src_width) {
      xmax = std::min(xmax, dx);
      if (sx >= src_width - 1 && (interpolation != INTER_CUBIC && interpolation != INTER_LANCZOS4))
        fx = 0, sx = src_width - 1;
    }

    for (k = 0, sx *= cn; k < cn; k++) xofs[dx * cn + k] = sx + k;

    if (interpolation == INTER_CUBIC) {
      CV_Assert(0);
    } else if (interpolation == INTER_LANCZOS4) {
      CV_Assert(0);
    } else {
      cbuf[0] = 1.f - fx;
      cbuf[1] = fx;
    }
    if (fixpt) {
      for (k = 0; k < ksize; k++)
        ialpha[dx * cn * ksize + k] = cv::saturate_cast<short>(cbuf[k] * INTER_RESIZE_COEF_SCALE);
      for (; k < cn * ksize; k++) ialpha[dx * cn * ksize + k] = ialpha[dx * cn * ksize + k - ksize];
    } else {
      CV_Assert(0);
    }
  }

  for (dy = 0; dy < dsize.height; dy++) {
    if (!area_mode) {
      fy = (float)((dy + 0.5) * scale_y - 0.5);
      sy = cvFloor(fy);
      fy -= sy;
    } else {
      sy = cvFloor(dy * scale_y);
      fy = (float)((dy + 1) - (sy + 1) * inv_scale_y);
      fy = fy <= 0 ? 0.f : fy - cvFloor(fy);
    }

    yofs[dy] = sy;
    if (interpolation == INTER_CUBIC) {
      CV_Assert(0);
    } else if (interpolation == INTER_LANCZOS4) {
      CV_Assert(0);
    } else {
      cbuf[0] = 1.f - fy;
      cbuf[1] = fy;
    }

    if (fixpt) {
      for (k = 0; k < ksize; k++)
        ibeta[dy * ksize + k] = cv::saturate_cast<short>(cbuf[k] * INTER_RESIZE_COEF_SCALE);
    } else {
      CV_Assert(0);
    }
  }

  func(src, dst, xofs, fixpt ? (void*)ialpha : (void*)alpha, yofs,
       fixpt ? (void*)ibeta : (void*)beta, xmin, xmax, ksize);
}

//==================================================================================================

// 3485
void cvitdl::resize(cv::InputArray _src, cv::OutputArray _dst, cv::Size dsize, double inv_scale_x,
                    double inv_scale_y, int interpolation) {
  // printf("cvitdl::resize(1)\n");
  cv::Size ssize = _src.size();

  CV_Assert(ssize.width > 0 && ssize.height > 0);
  CV_Assert(dsize.area() > 0 || (inv_scale_x > 0 && inv_scale_y > 0));
  if (dsize.area() == 0) {
    dsize = cv::Size(cv::saturate_cast<int>(ssize.width * inv_scale_x),
                     cv::saturate_cast<int>(ssize.height * inv_scale_y));
    CV_Assert(dsize.area() > 0);
  } else {
    inv_scale_x = (double)dsize.width / ssize.width;
    inv_scale_y = (double)dsize.height / ssize.height;
  }

  cv::Mat src = _src.getMat();
  _dst.create(dsize, src.type());
  cv::Mat dst = _dst.getMat();

  if (dsize == ssize) {
    // Source and destination are of same size. Use simple copy.
    src.copyTo(dst);
    return;
  }

  _resize(src.type(), src.data, src.step, src.cols, src.rows, dst.data, dst.step, dst.cols,
          dst.rows, inv_scale_x, inv_scale_y, interpolation);
}

/****************************************************************************************\
*                       General warping (affine, perspective, remap)                     *
\****************************************************************************************/

// L3531
template <typename T>
static void remapNearest(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat& _xy, int borderType,
                         const cv::Scalar& _borderValue) {
  cv::Size ssize = _src.size(), dsize = _dst.size();
  int cn = _src.channels();
  // printf("[remapNearest] cn = %d\n", cn);
  const T* S0 = _src.ptr<T>();
  size_t sstep = _src.step / sizeof(S0[0]);
  cv::Scalar_<T> cval(cv::saturate_cast<T>(_borderValue[0]), cv::saturate_cast<T>(_borderValue[1]),
                      cv::saturate_cast<T>(_borderValue[2]), cv::saturate_cast<T>(_borderValue[3]));
  int dx, dy;

  unsigned width1 = ssize.width, height1 = ssize.height;

  if (_dst.isContinuous() && _xy.isContinuous()) {
    dsize.width *= dsize.height;
    dsize.height = 1;
  }

  for (dy = 0; dy < dsize.height; dy++) {
    T* D = _dst.ptr<T>(dy);
    const short* XY = _xy.ptr<short>(dy);

    if (cn == 1) {
      CV_Assert(0);
    } else {
      for (dx = 0; dx < dsize.width; dx++, D += cn) {
        int sx = XY[dx * 2], sy = XY[dx * 2 + 1], k;
        const T* S = 0; /* NOTE: To avoid -Werror=maybe-uninitialized */
        if ((unsigned)sx < width1 && (unsigned)sy < height1) {
          if (cn == 3) {
            S = S0 + sy * sstep + sx * 3;
            D[0] = S[0], D[1] = S[1], D[2] = S[2];
          } else if (cn == 4) {
            CV_Assert(0);
          } else {
            CV_Assert(0);
          }
        } else if (borderType != cv::BORDER_TRANSPARENT) {
          if (borderType == cv::BORDER_REPLICATE) {
            CV_Assert(0);
          } else if (borderType == cv::BORDER_CONSTANT)
            S = &cval[0];
          else {
            CV_Assert(0);
          }
          for (k = 0; k < cn; k++) D[k] = S[k];
        }
      }
    }
  }
}

// L3634
struct RemapNoVec {
  int operator()(const cv::Mat&, void*, const short*, const ushort*, const void*, int) const {
    return 0;
  }
};

// L3842
typedef RemapNoVec RemapVec_8u;

// L3847
template <class CastOp, class VecOp, typename AT>
static void remapBilinear(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat& _xy,
                          const cv::Mat& _fxy, const void* _wtab, int borderType,
                          const cv::Scalar& _borderValue) {
  typedef typename CastOp::rtype T;
  typedef typename CastOp::type1 WT;
  cv::Size ssize = _src.size(), dsize = _dst.size();
  int k, cn = _src.channels();
  // printf("[remapBilinear] cn = %d\n", cn);
  const AT* wtab = (const AT*)_wtab;
  const T* S0 = _src.ptr<T>();
  size_t sstep = _src.step / sizeof(S0[0]);
  T cval[CV_CN_MAX];
  int dx, dy;
  CastOp castOp;
  VecOp vecOp;

  for (k = 0; k < cn; k++) cval[k] = cv::saturate_cast<T>(_borderValue[k & 3]);

  unsigned width1 = std::max(ssize.width - 1, 0), height1 = std::max(ssize.height - 1, 0);
  CV_Assert(ssize.area() > 0);

  for (dy = 0; dy < dsize.height; dy++) {
    T* D = _dst.ptr<T>(dy);
    const short* XY = _xy.ptr<short>(dy);
    const ushort* FXY = _fxy.ptr<ushort>(dy);
    int X0 = 0;
    bool prevInlier = false;

    for (dx = 0; dx <= dsize.width; dx++) {
      bool curInlier = dx < dsize.width
                           ? (unsigned)XY[dx * 2] < width1 && (unsigned)XY[dx * 2 + 1] < height1
                           : !prevInlier;
      if (curInlier == prevInlier) continue;

      int X1 = dx;
      dx = X0;
      X0 = X1;
      prevInlier = curInlier;

      if (!curInlier) {
        int len = vecOp(_src, D, XY + dx * 2, FXY + dx, wtab, X1 - dx);
        D += len * cn;
        dx += len;

        if (cn == 1) {
          CV_Assert(0);
        } else if (cn == 2) {
          CV_Assert(0);
        } else if (cn == 3)
          for (; dx < X1; dx++, D += 3) {
            int sx = XY[dx * 2], sy = XY[dx * 2 + 1];
            const AT* w = wtab + FXY[dx] * 4;
            const T* S = S0 + sy * sstep + sx * 3;
            WT t0 = S[0] * w[0] + S[3] * w[1] + S[sstep] * w[2] + S[sstep + 3] * w[3];
            WT t1 = S[1] * w[0] + S[4] * w[1] + S[sstep + 1] * w[2] + S[sstep + 4] * w[3];
            WT t2 = S[2] * w[0] + S[5] * w[1] + S[sstep + 2] * w[2] + S[sstep + 5] * w[3];
            D[0] = castOp(t0);
            D[1] = castOp(t1);
            D[2] = castOp(t2);
          }
        else if (cn == 4) {
          CV_Assert(0);
        } else {
          CV_Assert(0);
        }
      } else {
        if (borderType == cv::BORDER_TRANSPARENT && cn != 3) {
          CV_Assert(0);
        }

        if (cn == 1) {
          CV_Assert(0);
        } else
          for (; dx < X1; dx++, D += cn) {
            int sx = XY[dx * 2], sy = XY[dx * 2 + 1];
            if (borderType == cv::BORDER_CONSTANT &&
                (sx >= ssize.width || sx + 1 < 0 || sy >= ssize.height || sy + 1 < 0)) {
              for (k = 0; k < cn; k++) D[k] = cval[k];
            } else {
              int sx0, sx1, sy0, sy1;
              const T *v0, *v1, *v2, *v3;
              const AT* w = wtab + FXY[dx] * 4;
#if 0
              if (borderType == cv::BORDER_REPLICATE) {
                CV_Assert(0);
              } else if (borderType == cv::BORDER_TRANSPARENT &&
                         ((unsigned)sx >= (unsigned)(ssize.width - 1) ||
                          (unsigned)sy >= (unsigned)(ssize.height - 1))) {
                CV_Assert(0);
              } else {
                /* this case*/
              }
#endif

#if 1 /* this case */
              sx0 = cv::borderInterpolate(sx, ssize.width, borderType);
              sx1 = cv::borderInterpolate(sx + 1, ssize.width, borderType);
              sy0 = cv::borderInterpolate(sy, ssize.height, borderType);
              sy1 = cv::borderInterpolate(sy + 1, ssize.height, borderType);
              v0 = sx0 >= 0 && sy0 >= 0 ? S0 + sy0 * sstep + sx0 * cn : &cval[0];
              v1 = sx1 >= 0 && sy0 >= 0 ? S0 + sy0 * sstep + sx1 * cn : &cval[0];
              v2 = sx0 >= 0 && sy1 >= 0 ? S0 + sy1 * sstep + sx0 * cn : &cval[0];
              v3 = sx1 >= 0 && sy1 >= 0 ? S0 + sy1 * sstep + sx1 * cn : &cval[0];
#endif
              for (k = 0; k < cn; k++)
                D[k] = castOp(WT(v0[k] * w[0] + v1[k] * w[1] + v2[k] * w[2] + v3[k] * w[3]));
            }
          }
      }
    }
  }
}

// L4275
typedef void (*RemapNNFunc)(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat& _xy, int borderType,
                            const cv::Scalar& _borderValue);

typedef void (*RemapFunc)(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat& _xy,
                          const cv::Mat& _fxy, const void* _wtab, int borderType,
                          const cv::Scalar& _borderValue);

// L4282
class RemapInvoker : public cv::ParallelLoopBody {
 public:
  RemapInvoker(const cv::Mat& _src, cv::Mat& _dst, const cv::Mat* _m1, const cv::Mat* _m2,
               int _borderType, const cv::Scalar& _borderValue, int _planar_input,
               RemapNNFunc _nnfunc, RemapFunc _ifunc, const void* _ctab)
      : ParallelLoopBody(),
        src(&_src),
        dst(&_dst),
        m1(_m1),
        m2(_m2),
        borderType(_borderType),
        borderValue(_borderValue),
        planar_input(_planar_input),
        nnfunc(_nnfunc),
        ifunc(_ifunc),
        ctab(_ctab) {}

  virtual void operator()(const cv::Range& range) const {
    int x, y, x1, y1;
    const int buf_size = 1 << 14;
    int brows0 = std::min(128, dst->rows), map_depth = m1->depth();
    int bcols0 = std::min(buf_size / brows0, dst->cols);
    brows0 = std::min(buf_size / bcols0, dst->rows);

    cv::Mat _bufxy(brows0, bcols0, CV_16SC2), _bufa;
    if (!nnfunc) _bufa.create(brows0, bcols0, CV_16UC1);

    for (y = range.start; y < range.end; y += brows0) {
      for (x = 0; x < dst->cols; x += bcols0) {
        int brows = std::min(brows0, range.end - y);
        int bcols = std::min(bcols0, dst->cols - x);
        cv::Mat dpart(*dst, cv::Rect(x, y, bcols, brows));
        cv::Mat bufxy(_bufxy, cv::Rect(0, 0, bcols, brows));

        if (nnfunc) {
          if (m1->type() == CV_16SC2 && m2->empty()) {  // the data is already in the right format
            bufxy = (*m1)(cv::Rect(x, y, bcols, brows));
          } else if (map_depth != CV_32F) {
            CV_Assert(0);
          } else if (!planar_input) {
            CV_Assert(0);
          } else {
            CV_Assert(0);
          }
          nnfunc(*src, dpart, bufxy, borderType, borderValue);
          continue;
        }

        cv::Mat bufa(_bufa, cv::Rect(0, 0, bcols, brows));
        for (y1 = 0; y1 < brows; y1++) {
          // short* XY = bufxy.ptr<short>(y1);        /* not used */
          ushort* A = bufa.ptr<ushort>(y1);

          if (m1->type() == CV_16SC2 && (m2->type() == CV_16UC1 || m2->type() == CV_16SC1)) {
            bufxy = (*m1)(cv::Rect(x, y, bcols, brows));

            const ushort* sA = m2->ptr<ushort>(y + y1) + x;
            x1 = 0;

#if CV_NEON
            uint16x8_t v_scale = vdupq_n_u16(INTER_TAB_SIZE2 - 1);
            for (; x1 <= bcols - 8; x1 += 8)
              vst1q_u16(A + x1, vandq_u16(vld1q_u16(sA + x1), v_scale));
#endif

            for (; x1 < bcols; x1++) A[x1] = (ushort)(sA[x1] & (INTER_TAB_SIZE2 - 1));
          } else if (planar_input) {
            CV_Assert(0);
          } else {
            CV_Assert(0);
          }
        }
        ifunc(*src, dpart, bufxy, bufa, ctab, borderType, borderValue);
      }
    }
  }

 private:
  const cv::Mat* src;
  cv::Mat* dst;
  const cv::Mat *m1, *m2;
  int borderType;
  cv::Scalar borderValue;
  int planar_input;
  RemapNNFunc nnfunc;
  RemapFunc ifunc;
  const void* ctab;
};

// L4897
void remap(cv::InputArray _src, cv::OutputArray _dst, cv::InputArray _map1, cv::InputArray _map2,
           int interpolation, int borderType, const cv::Scalar& borderValue) {
  static RemapNNFunc nn_tab[] = {remapNearest<uchar>, 0, 0, 0, 0, 0, 0, 0};

  static RemapFunc linear_tab[] = {
      remapBilinear<FixedPtCast<int, uchar, INTER_REMAP_COEF_BITS>, RemapVec_8u, short>,
      0,
      0,
      0,
      0,
      0,
      0,
      0};

#if EXT_FUNCTION
  static RemapFunc cubic_tab[];

  static RemapFunc lanczos4_tab[];
#endif

  CV_Assert(_map1.size().area() > 0);
  CV_Assert(_map2.empty() || (_map2.size() == _map1.size()));

  cv::Mat src = _src.getMat(), map1 = _map1.getMat(), map2 = _map2.getMat();
  _dst.create(map1.size(), src.type());
  cv::Mat dst = _dst.getMat();

  CV_Assert(dst.cols < SHRT_MAX && dst.rows < SHRT_MAX && src.cols < SHRT_MAX &&
            src.rows < SHRT_MAX);

  if (dst.data == src.data) src = src.clone();

  if (interpolation == INTER_AREA) interpolation = INTER_LINEAR;

  int type = src.type(), depth = CV_MAT_DEPTH(type);
  assert(depth == 0); /* NOTE: Because we remove other index on the function table */

  RemapNNFunc nnfunc = 0;
  RemapFunc ifunc = 0;
  const void* ctab = 0;
  bool fixpt = depth == CV_8U;
  bool planar_input = false;

  if (interpolation == INTER_NEAREST) {
    nnfunc = nn_tab[depth];
    CV_Assert(nnfunc != 0);
  } else {
    if (interpolation == INTER_LINEAR) {
      ifunc = linear_tab[depth];
    } else if (interpolation == INTER_CUBIC) {
      CV_Assert(0);
    } else if (interpolation == INTER_LANCZOS4) {
      CV_Assert(0);
    } else
      CV_Error(CV_StsBadArg, "Unknown interpolation method");
    CV_Assert(ifunc != 0);
    ctab = initInterTab2D(interpolation, fixpt);
  }

  const cv::Mat *m1 = &map1, *m2 = &map2;

  if ((map1.type() == CV_16SC2 &&
       (map2.type() == CV_16UC1 || map2.type() == CV_16SC1 || map2.empty())) ||
      (map2.type() == CV_16SC2 &&
       (map1.type() == CV_16UC1 || map1.type() == CV_16SC1 || map1.empty()))) {
    if (map1.type() != CV_16SC2) {
      std::swap(m1, m2);
    }
  } else {
    CV_Assert(((map1.type() == CV_32FC2 || map1.type() == CV_16SC2) && map2.empty()) ||
              (map1.type() == CV_32FC1 && map2.type() == CV_32FC1));
    planar_input = map1.channels() == 1;
  }

  RemapInvoker invoker(src, dst, m1, m2, borderType, borderValue, planar_input, nnfunc, ifunc,
                       ctab);
  cv::parallel_for_(cv::Range(0, dst.rows), invoker, dst.total() / (double)(1 << 16));
}

// L6074
class WarpPerspectiveInvoker : public cv::ParallelLoopBody {
 public:
  WarpPerspectiveInvoker(const cv::Mat& _src, cv::Mat& _dst, const double* _M, int _interpolation,
                         int _borderType, const cv::Scalar& _borderValue)
      : ParallelLoopBody(),
        src(_src),
        dst(_dst),
        M(_M),
        interpolation(_interpolation),
        borderType(_borderType),
        borderValue(_borderValue) {}

  virtual void operator()(const cv::Range& range) const {
    const int BLOCK_SZ = 32;
    short XY[BLOCK_SZ * BLOCK_SZ * 2], A[BLOCK_SZ * BLOCK_SZ];
    int x, y, x1, y1, width = dst.cols, height = dst.rows;

    int bh0 = std::min(BLOCK_SZ / 2, height);
    int bw0 = std::min(BLOCK_SZ * BLOCK_SZ / bh0, width);
    bh0 = std::min(BLOCK_SZ * BLOCK_SZ / bw0, height);

    for (y = range.start; y < range.end; y += bh0) {
      for (x = 0; x < width; x += bw0) {
        int bw = std::min(bw0, width - x);
        int bh = std::min(bh0, range.end - y);  // height

        cv::Mat _XY(bh, bw, CV_16SC2, XY), matA;
        cv::Mat dpart(dst, cv::Rect(x, y, bw, bh));

        for (y1 = 0; y1 < bh; y1++) {
          short* xy = XY + y1 * bw * 2;
          double X0 = M[0] * x + M[1] * (y + y1) + M[2];
          double Y0 = M[3] * x + M[4] * (y + y1) + M[5];
          double W0 = M[6] * x + M[7] * (y + y1) + M[8];

          if (interpolation == INTER_NEAREST) {
            CV_Assert(0);
          } else {
            short* alpha = A + y1 * bw;
            x1 = 0;
            for (; x1 < bw; x1++) {
              double W = W0 + M[6] * x1;
              W = W ? INTER_TAB_SIZE / W : 0;
              double fX =
                  std::max((double)INT_MIN, std::min((double)INT_MAX, (X0 + M[0] * x1) * W));
              double fY =
                  std::max((double)INT_MIN, std::min((double)INT_MAX, (Y0 + M[3] * x1) * W));
              int X = cv::saturate_cast<int>(fX);
              int Y = cv::saturate_cast<int>(fY);

              xy[x1 * 2] = cv::saturate_cast<short>(X >> INTER_BITS);
              xy[x1 * 2 + 1] = cv::saturate_cast<short>(Y >> INTER_BITS);
              alpha[x1] =
                  (short)((Y & (INTER_TAB_SIZE - 1)) * INTER_TAB_SIZE + (X & (INTER_TAB_SIZE - 1)));
            }
          }
        }

        if (interpolation == INTER_NEAREST) {
          CV_Assert(0);
        } else {
          cv::Mat _matA(bh, bw, CV_16U, A);
          remap(src, dpart, _XY, _matA, interpolation, borderType, borderValue);
        }
      }
    }
  }

 private:
  cv::Mat src;
  cv::Mat dst;
  const double* M;
  int interpolation, borderType;
  cv::Scalar borderValue;
};

// L6472
void _warpPerspectve(int src_type, const uchar* src_data, size_t src_step, int src_width,
                     int src_height, uchar* dst_data, size_t dst_step, int dst_width,
                     int dst_height, const double M[9], int interpolation, int borderType,
                     const double borderValue[4]) {
  cv::Mat src(cv::Size(src_width, src_height), src_type, const_cast<uchar*>(src_data), src_step);
  cv::Mat dst(cv::Size(dst_width, dst_height), src_type, dst_data, dst_step);

  cv::Range range(0, dst.rows);
  WarpPerspectiveInvoker invoker(
      src, dst, M, interpolation, borderType,
      cv::Scalar(borderValue[0], borderValue[1], borderValue[2], borderValue[3]));
  cv::parallel_for_(range, invoker, dst.total() / (double)(1 << 16));
}

// L6489
void cvitdl::warpPerspective(cv::InputArray _src, cv::OutputArray _dst, cv::InputArray _M0,
                             cv::Size dsize, int flags, int borderType,
                             const cv::Scalar& borderValue) {
  CV_Assert(_src.total() > 0);

  cv::Mat src = _src.getMat(), M0 = _M0.getMat();
  _dst.create(dsize.area() == 0 ? src.size() : dsize, src.type());
  cv::Mat dst = _dst.getMat();

  if (dst.data == src.data) src = src.clone();

  double M[9];
  cv::Mat matM(3, 3, CV_64F, M);
  int interpolation = flags & INTER_MAX;
  if (interpolation == INTER_AREA) interpolation = INTER_LINEAR;

  CV_Assert((M0.type() == CV_32F || M0.type() == CV_64F) && M0.rows == 3 && M0.cols == 3);
  M0.convertTo(matM, matM.type());

  if (!(flags & WARP_INVERSE_MAP)) {
    cv::invert(matM, matM);
  }

  // printf("[warpPerspective] flags = %d, borderType = %d\n", flags, borderType);
  _warpPerspectve(src.type(), src.data, src.step, src.cols, src.rows, dst.data, dst.step, dst.cols,
                  dst.rows, matM.ptr<double>(), interpolation, borderType, borderValue.val);
}

// L5507
class WarpAffineInvoker : public cv::ParallelLoopBody {
 public:
  WarpAffineInvoker(const cv::Mat& _src, cv::Mat& _dst, int _interpolation, int _borderType,
                    const cv::Scalar& _borderValue, int* _adelta, int* _bdelta, const double* _M)
      : ParallelLoopBody(),
        src(_src),
        dst(_dst),
        interpolation(_interpolation),
        borderType(_borderType),
        borderValue(_borderValue),
        adelta(_adelta),
        bdelta(_bdelta),
        M(_M) {}

  virtual void operator()(const cv::Range& range) const {
    const int BLOCK_SZ = 64;
    short XY[BLOCK_SZ * BLOCK_SZ * 2], A[BLOCK_SZ * BLOCK_SZ];
    const int AB_BITS = MAX(10, (int)INTER_BITS);
    const int AB_SCALE = 1 << AB_BITS;
    int round_delta = interpolation == INTER_NEAREST ? AB_SCALE / 2 : AB_SCALE / INTER_TAB_SIZE / 2,
        x, y, x1, y1;

    int bh0 = std::min(BLOCK_SZ / 2, dst.rows);
    int bw0 = std::min(BLOCK_SZ * BLOCK_SZ / bh0, dst.cols);
    bh0 = std::min(BLOCK_SZ * BLOCK_SZ / bw0, dst.rows);

    for (y = range.start; y < range.end; y += bh0) {
      for (x = 0; x < dst.cols; x += bw0) {
        int bw = std::min(bw0, dst.cols - x);
        int bh = std::min(bh0, range.end - y);

        cv::Mat _XY(bh, bw, CV_16SC2, XY), matA;
        cv::Mat dpart(dst, cv::Rect(x, y, bw, bh));

        for (y1 = 0; y1 < bh; y1++) {
          short* xy = XY + y1 * bw * 2;
          int X0 = cv::saturate_cast<int>((M[1] * (y + y1) + M[2]) * AB_SCALE) + round_delta;
          int Y0 = cv::saturate_cast<int>((M[4] * (y + y1) + M[5]) * AB_SCALE) + round_delta;

          if (interpolation == INTER_NEAREST) {
            x1 = 0;
#if CV_NEON
            int32x4_t v_X0 = vdupq_n_s32(X0), v_Y0 = vdupq_n_s32(Y0);
            for (; x1 <= bw - 8; x1 += 8) {
              int16x8x2_t v_dst;
              v_dst.val[0] = vcombine_s16(
                  vqmovn_s32(vshrq_n_s32(vaddq_s32(v_X0, vld1q_s32(adelta + x + x1)), AB_BITS)),
                  vqmovn_s32(
                      vshrq_n_s32(vaddq_s32(v_X0, vld1q_s32(adelta + x + x1 + 4)), AB_BITS)));
              v_dst.val[1] = vcombine_s16(
                  vqmovn_s32(vshrq_n_s32(vaddq_s32(v_Y0, vld1q_s32(bdelta + x + x1)), AB_BITS)),
                  vqmovn_s32(
                      vshrq_n_s32(vaddq_s32(v_Y0, vld1q_s32(bdelta + x + x1 + 4)), AB_BITS)));

              vst2q_s16(xy + (x1 << 1), v_dst);
            }
#endif
            for (; x1 < bw; x1++) {
              int X = (X0 + adelta[x + x1]) >> AB_BITS;
              int Y = (Y0 + bdelta[x + x1]) >> AB_BITS;
              xy[x1 * 2] = cv::saturate_cast<short>(X);
              xy[x1 * 2 + 1] = cv::saturate_cast<short>(Y);
            }
          } else {
            short* alpha = A + y1 * bw;
            x1 = 0;
#if CV_NEON
            int32x4_t v__X0 = vdupq_n_s32(X0), v__Y0 = vdupq_n_s32(Y0),
                      v_mask = vdupq_n_s32(INTER_TAB_SIZE - 1);
            for (; x1 <= bw - 8; x1 += 8) {
              int32x4_t v_X0 =
                  vshrq_n_s32(vaddq_s32(v__X0, vld1q_s32(adelta + x + x1)), AB_BITS - INTER_BITS);
              int32x4_t v_Y0 =
                  vshrq_n_s32(vaddq_s32(v__Y0, vld1q_s32(bdelta + x + x1)), AB_BITS - INTER_BITS);
              int32x4_t v_X1 = vshrq_n_s32(vaddq_s32(v__X0, vld1q_s32(adelta + x + x1 + 4)),
                                           AB_BITS - INTER_BITS);
              int32x4_t v_Y1 = vshrq_n_s32(vaddq_s32(v__Y0, vld1q_s32(bdelta + x + x1 + 4)),
                                           AB_BITS - INTER_BITS);

              int16x8x2_t v_xy;
              v_xy.val[0] = vcombine_s16(vqmovn_s32(vshrq_n_s32(v_X0, INTER_BITS)),
                                         vqmovn_s32(vshrq_n_s32(v_X1, INTER_BITS)));
              v_xy.val[1] = vcombine_s16(vqmovn_s32(vshrq_n_s32(v_Y0, INTER_BITS)),
                                         vqmovn_s32(vshrq_n_s32(v_Y1, INTER_BITS)));

              vst2q_s16(xy + (x1 << 1), v_xy);

              int16x4_t v_alpha0 = vmovn_s32(vaddq_s32(
                  vshlq_n_s32(vandq_s32(v_Y0, v_mask), INTER_BITS), vandq_s32(v_X0, v_mask)));
              int16x4_t v_alpha1 = vmovn_s32(vaddq_s32(
                  vshlq_n_s32(vandq_s32(v_Y1, v_mask), INTER_BITS), vandq_s32(v_X1, v_mask)));
              vst1q_s16(alpha + x1, vcombine_s16(v_alpha0, v_alpha1));
            }
#endif
            for (; x1 < bw; x1++) {
              int X = (X0 + adelta[x + x1]) >> (AB_BITS - INTER_BITS);
              int Y = (Y0 + bdelta[x + x1]) >> (AB_BITS - INTER_BITS);
              xy[x1 * 2] = cv::saturate_cast<short>(X >> INTER_BITS);
              xy[x1 * 2 + 1] = cv::saturate_cast<short>(Y >> INTER_BITS);
              alpha[x1] =
                  (short)((Y & (INTER_TAB_SIZE - 1)) * INTER_TAB_SIZE + (X & (INTER_TAB_SIZE - 1)));
            }
          }
        }

        if (interpolation == INTER_NEAREST)
          remap(src, dpart, _XY, cv::Mat(), interpolation, borderType, borderValue);
        else {
          cv::Mat _matA(bh, bw, CV_16U, A);
          remap(src, dpart, _XY, _matA, interpolation, borderType, borderValue);
        }
      }
    }
  }

 private:
  cv::Mat src;
  cv::Mat dst;
  int interpolation, borderType;
  cv::Scalar borderValue;
  int *adelta, *bdelta;
  const double* M;
};

// L5926
void _warpAffine(int src_type, const uchar* src_data, size_t src_step, int src_width,
                 int src_height, uchar* dst_data, size_t dst_step, int dst_width, int dst_height,
                 const double M[6], int interpolation, int borderType,
                 const double borderValue[4]) {
  cv::Mat src(cv::Size(src_width, src_height), src_type, const_cast<uchar*>(src_data), src_step);
  cv::Mat dst(cv::Size(dst_width, dst_height), src_type, dst_data, dst_step);

  int x;
  cv::AutoBuffer<int> _abdelta(dst.cols * 2);
  int *adelta = &_abdelta[0], *bdelta = adelta + dst.cols;
  const int AB_BITS = MAX(10, (int)INTER_BITS);
  const int AB_SCALE = 1 << AB_BITS;

  for (x = 0; x < dst.cols; x++) {
    adelta[x] = cv::saturate_cast<int>(M[0] * x * AB_SCALE);
    bdelta[x] = cv::saturate_cast<int>(M[3] * x * AB_SCALE);
  }

  cv::Range range(0, dst.rows);
  WarpAffineInvoker invoker(
      src, dst, interpolation, borderType,
      cv::Scalar(borderValue[0], borderValue[1], borderValue[2], borderValue[3]), adelta, bdelta,
      M);
  cv::parallel_for_(range, invoker, dst.total() / (double)(1 << 16));
}

// L5959
void cvitdl::warpAffine(cv::InputArray _src, cv::OutputArray _dst, cv::InputArray _M0,
                        cv::Size dsize, int flags, int borderType, const cv::Scalar& borderValue) {
  cv::Mat src = _src.getMat(), M0 = _M0.getMat();
  _dst.create(dsize.area() == 0 ? src.size() : dsize, src.type());
  cv::Mat dst = _dst.getMat();
  CV_Assert(src.cols > 0 && src.rows > 0);
  if (dst.data == src.data) src = src.clone();

  double M[6];
  cv::Mat matM(2, 3, CV_64F, M);
  int interpolation = flags & INTER_MAX;
  if (interpolation == INTER_AREA) interpolation = INTER_LINEAR;

  CV_Assert((M0.type() == CV_32F || M0.type() == CV_64F) && M0.rows == 2 && M0.cols == 3);
  M0.convertTo(matM, matM.type());

  if (!(flags & WARP_INVERSE_MAP)) {
    double D = M[0] * M[4] - M[1] * M[3];
    D = D != 0 ? 1. / D : 0;
    double A11 = M[4] * D, A22 = M[0] * D;
    M[0] = A11;
    M[1] *= -D;
    M[3] *= -D;
    M[4] = A22;
    double b1 = -M[0] * M[2] - M[1] * M[5];
    double b2 = -M[3] * M[2] - M[4] * M[5];
    M[2] = b1;
    M[5] = b2;
  }

  // printf("[warpAffine] flags = %d, borderType = %d\n", flags, borderType);
  _warpAffine(src.type(), src.data, src.step, src.cols, src.rows, dst.data, dst.step, dst.cols,
              dst.rows, M, interpolation, borderType, borderValue.val);
}

/* Calculates coefficients of perspective transformation
 */
// L6633
cv::Mat cvitdl::getPerspectiveTransform(const cv::Point2f src[], const cv::Point2f dst[]) {
  cv::Mat M(3, 3, CV_64F), X(8, 1, CV_64F, M.ptr());
  double a[8][8], b[8];
  cv::Mat A(8, 8, CV_64F, a), B(8, 1, CV_64F, b);

  for (int i = 0; i < 4; ++i) {
    a[i][0] = a[i + 4][3] = src[i].x;
    a[i][1] = a[i + 4][4] = src[i].y;
    a[i][2] = a[i + 4][5] = 1;
    a[i][3] = a[i][4] = a[i][5] = a[i + 4][0] = a[i + 4][1] = a[i + 4][2] = 0;
    a[i][6] = -src[i].x * dst[i].x;
    a[i][7] = -src[i].y * dst[i].x;
    a[i + 4][6] = -src[i].x * dst[i].y;
    a[i + 4][7] = -src[i].y * dst[i].y;
    b[i] = dst[i].x;
    b[i + 4] = dst[i].y;
  }

  cv::solve(A, B, X, cv::DECOMP_SVD);
  M.ptr<double>()[8] = 1.;

  return M;
}

/* Calculates coefficients of affine transformation
 */
// L6681
cv::Mat cvitdl::getAffineTransform(const cv::Point2f src[], const cv::Point2f dst[]) {
  cv::Mat M(2, 3, CV_64F), X(6, 1, CV_64F, M.ptr());
  double a[6 * 6], b[6];
  cv::Mat A(6, 6, CV_64F, a), B(6, 1, CV_64F, b);

  for (int i = 0; i < 3; i++) {
    int j = i * 12;
    int k = i * 12 + 6;
    a[j] = a[k + 3] = src[i].x;
    a[j + 1] = a[k + 4] = src[i].y;
    a[j + 2] = a[k + 5] = 1;
    a[j + 3] = a[j + 4] = a[j + 5] = 0;
    a[k] = a[k + 1] = a[k + 2] = 0;
    b[i * 2] = dst[i].x;
    b[i * 2 + 1] = dst[i].y;
  }

  cv::solve(A, B, X);
  return M;
}

// L6745
cv::Mat cvitdl::getPerspectiveTransform(cv::InputArray _src, cv::InputArray _dst) {
  cv::Mat src = _src.getMat(), dst = _dst.getMat();
  CV_Assert(src.checkVector(2, CV_32F) == 4 && dst.checkVector(2, CV_32F) == 4);
  return getPerspectiveTransform((const cv::Point2f*)src.data, (const cv::Point2f*)dst.data);
}

// L6752
cv::Mat cvitdl::getAffineTransform(cv::InputArray _src, cv::InputArray _dst) {
  cv::Mat src = _src.getMat(), dst = _dst.getMat();
  CV_Assert(src.checkVector(2, CV_32F) == 3 && dst.checkVector(2, CV_32F) == 3);
  return getAffineTransform((const cv::Point2f*)src.data, (const cv::Point2f*)dst.data);
}
