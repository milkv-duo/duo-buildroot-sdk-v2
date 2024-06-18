/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_feature_ctrl.c
 * Description:
 *
 */

#include "isp_defines.h"

#include "isp_feature_ctrl.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_feature.h"
#include "isp_freeze.h"
#include "isp_main_local.h"

#ifndef ARCH_RTOS_CV181X
#ifdef ENABLE_ISP_C906L
static const struct isp_module_ctrl *mod[ISP_IQ_BLOCK_MAX] = {
	[ISP_IQ_BLOCK_BLC] = &blc_mod,
	//[ISP_IQ_BLOCK_DPC] = &dpc_mod,
	//[ISP_IQ_BLOCK_CROSSTALK] = &crosstalk_mod,
	//[ISP_IQ_BLOCK_WBGAIN] = &wb_mod,
	[ISP_IQ_BLOCK_GMS] = &dis_mod,
	//[ISP_IQ_BLOCK_BNR] = &bnr_mod,
	//[ISP_IQ_BLOCK_DEMOSAIC] = &demosaic_mod,
	//[ISP_IQ_BLOCK_RGBCAC] = &rgbcac_mod,
	//[ISP_IQ_BLOCK_LCAC] = &lcac_mod,
	[ISP_IQ_BLOCK_MLSC] = &mlsc_mod,
	//[ISP_IQ_BLOCK_CCM] = &ccm_mod,
#if !defined(__CV180X__)
	[ISP_IQ_BLOCK_FUSION] = &fswdr_mod,
#endif
	//[ISP_IQ_BLOCK_DRC] = &drc_mod,
	//[ISP_IQ_BLOCK_GAMMA] = &gamma_mod,
	//[ISP_IQ_BLOCK_DEHAZE] = &dehaze_mod,
	[ISP_IQ_BLOCK_CLUT] = &clut_mod,
	//[ISP_IQ_BLOCK_CSC] = &csc_mod,
	//[ISP_IQ_BLOCK_DCI] = &dci_mod,
	//[ISP_IQ_BLOCK_LDCI] = &ldci_mod,
	//[ISP_IQ_BLOCK_CA] = &ca_mod,
	//[ISP_IQ_BLOCK_PREYEE] = &presharpen_mod,
	[ISP_IQ_BLOCK_MOTION] = &motion_mod,
	//[ISP_IQ_BLOCK_3DNR] = &tnr_mod,
	//[ISP_IQ_BLOCK_YNR] = &ynr_mod,
	//[ISP_IQ_BLOCK_CNR] = &cnr_mod,
	//[ISP_IQ_BLOCK_CAC] = &cac_mod,
	//[ISP_IQ_BLOCK_CA2] = &ca2_mod,
	//[ISP_IQ_BLOCK_YEE] = &sharpen_mod,
	//[ISP_IQ_BLOCK_YCONTRAST] = &ycontrast_mod,
	//[ISP_IQ_BLOCK_MONO] = &mono_mod,
};
#else
static const struct isp_module_ctrl *mod[ISP_IQ_BLOCK_MAX] = {
	[ISP_IQ_BLOCK_BLC] = &blc_mod,
	[ISP_IQ_BLOCK_DPC] = &dpc_mod,
	[ISP_IQ_BLOCK_CROSSTALK] = &crosstalk_mod,
	[ISP_IQ_BLOCK_WBGAIN] = &wb_mod,
	[ISP_IQ_BLOCK_GMS] = &dis_mod,
	[ISP_IQ_BLOCK_BNR] = &bnr_mod,
	[ISP_IQ_BLOCK_DEMOSAIC] = &demosaic_mod,
	[ISP_IQ_BLOCK_RGBCAC] = &rgbcac_mod,
	[ISP_IQ_BLOCK_LCAC] = &lcac_mod,
	[ISP_IQ_BLOCK_MLSC] = &mlsc_mod,
	[ISP_IQ_BLOCK_CCM] = &ccm_mod,
#if !defined(__CV180X__)
	[ISP_IQ_BLOCK_FUSION] = &fswdr_mod,
#endif
	[ISP_IQ_BLOCK_DRC] = &drc_mod,
	[ISP_IQ_BLOCK_GAMMA] = &gamma_mod,
	[ISP_IQ_BLOCK_DEHAZE] = &dehaze_mod,
	[ISP_IQ_BLOCK_CLUT] = &clut_mod,
	[ISP_IQ_BLOCK_CSC] = &csc_mod,
	[ISP_IQ_BLOCK_DCI] = &dci_mod,
	[ISP_IQ_BLOCK_LDCI] = &ldci_mod,
	[ISP_IQ_BLOCK_CA] = &ca_mod,
	[ISP_IQ_BLOCK_PREYEE] = &presharpen_mod,
	[ISP_IQ_BLOCK_MOTION] = &motion_mod,
	[ISP_IQ_BLOCK_3DNR] = &tnr_mod,
	[ISP_IQ_BLOCK_YNR] = &ynr_mod,
	[ISP_IQ_BLOCK_CNR] = &cnr_mod,
	[ISP_IQ_BLOCK_CAC] = &cac_mod,
	[ISP_IQ_BLOCK_CA2] = &ca2_mod,
	[ISP_IQ_BLOCK_YEE] = &sharpen_mod,
	[ISP_IQ_BLOCK_YCONTRAST] = &ycontrast_mod,
	[ISP_IQ_BLOCK_MONO] = &mono_mod,
};
#endif
#else
//static const struct isp_module_ctrl *mod[ISP_IQ_BLOCK_MAX] = {
//	//[ISP_IQ_BLOCK_BLC] = &blc_mod,
//	[ISP_IQ_BLOCK_DPC] = &dpc_mod,
//	[ISP_IQ_BLOCK_CROSSTALK] = &crosstalk_mod,
//	[ISP_IQ_BLOCK_WBGAIN] = &wb_mod,
//	//[ISP_IQ_BLOCK_GMS] = &dis_mod,
//	[ISP_IQ_BLOCK_BNR] = &bnr_mod,
//	[ISP_IQ_BLOCK_DEMOSAIC] = &demosaic_mod,
//	[ISP_IQ_BLOCK_RGBCAC] = &rgbcac_mod,
//	[ISP_IQ_BLOCK_LCAC] = &lcac_mod,
//	[ISP_IQ_BLOCK_MLSC] = &mlsc_mod,
//	[ISP_IQ_BLOCK_CCM] = &ccm_mod,
//#if !defined(__CV180X__)
//	[ISP_IQ_BLOCK_FUSION] = &fswdr_mod,
//#endif
//	[ISP_IQ_BLOCK_DRC] = &drc_mod,
//	[ISP_IQ_BLOCK_GAMMA] = &gamma_mod,
//	[ISP_IQ_BLOCK_DEHAZE] = &dehaze_mod,
//	//[ISP_IQ_BLOCK_CLUT] = &clut_mod,
//	[ISP_IQ_BLOCK_CSC] = &csc_mod,
//	[ISP_IQ_BLOCK_DCI] = &dci_mod,
//	[ISP_IQ_BLOCK_LDCI] = &ldci_mod,
//	[ISP_IQ_BLOCK_CA] = &ca_mod,
//	[ISP_IQ_BLOCK_PREYEE] = &presharpen_mod,
//	//[ISP_IQ_BLOCK_MOTION] = &motion_mod,
//	[ISP_IQ_BLOCK_3DNR] = &tnr_mod,
//	[ISP_IQ_BLOCK_YNR] = &ynr_mod,
//	[ISP_IQ_BLOCK_CNR] = &cnr_mod,
//	[ISP_IQ_BLOCK_CAC] = &cac_mod,
//	[ISP_IQ_BLOCK_CA2] = &ca2_mod,
//	[ISP_IQ_BLOCK_YEE] = &sharpen_mod,
//	[ISP_IQ_BLOCK_YCONTRAST] = &ycontrast_mod,
//	[ISP_IQ_BLOCK_MONO] = &mono_mod,
//};
static const struct isp_module_ctrl *mod[ISP_IQ_BLOCK_MAX] = {
	//[ISP_IQ_BLOCK_BLC] = &blc_mod,
	//[ISP_IQ_BLOCK_DPC] = &dpc_mod,
	//[ISP_IQ_BLOCK_CROSSTALK] = &crosstalk_mod,
	//[ISP_IQ_BLOCK_WBGAIN] = &wb_mod,
	//[ISP_IQ_BLOCK_GMS] = &dis_mod,
	//[ISP_IQ_BLOCK_BNR] = &bnr_mod,
	//[ISP_IQ_BLOCK_DEMOSAIC] = &demosaic_mod,
	//[ISP_IQ_BLOCK_RGBCAC] = &rgbcac_mod,
	//[ISP_IQ_BLOCK_LCAC] = &lcac_mod,
	//[ISP_IQ_BLOCK_MLSC] = &mlsc_mod,
	//[ISP_IQ_BLOCK_CCM] = &ccm_mod,
	//[ISP_IQ_BLOCK_FUSION] = &fswdr_mod,
	//[ISP_IQ_BLOCK_DRC] = &drc_mod,
	//[ISP_IQ_BLOCK_GAMMA] = &gamma_mod,
	//[ISP_IQ_BLOCK_DEHAZE] = &dehaze_mod,
	//[ISP_IQ_BLOCK_CLUT] = &clut_mod,
	//[ISP_IQ_BLOCK_CSC] = &csc_mod,
	//[ISP_IQ_BLOCK_DCI] = &dci_mod,
	//[ISP_IQ_BLOCK_LDCI] = &ldci_mod,
	//[ISP_IQ_BLOCK_CA] = &ca_mod,
	//[ISP_IQ_BLOCK_PREYEE] = &presharpen_mod,
	//[ISP_IQ_BLOCK_MOTION] = &motion_mod,
	//[ISP_IQ_BLOCK_3DNR] = &tnr_mod,
	//[ISP_IQ_BLOCK_YNR] = &ynr_mod,
	//[ISP_IQ_BLOCK_CNR] = &cnr_mod,
	//[ISP_IQ_BLOCK_CAC] = &cac_mod,
	//[ISP_IQ_BLOCK_CA2] = &ca2_mod,
	//[ISP_IQ_BLOCK_YEE] = &sharpen_mod,
	//[ISP_IQ_BLOCK_YCONTRAST] = &ycontrast_mod,
	//[ISP_IQ_BLOCK_MONO] = &mono_mod,
};
#endif

#define MAX_ALGO_RESULT_QUEUE_NUM 10
// #define ISP_DGAIN_APPLY_DELAY 3        // When ae Set ISP Dgain. Need count 3 frame to apply to ISP.

#ifdef ARCH_RTOS_CV181X
#define ISP_DGAIN_APPLY_DELAY 1
#else
//TODO@CV181X Fix it
#define ISP_DGAIN_APPLY_DELAY 2
#endif

struct isp_feature_ctrl_runtime {
	ISP_ALGO_RESULT_S algoResultSave[MAX_ALGO_RESULT_QUEUE_NUM];
};

struct isp_feature_ctrl_runtime *feature_ctrl_runtime[VI_MAX_PIPE_NUM];
static struct isp_feature_ctrl_runtime  **_get_feature_ctrl_runtime(VI_PIPE ViPipe);

static CVI_S32 isp_feature_ctrl_init_param(VI_PIPE ViPipe);

CVI_S32 isp_feature_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_feature_ctrl_runtime **runtime_ptr = _get_feature_ctrl_runtime(ViPipe);

	ISP_CREATE_RUNTIME(*runtime_ptr, struct isp_feature_ctrl_runtime);

	isp_feature_ctrl_init_param(ViPipe);

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->init)
				mod[idx]->init(ViPipe);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_feature_ctrl_runtime **runtime_ptr = _get_feature_ctrl_runtime(ViPipe);

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->uninit)
				mod[idx]->uninit(ViPipe);
	}

	ISP_RELEASE_MEMORY(*runtime_ptr);

	return ret;
}

CVI_S32 isp_feature_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->suspend)
				mod[idx]->suspend(ViPipe);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->resume)
				mod[idx]->resume(ViPipe);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_pre_fe_eof(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->ctrl)
				mod[idx]->ctrl(ViPipe, MOD_CMD_PREFE_EOF, NULL);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_pre_be_eof(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->ctrl)
				mod[idx]->ctrl(ViPipe, MOD_CMD_PREBE_EOF, NULL);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_post_eof(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_feature_ctrl_runtime **runtime_ptr = _get_feature_ctrl_runtime(ViPipe);
	struct isp_feature_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 expRatio = 0;
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_U32 aeDelayIdx = 0;	// AE related setting apply to raw data delay.
	CVI_U32 ispPrerawDelayIdx = 0;	// ISP dgain setting apply to raw data delay.
	CVI_U32 currentFrameIdx = 0; // Current frame setting.
	CVI_U32 preFrameIdx = 0;
	ISP_AE_RESULT_S *pstAeResult;

	CVI_U8 u8IspSceneDelay = 0;
	CVI_U8 u8IspTimingDelay = 0;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	pstAeResult = &pstIspCtx->stAeResult;

	switch (pstIspCtx->scene) {
	case FE_ON_BE_ON_POST_OFF_SC:
	case FE_ON_BE_ON_POST_ON_SC:
		u8IspSceneDelay = 0;
		break;
	case FE_ON_BE_OFF_POST_OFF_SC:
	case FE_ON_BE_OFF_POST_ON_SC:
		u8IspSceneDelay = 0;
		{
			switch (pstIspCtx->enPreViEvent) {
			case VI_EVENT_PRE0_SOF:
			case VI_EVENT_PRE1_SOF:
				u8IspTimingDelay = 1;
				break;
			case VI_EVENT_PRE0_EOF:
			case VI_EVENT_PRE1_EOF:
				u8IspTimingDelay = 0;
				break;
			default:
				u8IspTimingDelay = 0;
				break;
			}
		}
		break;
	case FE_OFF_BE_ON_POST_OFF_SC:
	case FE_OFF_BE_ON_POST_ON_SC:
		u8IspSceneDelay = 1;
		u8IspTimingDelay = 0;
		break;
	//case FE_OFF_BE_OFF_POST_OFF_SC:
	//case FE_OFF_BE_OFF_POST_ON_SC:
	//	u8IspSceneDelay = 1;
	//	break;
	default:
		break;
	}

	// TODO@ST Refine
	currentFrameIdx = pstIspCtx->frameCnt % MAX_ALGO_RESULT_QUEUE_NUM;

	if (currentFrameIdx != 0) {
		preFrameIdx = currentFrameIdx - 1;
	} else {
		preFrameIdx = MAX_ALGO_RESULT_QUEUE_NUM - 1;
	}

	// apply to be blc, single sensor be post sbm on, dual sensor be post online,
	// so be post param effect together
	aeDelayIdx = (pstIspCtx->frameCnt + pstAeResult->u8MeterFramePeriod
		- ISP_DGAIN_APPLY_DELAY + u8IspSceneDelay - u8IspTimingDelay) % MAX_ALGO_RESULT_QUEUE_NUM;

	// apply to fe blc
	// TODO@mason.zou, if fe offline (dual sensor), different at 15fps or 25fps
	ispPrerawDelayIdx = (currentFrameIdx + pstAeResult->u8MeterFramePeriod
		- (ISP_DGAIN_APPLY_DELAY + u8IspSceneDelay) - u8IspTimingDelay) % MAX_ALGO_RESULT_QUEUE_NUM;

	if (pstIspCtx->u8SnsWDRMode == WDR_MODE_NONE)
		expRatio = 64;
	else {
		// Clamp exposure ratio smaller than 256.
		expRatio = MIN(pstAeResult->u32ExpRatio, ((1 << 14) - 1));
		expRatio = MAX(expRatio, 64);
	}
	// awb result. Apply at current frame.
	runtime->algoResultSave[currentFrameIdx].u32ColorTemp = pstIspCtx->stAwbResult.u32ColorTemp;
	for (CVI_U32 i = 0; i < ISP_BAYER_CHN_NUM; i++) {
		runtime->algoResultSave[currentFrameIdx].au32WhiteBalanceGainPre[i] =
		runtime->algoResultSave[preFrameIdx].au32WhiteBalanceGain[i];

		runtime->algoResultSave[currentFrameIdx].au32WhiteBalanceGain[i] =
		pstIspCtx->stAwbResult.au32WhiteBalanceGain[i];
	}

	// Current frame info.
	runtime->algoResultSave[currentFrameIdx].enFSWDRMode = pstIspCtx->u8SnsWDRMode;
	runtime->algoResultSave[currentFrameIdx].currentLV = pstAeResult->s16CurrentLV;
	runtime->algoResultSave[currentFrameIdx].u32AvgLuma = pstAeResult->u32AvgLuma;
	runtime->algoResultSave[currentFrameIdx].u32FrameIdx = pstIspCtx->frameCnt;

	// AE delay frame info.
	runtime->algoResultSave[aeDelayIdx].u32PostIso = pstAeResult->u32Iso;
	runtime->algoResultSave[aeDelayIdx].u32PostBlcIso = pstIspCtx->stAeResult.u32BlcIso;
	runtime->algoResultSave[aeDelayIdx].au32ExpRatio[0] = expRatio;
	runtime->algoResultSave[aeDelayIdx].u32IspPostDgain =
		(pstAeResult->u32IspDgain == 0) ? DGAIN_UNIT : pstAeResult->u32IspDgain;
	runtime->algoResultSave[aeDelayIdx].u32IspPostDgainSE = pstAeResult->u32IspDgainSF;
	runtime->algoResultSave[aeDelayIdx].u32PostNpIso = pstIspCtx->stAeResult.u32BlcIso;
	runtime->algoResultSave[aeDelayIdx].afAEEVRatio[ISP_CHANNEL_LE] = pstAeResult->fEvRatio[ISP_CHANNEL_LE];
	runtime->algoResultSave[aeDelayIdx].afAEEVRatio[ISP_CHANNEL_SE] = pstAeResult->fEvRatio[ISP_CHANNEL_SE];

	// dgain info.
	runtime->algoResultSave[ispPrerawDelayIdx].u32IspPreDgain =
		(pstAeResult->u32IspDgain == 0) ? DGAIN_UNIT : pstAeResult->u32IspDgain;
	runtime->algoResultSave[ispPrerawDelayIdx].u32IspPreDgainSE = pstAeResult->u32IspDgainSF;
	runtime->algoResultSave[ispPrerawDelayIdx].u32PreIso = pstAeResult->u32Iso;
	runtime->algoResultSave[ispPrerawDelayIdx].u32PreBlcIso = pstIspCtx->stAeResult.u32BlcIso;
	runtime->algoResultSave[ispPrerawDelayIdx].u32PreNpIso = pstIspCtx->stAeResult.u32BlcIso;
	// ispPrerawDelayIdx = aeDelayIdx = currentFrameIdx;

	isp_interpolate_update(ViPipe,
		runtime->algoResultSave[currentFrameIdx].u32PreIso,
		runtime->algoResultSave[currentFrameIdx].u32PostIso,
		runtime->algoResultSave[currentFrameIdx].u32PreBlcIso,
		runtime->algoResultSave[currentFrameIdx].u32PostBlcIso,
		expRatio, runtime->algoResultSave[currentFrameIdx].currentLV);

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		// struct timeval t1, t2;

		// gettimeofday(&t1, NULL);
		if (mod[idx])
			if (mod[idx]->ctrl)
				mod[idx]->ctrl(ViPipe, MOD_CMD_POST_EOF,
					(CVI_VOID *)&(runtime->algoResultSave[currentFrameIdx]));

		// gettimeofday(&t2, NULL);
		// printf("%d %ld\n", idx, t2.tv_usec - t1.tv_usec);
	}

#ifndef ARCH_RTOS_CV181X
	CVI_BOOL bFreezeState = CVI_FALSE;

	isp_freeze_get_state(ViPipe, &bFreezeState);

	if (bFreezeState) {
		isp_freeze_set_mute(ViPipe);
	}
#endif
	return ret;
}

CVI_S32 isp_feature_ctrl_set_module_ctrl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *modCtrl)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->ctrl)
				mod[idx]->ctrl(ViPipe, MOD_CMD_SET_MODCTRL, modCtrl);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_get_module_ctrl(VI_PIPE ViPipe, ISP_MODULE_CTRL_U *modCtrl)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	for (CVI_U32 idx = ISP_PRE_RAW_START ; idx < ISP_IQ_BLOCK_MAX ; idx++) {
		if (mod[idx])
			if (mod[idx]->ctrl)
				mod[idx]->ctrl(ViPipe, MOD_CMD_GET_MODCTRL, modCtrl);
	}

	return ret;
}

CVI_S32 isp_feature_ctrl_get_module_info(VI_PIPE ViPipe, ISP_INNER_STATE_INFO_S *innerParam)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct ccm_info ccm_info;
	struct blc_info blc_info;
	struct drc_algo_info drc_info;
	struct mlsc_info info;

	isp_ccm_ctrl_get_ccm_info(ViPipe, &ccm_info);
	isp_blc_ctrl_get_blc_info(ViPipe, &blc_info);
	isp_drc_ctrl_get_algo_info(ViPipe, &drc_info);
	isp_mlsc_ctrl_get_mlsc_info(ViPipe, &info);

	// Get ccm interpolation result.
	for (CVI_U32 i = 0; i < 9; i++) {
		innerParam->ccm[i] = ccm_info.CCM[i];
	}
	// Get blc interpolation result.
	innerParam->blcOffsetR = blc_info.OffsetR;
	innerParam->blcOffsetGr = blc_info.OffsetGr;
	innerParam->blcOffsetGb = blc_info.OffsetGb;
	innerParam->blcOffsetB = blc_info.OffsetB;
	innerParam->blcGainR = blc_info.GainR;
	innerParam->blcGainGr = blc_info.GainGr;
	innerParam->blcGainGb = blc_info.GainGb;
	innerParam->blcGainB = blc_info.GainB;

	innerParam->drcGlobalToneBinNum = drc_info.globalToneBinNum;
	innerParam->drcGlobalToneBinSEStep = drc_info.globalToneSeStep;
	ISP_FORLOOP_SET(innerParam->drcGlobalTone, drc_info.globalToneResult, LTM_GLOBAL_CURVE_NODE_NUM);
	ISP_FORLOOP_SET(innerParam->drcDarkTone, drc_info.darkToneResult, LTM_DARK_CURVE_NODE_NUM);
	ISP_FORLOOP_SET(innerParam->drcBrightTone, drc_info.brightToneResult, LTM_BRIGHT_CURVE_NODE_NUM);

	ISP_FORLOOP_SET(innerParam->mlscGainTable.RGain, info.lut_r, CVI_ISP_LSC_GRID_POINTS);
	ISP_FORLOOP_SET(innerParam->mlscGainTable.GGain, info.lut_g, CVI_ISP_LSC_GRID_POINTS);
	ISP_FORLOOP_SET(innerParam->mlscGainTable.BGain, info.lut_b, CVI_ISP_LSC_GRID_POINTS);

	return ret;
}

static CVI_S32 isp_feature_ctrl_init_param(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_feature_ctrl_runtime **runtime_ptr = _get_feature_ctrl_runtime(ViPipe);
	struct isp_feature_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	memset(runtime, 0, sizeof(struct isp_feature_ctrl_runtime));

	for (CVI_U32 idx = 0 ; idx < MAX_ALGO_RESULT_QUEUE_NUM ; idx++) {
		runtime->algoResultSave[idx].u32IspPostDgain = 1024;
		runtime->algoResultSave[idx].u32IspPreDgain = 1024;
		runtime->algoResultSave[idx].u32IspPostDgainSE = 1024;
		runtime->algoResultSave[idx].u32IspPreDgainSE = 1024;
		for (CVI_U32 i = 0 ; i < ISP_CHANNEL_MAX_NUM ; i++) {
			runtime->algoResultSave[idx].afAEEVRatio[i] = 1.0;
		}
		runtime->algoResultSave[idx].u32PostIso = 100;
		runtime->algoResultSave[idx].u32PreIso = 100;
		for (CVI_U32 i = 0 ; i < 3 ; i++) {
			runtime->algoResultSave[idx].au32ExpRatio[i] = 64;
		}

		// runtime->algoResultSave[idx].u8MeterFramePeriod = 4;
		// runtime->algoResultSave[idx].enFSWDRMode = WDR_MODE_NONE;
		runtime->algoResultSave[idx].currentLV = 10;
		runtime->algoResultSave[idx].u32AvgLuma = 56;
		runtime->algoResultSave[idx].u32ColorTemp = 6500;
		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			runtime->algoResultSave[idx].au32WhiteBalanceGain[i] = 1024;
			runtime->algoResultSave[idx].au32WhiteBalanceGainPre[i] = 1024;
		}
	}

	return ret;
}

static struct isp_feature_ctrl_runtime  **_get_feature_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &feature_ctrl_runtime[ViPipe];
}
