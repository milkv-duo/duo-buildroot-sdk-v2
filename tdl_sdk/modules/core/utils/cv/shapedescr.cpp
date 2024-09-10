#include "shapedescr.hpp"
// #include "utils.hpp"

#define ENABLE_maskBoundingRect 0

// #include "opencv2/core/private.hpp"
/* IEEE754 constants and macros */
#define CV_TOGGLE_FLT(x) ((x) ^ ((int)(x) < 0 ? 0x7fffffff : 0))

// Calculates bounding rectagnle of a point set or retrieves already calculated
// L462
static cv::Rect pointSetBoundingRect(const cv::Mat& points) {
  int npoints = points.checkVector(2);
  int depth = points.depth();
  CV_Assert(npoints >= 0 && (depth == CV_32F || depth == CV_32S));

  int xmin = 0, ymin = 0, xmax = -1, ymax = -1, i;
  bool is_float = depth == CV_32F;

  if (npoints == 0) return cv::Rect();

  const cv::Point* pts = points.ptr<cv::Point>();
  cv::Point pt = pts[0];

#if CV_SSE4_2
#endif
  {
    if (!is_float) {
      xmin = xmax = pt.x;
      ymin = ymax = pt.y;

      for (i = 1; i < npoints; i++) {
        pt = pts[i];

        if (xmin > pt.x) xmin = pt.x;

        if (xmax < pt.x) xmax = pt.x;

        if (ymin > pt.y) ymin = pt.y;

        if (ymax < pt.y) ymax = pt.y;
      }
    } else {
      CV_Assert(0);
    }
  }

  return cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
}

#if ENABLE_maskBoundingRect
// L582
static cv::Rect maskBoundingRect(const cv::Mat& img) {
  CV_Assert(img.depth() <= CV_8S && img.channels() == 1);

  cv::Size size = img.size();
  int xmin = size.width, ymin = -1, xmax = -1, ymax = -1, i, j, k;

  for (i = 0; i < size.height; i++) {
    const uchar* _ptr = img.ptr(i);
    const uchar* ptr = (const uchar*)cv::alignPtr(_ptr, 4);
    int have_nz = 0, k_min, offset = (int)(ptr - _ptr);
    j = 0;
    offset = MIN(offset, size.width);
    for (; j < offset; j++)
      if (_ptr[j]) {
        have_nz = 1;
        break;
      }
    if (j < offset) {
      if (j < xmin) xmin = j;
      if (j > xmax) xmax = j;
    }
    if (offset < size.width) {
      xmin -= offset;
      xmax -= offset;
      size.width -= offset;
      j = 0;
      for (; j <= xmin - 4; j += 4)
        if (*((int*)(ptr + j))) break;
      for (; j < xmin; j++)
        if (ptr[j]) {
          xmin = j;
          if (j > xmax) xmax = j;
          have_nz = 1;
          break;
        }
      k_min = MAX(j - 1, xmax);
      k = size.width - 1;
      for (; k > k_min && (k & 3) != 3; k--)
        if (ptr[k]) break;
      if (k > k_min && (k & 3) == 3) {
        for (; k > k_min + 3; k -= 4)
          if (*((int*)(ptr + k - 3))) break;
      }
      for (; k > k_min; k--)
        if (ptr[k]) {
          xmax = k;
          have_nz = 1;
          break;
        }
      if (!have_nz) {
        j &= ~3;
        for (; j <= k - 3; j += 4)
          if (*((int*)(ptr + j))) break;
        for (; j <= k; j++)
          if (ptr[j]) {
            have_nz = 1;
            break;
          }
      }
      xmin += offset;
      xmax += offset;
      size.width += offset;
    }
    if (have_nz) {
      if (ymin < 0) ymin = i;
      ymax = i;
    }
  }

  if (xmin >= size.width) xmin = ymin = 0;
  return cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
}
#endif

/* Calculates bounding rectagnle of a point set or retrieves already calculated */
// L1026
CvRect cvBoundingRect(CvArr* array, int update) {
  CvRect rect;
  CvContour contour_header;
  CvSeq* ptseq = 0;
  // CvSeqBlock block;             /* not used */

  CvMat stub, *mat = 0;
  int calculate = update;

  if (CV_IS_SEQ(array)) {
    ptseq = (CvSeq*)array;
    if (!CV_IS_SEQ_POINT_SET(ptseq)) CV_Error(CV_StsBadArg, "Unsupported sequence type");

    if (ptseq->header_size < (int)sizeof(CvContour)) {
      update = 0;
      calculate = 1;
    }
  } else {
    CV_Assert(0);
  }

  if (!calculate) return ((CvContour*)ptseq)->rect;

  if (mat) {
    CV_Assert(0);
#if ENABLE_maskBoundingRect
    rect = maskBoundingRect(cv::cvarrToMat(mat));
#endif
  } else if (ptseq->total) {
    cv::AutoBuffer<double> abuf;
    rect = pointSetBoundingRect(cv::cvarrToMat(ptseq, false, false, 0, &abuf));
  }
  if (update) ((CvContour*)ptseq)->rect = rect;
  return rect;
}

// L677
cv::Rect cvitdl::boundingRect(cv::InputArray array) {
  cv::Mat m = array.getMat();
#if ENABLE_maskBoundingRect
  return m.depth() <= CV_8U ? maskBoundingRect(m) : pointSetBoundingRect(m);
#else
  if (m.depth() <= CV_8U) {
    CV_Assert(0);
  }
  return pointSetBoundingRect(m);
#endif
}

// area of a whole sequence
// L313
double cvitdl::contourArea(cv::InputArray _contour, bool oriented) {
  cv::Mat contour = _contour.getMat();
  int npoints = contour.checkVector(2);
  int depth = contour.depth();
  CV_Assert(npoints >= 0 && (depth == CV_32F || depth == CV_32S));

  if (npoints == 0) return 0.;

  double a00 = 0;
  bool is_float = depth == CV_32F;
  const cv::Point* ptsi = contour.ptr<cv::Point>();
  const cv::Point2f* ptsf = contour.ptr<cv::Point2f>();
  cv::Point2f prev = is_float ? ptsf[npoints - 1]
                              : cv::Point2f((float)ptsi[npoints - 1].x, (float)ptsi[npoints - 1].y);

  for (int i = 0; i < npoints; i++) {
    cv::Point2f p = is_float ? ptsf[i] : cv::Point2f((float)ptsi[i].x, (float)ptsi[i].y);
    a00 += (double)prev.x * p.y - (double)prev.y * p.x;
    prev = p;
  }

  a00 *= 0.5;
  if (!oriented) a00 = fabs(a00);

  return a00;
}