#pragma once
#include "core/face/cvtdl_face_types.h"
#include "opencv2/core/core.hpp"

namespace cvitdl {
namespace service {

// Implementation of Determining the gaze of faces in images
// x1, x2, x3, x4, x5, y1, y2, y3, y4, y5
// (x1, y1) = Left eye center
// (x2, y2) = Right eye center
// (x3, y3) = Nose tip
// (x4, y4) = Left Mouth corner
// (x5, y5) = Right mouth corner
int Predict(const cvtdl_pts_t *pFacial5points, cvtdl_head_pose_t *hp);

// Camera-centered coordinate system
// x & y axis aligned along the horizontal and vertical directions in the image.
// z axis along the normal to the image plain.
int Predict3DFacialNormal(const cv::Point &noseTip, const cv::Point &noseBase,
                          const cv::Point &midEye, const cv::Point &midMouth,
                          cvtdl_head_pose_t *hp);

float CalDistance(const cv::Point &p1, const cv::Point &p2);
float CalAngle(const cv::Point &pt1, const cv::Point &pt2);
float CalSlant(int ln, int lf, const float Rn, float theta);
}  // namespace service
}  // namespace cvitdl
