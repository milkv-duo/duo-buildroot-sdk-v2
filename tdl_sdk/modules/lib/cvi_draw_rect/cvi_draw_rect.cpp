#include "cvi_draw_rect.h"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "draw_rect/draw_rect.hpp"

template <typename T>
inline CVI_S32 Lib_DrawRect(const T *meta, VIDEO_FRAME_INFO_S *frame, const bool drawText,
                            cvtdl_service_brush_t brush) {
  if (meta->size <= 0) return CVI_TDL_SUCCESS;

  std::vector<cvtdl_service_brush_t> brushes(meta->size, brush);
  return cvitdl::service::DrawMeta(meta, frame, drawText, brushes);
}

template <typename T>
inline CVI_S32 Lib_DrawRect(const T *meta, VIDEO_FRAME_INFO_S *frame, const bool drawText,
                            cvtdl_service_brush_t *brushes) {
  if (meta->size <= 0) return CVI_TDL_SUCCESS;

  std::vector<cvtdl_service_brush_t> vec_brushes(brushes, brushes + meta->size);
  return cvitdl::service::DrawMeta(meta, frame, drawText, vec_brushes);
}

CVI_S32 CVI_TDL_FaceDrawRect(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                             const bool drawText, cvtdl_service_brush_t brush) {
  return Lib_DrawRect(meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_FaceDrawRect2(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                              const bool drawText, cvtdl_service_brush_t *brush) {
  return Lib_DrawRect(meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_ObjectDrawRect(const cvtdl_object_t *meta, VIDEO_FRAME_INFO_S *frame,
                               const bool drawText, cvtdl_service_brush_t brush) {
  return Lib_DrawRect(meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_ObjectDrawRect2(const cvtdl_object_t *meta, VIDEO_FRAME_INFO_S *frame,
                                const bool drawText, cvtdl_service_brush_t *brush) {
  return Lib_DrawRect(meta, frame, drawText, brush);
}
