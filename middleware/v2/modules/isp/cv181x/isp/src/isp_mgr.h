/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mgr.h
 * Description:
 *
 */

#ifndef _ISP_MGR_H_
#define _ISP_MGR_H_

#include "cvi_comm_isp.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_mgr_init(VI_PIPE ViPipe);
CVI_S32 isp_mgr_uninit(VI_PIPE ViPipe);
CVI_S32 isp_mgr_pre_sof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_pre_eof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_pre_fe_sof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_pre_fe_eof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_pre_be_sof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_pre_be_eof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_post_sof(VI_PIPE ViPipe, CVI_U32 frame_idx);
CVI_S32 isp_mgr_post_eof(VI_PIPE ViPipe, CVI_U32 frame_idx);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MGR_H_
