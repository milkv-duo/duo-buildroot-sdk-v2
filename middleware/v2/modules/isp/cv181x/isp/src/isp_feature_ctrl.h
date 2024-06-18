/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_feature_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_FEATURE_CTRL_H_
#define _ISP_FEATURE_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_main_local.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

// TODO@ST Refine
#define DGAIN_UNIT 1024

// TODO@ST Refine
typedef struct _ISP_ALGO_RESULT_S {
	CVI_U32 u32FrameIdx;
	CVI_U32 u32IspPostDgain;
	CVI_U32 u32IspPreDgain;
	CVI_U32 u32IspPostDgainSE;
	CVI_U32 u32IspPreDgainSE;
	CVI_FLOAT afAEEVRatio[ISP_CHANNEL_MAX_NUM];
//	CVI_U32 u32Again;
//	CVI_U32 u32Dgain;
	CVI_U32 u32PreIso;
	CVI_U32 u32PostIso;
	CVI_U32 u32PreBlcIso;
	CVI_U32 u32PostBlcIso;
	CVI_U32 u32PostNpIso;
	CVI_U32 u32PreNpIso;
//	CVI_U32 u32IsoMax;
//	CVI_U32 u32SensorIso;
//	CVI_U32 u32ExpRatio;
	CVI_U32 au32ExpRatio[3];
	// CVI_U16 *global_tone;
	// Q bits defines bin width as (2^Q bits) in the 2nd piece of
	// the two piece-wise LUT in DRC.
	// The usage follows the rule:
	// if an input x > 4096, the LUT indexing for output is (x - 4096) >> Q bits.
	CVI_S16  currentLV;
	CVI_U32  u32AvgLuma;
	WDR_MODE_E enFSWDRMode;

	CVI_U32 u32ColorTemp;
	CVI_U32 au32WhiteBalanceGain[ISP_BAYER_CHN_NUM];
	CVI_U32 au32WhiteBalanceGainPre[ISP_BAYER_CHN_NUM];
} ISP_ALGO_RESULT_S;

enum isp_module_cmd {
	MOD_CMD_MIN = 0,
	MOD_CMD_PREFE_SOF,
	MOD_CMD_PREFE_EOF,
	MOD_CMD_PREBE_SOF,
	MOD_CMD_PREBE_EOF,
	MOD_CMD_POST_SOF,
	MOD_CMD_POST_EOF,
	MOD_CMD_SET_MODCTRL,
	MOD_CMD_GET_MODCTRL,
	MOD_CMD_MAX
};

struct isp_module_ctrl {
	CVI_S32 (*init)(VI_PIPE);
	CVI_S32 (*uninit)(VI_PIPE);
	CVI_S32 (*suspend)(VI_PIPE);
	CVI_S32 (*resume)(VI_PIPE);
	CVI_S32 (*ctrl)(VI_PIPE, enum isp_module_cmd, CVI_VOID *input);
};

enum isp_module_run_ctrl {
	CTRL_RUN_IN_MASTER,
	CTRL_RUN_IN_SLAVE,
	CTRL_RUN_IN_MASTER_AND_SLAVE,
	CTRL_RUN_MAX
};

struct isp_module_run_ctrl_mark {
	CVI_U8 pre_sof_run_ctrl;
	CVI_U8 pre_eof_run_ctrl;
	CVI_U8 post_eof_run_ctrl;
};

CVI_S32 isp_feature_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_pre_fe_eof(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_pre_be_eof(VI_PIPE ViPipe);
CVI_S32 isp_feature_ctrl_post_eof(VI_PIPE ViPipe);

CVI_S32 isp_feature_ctrl_set_module_ctrl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *modCtrl);
CVI_S32 isp_feature_ctrl_get_module_ctrl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *modCtrl);

CVI_S32 isp_feature_ctrl_get_module_info(VI_PIPE ViPipe, ISP_INNER_STATE_INFO_S *innerParam);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_FEATURE_CTRL_H_
