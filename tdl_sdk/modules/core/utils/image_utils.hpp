#pragma once
#include "core/core/cvtdl_core_types.h"
#include "face_utils.hpp"

namespace cvitdl {

CVI_S32 ALIGN_FACE_TO_FRAME(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                            cvtdl_face_info_t &face_info);

CVI_S32 crop_image(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst, cvtdl_bbox_t *bbox,
                   bool cvtRGB888 = false);

CVI_S32 crop_image_exten(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst, cvtdl_bbox_t *bbox,
                         bool cvtRGB888, float exten_ratio, float *offset_x, float *offset_y);

CVI_S32 crop_image_face(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_image_t *dst,
                        cvtdl_face_info_t *face_info, bool align = false, bool cvtRGB888 = false);

CVI_S32 face_quality_laplacian(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_info_t *face_info,
                               float *score);

}  // namespace cvitdl