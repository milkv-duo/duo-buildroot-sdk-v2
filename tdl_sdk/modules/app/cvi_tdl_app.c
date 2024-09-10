#include "cvi_tdl_app/cvi_tdl_app.h"

#include "face_cap_utils/face_cap_utils.h"
#include "face_capture/face_capture.h"
#include "face_pet_capture/face_pet_capture.h"
#include "person_capture/person_capture.h"
#include "personvehicle_capture/personvehicle_capture.h"
#include "vehicle_adas/vehicle_adas.h"

CVI_S32 CVI_TDL_APP_CreateHandle(cvitdl_app_handle_t *handle, cvitdl_handle_t tdl_handle) {
  if (tdl_handle == NULL) {
    printf("tdl_handle is empty.");
    return CVI_TDL_FAILURE;
  }
  cvitdl_app_context_t *ctx = (cvitdl_app_context_t *)malloc(sizeof(cvitdl_app_context_t));
  ctx->tdl_handle = tdl_handle;
  ctx->face_cpt_info = NULL;
  ctx->person_cpt_info = NULL;
  ctx->personvehicle_cpt_info = NULL;
  ctx->adas_info = NULL;
  *handle = ctx;
  return CVI_TDL_SUCCESS;
}

CVI_S32 CVI_TDL_APP_DestroyHandle(cvitdl_app_handle_t handle) {
  cvitdl_app_context_t *ctx = handle;
  _FaceCapture_Free(ctx->face_cpt_info);
  _PersonCapture_Free(ctx->person_cpt_info);
  _PersonVehicleCapture_Free(ctx->personvehicle_cpt_info);
  _ADAS_Free(ctx->adas_info);
  ctx->face_cpt_info = NULL;
  ctx->person_cpt_info = NULL;
  ctx->personvehicle_cpt_info = NULL;
  ctx->adas_info = NULL;
  return CVI_TDL_SUCCESS;
}

/* Face Capture */
CVI_S32 CVI_TDL_APP_FaceCapture_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_Init(&(ctx->face_cpt_info), buffer_size);
}

CVI_S32 CVI_TDL_APP_FaceCapture_QuickSetUp(const cvitdl_app_handle_t handle, int fd_model_id,
                                           int fr_model_id, const char *fd_model_path,
                                           const char *fr_model_path, const char *fq_model_path,
                                           const char *fl_model_path, const char *fa_model_path) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_QuickSetUp(ctx->tdl_handle, ctx->face_cpt_info, fd_model_id, fr_model_id,
                                 fd_model_path, fr_model_path, fq_model_path, fl_model_path,
                                 fa_model_path);
}

CVI_S32 CVI_TDL_APP_FaceCapture_FusePedSetup(const cvitdl_app_handle_t handle, int ped_model_id,
                                             const char *ped_model_path) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_FusePedSetUp(ctx->tdl_handle, ctx->face_cpt_info, ped_model_id,
                                   ped_model_path);
}

CVI_S32 CVI_TDL_APP_FacePetCapture_QuickSetUp(const cvitdl_app_handle_t handle, int od_model_id,
                                              int fr_model_id, const char *od_model_path,
                                              const char *fr_model_path, const char *fl_model_path,
                                              const char *fa_model_path) {
  cvitdl_app_context_t *ctx = handle;
  return _FacePetCapture_QuickSetUp(ctx->tdl_handle, ctx->face_cpt_info, od_model_id, fr_model_id,
                                    od_model_path, fr_model_path, fl_model_path, fa_model_path);
}

CVI_S32 CVI_TDL_APP_FaceCapture_GetDefaultConfig(face_capture_config_t *cfg) {
  return _FaceCapture_GetDefaultConfig(cfg);
}

CVI_S32 CVI_TDL_APP_FaceCapture_SetConfig(const cvitdl_app_handle_t handle,
                                          face_capture_config_t *cfg) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_SetConfig(ctx->face_cpt_info, cfg, handle->tdl_handle);
}

CVI_S32 CVI_TDL_APP_FaceCapture_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_Run(ctx->face_cpt_info, ctx->tdl_handle, frame);
}

CVI_S32 CVI_TDL_APP_FacePetCapture_Run(const cvitdl_app_handle_t handle,
                                       VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _FacePetCapture_Run(ctx->face_cpt_info, ctx->tdl_handle, frame);
}

CVI_S32 CVI_TDL_APP_FaceCapture_FDFR(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame,
                                     cvtdl_face_t *p_face) {
  cvitdl_app_context_t *ctx = handle;
  face_capture_t *face_cpt_info = ctx->face_cpt_info;
  cvitdl_handle_t tdl_handle = ctx->tdl_handle;
  if (CVI_SUCCESS !=
      face_cpt_info->fd_inference(tdl_handle, frame, face_cpt_info->fd_model, p_face)) {
    printf("fd_inference failed\n");
    return CVI_FAILURE;
  }
  printf("detect face num:%u\n", p_face->size);

  if (CVI_SUCCESS != face_cpt_info->fr_inference(tdl_handle, frame, p_face)) {
    printf("fr inference failed\n");
    return CVI_FAILURE;
  }
  return CVI_SUCCESS;
}

CVI_S32 CVI_TDL_APP_FaceCapture_SetMode(const cvitdl_app_handle_t handle, capture_mode_e mode) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_SetMode(ctx->face_cpt_info, mode);
}

CVI_S32 CVI_TDL_APP_FaceCapture_CleanAll(const cvitdl_app_handle_t handle) {
  cvitdl_app_context_t *ctx = handle;
  return _FaceCapture_CleanAll(ctx->face_cpt_info);
}

/* Person Capture */
CVI_S32 CVI_TDL_APP_PersonCapture_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_Init(&(ctx->person_cpt_info), buffer_size);
}

CVI_S32 CVI_TDL_APP_PersonCapture_QuickSetUp(const cvitdl_app_handle_t handle,
                                             const char *od_model_name, const char *od_model_path,
                                             const char *reid_model_path) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_QuickSetUp(ctx->tdl_handle, ctx->person_cpt_info, od_model_name,
                                   od_model_path, reid_model_path);
}

CVI_S32 CVI_TDL_APP_PersonCapture_GetDefaultConfig(person_capture_config_t *cfg) {
  return _PersonCapture_GetDefaultConfig(cfg);
}

CVI_S32 CVI_TDL_APP_PersonCapture_SetConfig(const cvitdl_app_handle_t handle,
                                            person_capture_config_t *cfg) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_SetConfig(ctx->person_cpt_info, cfg, handle->tdl_handle);
}

CVI_S32 CVI_TDL_APP_PersonCapture_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_Run(ctx->person_cpt_info, ctx->tdl_handle, frame);
}

// cousumer counting
CVI_S32 CVI_TDL_APP_ConsumerCounting_Run(const cvitdl_app_handle_t handle,
                                         VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _ConsumerCounting_Run(ctx->person_cpt_info, ctx->tdl_handle, frame);
}
DLL_EXPORT CVI_S32 CVI_TDL_APP_ConsumerCounting_Line(const cvitdl_app_handle_t handle, int A_x,
                                                     int A_y, int B_x, int B_y,
                                                     statistics_mode s_mode) {
  cvitdl_app_context_t *ctx = handle;
  return _ConsumerCounting_Line(ctx->person_cpt_info, A_x, A_y, B_x, B_y, s_mode);
}

CVI_S32 CVI_TDL_APP_PersonCapture_SetMode(const cvitdl_app_handle_t handle, capture_mode_e mode) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_SetMode(ctx->person_cpt_info, mode);
}

CVI_S32 CVI_TDL_APP_PersonCapture_CleanAll(const cvitdl_app_handle_t handle) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonCapture_CleanAll(ctx->person_cpt_info);
}

// personvehicle cross the border
CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Init(const cvitdl_app_handle_t handle,
                                              uint32_t buffer_size) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_Init(&(ctx->personvehicle_cpt_info), buffer_size);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCapture_QuickSetUp(const cvitdl_app_handle_t handle,
                                                    const char *od_model_name,
                                                    const char *od_model_path,
                                                    const char *reid_model_path) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_QuickSetUp(ctx->tdl_handle, ctx->personvehicle_cpt_info,
                                          od_model_name, od_model_path, reid_model_path);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCapture_GetDefaultConfig(personvehicle_capture_config_t *cfg) {
  return _PersonVehicleCapture_GetDefaultConfig(cfg);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCapture_SetConfig(const cvitdl_app_handle_t handle,
                                                   personvehicle_capture_config_t *cfg) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_SetConfig(ctx->personvehicle_cpt_info, cfg, handle->tdl_handle);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Run(const cvitdl_app_handle_t handle,
                                             VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_Run(ctx->personvehicle_cpt_info, ctx->tdl_handle, frame);
}
DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Line(const cvitdl_app_handle_t handle, int A_x,
                                                         int A_y, int B_x, int B_y,
                                                         statistics_mode s_mode) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_Line(ctx->personvehicle_cpt_info, A_x, A_y, B_x, B_y, s_mode);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCapture_CleanAll(const cvitdl_app_handle_t handle) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCapture_CleanAll(ctx->personvehicle_cpt_info);
}

/* ADAS */

CVI_S32 CVI_TDL_APP_ADAS_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size,
                              int det_type) {
  cvitdl_app_context_t *ctx = handle;
  return _ADAS_Init(&(ctx->adas_info), buffer_size, det_type);
}

CVI_S32 CVI_TDL_APP_ADAS_Run(const cvitdl_app_handle_t handle, VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _ADAS_Run(ctx->adas_info, ctx->tdl_handle, frame);
}

// Irregular
CVI_S32 CVI_TDL_APP_PersonVehicleCaptureIrregular_Run(const cvitdl_app_handle_t handle,
                                                      VIDEO_FRAME_INFO_S *frame) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCaptureIrregular_Run(ctx->personvehicle_cpt_info, ctx->tdl_handle, frame);
}

CVI_S32 CVI_TDL_APP_PersonVehicleCaptureIrregular_Region(const cvitdl_app_handle_t handle,
                                                         int h_num, int w_num, bool *regin_flags) {
  cvitdl_app_context_t *ctx = handle;
  return _PersonVehicleCaptureIrregular_Region(ctx->personvehicle_cpt_info, w_num, h_num,
                                               regin_flags);
}