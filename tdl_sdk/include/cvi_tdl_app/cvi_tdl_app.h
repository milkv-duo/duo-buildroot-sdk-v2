#ifndef _CVI_TDL_APP_H_
#define _CVI_TDL_APP_H_
#include "capture/adas_capture_type.h"
#include "capture/face_capture_type.h"
#include "capture/person_capture_type.h"
#include "capture/personvehicle_capture_type.h"
#include "core/core/cvtdl_core_types.h"
#include "core/cvi_tdl_core.h"
#include "cvi_comm.h"

typedef struct {
  cvitdl_handle_t tdl_handle;
  face_capture_t *face_cpt_info;
  person_capture_t *person_cpt_info;
  personvehicle_capture_t *personvehicle_cpt_info;
  adas_info_t *adas_info;
} cvitdl_app_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/** @typedef cvitdl_app_handle_t
 *  @ingroup core_cvitdlapp
 *  @brief A cvitdl application handle.
 */
typedef cvitdl_app_context_t *cvitdl_app_handle_t;

/**
 * @brief Create a cvitdl_app_handle_t.
 * @ingroup core_cvitdlapp
 *
 * @param handle A app handle.
 * @param tdl_handle A cvitdl handle.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if succeed.
 */
DLL_EXPORT CVI_S32 CVI_TDL_APP_CreateHandle(cvitdl_app_handle_t *handle,
                                            cvitdl_handle_t tdl_handle);

/**
 * @brief Destroy a cvitdl_app_handle_t.
 * @ingroup core_cvitdlapp
 *
 * @param handle A app handle.
 * @return CVI_S32 Return CVI_TDL_SUCCESS if success to destroy handle.
 */
DLL_EXPORT CVI_S32 CVI_TDL_APP_DestroyHandle(cvitdl_app_handle_t handle);

/* Face Capture */
DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_Init(const cvitdl_app_handle_t handle,
                                                uint32_t buffer_size);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_QuickSetUp(
    const cvitdl_app_handle_t handle, int fd_model_id, int fr_model_id, const char *fd_model_path,
    const char *fr_model_path, const char *fq_model_path, const char *fl_model_path,
    const char *fa_model_path);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FacePetCapture_QuickSetUp(
    const cvitdl_app_handle_t handle, int od_model_id, int fr_model_id, const char *od_model_path,
    const char *fr_model_path, const char *fl_model_path, const char *fa_model_path);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_FusePedSetup(const cvitdl_app_handle_t handle,
                                                        int ped_model_id,
                                                        const char *ped_model_path);
DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_GetDefaultConfig(face_capture_config_t *cfg);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_SetConfig(const cvitdl_app_handle_t handle,
                                                     face_capture_config_t *cfg);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_Run(const cvitdl_app_handle_t handle,
                                               VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FacePetCapture_Run(const cvitdl_app_handle_t handle,
                                                  VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_FDFR(const cvitdl_app_handle_t handle,
                                                VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *p_face);

DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_SetMode(const cvitdl_app_handle_t handle,
                                                   capture_mode_e mode);
DLL_EXPORT CVI_S32 CVI_TDL_APP_FaceCapture_CleanAll(const cvitdl_app_handle_t handle);

/* Person Capture */
DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_Init(const cvitdl_app_handle_t handle,
                                                  uint32_t buffer_size);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_QuickSetUp(const cvitdl_app_handle_t handle,
                                                        const char *od_model_name,
                                                        const char *od_model_path,
                                                        const char *reid_model_path);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_GetDefaultConfig(person_capture_config_t *cfg);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_SetConfig(const cvitdl_app_handle_t handle,
                                                       person_capture_config_t *cfg);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_Run(const cvitdl_app_handle_t handle,
                                                 VIDEO_FRAME_INFO_S *frame);

// cousumer counting
DLL_EXPORT CVI_S32 CVI_TDL_APP_ConsumerCounting_Run(const cvitdl_app_handle_t handle,
                                                    VIDEO_FRAME_INFO_S *frame);
DLL_EXPORT CVI_S32 CVI_TDL_APP_ConsumerCounting_Line(const cvitdl_app_handle_t handle, int A_x,
                                                     int A_y, int B_x, int B_y,
                                                     statistics_mode s_mode);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_SetMode(const cvitdl_app_handle_t handle,
                                                     capture_mode_e mode);
DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonCapture_CleanAll(const cvitdl_app_handle_t handle);

// personvehicle cross the border
DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Init(const cvitdl_app_handle_t handle,
                                                         uint32_t buffer_size);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_QuickSetUp(const cvitdl_app_handle_t handle,
                                                               const char *od_model_name,
                                                               const char *od_model_path,
                                                               const char *reid_model_path);

DLL_EXPORT CVI_S32
CVI_TDL_APP_PersonVehicleCapture_GetDefaultConfig(personvehicle_capture_config_t *cfg);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_SetConfig(const cvitdl_app_handle_t handle,
                                                              personvehicle_capture_config_t *cfg);
DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Run(const cvitdl_app_handle_t handle,
                                                        VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_Line(const cvitdl_app_handle_t handle, int A_x,
                                                         int A_y, int B_x, int B_y,
                                                         statistics_mode s_mode);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCaptureIrregular_Run(const cvitdl_app_handle_t handle,
                                                                 VIDEO_FRAME_INFO_S *frame);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCaptureIrregular_Region(
    const cvitdl_app_handle_t handle, int w_num, int h_num, bool *regin_flags);

DLL_EXPORT CVI_S32 CVI_TDL_APP_PersonVehicleCapture_CleanAll(const cvitdl_app_handle_t handle);

DLL_EXPORT CVI_S32 CVI_TDL_APP_ADAS_Init(const cvitdl_app_handle_t handle, uint32_t buffer_size,
                                         int det_type);

DLL_EXPORT CVI_S32 CVI_TDL_APP_ADAS_Run(const cvitdl_app_handle_t handle,
                                        VIDEO_FRAME_INFO_S *frame);

#ifdef __cplusplus
}
#endif

#endif  // End of _CVI_TDL_APP_H_
