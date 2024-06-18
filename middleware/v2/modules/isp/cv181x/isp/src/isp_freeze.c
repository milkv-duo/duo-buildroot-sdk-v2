/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_freeze.c
 * Description:
 *
 */

#include "cvi_base.h"
#include "cvi_comm_isp.h"

#include "isp_freeze.h"

#include "isp_debug.h"

#include "isp_tun_buf_ctrl.h"
#include "isp_algo_ycontrast.h"

#define FRAME_FREEZE_BY_3A_ALGO_MIN_CHECK_FRAMES 10
#define FRAME_FREEZE_BY_3A_ALGO_MAX_CHECK_FRAMES 30
#define FRAME_FREEZE_BY_3A_ALGO_INITIAL_FREEZE_OFFSET 8
#define FRAME_FREEZE_BY_3A_ALGO_FREEZE_OFFSET 3

struct isp_freeze_runtime {
	CVI_BOOL enable;
	CVI_BOOL stable;
	CVI_BOOL yuvbypass;
	ISP_FMW_STATE_E fw_state;
	CVI_U64 maxCheckFrameIndex;
};
struct isp_freeze_runtime freeze_runtime[VI_MAX_PIPE_NUM];

static inline struct isp_freeze_runtime *_get_freeze_runtime(VI_PIPE ViPipe);

CVI_S32 isp_freeze_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->enable = CVI_FALSE;
	runtime->stable = CVI_FALSE;
	runtime->yuvbypass = CVI_FALSE;
	runtime->fw_state = ISP_FMW_STATE_RUN;
	runtime->maxCheckFrameIndex = FRAME_FREEZE_BY_3A_ALGO_MAX_CHECK_FRAMES;

	VI_PIPE_ATTR_S stPipeAttr;

	ret = CVI_VI_GetPipeAttr(ViPipe, &stPipeAttr);
	if (ret == CVI_SUCCESS) {
		runtime->yuvbypass = stPipeAttr.bYuvBypassPath;

		#ifndef FREEZE_FRAME_CO_RELEASE_MODE
		runtime->enable = !stPipeAttr.bYuvBypassPath;
		#else
		runtime->enable = CVI_TRUE;
		#endif // FREEZE_FRAME_CO_RELEASE_MODE

		if (runtime->enable) {
			CVI_VI_SetBypassFrm(ViPipe, runtime->maxCheckFrameIndex);
		}
	} else {
		ISP_LOG_WARNING("Get VI Pipe (%d) Attr fail\n", ViPipe);
	}

	return ret;
}

CVI_S32 isp_freeze_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	runtime->enable = CVI_FALSE;
	runtime->stable = CVI_FALSE;
	runtime->yuvbypass = CVI_FALSE;
	runtime->maxCheckFrameIndex = 0;

	return ret;
}

CVI_S32 isp_freeze_set_freeze(VI_PIPE ViPipe, CVI_U64 frmae_idx, CVI_BOOL stable)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	runtime->stable = stable;

	if (runtime->enable) {
#ifndef FREEZE_FRAME_CO_RELEASE_MODE

		CVI_BOOL stop_freeze = CVI_FALSE;

		if (frmae_idx >= FRAME_FREEZE_BY_3A_ALGO_MIN_CHECK_FRAMES) {
			stop_freeze = (frmae_idx >= runtime->maxCheckFrameIndex) || (stable);
		}

		if (stop_freeze) {
			runtime->enable = CVI_FALSE;
			CVI_VI_SetBypassFrm(ViPipe, 0);
		}
#else
		// unlock when all pipe are stable
		CVI_BOOL all_stable = CVI_TRUE;

		for (CVI_U32 u32VIPipe = 0; u32VIPipe < VI_MAX_PIPE_NUM; ++u32VIPipe) {
			if (freeze_runtime[u32VIPipe].enable) {
				all_stable &= (freeze_runtime[u32VIPipe].stable || freeze_runtime[u32VIPipe].yuvbypass);
			} else {
				all_stable &= CVI_TRUE;
			}
		}
		CVI_BOOL all_stop_freeze = (frmae_idx >= runtime->maxCheckFrameIndex) || (all_stable);

		if (all_stop_freeze) {
			for (CVI_U32 u32VIPipe = 0; u32VIPipe < VI_MAX_PIPE_NUM; ++u32VIPipe) {
				if (freeze_runtime[u32VIPipe].enable) {
					freeze_runtime[u32VIPipe].enable = CVI_FALSE;
					CVI_VI_SetBypassFrm(u32VIPipe, 0);
				}
			}
		}
#endif // FREEZE_FRAME_CO_RELEASE_MODE
	}

	return ret;
}

CVI_S32 isp_freeze_set_fw_state(VI_PIPE ViPipe, ISP_FMW_STATE_E state)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	if (state >= ISP_FMW_STATE_BUTT)
		return CVI_FAILURE;

	runtime->fw_state = state;

	return ret;
}

CVI_S32 isp_freeze_get_fw_state(VI_PIPE ViPipe, ISP_FMW_STATE_E *state)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	*state = runtime->fw_state;

	return ret;
}

CVI_S32 isp_freeze_get_state(VI_PIPE ViPipe, CVI_BOOL *bFreezeState)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_freeze_runtime *runtime = _get_freeze_runtime(ViPipe);

	*bFreezeState = runtime->enable;

	return ret;
}

CVI_S32 isp_freeze_set_mute(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_mono_config *mono_cfg =
		(struct cvi_vip_isp_mono_config *)&(post_addr->tun_cfg[tun_idx].mono_cfg);

	struct cvi_vip_isp_ycur_config *ycur_cfg =
		(struct cvi_vip_isp_ycur_config *)&(post_addr->tun_cfg[tun_idx].ycur_cfg);

	mono_cfg->update = 1;
	mono_cfg->force_mono_enable = 1;

	ycur_cfg->update = 1;
	ycur_cfg->enable = 1;

	for (int i = 0; i < YCURVE_LUT_ENTRY_NUM; i++) {
		ycur_cfg->lut[i] = 0x00;
	}

	ycur_cfg->lut_256 = 0x00;

	return CVI_SUCCESS;
}

static inline struct isp_freeze_runtime  *_get_freeze_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &freeze_runtime[ViPipe];
}
