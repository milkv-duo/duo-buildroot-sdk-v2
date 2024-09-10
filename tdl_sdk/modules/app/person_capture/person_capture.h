#ifndef _CVI_TDL_APP_PERSON_CAPTURE_H_
#define _CVI_TDL_APP_PERSON_CAPTURE_H_

#include "core/cvi_tdl_core.h"
#include "cvi_tdl_app/capture/person_capture_type.h"

CVI_S32 _PersonCapture_Free(person_capture_t *person_cpt_info);

CVI_S32 _PersonCapture_Init(person_capture_t **person_cpt_info, uint32_t buffer_size);

CVI_S32 _PersonCapture_QuickSetUp(cvitdl_handle_t tdl_handle, person_capture_t *person_cpt_info,
                                  const char *od_model_name, const char *od_model_path,
                                  const char *reid_model_path);

CVI_S32 _PersonCapture_GetDefaultConfig(person_capture_config_t *cfg);

CVI_S32 _PersonCapture_SetConfig(person_capture_t *person_cpt_info, person_capture_config_t *cfg,
                                 cvitdl_handle_t tdl_handle);

CVI_S32 _PersonCapture_Run(person_capture_t *person_cpt_info, const cvitdl_handle_t tdl_handle,
                           VIDEO_FRAME_INFO_S *frame);

// consumer counting
CVI_S32 _ConsumerCounting_Run(person_capture_t *person_cpt_info, const cvitdl_handle_t tdl_handle,
                              VIDEO_FRAME_INFO_S *frame);
// Draw line
CVI_S32 _ConsumerCounting_Line(person_capture_t *person_cpt_info, int A_x, int A_y, int B_x,
                               int B_y, statistics_mode s_mode);

CVI_S32 _PersonCapture_SetMode(person_capture_t *person_cpt_info, capture_mode_e mode);

CVI_S32 _PersonCapture_CleanAll(person_capture_t *person_cpt_info);

#endif  // End of _CVI_TDL_APP_PERSON_CAPTURE_H_
