#ifndef _AFFINIE_HW_H
#define _AFFINIE_HW_H_

#include "opencv2/opencv.hpp"

#include "cvi_type.h"
#include "cvi_sys.h"

#include "type_define.h"

int face_align_gdc(const VIDEO_FRAME_INFO_S *inFrame, VIDEO_FRAME_INFO_S *outFrame,
                   const face_rect_t &face_info);
#endif
