#pragma once
#include "core/face/cvtdl_face_types.h"
#include "opencv2/core.hpp"

#include "cvi_comm.h"

namespace cvitdl {
int face_align(const cv::Mat &image, cv::Mat &aligned, const cvtdl_face_info_t &face_info);
int face_align_gdc(const VIDEO_FRAME_INFO_S *inFrame, VIDEO_FRAME_INFO_S *outFrame,
                   const cvtdl_face_info_t &face_info);
}  // namespace cvitdl
