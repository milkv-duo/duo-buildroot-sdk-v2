/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_freeze.h
 * Description:
 *
 */

#ifndef _ISP_FREEZE_H_
#define _ISP_FREEZE_H_

#include "cvi_comm_isp.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_freeze_init(VI_PIPE ViPipe);
CVI_S32 isp_freeze_uninit(VI_PIPE ViPipe);
CVI_S32 isp_freeze_set_freeze(VI_PIPE ViPipe, CVI_U64 frmae_idx, CVI_BOOL stable);
CVI_S32 isp_freeze_set_fw_state(VI_PIPE ViPipe, ISP_FMW_STATE_E state);
CVI_S32 isp_freeze_get_fw_state(VI_PIPE ViPipe, ISP_FMW_STATE_E *state);
CVI_S32 isp_freeze_get_state(VI_PIPE ViPipe, CVI_BOOL *bFreezeState);
CVI_S32 isp_freeze_set_mute(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_FREEZE_H_
