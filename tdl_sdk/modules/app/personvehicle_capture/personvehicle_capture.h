#ifndef _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_H_
#define _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_H_

#include "core/cvi_tdl_core.h"
#include "cvi_tdl_app/capture/personvehicle_capture_type.h"

CVI_S32 _PersonVehicleCapture_Free(personvehicle_capture_t *personvehicle_cpt_info);

CVI_S32 _PersonVehicleCapture_Init(personvehicle_capture_t **personvehicle_cpt_info,
                                   uint32_t buffer_size);

CVI_S32 _PersonVehicleCapture_QuickSetUp(cvitdl_handle_t tdl_handle,
                                         personvehicle_capture_t *personvehicle_cpt_info,
                                         const char *od_model_name, const char *od_model_path,
                                         const char *reid_model_path);

CVI_S32 _PersonVehicleCapture_GetDefaultConfig(personvehicle_capture_config_t *cfg);

CVI_S32 _PersonVehicleCapture_SetConfig(personvehicle_capture_t *personvehicle_cpt_info,
                                        personvehicle_capture_config_t *cfg,
                                        cvitdl_handle_t tdl_handle);

CVI_S32 _PersonVehicleCapture_Run(personvehicle_capture_t *personvehicle_cpt_info,
                                  const cvitdl_handle_t tdl_handle, VIDEO_FRAME_INFO_S *frame);
// Irregular
CVI_S32 _PersonVehicleCaptureIrregular_Run(personvehicle_capture_t *personvehicle_cpt_info,
                                           const cvitdl_handle_t tdl_handle,
                                           VIDEO_FRAME_INFO_S *frame);
CVI_S32 _PersonVehicleCaptureIrregular_Region(personvehicle_capture_t *personvehicle_cpt_info,
                                              int w_num, int h_num, bool *regin_flags);

// Draw line
CVI_S32 _PersonVehicleCapture_Line(personvehicle_capture_t *personvehicle_cpt_info, int A_x,
                                   int A_y, int B_x, int B_y, statistics_mode s_mode);

CVI_S32 _PersonVehicleCapture_CleanAll(personvehicle_capture_t *personvehicle_cpt_info);

#endif  // End of _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_H_
