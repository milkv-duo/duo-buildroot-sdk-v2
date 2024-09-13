#ifndef _CVI_DARW_RECT_HEAD_
#define _CVI_DARW_RECT_HEAD_

#include <linux/cvi_type.h>
#define DLL_EXPORT __attribute__((visibility("default")))
#include "cvi_tdl.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Draw rect to frame with given face meta with a global brush.
 * @ingroup core_cvitdlservice
 *
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the face.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceDrawRect(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                                               const bool drawText, cvtdl_service_brush_t brush);

/**
 * @brief Draw rect to frame with given face meta with individual brushes.
 * @ingroup core_cvitdlservice
 *
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the face.
 * @param brushes brushes for drawing. The count of brushes must be same as meta->size.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_FaceDrawRect2(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                                                const bool drawText, cvtdl_service_brush_t *brushes);

/**
 * @brief Draw rect to frame with given object meta with a global brush.
 * @ingroup core_cvitdlservice
 *
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_ObjectDrawRect(const cvtdl_object_t *meta,
                                                 VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                                 cvtdl_service_brush_t brush);

/**
 * @brief Draw rect to frame with given object meta with individual brushes.
 * @ingroup core_cvitdlservice
 *
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @param brushes brushes for drawing. The count of brushes must be same as meta->size.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_ObjectDrawRect2(const cvtdl_object_t *meta,
                                                  VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                                  cvtdl_service_brush_t *brushes);


#ifdef __cplusplus
}
#endif
#endif