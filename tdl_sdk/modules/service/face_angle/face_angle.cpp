#include "face_angle.hpp"
#include <cmath>
#include "core/core/cvtdl_errno.h"
// include core_c.h if opencv version greater than 4.5
#if CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 5
#include "opencv2/core/core_c.h"
#endif

namespace cvitdl {
namespace service {

template <typename T>
T Saturate(const T& val, const T& minVal, const T& maxVal) {
  return std::min(std::max(val, minVal), maxVal);
}

// x1, x2, x3, x4, x5, y1, y2, y3, y4, y5
// (x1, y1) = Left eye center
// (x2, y2) = Right eye center
// (x3, y3) = Nose tip
// (x4, y4) = Left Mouth corner
// (x5, y5) = Right mouth corner
int Predict(const cvtdl_pts_t* pFacial5points, cvtdl_head_pose_t* hp) {
  cv::Point leye = cv::Point(pFacial5points->x[0], pFacial5points->y[0]);
  cv::Point reye = cv::Point(pFacial5points->x[1], pFacial5points->y[1]);
  cv::Point lmouth = cv::Point(pFacial5points->x[3], pFacial5points->y[3]);
  cv::Point rmouth = cv::Point(pFacial5points->x[4], pFacial5points->y[4]);
  cv::Point noseTip = cv::Point(pFacial5points->x[2], pFacial5points->y[2]);
  cv::Point midEye = cv::Point((leye.x + reye.x) * 0.5, (leye.y + reye.y) * 0.5);
  cv::Point midMouth = cv::Point((lmouth.x + rmouth.x) * 0.5, (lmouth.y + rmouth.y) * 0.5);
  cv::Point noseBase = cv::Point((midMouth.x + midEye.x) * 0.5, (midMouth.y + midEye.y) * 0.5);

  Predict3DFacialNormal(noseTip, noseBase, midEye, midMouth, hp);

  hp->yaw = acos((std::abs(hp->facialUnitNormalVector[2])) /
                 (std::sqrt(hp->facialUnitNormalVector[0] * hp->facialUnitNormalVector[0] +
                            hp->facialUnitNormalVector[2] * hp->facialUnitNormalVector[2])));
  if (noseTip.x < noseBase.x) hp->yaw = -hp->yaw;
  hp->yaw = Saturate(hp->yaw, -1.f, 1.f);

  hp->pitch = acos(std::sqrt((hp->facialUnitNormalVector[0] * hp->facialUnitNormalVector[0] +
                              hp->facialUnitNormalVector[2] * hp->facialUnitNormalVector[2]) /
                             (hp->facialUnitNormalVector[0] * hp->facialUnitNormalVector[0] +
                              hp->facialUnitNormalVector[1] * hp->facialUnitNormalVector[1] +
                              hp->facialUnitNormalVector[2] * hp->facialUnitNormalVector[2])));
  if (noseTip.y > noseBase.y) hp->pitch = -hp->pitch;
  hp->pitch = Saturate(hp->pitch, -1.f, 1.f);

  hp->roll = CalAngle(leye, reye);
  if (hp->roll > 180) hp->roll = hp->roll - 360;
  hp->roll /= 90;
  hp->roll = Saturate(hp->roll, -1.f, 1.f);

  return CVI_TDL_SUCCESS;
}

int Predict3DFacialNormal(const cv::Point& noseTip, const cv::Point& noseBase,
                          const cv::Point& midEye, const cv::Point& midMouth,
                          cvtdl_head_pose_t* hp) {
  float noseBase_noseTip_distance = CalDistance(noseTip, noseBase);
  float midEye_midMouth_distance = CalDistance(midEye, midMouth);

  // Angle facial middle (symmetric) line.
  float symm = CalAngle(noseBase, midEye);

  // Angle between 2D image facial normal & x-axis.
  float tilt = CalAngle(noseBase, noseTip);

  // Angle between 2D image facial normal & facial middle (symmetric) line.
  float theta = (std::abs(tilt - symm)) * (PI / 180.0);

  // Angle between 3D image facial normal & image plain normal (optical axis).
  float slant = CalSlant(noseBase_noseTip_distance, midEye_midMouth_distance, 0.5, theta);

  // Define a 3D vector for the facial normal
  hp->facialUnitNormalVector[0] = sin(slant) * (cos((360 - tilt) * (PI / 180.0)));
  hp->facialUnitNormalVector[1] = sin(slant) * (sin((360 - tilt) * (PI / 180.0)));
  hp->facialUnitNormalVector[2] = -cos(slant);

  return CVI_TDL_SUCCESS;
}

float CalDistance(const cv::Point& p1, const cv::Point& p2) {
  float x = p1.x - p2.x;
  float y = p1.y - p2.y;
  return sqrtf(x * x + y * y);
}

float CalAngle(const cv::Point& pt1, const cv::Point& pt2) {
  return 360 - cvFastArctan(pt2.y - pt1.y, pt2.x - pt1.x);
}

float CalSlant(int ln, int lf, const float Rn, float theta) {
  float dz = 0;
  float slant = 0;
  const float m1 = ((float)ln * ln) / ((float)lf * lf);
  const float m2 = (cos(theta)) * (cos(theta));
  const float Rn_sq = Rn * Rn;

  if (m2 == 1) {
    dz = sqrt(Rn_sq / (m1 + Rn_sq));
  }
  if (m2 >= 0 && m2 < 1) {
    dz = sqrt(
        (Rn_sq - m1 - 2 * m2 * Rn_sq + sqrt(((m1 - Rn_sq) * (m1 - Rn_sq)) + 4 * m1 * m2 * Rn_sq)) /
        (2 * (1 - m2) * Rn_sq));
  }
  slant = acos(dz);
  return slant;
}
}  // namespace service
}  // namespace cvitdl
