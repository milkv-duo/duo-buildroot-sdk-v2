#ifndef _CVI_TDL_BBOX_RESCALE_H_
#define _CVI_TDL_BBOX_RESCALE_H_
#include "core/core/cvtdl_core_types.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

#include <cvi_comm.h>
#ifdef __cplusplus
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaCenterCpp(const VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_face_t *face);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaCenterCpp(const VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_object_t *obj);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaRBCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaRBCpp(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj);
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaCenterFace(const VIDEO_FRAME_INFO_S *frame,
                                                 cvtdl_face_t *face);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaCenterObj(const VIDEO_FRAME_INFO_S *frame,
                                                cvtdl_object_t *obj);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaRBFace(const VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face);
DLL_EXPORT CVI_S32 CVI_TDL_RescaleMetaRBObj(const VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj);

#ifdef __cplusplus
}
#endif
#endif  // End of _CVI_TDL_BBOX_RESCALE_H_
