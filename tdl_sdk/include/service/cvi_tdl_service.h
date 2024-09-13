#ifndef _CVI_TDL_OBJSERVICE_H_
#define _CVI_TDL_OBJSERVICE_H_
#include "service/cvi_tdl_service_types.h"

#include "core/cvi_tdl_core.h"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

/** @typedef cvitdl_service_handle_t
 *  @ingroup core_cvitdlservice
 *  @brief A cvitdl objservice handle.
 */
typedef void *cvitdl_service_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a cvitdl_service_handle_t.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param tdl_handle A cvitdl handle.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_CreateHandle(cvitdl_service_handle_t *handle,
                                                cvitdl_handle_t tdl_handle);

/**
 * @brief Destroy a cvitdl_service_handle_t.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if success to destroy handle.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_DestroyHandle(cvitdl_service_handle_t handle);

/**
 * @brief Register a feature array to OBJ Service.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param featureArray Input registered feature array.
 * @param method Set feature matching method.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_RegisterFeatureArray(
    cvitdl_service_handle_t handle, const cvtdl_service_feature_array_t featureArray,
    const cvtdl_service_feature_matching_e method);

/**
 * @brief Do a single cvitdl_face_t feature matching with registed feature array.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param object_info The cvtdl_object_info_t from NN output with feature data.
 * @param threshold threshold. Set 0 to ignore.
 * @param topk top-k matching results. Set 0 to ignore.
 * @param indices Output top k indices. Array size should be same as number of dataset features if
 * topk is ignored.
 * @param sims Output simlarities. Array size should be same as number of dataset features if topk
 * is ignored.
 * @param size Output length of indices and sims array
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceInfoMatching(cvitdl_service_handle_t handle,
                                                    const cvtdl_face_info_t *face_info,
                                                    const uint32_t topk, float threshold,
                                                    uint32_t *indices, float *sims, uint32_t *size);

/**
 * @brief Do a single cvtdl_object_info_t feature matching with registed feature array.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param object_info The cvtdl_object_info_t from NN output with feature data.
 * @param topk top-k matching results. Set 0 to ignore.
 * @param threshold threshold. Set 0 to ignore.
 * @param indices Output top k indices. Array size should be same as number of dataset features if
 * topk is ignored.
 * @param sims Output similarities. Array size should be same as number of dataset features if topk
 * is ignored.
 * @param size Output length of indices and sims array
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectInfoMatching(cvitdl_service_handle_t handle,
                                                      const cvtdl_object_info_t *object_info,
                                                      const uint32_t topk, float threshold,
                                                      uint32_t *indices, float *sims,
                                                      uint32_t *size);

/**
 * @brief Calculate similarity of two features
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param feature_rhs Righ hand side feature vector.
 * @param feature_lhs Left hand side feature vector.
 * @param score Output similarities of two features
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_CalculateSimilarity(cvitdl_service_handle_t handle,
                                                       const cvtdl_feature_t *feature_rhs,
                                                       const cvtdl_feature_t *feature_lhs,
                                                       float *score);

/**
 * @brief Do a single raw data with registed feature array.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param feature Raw feature vector.
 * @param type The data type of the feature vector.
 * @param topk Output top k results.
 * @param threshold threshold. Set 0 to ignore.
 * @param indices Output top k indices. Array size should be same as number of dataset features if
 * topk is ignored.
 * @param sims Output similarities. Array size should be same as number of dataset features if topk
 * is ignored.
 * @param size Output length of indices and sims array
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_RawMatching(cvitdl_service_handle_t handle, const void *feature,
                                               const feature_type_e type, const uint32_t topk,
                                               float threshold, uint32_t *indices, float *sims,
                                               uint32_t *size);

/**
 * @brief Zoom in to the union of faces from the output of face detection results.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param inFrame Input frame.
 * @param meta THe result from face detection.
 * @param face_skip_ratio Skip the faces that are too small comparing to the area of the image.
 * Default is 0.05.
 * @param trans_ratio Change to zoom in ratio. Default is 0.1.
 * @param padding_ratio Bounding box padding ratio. Default is 0.3. (0 ~ 1)
 * @param outFrame Output result image, will keep aspect ratio.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDigitalZoom(
    cvitdl_service_handle_t handle, const VIDEO_FRAME_INFO_S *inFrame, const cvtdl_face_t *meta,
    const float face_skip_ratio, const float trans_ratio, const float padding_ratio,
    VIDEO_FRAME_INFO_S *outFrame);

/**
 * @brief Zoom in to the union of objects from the output of object detection results.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param inFrame Input frame.
 * @param meta THe result from face detection.
 * @param obj_skip_ratio Skip the objects that are too small comparing to the area of the image.
 * Default is 0.05.
 * @param trans_ratio Change to zoom in ratio. Default is 0.1.
 * @param padding_ratio Bounding box padding ratio. Default is 0.3. (0 ~ 1)
 * @param outFrame Output result image, will keep aspect ratio.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectDigitalZoom(
    cvitdl_service_handle_t handle, const VIDEO_FRAME_INFO_S *inFrame, const cvtdl_object_t *meta,
    const float obj_skip_ratio, const float trans_ratio, const float padding_ratio,
    VIDEO_FRAME_INFO_S *outFrame);

/**
 * @brief Zoom in to the union of objects from the output of object detection results.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param inFrame Input frame.
 * @param meta THe result from face detection.
 * @param obj_skip_ratio Skip the objects that are too small comparing to the area of the image.
 * Default is 0.05.
 * @param trans_ratio Change to zoom in ratio. Default is 0.1.
 * @param pad_ratio_left Left bounding box padding ratio. Default is 0.3. (-1 ~ 1)
 * @param pad_ratio_right Right bounding box padding ratio. Default is 0.3. (-1 ~ 1)
 * @param pad_ratio_top Top bounding box padding ratio. Default is 0.3. (-1 ~ 1)
 * @param pad_ratio_bottom Bottom bounding box padding ratio. Default is 0.3. (-1 ~ 1)
 * @param outFrame Output result image, will keep aspect ratio.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectDigitalZoomExt(
    cvitdl_service_handle_t handle, const VIDEO_FRAME_INFO_S *inFrame, const cvtdl_object_t *meta,
    const float obj_skip_ratio, const float trans_ratio, const float pad_ratio_left,
    const float pad_ratio_right, const float pad_ratio_top, const float pad_ratio_bottom,
    VIDEO_FRAME_INFO_S *outFrame);

/**
 * @brief Draw rect to frame with given face meta with a global brush.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the face.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDrawRect(cvitdl_service_handle_t handle,
                                                const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame,
                                                const bool drawText, cvtdl_service_brush_t brush);

/**
 * @brief Draw rect to frame with given face meta with individual brushes.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the face.
 * @param brushes brushes for drawing. The count of brushes must be same as meta->size.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDrawRect2(cvitdl_service_handle_t handle,
                                                 const cvtdl_face_t *meta,
                                                 VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                                 cvtdl_service_brush_t *brushes);

DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDraw5Landmark(const cvtdl_face_t *meta,
                                                     VIDEO_FRAME_INFO_S *frame);

/**
 * @brief Draw rect to frame with given object meta with a global brush.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectDrawRect(cvitdl_service_handle_t handle,
                                                  const cvtdl_object_t *meta,
                                                  VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                                  cvtdl_service_brush_t brush);

/**
 * @brief Draw rect to frame with given object meta with individual brushes.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @param brushes brushes for drawing. The count of brushes must be same as meta->size.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectDrawRect2(cvitdl_service_handle_t handle,
                                                   const cvtdl_object_t *meta,
                                                   VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                                   cvtdl_service_brush_t *brushes);

/**
 * @brief Draw rect to frame with given dms meta.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_Incar_ObjectDrawRect(cvitdl_service_handle_t handle,
                                                        const cvtdl_dms_od_t *meta,
                                                        VIDEO_FRAME_INFO_S *frame,
                                                        const bool drawText,
                                                        cvtdl_service_brush_t brush);

/**
 * @brief Draw a closed polygon to frame.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param frame In/out YUV frame.
 * @param pts Points of a closed polygon.
 * @param brush A brush for drawing
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_DrawPolygon(cvitdl_service_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame, const cvtdl_pts_t *pts,
                                               cvtdl_service_brush_t brush);

/**
 * @brief Draw text to YUV frame with given text.
 * @ingroup core_cvitdlservice
 *
 * @paran name text content.
 * @param x the x coordinate of content.
 * @param y the y coordinate of content.
 * @param frame In/ out YUV frame.
 * @param drawText Choose to draw name of the object.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectWriteText(char *name, int x, int y,
                                                   VIDEO_FRAME_INFO_S *frame, float r, float g,
                                                   float b);

/**
 * @brief Set target convex polygon for detection.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param pts polygon points. (pts->size must larger than 2.)
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_SetTarget(cvitdl_service_handle_t handle,
                                                     const cvtdl_pts_t *pts);

/**
 * @brief Get the convex polygons.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param region_pts the vertices of regions.
 * @param size the number of regions.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_GetTarget(cvitdl_service_handle_t handle,
                                                     cvtdl_pts_t ***regions_pts, uint32_t *size);

/**
 * @brief Clean all of the convex polygons.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_CleanAll(cvitdl_service_handle_t handle);

/**
 * @brief Check if a convex polygon intersected with target convex polygon.
 * @ingroup core_cvitdlservice
 *
 * @param handle A service handle.
 * @param bbox Object meta structure.
 * @param has_intersect true if two polygons has intersection, otherwise return false.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_Polygon_Intersect(cvitdl_service_handle_t handle,
                                                     const cvtdl_bbox_t *bbox, bool *has_intersect);

/**
 * @brief Calculate the head pose angle.
 * @ingroup core_cvitdlservice
 *
 * @param pts facial landmark structure.
 * @param hp head pose structure.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceAngle(const cvtdl_pts_t *pts, cvtdl_head_pose_t *hp);

/**
 * @brief Calculate the head pose angle for all faces.
 * @ingroup core_cvitdlservice
 *
 * @param meta Face meta structure.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceAngleForAll(const cvtdl_face_t *meta);

/**
 * @brief Draw limbs to YUV frame with given object pose.
 * @ingroup core_cvitdlservice
 *
 * @param meta meta structure.
 * @param frame In/ out YUV frame.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_Service_ObjectDrawPose(const cvtdl_object_t *meta,
                                                  VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_Service_FaceDrawPts(cvtdl_pts_t *pts, VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT cvtdl_service_brush_t CVI_TDL_Service_GetDefaultBrush();

DLL_EXPORT CVI_S32 CVI_TDL_Service_DrawHandKeypoint(cvitdl_service_handle_t handle,
                                                    VIDEO_FRAME_INFO_S *frame,
                                                    const cvtdl_handpose21_meta_ts *meta);
#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_OBJSERVICE_H_
