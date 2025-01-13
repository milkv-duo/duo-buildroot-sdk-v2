#include "service/cvi_tdl_service.h"

#ifndef __CV186X__
#include <cvimath/cvimath.h>
#include "feature_matching/feature_matching.hpp"
#endif
#include "area_detect/intrusion_detect.hpp"
#include "cvi_tdl_core_internal.hpp"
#include "digital_tracking/digital_tracking.hpp"
#include "draw_rect/draw_rect.hpp"
#ifndef NO_OPENCV
#include "face_angle/face_angle.hpp"
#endif

typedef struct {
  cvitdl_handle_t tdl_handle = NULL;
#ifndef __CV186X__
  cvitdl::service::FeatureMatching *m_fm = nullptr;
#endif
  cvitdl::service::DigitalTracking *m_dt = nullptr;
  cvitdl::service::IntrusionDetect *m_intrusion_det = nullptr;
} cvitdl_service_context_t;

CVI_S32 CVI_TDL_Service_CreateHandle(cvitdl_service_handle_t *handle, cvitdl_handle_t tdl_handle) {
  if (tdl_handle == NULL) {
    LOGC("tdl_handle is empty.");
    return CVI_TDL_FAILURE;
  }
  cvitdl_service_context_t *ctx = new cvitdl_service_context_t;
  ctx->tdl_handle = tdl_handle;
  *handle = ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_DestroyHandle(cvitdl_service_handle_t handle) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
#ifndef __CV186X__
  delete ctx->m_fm;
#endif
  delete ctx->m_dt;
  delete ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_RegisterFeatureArray(cvitdl_service_handle_t handle,
                                             const cvtdl_service_feature_array_t featureArray,
                                             const cvtdl_service_feature_matching_e method) {
#ifdef __CV186X__
  return CVI_TDL_SUCCESS;
#else
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  int ret = CVI_TDL_SUCCESS;
  if (ctx->m_fm == nullptr) {
    ctx->m_fm = new cvitdl::service::FeatureMatching();
    if ((ret = ctx->m_fm->init()) != CVI_TDL_SUCCESS) {
      LOGE("Feature matching instance initialization failed with %#x!\n", ret);
      delete ctx->m_fm;
      ctx->m_fm = nullptr;
      return ret;
    }
  }
  return ctx->m_fm->registerData(featureArray, method);
#endif
}

CVI_S32 CVI_TDL_Service_CalculateSimilarity(cvitdl_service_handle_t handle,
                                            const cvtdl_feature_t *feature_rhs,
                                            const cvtdl_feature_t *feature_lhs, float *score) {
  if (feature_lhs->type != feature_rhs->type) {
    LOGE("feature type not matched! rhs=%d, lhs=%d\n", feature_rhs->type, feature_lhs->type);
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  if (feature_lhs->size != feature_rhs->size) {
    LOGE("feature size not matched!, rhs: %u, lhs: %u\n", feature_rhs->size, feature_lhs->size);
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  if (feature_rhs->type == TYPE_INT8) {
    int32_t value1 = 0, value2 = 0, value3 = 0;
    for (uint32_t i = 0; i < feature_rhs->size; i++) {
      value1 += (short)feature_rhs->ptr[i] * feature_rhs->ptr[i];
      value2 += (short)feature_lhs->ptr[i] * feature_lhs->ptr[i];
      value3 += (short)feature_rhs->ptr[i] * feature_lhs->ptr[i];
    }

    *score = (float)value3 / (sqrt((double)value1) * sqrt((double)value2));
  } else {
    LOGE("Unsupported feature type: %d\n", feature_rhs->type);
    return CVI_TDL_ERR_INVALID_ARGS;
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_FaceInfoMatching(cvitdl_service_handle_t handle,
                                         const cvtdl_face_info_t *face_info, const uint32_t topk,
                                         float threshold, uint32_t *indices, float *sims,
                                         uint32_t *size) {
#ifdef __CV186X__
  return CVI_TDL_SUCCESS;
#else
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_fm == nullptr) {
    LOGE(
        "Not yet register features, please invoke CVI_TDL_Service_RegisterFeatureArray to "
        "register.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  return ctx->m_fm->run((uint8_t *)face_info->feature.ptr, face_info->feature.type, topk, indices,
                        sims, size, threshold);
#endif
}

CVI_S32 CVI_TDL_Service_ObjectInfoMatching(cvitdl_service_handle_t handle,
                                           const cvtdl_object_info_t *object_info,
                                           const uint32_t topk, float threshold, uint32_t *indices,
                                           float *sims, uint32_t *size) {
#ifdef __CV186X__
  return CVI_TDL_SUCCESS;
#else
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_fm == nullptr) {
    LOGE(
        "Not yet register features, please invoke CVI_TDL_Service_RegisterFeatureArray to "
        "register.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  return ctx->m_fm->run((uint8_t *)object_info->feature.ptr, object_info->feature.type, topk,
                        indices, sims, size, threshold);
#endif
}

CVI_S32 CVI_TDL_Service_RawMatching(cvitdl_service_handle_t handle, const void *feature,
                                    const feature_type_e type, const uint32_t topk, float threshold,
                                    uint32_t *indices, float *scores, uint32_t *size) {
#ifdef __CV186X__
  return CVI_TDL_SUCCESS;
#else
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_fm == nullptr) {
    LOGE(
        "Not yet register features, please invoke CVI_TDL_Service_RegisterFeatureArray to "
        "register.\n");
    return CVI_TDL_ERR_NOT_YET_INITIALIZED;
  }
  return ctx->m_fm->run(feature, type, topk, indices, scores, size, threshold);
#endif
}

CVI_S32 CVI_TDL_Service_FaceDigitalZoom(cvitdl_service_handle_t handle,
                                        const VIDEO_FRAME_INFO_S *inFrame, const cvtdl_face_t *meta,
                                        const float face_skip_ratio, const float padding_ratio,
                                        const float trans_ratio, VIDEO_FRAME_INFO_S *outFrame) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_dt == nullptr) {
    ctx->m_dt = new cvitdl::service::DigitalTracking();
  }

  ctx->m_dt->setVpssTimeout(CVI_TDL_GetVpssTimeout(ctx->tdl_handle));
  ctx->m_dt->setVpssEngine(CVI_TDL_GetVpssEngine(ctx->tdl_handle, 0));
  return ctx->m_dt->run(inFrame, meta, outFrame, padding_ratio, padding_ratio, padding_ratio,
                        padding_ratio, face_skip_ratio, trans_ratio);
}

CVI_S32 CVI_TDL_Service_ObjectDigitalZoom(cvitdl_service_handle_t handle,
                                          const VIDEO_FRAME_INFO_S *inFrame,
                                          const cvtdl_object_t *meta, const float obj_skip_ratio,
                                          const float trans_ratio, const float padding_ratio,
                                          VIDEO_FRAME_INFO_S *outFrame) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_dt == nullptr) {
    ctx->m_dt = new cvitdl::service::DigitalTracking();
  }

  ctx->m_dt->setVpssTimeout(CVI_TDL_GetVpssTimeout(ctx->tdl_handle));
  ctx->m_dt->setVpssEngine(CVI_TDL_GetVpssEngine(ctx->tdl_handle, 0));
  return ctx->m_dt->run(inFrame, meta, outFrame, padding_ratio, padding_ratio, padding_ratio,
                        padding_ratio, obj_skip_ratio, trans_ratio);
}

CVI_S32 CVI_TDL_Service_ObjectDigitalZoomExt(cvitdl_service_handle_t handle,
                                             const VIDEO_FRAME_INFO_S *inFrame,
                                             const cvtdl_object_t *meta, const float obj_skip_ratio,
                                             const float trans_ratio, const float pad_ratio_left,
                                             const float pad_ratio_right, const float pad_ratio_top,
                                             const float pad_ratio_bottom,
                                             VIDEO_FRAME_INFO_S *outFrame) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_dt == nullptr) {
    ctx->m_dt = new cvitdl::service::DigitalTracking();
  }

  ctx->m_dt->setVpssTimeout(CVI_TDL_GetVpssTimeout(ctx->tdl_handle));
  ctx->m_dt->setVpssEngine(CVI_TDL_GetVpssEngine(ctx->tdl_handle, 0));
  return ctx->m_dt->run(inFrame, meta, outFrame, pad_ratio_left, pad_ratio_right, pad_ratio_top,
                        pad_ratio_bottom, obj_skip_ratio, trans_ratio);
}

template <typename T>
inline CVI_S32 DrawRect(cvitdl_service_handle_t handle, const T *meta, VIDEO_FRAME_INFO_S *frame,
                        const bool drawText, cvtdl_service_brush_t brush) {
  if (meta->size <= 0) return CVI_TDL_SUCCESS;

  if (handle != NULL) {
    std::vector<cvtdl_service_brush_t> brushes(meta->size, brush);
    return cvitdl::service::DrawMeta(meta, frame, drawText, brushes);
  }

  LOGE("service handle is NULL\n");
  return CVI_TDL_FAILURE;
}

template <typename T>
inline CVI_S32 DrawRect(cvitdl_service_handle_t handle, const T *meta, VIDEO_FRAME_INFO_S *frame,
                        const bool drawText, cvtdl_service_brush_t *brushes) {
  if (meta->size <= 0) return CVI_TDL_SUCCESS;

  if (handle != NULL) {
    std::vector<cvtdl_service_brush_t> vec_brushes(brushes, brushes + meta->size);
    return cvitdl::service::DrawMeta(meta, frame, drawText, vec_brushes);
  }

  LOGE("service handle is NULL\n");
  return CVI_TDL_FAILURE;
}

CVI_S32 CVI_TDL_Service_FaceDrawRect(cvitdl_service_handle_t handle, const cvtdl_face_t *meta,
                                     VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                     cvtdl_service_brush_t brush) {
  return DrawRect(handle, meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_Service_FaceDrawRect2(cvitdl_service_handle_t handle, const cvtdl_face_t *meta,
                                      VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                      cvtdl_service_brush_t *brushes) {
  return DrawRect(handle, meta, frame, drawText, brushes);
}

CVI_S32 CVI_TDL_Service_ObjectDrawRect(cvitdl_service_handle_t handle, const cvtdl_object_t *meta,
                                       VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                       cvtdl_service_brush_t brush) {
  return DrawRect(handle, meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_Service_ObjectDrawRect2(cvitdl_service_handle_t handle, const cvtdl_object_t *meta,
                                        VIDEO_FRAME_INFO_S *frame, const bool drawText,
                                        cvtdl_service_brush_t *brushes) {
  return DrawRect(handle, meta, frame, drawText, brushes);
}

CVI_S32 CVI_TDL_Service_Incar_ObjectDrawRect(cvitdl_service_handle_t handle,
                                             const cvtdl_dms_od_t *meta, VIDEO_FRAME_INFO_S *frame,
                                             const bool drawText, cvtdl_service_brush_t brush) {
  return DrawRect(handle, meta, frame, drawText, brush);
}

CVI_S32 CVI_TDL_Service_Incar_ObjectDrawRect2(cvitdl_service_handle_t handle,
                                              const cvtdl_dms_od_t *meta, VIDEO_FRAME_INFO_S *frame,
                                              const bool drawText, cvtdl_service_brush_t *brushes) {
  return DrawRect(handle, meta, frame, drawText, brushes);
}

CVI_S32 CVI_TDL_Service_ObjectWriteText(char *name, int x, int y, VIDEO_FRAME_INFO_S *frame,
                                        float r, float g, float b) {
  return cvitdl::service::WriteText(name, x, y, frame, r, g, b);
}

CVI_S32 CVI_TDL_Service_DrawPolygon(cvitdl_service_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                    const cvtdl_pts_t *pts, cvtdl_service_brush_t brush) {
  return cvitdl::service::DrawPolygon(frame, pts, brush);
}

CVI_S32 CVI_TDL_Service_Polygon_SetTarget(cvitdl_service_handle_t handle, const cvtdl_pts_t *pts) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_intrusion_det == nullptr) {
    ctx->m_intrusion_det = new cvitdl::service::IntrusionDetect();
  }
  return ctx->m_intrusion_det->setRegion(*pts);
}

CVI_S32 CVI_TDL_Service_Polygon_GetTarget(cvitdl_service_handle_t handle,
                                          cvtdl_pts_t ***regions_pts, uint32_t *size) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_intrusion_det == nullptr) {
    LOGE("Please set intersect area first.\n");
    return CVI_TDL_FAILURE;
  }
  ctx->m_intrusion_det->getRegion(regions_pts, size);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_Polygon_CleanAll(cvitdl_service_handle_t handle) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_intrusion_det == nullptr) {
    LOGE("Please set intersect area first.\n");
    return CVI_TDL_FAILURE;
  }
  ctx->m_intrusion_det->clean();
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_Polygon_Intersect(cvitdl_service_handle_t handle, const cvtdl_bbox_t *bbox,
                                          bool *has_intersect) {
  cvitdl_service_context_t *ctx = static_cast<cvitdl_service_context_t *>(handle);
  if (ctx->m_intrusion_det == nullptr) {
    LOGE("Please set intersect area first.\n");
    return CVI_TDL_FAILURE;
  }
  *has_intersect = ctx->m_intrusion_det->run(*bbox);
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_Service_FaceAngle(const cvtdl_pts_t *pts, cvtdl_head_pose_t *hp) {
#ifdef NO_OPENCV
  return CVI_TDL_FAILURE;
#else
  return cvitdl::service::Predict(pts, hp);
#endif
}

CVI_S32 CVI_TDL_Service_FaceAngleForAll(const cvtdl_face_t *meta) {
#ifdef NO_OPENCV
  return CVI_TDL_FAILURE;
#else
  CVI_S32 ret = CVI_TDL_SUCCESS;
  for (uint32_t i = 0; i < meta->size; i++) {
    ret |= cvitdl::service::Predict(&meta->info[i].pts, &meta->info[i].head_pose);
  }
  return ret;
#endif
}

CVI_S32 CVI_TDL_Service_ObjectDrawPose(const cvtdl_object_t *meta, VIDEO_FRAME_INFO_S *frame) {
  return cvitdl::service::DrawPose17(meta, frame);
}
CVI_S32 CVI_TDL_Service_FaceDrawPts(cvtdl_pts_t *pts, VIDEO_FRAME_INFO_S *frame) {
  return cvitdl::service::DrawPts(pts, frame);
}

CVI_S32 CVI_TDL_Service_FaceDraw5Landmark(const cvtdl_face_t *meta, VIDEO_FRAME_INFO_S *frame) {
  return cvitdl::service::Draw5Landmark(meta, frame);
}

cvtdl_service_brush_t CVI_TDL_Service_GetDefaultBrush() {
  cvtdl_service_brush_t brush;
  brush.color.b = DEFAULT_RECT_COLOR_B * 255;
  brush.color.g = DEFAULT_RECT_COLOR_G * 255;
  brush.color.r = DEFAULT_RECT_COLOR_R * 255;
  brush.size = 4;
  return brush;
}

CVI_S32 CVI_TDL_Service_DrawHandKeypoint(cvitdl_service_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                         const cvtdl_handpose21_meta_ts *meta) {
  if (meta->size <= 0) return CVI_TDL_SUCCESS;

  if (handle != NULL) {
    return cvitdl::service::DrawHandPose21(meta, frame);
  }
  LOGE("service handle is NULL\n");
  return CVI_TDL_FAILURE;
}
