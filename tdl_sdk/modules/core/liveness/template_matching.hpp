#pragma once

#include <stdlib.h>
#include <opencv2/core.hpp>
#include "core/face/cvtdl_face_types.h"

cv::Mat template_matching(const cv::Mat &crop_rgb_frame, const cv::Mat &ir_frame, cv::Rect box,
                          cvtdl_liveness_ir_position_e ir_pos);
std::vector<cv::Mat> TTA_9_cropps(cv::Mat image);
