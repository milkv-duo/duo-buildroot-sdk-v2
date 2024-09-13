#include "color.hpp"
#include "types_c.h"

#define EXT_FUNCTION 0

// constants for conversion from/to RGB and Gray, YUV, YCrCb according to BT.601
// L109
const float B2YF = 0.114f;
const float G2YF = 0.587f;
const float R2YF = 0.299f;

// L236
template <typename _Tp>
struct ColorChannel {
  typedef float worktype_f;
  static _Tp max() { return std::numeric_limits<_Tp>::max(); }
  static _Tp half() { return (_Tp)(max() / 2 + 1); }
};

///////////////////////////// Top-level template function ////////////////////////////////
// L260
template <typename Cvt>
class CvtColorLoop_Invoker : public cv::ParallelLoopBody {
  typedef typename Cvt::channel_type _Tp;

 public:
  CvtColorLoop_Invoker(const uchar* src_data_, size_t src_step_, uchar* dst_data_, size_t dst_step_,
                       int width_, const Cvt& _cvt)
      : ParallelLoopBody(),
        src_data(src_data_),
        src_step(src_step_),
        dst_data(dst_data_),
        dst_step(dst_step_),
        width(width_),
        cvt(_cvt) {}

  virtual void operator()(const cv::Range& range) const {
    const uchar* yS = src_data + static_cast<size_t>(range.start) * src_step;
    uchar* yD = dst_data + static_cast<size_t>(range.start) * dst_step;

    for (int i = range.start; i < range.end; ++i, yS += src_step, yD += dst_step)
      cvt(reinterpret_cast<const _Tp*>(yS), reinterpret_cast<_Tp*>(yD), width);
  }

 private:
  const uchar* src_data;
  size_t src_step;
  uchar* dst_data;
  size_t dst_step;
  int width;
  const Cvt& cvt;

  const CvtColorLoop_Invoker& operator=(const CvtColorLoop_Invoker&);
};

// L292
template <typename Cvt>
void CvtColorLoop(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                  int width, int height, const Cvt& cvt) {
  cv::parallel_for_(cv::Range(0, height),
                    CvtColorLoop_Invoker<Cvt>(src_data, src_step, dst_data, dst_step, width, cvt),
                    (width * height) / static_cast<double>(1 << 16));
}

////////////////// Various 3/4-channel to 3/4-channel RGB transformations /////////////////
// L652
template <typename _Tp>
struct RGB2RGB {
  typedef _Tp channel_type;

  RGB2RGB(int _srccn, int _dstcn, int _blueIdx) : srccn(_srccn), dstcn(_dstcn), blueIdx(_blueIdx) {}
  void operator()(const _Tp* src, _Tp* dst, int n) const {
    int scn = srccn, dcn = dstcn, bidx = blueIdx;
    if (dcn == 3) {
      n *= 3;
      for (int i = 0; i < n; i += 3, src += scn) {
        _Tp t0 = src[bidx], t1 = src[1], t2 = src[bidx ^ 2];
        dst[i] = t0;
        dst[i + 1] = t1;
        dst[i + 2] = t2;
      }
    } else if (scn == 3) {
      n *= 3;
      _Tp alpha = ColorChannel<_Tp>::max();
      for (int i = 0; i < n; i += 3, dst += 4) {
        _Tp t0 = src[i], t1 = src[i + 1], t2 = src[i + 2];
        dst[bidx] = t0;
        dst[1] = t1;
        dst[bidx ^ 2] = t2;
        dst[3] = alpha;
      }
    } else {
      n *= 4;
      for (int i = 0; i < n; i += 4) {
        _Tp t0 = src[i], t1 = src[i + 1], t2 = src[i + 2], t3 = src[i + 3];
        dst[i] = t2;
        dst[i + 1] = t1;
        dst[i + 2] = t0;
        dst[i + 3] = t3;
      }
    }
  }

  int srccn, dstcn, blueIdx;
};

///////////////////////////////// Color to/from Grayscale ////////////////////////////////
// L1037
template <typename _Tp>
struct Gray2RGB {
  typedef _Tp channel_type;

  Gray2RGB(int _dstcn) : dstcn(_dstcn) {}
  void operator()(const _Tp* src, _Tp* dst, int n) const {
    if (dstcn == 3)
      for (int i = 0; i < n; i++, dst += 3) {
        dst[0] = dst[1] = dst[2] = src[i];
      }
    else {
      _Tp alpha = ColorChannel<_Tp>::max();
      for (int i = 0; i < n; i++, dst += 4) {
        dst[0] = dst[1] = dst[2] = src[i];
        dst[3] = alpha;
      }
    }
  }

  int dstcn;
};

// L1171
#undef R2Y
#undef G2Y
#undef B2Y
enum {
  yuv_shift = 14,
  xyz_shift = 12,
  R2Y = 4899,  // == R2YF*16384
  G2Y = 9617,  // == G2YF*16384
  B2Y = 1868,  // == B2YF*16384
  BLOCK_SIZE = 256
};

// L1343
template <typename _Tp>
struct RGB2Gray {
  typedef _Tp channel_type;

  RGB2Gray(int _srccn, int blueIdx, const float* _coeffs) : srccn(_srccn) {
    static const float coeffs0[] = {R2YF, G2YF, B2YF};
    memcpy(coeffs, _coeffs ? _coeffs : coeffs0, 3 * sizeof(coeffs[0]));
    if (blueIdx == 0) std::swap(coeffs[0], coeffs[2]);
  }

  void operator()(const _Tp* src, _Tp* dst, int n) const {
    int scn = srccn;
    float cb = coeffs[0], cg = coeffs[1], cr = coeffs[2];
    for (int i = 0; i < n; i++, src += scn)
      dst[i] = cv::saturate_cast<_Tp>(src[0] * cb + src[1] * cg + src[2] * cr);
  }
  int srccn;
  float coeffs[3];
};

// L1366
template <>
struct RGB2Gray<uchar> {
  typedef uchar channel_type;

  RGB2Gray(int _srccn, int blueIdx, const int* coeffs) : srccn(_srccn) {
    const int coeffs0[] = {R2Y, G2Y, B2Y};
    if (!coeffs) coeffs = coeffs0;

    int b = 0, g = 0, r = (1 << (yuv_shift - 1));
    int db = coeffs[blueIdx ^ 2], dg = coeffs[1], dr = coeffs[blueIdx];

    for (int i = 0; i < 256; i++, b += db, g += dg, r += dr) {
      tab[i] = b;
      tab[i + 256] = g;
      tab[i + 512] = r;
    }
  }
  void operator()(const uchar* src, uchar* dst, int n) const {
    int scn = srccn;
    const int* _tab = tab;
    for (int i = 0; i < n; i++, src += scn)
      dst[i] = (uchar)((_tab[src[0]] + _tab[src[1] + 256] + _tab[src[2] + 512]) >> yuv_shift);
  }
  int srccn;
  int tab[256 * 3];
};

// L7373
///////////////////////////////////// YUV420 -> RGB /////////////////////////////////////

const int ITUR_BT_601_CY = 1220542;
const int ITUR_BT_601_CUB = 2116026;
const int ITUR_BT_601_CUG = -409993;
const int ITUR_BT_601_CVG = -852492;
const int ITUR_BT_601_CVR = 1673527;
const int ITUR_BT_601_SHIFT = 20;

// Coefficients for RGB to YUV420p conversion
const int ITUR_BT_601_CRY = 269484;
const int ITUR_BT_601_CGY = 528482;
const int ITUR_BT_601_CBY = 102760;
const int ITUR_BT_601_CRU = -155188;
const int ITUR_BT_601_CGU = -305135;
const int ITUR_BT_601_CBU = 460324;
const int ITUR_BT_601_CGV = -385875;
const int ITUR_BT_601_CBV = -74448;

// L7392
template <int bIdx, int uIdx>
struct YUV420sp2RGB888Invoker : cv::ParallelLoopBody {
  uchar* dst_data;
  size_t dst_step;
  int width;
  const uchar *my1, *muv;
  size_t stride;

  YUV420sp2RGB888Invoker(uchar* _dst_data, size_t _dst_step, int _dst_width, size_t _stride,
                         const uchar* _y1, const uchar* _uv)
      : dst_data(_dst_data),
        dst_step(_dst_step),
        width(_dst_width),
        my1(_y1),
        muv(_uv),
        stride(_stride) {}

  void operator()(const cv::Range& range) const {
    int rangeBegin = range.start * 2;
    int rangeEnd = range.end * 2;

    // R = 1.164(Y - 16) + 1.596(V - 128)
    // G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128)
    // B = 1.164(Y - 16)                  + 2.018(U - 128)

    // R = (1220542(Y - 16) + 1673527(V - 128)                  + (1 << 19)) >> 20
    // G = (1220542(Y - 16) - 852492(V - 128) - 409993(U - 128) + (1 << 19)) >> 20
    // B = (1220542(Y - 16)                  + 2116026(U - 128) + (1 << 19)) >> 20

    const uchar *y1 = my1 + rangeBegin * stride, *uv = muv + rangeBegin * stride / 2;

    for (int j = rangeBegin; j < rangeEnd; j += 2, y1 += stride * 2, uv += stride) {
      uchar* row1 = dst_data + dst_step * j;
      uchar* row2 = dst_data + dst_step * (j + 1);
      const uchar* y2 = y1 + stride;

      for (int i = 0; i < width; i += 2, row1 += 6, row2 += 6) {
        int u = int(uv[i + 0 + uIdx]) - 128;
        int v = int(uv[i + 1 - uIdx]) - 128;

        int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
        int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
        int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

        int y00 = std::max(0, int(y1[i]) - 16) * ITUR_BT_601_CY;
        row1[2 - bIdx] = cv::saturate_cast<uchar>((y00 + ruv) >> ITUR_BT_601_SHIFT);
        row1[1] = cv::saturate_cast<uchar>((y00 + guv) >> ITUR_BT_601_SHIFT);
        row1[bIdx] = cv::saturate_cast<uchar>((y00 + buv) >> ITUR_BT_601_SHIFT);

        int y01 = std::max(0, int(y1[i + 1]) - 16) * ITUR_BT_601_CY;
        row1[5 - bIdx] = cv::saturate_cast<uchar>((y01 + ruv) >> ITUR_BT_601_SHIFT);
        row1[4] = cv::saturate_cast<uchar>((y01 + guv) >> ITUR_BT_601_SHIFT);
        row1[3 + bIdx] = cv::saturate_cast<uchar>((y01 + buv) >> ITUR_BT_601_SHIFT);

        int y10 = std::max(0, int(y2[i]) - 16) * ITUR_BT_601_CY;
        row2[2 - bIdx] = cv::saturate_cast<uchar>((y10 + ruv) >> ITUR_BT_601_SHIFT);
        row2[1] = cv::saturate_cast<uchar>((y10 + guv) >> ITUR_BT_601_SHIFT);
        row2[bIdx] = cv::saturate_cast<uchar>((y10 + buv) >> ITUR_BT_601_SHIFT);

        int y11 = std::max(0, int(y2[i + 1]) - 16) * ITUR_BT_601_CY;
        row2[5 - bIdx] = cv::saturate_cast<uchar>((y11 + ruv) >> ITUR_BT_601_SHIFT);
        row2[4] = cv::saturate_cast<uchar>((y11 + guv) >> ITUR_BT_601_SHIFT);
        row2[3 + bIdx] = cv::saturate_cast<uchar>((y11 + buv) >> ITUR_BT_601_SHIFT);
      }
    }
  }
};

// L7528
template <int bIdx>
struct YUV420p2RGB888Invoker : cv::ParallelLoopBody {
  uchar* dst_data;
  size_t dst_step;
  int width;
  const uchar *my1, *mu, *mv;
  size_t stride;
  int ustepIdx, vstepIdx;

  YUV420p2RGB888Invoker(uchar* _dst_data, size_t _dst_step, int _dst_width, size_t _stride,
                        const uchar* _y1, const uchar* _u, const uchar* _v, int _ustepIdx,
                        int _vstepIdx)
      : dst_data(_dst_data),
        dst_step(_dst_step),
        width(_dst_width),
        my1(_y1),
        mu(_u),
        mv(_v),
        stride(_stride),
        ustepIdx(_ustepIdx),
        vstepIdx(_vstepIdx) {}

  void operator()(const cv::Range& range) const {
    const int rangeBegin = range.start * 2;
    const int rangeEnd = range.end * 2;

    int uvsteps[2] = {width / 2, static_cast<int>(stride) - width / 2};
    int usIdx = ustepIdx, vsIdx = vstepIdx;

    const uchar* y1 = my1 + rangeBegin * stride;
    const uchar* u1 = mu + (range.start / 2) * stride;
    const uchar* v1 = mv + (range.start / 2) * stride;

    if (range.start % 2 == 1) {
      u1 += uvsteps[(usIdx++) & 1];
      v1 += uvsteps[(vsIdx++) & 1];
    }

    for (int j = rangeBegin; j < rangeEnd;
         j += 2, y1 += stride * 2, u1 += uvsteps[(usIdx++) & 1], v1 += uvsteps[(vsIdx++) & 1]) {
      uchar* row1 = dst_data + dst_step * j;
      uchar* row2 = dst_data + dst_step * (j + 1);
      const uchar* y2 = y1 + stride;

      for (int i = 0; i < width / 2; i += 1, row1 += 6, row2 += 6) {
        int u = int(u1[i]) - 128;
        int v = int(v1[i]) - 128;

        int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
        int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
        int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

        int y00 = std::max(0, int(y1[2 * i]) - 16) * ITUR_BT_601_CY;
        row1[2 - bIdx] = cv::saturate_cast<uchar>((y00 + ruv) >> ITUR_BT_601_SHIFT);
        row1[1] = cv::saturate_cast<uchar>((y00 + guv) >> ITUR_BT_601_SHIFT);
        row1[bIdx] = cv::saturate_cast<uchar>((y00 + buv) >> ITUR_BT_601_SHIFT);

        int y01 = std::max(0, int(y1[2 * i + 1]) - 16) * ITUR_BT_601_CY;
        row1[5 - bIdx] = cv::saturate_cast<uchar>((y01 + ruv) >> ITUR_BT_601_SHIFT);
        row1[4] = cv::saturate_cast<uchar>((y01 + guv) >> ITUR_BT_601_SHIFT);
        row1[3 + bIdx] = cv::saturate_cast<uchar>((y01 + buv) >> ITUR_BT_601_SHIFT);

        int y10 = std::max(0, int(y2[2 * i]) - 16) * ITUR_BT_601_CY;
        row2[2 - bIdx] = cv::saturate_cast<uchar>((y10 + ruv) >> ITUR_BT_601_SHIFT);
        row2[1] = cv::saturate_cast<uchar>((y10 + guv) >> ITUR_BT_601_SHIFT);
        row2[bIdx] = cv::saturate_cast<uchar>((y10 + buv) >> ITUR_BT_601_SHIFT);

        int y11 = std::max(0, int(y2[2 * i + 1]) - 16) * ITUR_BT_601_CY;
        row2[5 - bIdx] = cv::saturate_cast<uchar>((y11 + ruv) >> ITUR_BT_601_SHIFT);
        row2[4] = cv::saturate_cast<uchar>((y11 + guv) >> ITUR_BT_601_SHIFT);
        row2[3 + bIdx] = cv::saturate_cast<uchar>((y11 + buv) >> ITUR_BT_601_SHIFT);
      }
    }
  }
};

// L7672
#define MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION (320 * 240)

template <int bIdx, int uIdx>
inline void cvtYUV420sp2RGB(uchar* dst_data, size_t dst_step, int dst_width, int dst_height,
                            size_t _stride, const uchar* _y1, const uchar* _uv) {
  YUV420sp2RGB888Invoker<bIdx, uIdx> converter(dst_data, dst_step, dst_width, _stride, _y1, _uv);
  if (dst_width * dst_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
    parallel_for_(cv::Range(0, dst_height / 2), converter);
  else
    converter(cv::Range(0, dst_height / 2));
}

// L7694
template <int bIdx>
inline void cvtYUV420p2RGB(uchar* dst_data, size_t dst_step, int dst_width, int dst_height,
                           size_t _stride, const uchar* _y1, const uchar* _u, const uchar* _v,
                           int ustepIdx, int vstepIdx) {
  YUV420p2RGB888Invoker<bIdx> converter(dst_data, dst_step, dst_width, _stride, _y1, _u, _v,
                                        ustepIdx, vstepIdx);
  if (dst_width * dst_height >= MIN_SIZE_FOR_PARALLEL_YUV420_CONVERSION)
    parallel_for_(cv::Range(0, dst_height / 2), converter);
  else
    converter(cv::Range(0, dst_height / 2));
}

// TODO: RGB2Gray CV_NEON version

// 8u, 16u, 32f
// L8643
void cvtBGRtoBGR(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                 int width, int height, int depth, int scn, int dcn, bool swapBlue) {
  int blueIdx = swapBlue ? 2 : 0;
  if (depth == CV_8U) {
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height,
                 RGB2RGB<uchar>(scn, dcn, blueIdx));
  } else if (depth == CV_16U) {
    CV_Assert(0);
  } else {
    CV_Assert(0);
  }
}

// 8u, 16u, 32f
// L8804
void cvtBGRtoGray(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                  int width, int height, int depth, int scn, bool swapBlue) {
  int blueIdx = swapBlue ? 2 : 0;
  if (depth == CV_8U) {
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height,
                 RGB2Gray<uchar>(scn, blueIdx, 0));
  } else if (depth == CV_16U) {
    CV_Assert(0);
  } else {
    CV_Assert(0);
  }
}

// 8u, 16u, 32f
// L8853
void cvtGraytoBGR(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                  int width, int height, int depth, int dcn) {
  if (depth == CV_8U) {
    CvtColorLoop(src_data, src_step, dst_data, dst_step, width, height, Gray2RGB<uchar>(dcn));
  } else if (depth == CV_16U) {
    CV_Assert(0);
  } else {
    CV_Assert(0);
  }
}

// L9483
void cvtTwoPlaneYUVtoBGR(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                         int dst_width, int dst_height, int dcn, bool swapBlue, int uIdx) {
#if 0
    CV_INSTRUMENT_REGION()

    CALL_HAL(cvtTwoPlaneYUVtoBGR, cv_hal_cvtTwoPlaneYUVtoBGR, src_data, src_step, dst_data, dst_step, dst_width, dst_height, dcn, swapBlue, uIdx);
#endif
  int blueIdx = swapBlue ? 2 : 0;
  const uchar* uv = src_data + src_step * static_cast<size_t>(dst_height);
  switch (dcn * 100 + blueIdx * 10 + uIdx) {
    case 300:
      cvtYUV420sp2RGB<0, 0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv);
      break;
    case 301:
      cvtYUV420sp2RGB<0, 1>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv);
      break;
    case 320:
      cvtYUV420sp2RGB<2, 0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv);
      break;
    case 321:
      cvtYUV420sp2RGB<2, 1>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv);
      break;
#if 0 /* disable RGBA */
    case 400: cvtYUV420sp2RGBA<0, 0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv); break;
    case 401: cvtYUV420sp2RGBA<0, 1>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv); break;
    case 420: cvtYUV420sp2RGBA<2, 0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv); break;
    case 421: cvtYUV420sp2RGBA<2, 1>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, uv); break;
#endif
    default:
      CV_Error(CV_StsBadFlag, "Unknown/unsupported color conversion code");
      break;
  };
}

// L9507
void cvtThreePlaneYUVtoBGR(const uchar* src_data, size_t src_step, uchar* dst_data, size_t dst_step,
                           int dst_width, int dst_height, int dcn, bool swapBlue, int uIdx) {
#if 0
    CV_INSTRUMENT_REGION()

    CALL_HAL(cvtThreePlaneYUVtoBGR, cv_hal_cvtThreePlaneYUVtoBGR, src_data, src_step, dst_data, dst_step, dst_width, dst_height, dcn, swapBlue, uIdx);
#endif
  const uchar* u = src_data + src_step * static_cast<size_t>(dst_height);
  const uchar* v = src_data + src_step * static_cast<size_t>(dst_height + dst_height / 4) +
                   (dst_width / 2) * ((dst_height % 4) / 2);

  int ustepIdx = 0;
  int vstepIdx = dst_height % 4 == 2 ? 1 : 0;

  if (uIdx == 1) {
    std::swap(u, v), std::swap(ustepIdx, vstepIdx);
  }
  int blueIdx = swapBlue ? 2 : 0;

  switch (dcn * 10 + blueIdx) {
    case 30:
      cvtYUV420p2RGB<0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, u, v,
                        ustepIdx, vstepIdx);
      break;
    case 32:
      cvtYUV420p2RGB<2>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, u, v,
                        ustepIdx, vstepIdx);
      break;
#if 0 /* disable RGBA */
    case 40: cvtYUV420p2RGBA<0>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, u, v, ustepIdx, vstepIdx); break;
    case 42: cvtYUV420p2RGBA<2>(dst_data, dst_step, dst_width, dst_height, src_step, src_data, u, v, ustepIdx, vstepIdx); break;
#endif
    default:
      CV_Error(CV_StsBadFlag, "Unknown/unsupported color conversion code");
      break;
  };
}

//
// Helper functions
//

// clang-format off
// L9652
inline bool swapBlue(int code) {
  switch (code)
  {
  case CV_BGR2BGRA: case CV_BGRA2BGR:
  case CV_BGR2BGR565: case CV_BGR2BGR555: case CV_BGRA2BGR565: case CV_BGRA2BGR555:
  case CV_BGR5652BGR: case CV_BGR5552BGR: case CV_BGR5652BGRA: case CV_BGR5552BGRA:
  case CV_BGR2GRAY: case CV_BGRA2GRAY:
  case CV_BGR2YCrCb: case CV_BGR2YUV:
  case CV_YCrCb2BGR: case CV_YUV2BGR:
  case CV_BGR2XYZ: case CV_XYZ2BGR:
  case CV_BGR2HSV: case CV_BGR2HLS: case CV_BGR2HSV_FULL: case CV_BGR2HLS_FULL:
  case CV_YUV2BGR_YV12: case CV_YUV2BGRA_YV12: case CV_YUV2BGR_IYUV: case CV_YUV2BGRA_IYUV:
  case CV_YUV2BGR_NV21: case CV_YUV2BGRA_NV21: case CV_YUV2BGR_NV12: case CV_YUV2BGRA_NV12:
  case CV_Lab2BGR: case CV_Luv2BGR: case CV_Lab2LBGR: case CV_Luv2LBGR:
  case CV_BGR2Lab: case CV_BGR2Luv: case CV_LBGR2Lab: case CV_LBGR2Luv:
  case CV_HSV2BGR: case CV_HLS2BGR: case CV_HSV2BGR_FULL: case CV_HLS2BGR_FULL:
  case CV_YUV2BGR_UYVY: case CV_YUV2BGRA_UYVY: case CV_YUV2BGR_YUY2:
  case CV_YUV2BGRA_YUY2:  case CV_YUV2BGR_YVYU: case CV_YUV2BGRA_YVYU:
  case CV_BGR2YUV_IYUV: case CV_BGRA2YUV_IYUV: case CV_BGR2YUV_YV12: case CV_BGRA2YUV_YV12:
      return false;
  default:
      return true;
  }
}

// clang-format on

// clang-format off
//////////////////////////////////////////////////////////////////////////////////////////
//                                   The main function                                  //
//////////////////////////////////////////////////////////////////////////////////////////
// L9694
void cvitdl::cvtColor(cv::InputArray _src, cv::OutputArray _dst, int code, int dcn) {
  int stype = _src.type();
  int scn = CV_MAT_CN(stype), depth = CV_MAT_DEPTH(stype);
  int uidx;
  // int uidx, gbits, ycn; /* not used variable */

  cv::Mat src, dst;
  if (_src.getObj() == _dst.getObj())  // inplace processing (#6653)
    _src.copyTo(src);
  else
    src = _src.getMat();
  cv::Size sz = src.size();
  CV_Assert(depth == CV_8U || depth == CV_16U || depth == CV_32F);

  switch (code) {
    case CV_BGR2BGRA: case CV_RGB2BGRA: case CV_BGRA2BGR:
    case CV_RGBA2BGR: case CV_RGB2BGR: case CV_BGRA2RGBA:
      CV_Assert(scn == 3 || scn == 4);
      dcn = code == CV_BGR2BGRA || code == CV_RGB2BGRA || code == CV_BGRA2RGBA ? 4 : 3;
      _dst.create(sz, CV_MAKETYPE(depth, dcn));
      dst = _dst.getMat();
      cvtBGRtoBGR(src.data, src.step, dst.data, dst.step, src.cols, src.rows, depth, scn, dcn,
                  swapBlue(code));
      break;

    case CV_BGR2GRAY: case CV_BGRA2GRAY: case CV_RGB2GRAY: case CV_RGBA2GRAY:
      CV_Assert(scn == 3 || scn == 4);
      _dst.create(sz, CV_MAKETYPE(depth, 1));
      dst = _dst.getMat();
      cvtBGRtoGray(src.data, src.step, dst.data, dst.step, src.cols, src.rows, depth, scn,
                   swapBlue(code));
      break;

    case CV_GRAY2BGR: case CV_GRAY2BGRA:
      if (dcn <= 0) dcn = (code == CV_GRAY2BGRA) ? 4 : 3;
      CV_Assert(scn == 1 && (dcn == 3 || dcn == 4));
      _dst.create(sz, CV_MAKETYPE(depth, dcn));
      dst = _dst.getMat();
      cvtGraytoBGR(src.data, src.step, dst.data, dst.step, src.cols, src.rows, depth, dcn);
      break;

    case CV_YUV2BGR_NV21:  case CV_YUV2RGB_NV21:  case CV_YUV2BGR_NV12:  case CV_YUV2RGB_NV12:
      // case CV_YUV2BGRA_NV21: case CV_YUV2RGBA_NV21: case CV_YUV2BGRA_NV12: case CV_YUV2RGBA_NV12:
      // http://www.fourcc.org/yuv.php#NV21 == yuv420sp -> a plane of 8 bit Y samples followed by an interleaved V/U plane containing 8 bit 2x2 subsampled chroma samples
      // http://www.fourcc.org/yuv.php#NV12 -> a plane of 8 bit Y samples followed by an interleaved U/V plane containing 8 bit 2x2 subsampled colour difference samples
      if (dcn <= 0) dcn = (code==CV_YUV420sp2BGRA || code==CV_YUV420sp2RGBA || code==CV_YUV2BGRA_NV12 || code==CV_YUV2RGBA_NV12) ? 4 : 3;
          uidx = (code==CV_YUV2BGR_NV21 || code==CV_YUV2BGRA_NV21 || code==CV_YUV2RGB_NV21 || code==CV_YUV2RGBA_NV21) ? 1 : 0;
      CV_Assert( dcn == 3 || dcn == 4 );
      CV_Assert( sz.width % 2 == 0 && sz.height % 3 == 0 && depth == CV_8U );
      _dst.create(cv::Size(sz.width, sz.height * 2 / 3), CV_MAKETYPE(depth, dcn));
      dst = _dst.getMat();
      cvtTwoPlaneYUVtoBGR(src.data, src.step, dst.data, dst.step, dst.cols, dst.rows,
                          dcn, swapBlue(code), uidx);
      break;

    case CV_YUV2BGR_IYUV: case CV_YUV2RGB_IYUV: case CV_YUV2BGRA_IYUV: case CV_YUV2RGBA_IYUV:
      //http://www.fourcc.org/yuv.php#YV12 == yuv420p -> It comprises an NxM Y plane followed by (N/2)x(M/2) V and U planes.
      //http://www.fourcc.org/yuv.php#IYUV == I420 -> It comprises an NxN Y plane followed by (N/2)x(N/2) U and V planes
      if (dcn <= 0) dcn = (code==CV_YUV2BGRA_YV12 || code==CV_YUV2RGBA_YV12 || code==CV_YUV2RGBA_IYUV || code==CV_YUV2BGRA_IYUV) ? 4 : 3;
      uidx  = (code==CV_YUV2BGR_YV12 || code==CV_YUV2RGB_YV12 || code==CV_YUV2BGRA_YV12 || code==CV_YUV2RGBA_YV12) ? 1 : 0;
      CV_Assert( dcn == 3 || dcn == 4 );
      CV_Assert( sz.width % 2 == 0 && sz.height % 3 == 0 && depth == CV_8U );
      _dst.create(cv::Size(sz.width, sz.height * 2 / 3), CV_MAKETYPE(depth, dcn));
      dst = _dst.getMat();
      cvtThreePlaneYUVtoBGR(src.data, src.step, dst.data, dst.step, dst.cols, dst.rows,
                                 dcn, swapBlue(code), uidx);
      break;

    default:
      CV_Error(CV_StsBadFlag, "Unknown/unsupported color conversion code");
  }
}
// clang-format on