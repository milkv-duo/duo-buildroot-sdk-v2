#pragma once
#include "opencv2/core.hpp"

// clang-format off

/* From imgproc.h */
//! interpolation algorithm
enum InterpolationFlags{
  /** nearest neighbor interpolation */
  INTER_NEAREST        = 0,
  /** bilinear interpolation */
  INTER_LINEAR         = 1,
  /** bicubic interpolation */
  INTER_CUBIC          = 2,
  /** resampling using pixel area relation. It may be a preferred method for image decimation, as
  it gives moire'-free results. But when the image is zoomed, it is similar to the INTER_NEAREST
  method. */
  INTER_AREA           = 3,
  /** Lanczos interpolation over 8x8 neighborhood */
  INTER_LANCZOS4       = 4,
  /** mask for interpolation codes */
  INTER_MAX            = 7,
  /** flag, fills all of the destination image pixels. If some of them correspond to outliers in the
  source image, they are set to zero */
  WARP_FILL_OUTLIERS   = 8,
  /** flag, inverse transformation

  For example, @ref cv::linearPolar or @ref cv::logPolar transforms:
  - flag is __not__ set: \f$dst( \rho , \phi ) = src(x,y)\f$
  - flag is set: \f$dst(x,y) = src( \rho , \phi )\f$
  */
  WARP_INVERSE_MAP     = 16
};

/* From imgproc.h */
enum InterpolationMasks {
  INTER_BITS = 5,
  INTER_BITS2 = INTER_BITS * 2,
  INTER_TAB_SIZE = 1 << INTER_BITS,
  INTER_TAB_SIZE2 = INTER_TAB_SIZE * INTER_TAB_SIZE
};

// clang-format on

namespace cvitdl {

void resize(cv::InputArray src, cv::OutputArray dst, cv::Size dsize, double fx = 0, double fy = 0,
            int interpolation = INTER_LINEAR);

void warpPerspective(cv::InputArray src, cv::OutputArray dst, cv::InputArray M, cv::Size dsize,
                     int flags = INTER_LINEAR, int borderMode = cv::BORDER_CONSTANT,
                     const cv::Scalar& borderValue = cv::Scalar());
void warpAffine(cv::InputArray src, cv::OutputArray dst, cv::InputArray M, cv::Size dsize,
                int flags = INTER_LINEAR, int borderMode = cv::BORDER_CONSTANT,
                const cv::Scalar& borderValue = cv::Scalar());

cv::Mat getAffineTransform(const cv::Point2f src[], const cv::Point2f dst[]);
cv::Mat getPerspectiveTransform(const cv::Point2f src[], const cv::Point2f dst[]);
cv::Mat getPerspectiveTransform(cv::InputArray _src, cv::InputArray _dst);
cv::Mat getAffineTransform(cv::InputArray _src, cv::InputArray _dst);

}  // namespace cvitdl
