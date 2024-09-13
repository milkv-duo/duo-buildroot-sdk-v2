#include "thresh.hpp"
#include "imgproc.hpp"

// #include "opencv2/core/hal/intrin_neon.hpp"

// L53
static void thresh_8u(const cv::Mat& _src, cv::Mat& _dst, uchar thresh, uchar maxval, int type) {
  cv::Size roi = _src.size();
  roi.width *= _src.channels();
  size_t src_step = _src.step;
  size_t dst_step = _dst.step;

  if (_src.isContinuous() && _dst.isContinuous()) {
    roi.width *= roi.height;
    roi.height = 1;
    src_step = dst_step = roi.width;
  }

  int j = 0;
  const uchar* src = _src.ptr();
  uchar* dst = _dst.ptr();
#if CV_SIMD128 /* NOTE: check this */
  bool useSIMD = cv::checkHardwareSupport(CV_CPU_SSE2) || cv::checkHardwareSupport(CV_CPU_NEON);
  if (useSIMD) {
    v_uint8x16 thresh_u = v_setall_u8(thresh);
    v_uint8x16 maxval16 = v_setall_u8(maxval);

    switch (type) {
      case THRESH_BINARY:
        for (int i = 0; i < roi.height; i++, src += src_step, dst += dst_step) {
          for (j = 0; j <= roi.width - 16; j += 16) {
            v_uint8x16 v0;
            v0 = v_load(src + j);
            v0 = thresh_u < v0;
            v0 = v0 & maxval16;
            v_store(dst + j, v0);
          }
        }
        break;

      case THRESH_BINARY_INV:
        CV_Assert(0);
        break;

      case THRESH_TRUNC:
        CV_Assert(0);
        break;

      case THRESH_TOZERO:
        CV_Assert(0);
        break;

      case THRESH_TOZERO_INV:
        CV_Assert(0);
        break;
    }
  }
#endif

  int j_scalar = j;
  if (j_scalar < roi.width) {
    const int thresh_pivot = thresh + 1;
    uchar tab[256];
    switch (type) {
      case THRESH_BINARY:
        memset(tab, 0, thresh_pivot);
        if (thresh_pivot < 256) {
          memset(tab + thresh_pivot, maxval, 256 - thresh_pivot);
        }
        break;
      case THRESH_BINARY_INV:
        CV_Assert(0);
        break;
      case THRESH_TRUNC:
        CV_Assert(0);
        break;
      case THRESH_TOZERO:
        CV_Assert(0);
        break;
      case THRESH_TOZERO_INV:
        CV_Assert(0);
        break;
    }

    src = _src.ptr();
    dst = _dst.ptr();
    for (int i = 0; i < roi.height; i++, src += src_step, dst += dst_step) {
      j = j_scalar;
#if 1 /* CV_ENABLE_UNROLLED */
      for (; j <= roi.width - 4; j += 4) {
        uchar t0 = tab[src[j]];
        uchar t1 = tab[src[j + 1]];

        dst[j] = t0;
        dst[j + 1] = t1;

        t0 = tab[src[j + 2]];
        t1 = tab[src[j + 3]];

        dst[j + 2] = t0;
        dst[j + 3] = t1;
      }
#endif
      for (; j < roi.width; j++) dst[j] = tab[src[j]];
    }
  }
}

// L1154
class ThresholdRunner : public cv::ParallelLoopBody {
 public:
  ThresholdRunner(cv::Mat _src, cv::Mat _dst, double _thresh, double _maxval, int _thresholdType) {
    src = _src;
    dst = _dst;

    thresh = _thresh;
    maxval = _maxval;
    thresholdType = _thresholdType;
  }

  void operator()(const cv::Range& range) const {
    int row0 = range.start;
    int row1 = range.end;

    cv::Mat srcStripe = src.rowRange(row0, row1);
    cv::Mat dstStripe = dst.rowRange(row0, row1);

    if (srcStripe.depth() == CV_8U) {
      thresh_8u(srcStripe, dstStripe, (uchar)thresh, (uchar)maxval, thresholdType);
    } else if (srcStripe.depth() == CV_16S) {
      CV_Assert(0);
    } else if (srcStripe.depth() == CV_32F) {
      CV_Assert(0);
    } else if (srcStripe.depth() == CV_64F) {
      CV_Assert(0);
    }
  }

 private:
  cv::Mat src;
  cv::Mat dst;

  double thresh;
  double maxval;
  int thresholdType;
};

// L1342
double _threshold(cv::InputArray _src, cv::OutputArray _dst, double thresh, double maxval,
                  int type) {
  cv::Mat src = _src.getMat();
  int automatic_thresh = (type & ~CV_THRESH_MASK);
  type &= THRESH_MASK;

  CV_Assert(automatic_thresh != (CV_THRESH_OTSU | CV_THRESH_TRIANGLE));
  if (automatic_thresh == CV_THRESH_OTSU) {
    CV_Assert(0);
  } else if (automatic_thresh == CV_THRESH_TRIANGLE) {
    CV_Assert(0);
  }

  _dst.create(src.size(), src.type());
  cv::Mat dst = _dst.getMat();

  if (src.depth() == CV_8U) {
    int ithresh = cvFloor(thresh);
    thresh = ithresh;
    int imaxval = cvRound(maxval);
    if (type == THRESH_TRUNC) imaxval = ithresh;
    imaxval = cv::saturate_cast<uchar>(imaxval);

    if (ithresh < 0 || ithresh >= 255) {
      CV_Assert(0);
    }

    thresh = ithresh;
    maxval = imaxval;
  } else if (src.depth() == CV_16S) {
    CV_Assert(0);
  } else if (src.depth() == CV_32F) {
    CV_Assert(0);
  } else if (src.depth() == CV_64F) {
    CV_Assert(0);
  } else
    CV_Error(CV_StsUnsupportedFormat, "");

  parallel_for_(cv::Range(0, dst.rows), ThresholdRunner(src, dst, thresh, maxval, type),
                dst.total() / (double)(1 << 16));
  return thresh;
}

// L1510
double cvThreshold(const void* srcarr, void* dstarr, double thresh, double maxval, int type) {
  cv::Mat src = cv::cvarrToMat(srcarr), dst = cv::cvarrToMat(dstarr), dst0 = dst;

  CV_Assert(src.size == dst.size && src.channels() == dst.channels() &&
            (src.depth() == dst.depth() || dst.depth() == CV_8U));

  thresh = _threshold(src, dst, thresh, maxval, type);
  if (dst0.data != dst.data) dst.convertTo(dst0, dst0.depth());
  return thresh;
}