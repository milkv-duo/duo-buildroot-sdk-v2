#ifndef _CVI_TDL_TYPES_FREE_H_
#define _CVI_TDL_TYPES_FREE_H_
#include "core/core/cvtdl_core_types.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

#ifdef __cplusplus
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_feature_t *feature);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_pts_t *pts);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_tracker_t *tracker);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_face_info_t *face_info);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_face_t *face);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_object_info_t *obj_info);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_object_t *obj);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_image_t *image);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_dms_od_t *dms_od);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_dms_t *dms);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_handpose21_meta_ts *handposes);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_class_meta_t *cls_meta);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_seg_logits_t *seg_logits);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_lane_t *lane_meta);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_clip_feature *clip_meta);
DLL_EXPORT void CVI_TDL_FreeCpp(cvtdl_seg_t *seg_ann);

DLL_EXPORT void CVI_TDL_CopyInfoCpp(const cvtdl_face_info_t *info, cvtdl_face_info_t *infoNew);
DLL_EXPORT void CVI_TDL_CopyInfoCpp(const cvtdl_dms_od_info_t *info, cvtdl_dms_od_info_t *infoNew);
DLL_EXPORT void CVI_TDL_CopyInfoCpp(const cvtdl_object_info_t *info, cvtdl_object_info_t *infoNew);
#endif

#ifdef __cplusplus
extern "C" {
#endif

DLL_EXPORT void CVI_TDL_FreeFeature(cvtdl_feature_t *feature);
DLL_EXPORT void CVI_TDL_FreePts(cvtdl_pts_t *pts);
DLL_EXPORT void CVI_TDL_FreeTracker(cvtdl_tracker_t *tracker);
DLL_EXPORT void CVI_TDL_FreeFaceInfo(cvtdl_face_info_t *face_info);
DLL_EXPORT void CVI_TDL_FreeFace(cvtdl_face_t *face);
DLL_EXPORT void CVI_TDL_FreeObjectInfo(cvtdl_object_info_t *obj_info);
DLL_EXPORT void CVI_TDL_FreeObject(cvtdl_object_t *obj);
DLL_EXPORT void CVI_TDL_FreeImage(cvtdl_image_t *image);
DLL_EXPORT void CVI_TDL_FreeDMS(cvtdl_dms_t *dms);
DLL_EXPORT void CVI_TDL_FreeHandPoses(cvtdl_handpose21_meta_ts *handposes);
DLL_EXPORT void CVI_TDL_FreeClassMeta(cvtdl_class_meta_t *cls_meta);
DLL_EXPORT void CVI_TDL_FreeSegLogits(cvtdl_seg_logits_t *seg_logits);
DLL_EXPORT void CVI_TDL_FreeLane(cvtdl_lane_t *lane_meta);
DLL_EXPORT void CVI_TDL_FreeClip(cvtdl_clip_feature *clip_meta);

DLL_EXPORT void CVI_TDL_CopyFaceInfo(const cvtdl_face_info_t *src, cvtdl_face_info_t *dst);
DLL_EXPORT void CVI_TDL_CopyObjectInfo(const cvtdl_object_info_t *src, cvtdl_object_info_t *dst);
DLL_EXPORT void CVI_TDL_CopyFaceMeta(const cvtdl_face_t *src, cvtdl_face_t *dst);
DLL_EXPORT void CVI_TDL_CopyObjectMeta(const cvtdl_object_t *src, cvtdl_object_t *dst);
DLL_EXPORT void CVI_TDL_CopyTrackerMeta(const cvtdl_tracker_t *src, cvtdl_tracker_t *dst);
DLL_EXPORT void CVI_TDL_CopyHandPoses(const cvtdl_handpose21_meta_ts *src,
                                      cvtdl_handpose21_meta_ts *dest);
DLL_EXPORT void CVI_TDL_CopyLaneMeta(cvtdl_lane_t *src, cvtdl_lane_t *dst);

DLL_EXPORT void CVI_TDL_CopyImage(const cvtdl_image_t *src_image, cvtdl_image_t *dst_image);
DLL_EXPORT void CVI_TDL_MapImage(VIDEO_FRAME_INFO_S *src_image, bool *p_is_mapped);
DLL_EXPORT void CVI_TDL_UnMapImage(VIDEO_FRAME_INFO_S *src_image);

DLL_EXPORT CVI_S32 CVI_TDL_CopyVpssImage(VIDEO_FRAME_INFO_S *src_image, cvtdl_image_t *dst_image);
#ifdef __cplusplus
}
#endif
#endif
