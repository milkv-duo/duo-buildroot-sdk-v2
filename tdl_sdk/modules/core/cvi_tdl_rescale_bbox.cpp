#include "core/cvi_tdl_rescale_bbox.h"
#include "core/core/cvtdl_errno.h"
#include "utils/core_utils.hpp"
// #include "utils/face_utils.hpp"

CVI_S32 CVI_TDL_RescaleMetaCenterCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face) {
  if (frame->stVFrame.u32Width == face->width && frame->stVFrame.u32Height == face->height) {
    return CVI_TDL_SUCCESS;
  }
  for (uint32_t i = 0; i < face->size; i++) {
    cvitdl::info_rescale_nocopy_c(face->width, face->height, frame->stVFrame.u32Width,
                                  frame->stVFrame.u32Height, &face->info[i]);
  }
  face->width = frame->stVFrame.u32Width;
  face->height = frame->stVFrame.u32Height;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_RescaleMetaCenterCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj) {
  if (frame->stVFrame.u32Width == obj->width && frame->stVFrame.u32Height == obj->height) {
    return CVI_TDL_SUCCESS;
  }
  float ratio, pad_width, pad_height;
  for (uint32_t i = 0; i < obj->size; i++) {
    obj->info[i].bbox =
        cvitdl::box_rescale_c(frame->stVFrame.u32Width, frame->stVFrame.u32Height, obj->width,
                              obj->height, obj->info[i].bbox, &ratio, &pad_width, &pad_height);
  }
  obj->width = frame->stVFrame.u32Width;
  obj->height = frame->stVFrame.u32Height;
  return CVI_TDL_SUCCESS;
}
CVI_S32 CVI_TDL_RescaleMetaRBCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face) {
  if (frame->stVFrame.u32Width == face->width && frame->stVFrame.u32Height == face->height) {
    return CVI_TDL_SUCCESS;
  }
  for (uint32_t i = 0; i < face->size; i++) {
    cvitdl::info_rescale_nocopy_rb(face->width, face->height, frame->stVFrame.u32Width,
                                   frame->stVFrame.u32Height, &face->info[i]);
  }
  face->width = frame->stVFrame.u32Width;
  face->height = frame->stVFrame.u32Height;
  return CVI_TDL_SUCCESS;
}
CVI_S32 CVI_TDL_RescaleMetaRBCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj) {
  if (frame->stVFrame.u32Width == obj->width && frame->stVFrame.u32Height == obj->height) {
    return CVI_TDL_SUCCESS;
  }
  float ratio;
  for (uint32_t i = 0; i < obj->size; i++) {
    obj->info[i].bbox = cvitdl::box_rescale_rb(frame->stVFrame.u32Width, frame->stVFrame.u32Height,
                                               obj->width, obj->height, obj->info[i].bbox, &ratio);
  }
  obj->width = frame->stVFrame.u32Width;
  obj->height = frame->stVFrame.u32Height;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_RescaleMetaCenterFace(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face) {
  return CVI_TDL_RescaleMetaCenterCpp(frame, face);
}
CVI_S32 CVI_TDL_RescaleMetaCenterObj(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj) {
  return CVI_TDL_RescaleMetaCenterCpp(frame, obj);
}
CVI_S32 CVI_TDL_RescaleMetaRBFace(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face) {
  return CVI_TDL_RescaleMetaRBCpp(frame, face);
}
CVI_S32 CVI_TDL_RescaleMetaRBObj(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj) {
  return CVI_TDL_RescaleMetaRBCpp(frame, obj);
}
