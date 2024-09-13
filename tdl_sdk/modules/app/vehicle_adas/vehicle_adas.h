#ifndef _CVI_TDL_APP_VEHICLE_ADAS_H_
#define _CVI_TDL_APP_VEHICLE_ADAS_H_

#include "core/cvi_tdl_core.h"
#include "cvi_tdl_app/capture/adas_capture_type.h"

CVI_S32 _ADAS_Init(adas_info_t **adas_info, uint32_t buffer_size, int det_type);

CVI_S32 _ADAS_Run(adas_info_t *adas_info, const cvitdl_handle_t tdl_handle,
                  VIDEO_FRAME_INFO_S *frame);

CVI_S32 _ADAS_Free(adas_info_t *adas_info);

#endif  // End of _CVI_TDL_APP_VEHICLE_ADAS_H_
