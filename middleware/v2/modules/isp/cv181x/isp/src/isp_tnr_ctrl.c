/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_tnr_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"

#include "isp_tnr_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L

#define TNR_MOTION_BLOCK_MIN_UNIT	(3)
#define TNR_BASE_1X_GAIN		(4096)
#define TNR_BASE_1X_GAIN_BIT	(12)

#define WB_UNIT_GAIN 1024

const struct isp_module_ctrl tnr_mod = {
	.init = isp_tnr_ctrl_init,
	.uninit = isp_tnr_ctrl_uninit,
	.suspend = isp_tnr_ctrl_suspend,
	.resume = isp_tnr_ctrl_resume,
	.ctrl = isp_tnr_ctrl_ctrl
};


static CVI_S32 isp_tnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_tnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_tnr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_tnr_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_tnr_ctrl_postprocess_compatible(VI_PIPE ViPipe);
static CVI_S32 set_tnr_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_tnr_ctrl_check_tnr_attr_valid(const ISP_TNR_ATTR_S *psttnrAttr);
static CVI_S32 isp_tnr_ctrl_check_tnr_noise_model_attr_valid(const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr);
static CVI_S32 isp_tnr_ctrl_check_tnr_luma_motion_attr_valid(const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr);
static CVI_S32 isp_tnr_ctrl_check_tnr_ghost_attr_valid(const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr);
static CVI_S32 isp_tnr_ctrl_check_tnr_mt_prt_attr_valid(const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr);
static CVI_S32 isp_tnr_ctrl_check_tnr_motion_adapt_attr_valid(const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr);
static CVI_S32 isp_tnr_ctrl_set_tnr_mt_prt_attr_compatible(VI_PIPE ViPipe, ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr);

static struct isp_tnr_ctrl_runtime  *_get_tnr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_tnr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->tnr_internal_attr.bUpdateMotionUnit = CVI_TRUE;
	for (CVI_U32 u32Idx = 0; u32Idx < ISP_AUTO_ISO_STRENGTH_NUM; ++u32Idx) {
		runtime->au8MtDetectUnit[u32Idx] = TNR_MOTION_BLOCK_MIN_UNIT;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_tnr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_tnr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_tnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_tnr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypass3dnr;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypass3dnr = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_tnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_tnr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_tnr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_tnr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_tnr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_tnr_update_internal_motion_block_list(VI_PIPE ViPipe,
	struct isp_tnr_ctrl_runtime *runtime,
	const ISP_TNR_ATTR_S *tnr_attr)
{
	if (!runtime->tnr_internal_attr.bUpdateMotionUnit) {
		return CVI_SUCCESS;
	}

	// update internal motion unit
	const CVI_U8						*pu8AutoMtUnitAttr = NULL;
	CVI_U8								*pu8AutoMtUnitInter = NULL;
	ISP_CMOS_NOISE_CALIBRATION_S		*ptNP = NULL;
	struct tnr_motion_block_size_in		stMotionBlockSizeIn;
	struct tnr_motion_block_size_out	stMotionBlockSizeOut;

	stMotionBlockSizeIn.u32RefLuma		= TNR_MOTION_BLOCK_SIZE_REF_LUMA;
	stMotionBlockSizeIn.fMotionTh		= TNR_MOTION_BLOCK_SIZE_MOTION_TH;
	stMotionBlockSizeIn.u32Tolerance	= TNR_MOTION_BLOCK_SIZE_TOLERANCE;

	isp_mgr_buf_get_np_addr(ViPipe, (CVI_VOID **) &ptNP);
	pu8AutoMtUnitAttr = tnr_attr->stAuto.MtDetectUnit;
	pu8AutoMtUnitInter = runtime->au8MtDetectUnit;

	for (CVI_U32 u32ISOIdx = 0; u32ISOIdx < ISP_AUTO_ISO_STRENGTH_NUM; ++u32ISOIdx) {
		if (pu8AutoMtUnitAttr[u32ISOIdx] > 0) {
			// manual (PQTool support 1, 2 (but not in ISP))
			pu8AutoMtUnitInter[u32ISOIdx] = MAX(pu8AutoMtUnitAttr[u32ISOIdx], TNR_MOTION_BLOCK_MIN_UNIT);
			continue;
		}

		// auto
		CVI_FLOAT fSlopeGR_GB, fInterceptGR_GB;

		fSlopeGR_GB = ptNP->CalibrationCoef[u32ISOIdx][1][0]
						+ ptNP->CalibrationCoef[u32ISOIdx][2][0];
		fInterceptGR_GB = ptNP->CalibrationCoef[u32ISOIdx][1][1]
							+ ptNP->CalibrationCoef[u32ISOIdx][2][1];

		stMotionBlockSizeIn.fSlopeB = ptNP->CalibrationCoef[u32ISOIdx][0][0];
		stMotionBlockSizeIn.fSlopeG = fSlopeGR_GB / 2.0f;
		stMotionBlockSizeIn.fSlopeR = ptNP->CalibrationCoef[u32ISOIdx][3][0];
		stMotionBlockSizeIn.fInterceptB = ptNP->CalibrationCoef[u32ISOIdx][0][1];
		stMotionBlockSizeIn.fInterceptG = fInterceptGR_GB / 2.0f;
		stMotionBlockSizeIn.fInterceptR = ptNP->CalibrationCoef[u32ISOIdx][3][1];

		isp_algo_tnr_generateblocksize(&stMotionBlockSizeIn, &stMotionBlockSizeOut);
		pu8AutoMtUnitInter[u32ISOIdx] = stMotionBlockSizeOut.u8BlockValue;
	}

#ifndef IGNORE_LOG_DEBUG
		ISP_LOG_DEBUG("=======================================================\n");
		for (CVI_U32 u32ISOIdx = 0; u32ISOIdx < ISP_AUTO_ISO_STRENGTH_NUM; ++u32ISOIdx) {
			ISP_LOG_DEBUG("ISO:%3d ==> %2d vs %2d\n", u32ISOIdx,
				tnr_attr->stAuto.MtDetectUnit[u32ISOIdx], runtime->au8MtDetectUnit[u32ISOIdx]);
		}
		ISP_LOG_DEBUG("=======================================================\n");
#endif //

	runtime->tnr_internal_attr.bUpdateMotionUnit = CVI_FALSE;

	return CVI_SUCCESS;
}

static CVI_S32 isp_tnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_TNR_ATTR_S *tnr_attr = NULL;

	const ISP_TNR_NOISE_MODEL_ATTR_S *tnr_noise_model_attr = NULL;

	const ISP_TNR_LUMA_MOTION_ATTR_S *tnr_luma_motion_attr = NULL;

	const ISP_TNR_GHOST_ATTR_S *tnr_ghost_attr = NULL;

	const ISP_TNR_MT_PRT_ATTR_S *tnr_mt_prt_attr = NULL;

	const ISP_TNR_MOTION_ADAPT_ATTR_S *tnr_motion_adapt_attr = NULL;

	isp_tnr_ctrl_get_tnr_attr(ViPipe, &tnr_attr);
	isp_tnr_ctrl_get_tnr_noise_model_attr(ViPipe, &tnr_noise_model_attr);
	isp_tnr_ctrl_get_tnr_luma_motion_attr(ViPipe, &tnr_luma_motion_attr);
	isp_tnr_ctrl_get_tnr_ghost_attr(ViPipe, &tnr_ghost_attr);
	isp_tnr_ctrl_get_tnr_mt_prt_attr(ViPipe, &tnr_mt_prt_attr);
	isp_tnr_ctrl_get_tnr_motion_adapt_attr(ViPipe, &tnr_motion_adapt_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(tnr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!tnr_attr->Enable || runtime->is_module_bypass)
		return ret;

	isp_tnr_update_internal_motion_block_list(ViPipe, runtime, tnr_attr);

	if (tnr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(tnr_attr, TnrStrength0);
		MANUAL(tnr_attr, MapThdLow0);
		MANUAL(tnr_attr, MapThdHigh0);
		MANUAL(tnr_attr, MtDetectUnit);
		MANUAL(tnr_attr, BrightnessNoiseLevelLE);
		MANUAL(tnr_attr, BrightnessNoiseLevelSE);
		MANUAL(tnr_attr, MtFiltMode);
		MANUAL(tnr_attr, MtFiltWgt);

		MANUAL(tnr_noise_model_attr, BNoiseLevel0);
		MANUAL(tnr_noise_model_attr, GNoiseLevel0);
		MANUAL(tnr_noise_model_attr, RNoiseLevel0);
		MANUAL(tnr_noise_model_attr, BNoiseLevel1);
		MANUAL(tnr_noise_model_attr, GNoiseLevel1);
		MANUAL(tnr_noise_model_attr, RNoiseLevel1);
		MANUAL(tnr_noise_model_attr, BNoiseHiLevel0);
		MANUAL(tnr_noise_model_attr, GNoiseHiLevel0);
		MANUAL(tnr_noise_model_attr, RNoiseHiLevel0);
		MANUAL(tnr_noise_model_attr, BNoiseHiLevel1);
		MANUAL(tnr_noise_model_attr, GNoiseHiLevel1);
		MANUAL(tnr_noise_model_attr, RNoiseHiLevel1);

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			MANUAL(tnr_luma_motion_attr, L2mIn0[i]);
			MANUAL(tnr_luma_motion_attr, L2mIn1[i]);
			MANUAL(tnr_luma_motion_attr, L2mOut0[i]);
			MANUAL(tnr_luma_motion_attr, L2mOut1[i]);
		}
		MANUAL(tnr_luma_motion_attr, MtLumaMode);

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			MANUAL(tnr_ghost_attr, PrvMotion0[i]);
			MANUAL(tnr_ghost_attr, PrtctWgt0[i]);
		}
		MANUAL(tnr_ghost_attr, MotionHistoryStr);

		MANUAL(tnr_mt_prt_attr, LowMtPrtLevelY);
		MANUAL(tnr_mt_prt_attr, LowMtPrtLevelU);
		MANUAL(tnr_mt_prt_attr, LowMtPrtLevelV);
		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			MANUAL(tnr_mt_prt_attr, LowMtPrtInY[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtInU[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtInV[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtOutY[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtOutU[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtOutV[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtAdvIn[i]);
			MANUAL(tnr_mt_prt_attr, LowMtPrtAdvOut[i]);
		}

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			MANUAL(tnr_motion_adapt_attr, AdaptNrLumaStrIn[i]);
			MANUAL(tnr_motion_adapt_attr, AdaptNrLumaStrOut[i]);
			MANUAL(tnr_motion_adapt_attr, AdaptNrChromaStrIn[i]);
			MANUAL(tnr_motion_adapt_attr, AdaptNrChromaStrOut[i]);
		}

		#undef MANUAL

		// motion unit in manual mode only support >= 3 in cv182x and cv183x
		// PQTool support 1, 2 (but not in ISP)
		if (runtime->tnr_attr.MtDetectUnit < TNR_MOTION_BLOCK_MIN_UNIT) {
			runtime->tnr_attr.MtDetectUnit = TNR_MOTION_BLOCK_MIN_UNIT;
		}
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(tnr_attr, TnrStrength0, INTPLT_POST_ISO);
		AUTO(tnr_attr, MapThdLow0, INTPLT_POST_ISO);
		AUTO(tnr_attr, MapThdHigh0, INTPLT_POST_ISO);
		// AUTO(tnr_attr, MtDetectUnit, INTPLT_POST_ISO);
		runtime->tnr_attr.MtDetectUnit = INTERPOLATE_LINEAR(ViPipe, INTPLT_POST_ISO, runtime->au8MtDetectUnit);
		AUTO(tnr_attr, BrightnessNoiseLevelLE, INTPLT_POST_ISO);
		AUTO(tnr_attr, BrightnessNoiseLevelSE, INTPLT_POST_ISO);
		AUTO(tnr_attr, MtFiltMode, INTPLT_POST_ISO);
		AUTO(tnr_attr, MtFiltWgt, INTPLT_POST_ISO);

		AUTO(tnr_noise_model_attr, BNoiseLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, GNoiseLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, RNoiseLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, BNoiseLevel1, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, GNoiseLevel1, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, RNoiseLevel1, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, BNoiseHiLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, GNoiseHiLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, RNoiseHiLevel0, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, BNoiseHiLevel1, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, GNoiseHiLevel1, INTPLT_POST_ISO);
		AUTO(tnr_noise_model_attr, RNoiseHiLevel1, INTPLT_POST_ISO);

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			AUTO(tnr_luma_motion_attr, L2mIn0[i], INTPLT_POST_ISO);
			AUTO(tnr_luma_motion_attr, L2mIn1[i], INTPLT_POST_ISO);
			AUTO(tnr_luma_motion_attr, L2mOut0[i], INTPLT_POST_ISO);
			AUTO(tnr_luma_motion_attr, L2mOut1[i], INTPLT_POST_ISO);
		}
		AUTO(tnr_luma_motion_attr, MtLumaMode, INTPLT_POST_ISO);

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			AUTO(tnr_ghost_attr, PrvMotion0[i], INTPLT_POST_ISO);
			AUTO(tnr_ghost_attr, PrtctWgt0[i], INTPLT_POST_ISO);
		}
		AUTO(tnr_ghost_attr, MotionHistoryStr, INTPLT_POST_ISO);

		AUTO(tnr_mt_prt_attr, LowMtPrtLevelY, INTPLT_POST_ISO);
		AUTO(tnr_mt_prt_attr, LowMtPrtLevelU, INTPLT_POST_ISO);
		AUTO(tnr_mt_prt_attr, LowMtPrtLevelV, INTPLT_POST_ISO);
		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			AUTO(tnr_mt_prt_attr, LowMtPrtInY[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtInU[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtInV[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtOutY[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtOutU[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtOutV[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtAdvIn[i], INTPLT_POST_ISO);
			AUTO(tnr_mt_prt_attr, LowMtPrtAdvOut[i], INTPLT_POST_ISO);
		}

		for (CVI_U32 i = 0 ; i < 4 ; i++) {
			AUTO(tnr_motion_adapt_attr, AdaptNrLumaStrIn[i], INTPLT_POST_ISO);
			AUTO(tnr_motion_adapt_attr, AdaptNrLumaStrOut[i], INTPLT_POST_ISO);
			AUTO(tnr_motion_adapt_attr, AdaptNrChromaStrIn[i], INTPLT_POST_ISO);
			AUTO(tnr_motion_adapt_attr, AdaptNrChromaStrOut[i], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	for (CVI_U32 i = 0 ; i < 4 ; i++) {
		runtime->tnr_param_in.L2mIn0[i] = runtime->tnr_luma_motion_attr.L2mIn0[i];
		runtime->tnr_param_in.L2mIn1[i] = runtime->tnr_luma_motion_attr.L2mIn1[i];
		runtime->tnr_param_in.L2mOut0[i] = runtime->tnr_luma_motion_attr.L2mOut0[i];
		runtime->tnr_param_in.L2mOut1[i] = runtime->tnr_luma_motion_attr.L2mOut1[i];
		runtime->tnr_param_in.PrvMotion0[i] = runtime->tnr_ghost_attr.PrvMotion0[i];
		runtime->tnr_param_in.PrtctWgt0[i] = runtime->tnr_ghost_attr.PrtctWgt0[i];
		runtime->tnr_param_in.LowMtPrtInY[i] = runtime->tnr_mt_prt_attr.LowMtPrtInY[i];
		runtime->tnr_param_in.LowMtPrtOutY[i] = runtime->tnr_mt_prt_attr.LowMtPrtOutY[i];
		runtime->tnr_param_in.LowMtPrtInU[i] = runtime->tnr_mt_prt_attr.LowMtPrtInU[i];
		runtime->tnr_param_in.LowMtPrtOutU[i] = runtime->tnr_mt_prt_attr.LowMtPrtOutU[i];
		runtime->tnr_param_in.LowMtPrtInV[i] = runtime->tnr_mt_prt_attr.LowMtPrtInV[i];
		runtime->tnr_param_in.LowMtPrtOutV[i] = runtime->tnr_mt_prt_attr.LowMtPrtOutV[i];
		runtime->tnr_param_in.LowMtPrtAdvIn[i] = runtime->tnr_mt_prt_attr.LowMtPrtAdvIn[i];
		runtime->tnr_param_in.LowMtPrtAdvOut[i] = runtime->tnr_mt_prt_attr.LowMtPrtAdvOut[i];
		runtime->tnr_param_in.LowMtPrtAdvDebugIn[i] = tnr_mt_prt_attr->LowMtPrtAdvDebugIn[i];
		runtime->tnr_param_in.LowMtPrtAdvDebugOut[i] = tnr_mt_prt_attr->LowMtPrtAdvDebugOut[i];
		runtime->tnr_param_in.AdaptNrLumaStrIn[i] = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrIn[i];
		runtime->tnr_param_in.AdaptNrLumaStrOut[i] = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrOut[i];
		runtime->tnr_param_in.AdaptNrChromaStrIn[i] = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrIn[i];
		runtime->tnr_param_in.AdaptNrChromaStrOut[i] = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrOut[i];
	}

	CVI_U8 _444_422_mode = tnr_attr->ChromaScalingDownMode;

	if (_444_422_mode == 0) {
		runtime->avg_mode_write = 1;
		runtime->drop_mode_write = 0;
	} else if (_444_422_mode == 1) {
		runtime->avg_mode_write = 0;
		runtime->drop_mode_write = 0;
	} else if (_444_422_mode == 2) {
		runtime->avg_mode_write = 0;
		runtime->drop_mode_write = 1;
	}  else if (_444_422_mode == 3) {
		runtime->avg_mode_write = 0;
		runtime->drop_mode_write = !runtime->drop_mode_write;
	}

	// TODO@CV181X check compensate gain effective timing
	for (CVI_U32 i = 0 ; i < ISP_BAYER_CHN_NUM ; i++) {
#if ENABLE_FE_WBG_UPDATE // removed fe wbg update in isp_wbg_ctrl.c
		CVI_U32 u32RatioTNRbase =
			(CVI_U32) algoResult->au32WhiteBalanceGainPre[i] << TNR_BASE_1X_GAIN_BIT;
		u32RatioTNRbase = u32RatioTNRbase / algoResult->au32WhiteBalanceGain[i];
#else
		CVI_U32 u32RatioTNRbase = TNR_BASE_1X_GAIN;
#endif
		CVI_U32 tmpMaxThr = TNR_BASE_1X_GAIN * 2;
		CVI_U32 tmpMinThr = TNR_BASE_1X_GAIN / 2;
		CVI_U16 tmpRatio = 0;

		for (CVI_U32 j = 0; j < ISP_CHANNEL_MAX_NUM; j++) {
			tmpRatio = (CVI_U16) ((CVI_FLOAT) u32RatioTNRbase * algoResult->afAEEVRatio[j]);
			if (tmpRatio > tmpMaxThr || tmpRatio < tmpMinThr) {
				runtime->GainCompensateRatio[j][i] = TNR_BASE_1X_GAIN;
			}
		}
	}

	// ParamIn
	CVI_ISP_GetNoiseProfileAttr(ViPipe, &runtime->tnr_param_in.np);
	runtime->tnr_param_in.iso = algoResult->u32PostNpIso;
	runtime->tnr_param_in.ispdgain = algoResult->u32IspPostDgain;
#if ENABLE_FE_WBG_UPDATE
	runtime->tnr_param_in.wb_rgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_R];
	runtime->tnr_param_in.wb_bgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_B];
#else
	runtime->tnr_param_in.wb_rgain = WB_UNIT_GAIN;
	runtime->tnr_param_in.wb_bgain = WB_UNIT_GAIN;
#endif
	runtime->tnr_param_in.gbmap_input_sel = 0;
	runtime->tnr_param_in.rgbmap_w_bit = runtime->tnr_attr.MtDetectUnit;
	runtime->tnr_param_in.rgbmap_h_bit = runtime->tnr_attr.MtDetectUnit;

	runtime->tnr_param_in.noiseLowLv[0][0] = runtime->tnr_noise_model_attr.BNoiseLevel0;
	runtime->tnr_param_in.noiseLowLv[0][1] = runtime->tnr_noise_model_attr.GNoiseLevel0;
	runtime->tnr_param_in.noiseLowLv[0][2] = runtime->tnr_noise_model_attr.RNoiseLevel0;
	runtime->tnr_param_in.noiseLowLv[1][0] = runtime->tnr_noise_model_attr.BNoiseLevel1;
	runtime->tnr_param_in.noiseLowLv[1][1] = runtime->tnr_noise_model_attr.GNoiseLevel1;
	runtime->tnr_param_in.noiseLowLv[1][2] = runtime->tnr_noise_model_attr.RNoiseLevel1;

	runtime->tnr_param_in.noiseHiLv[0][0] = runtime->tnr_noise_model_attr.BNoiseHiLevel0;
	runtime->tnr_param_in.noiseHiLv[0][1] = runtime->tnr_noise_model_attr.GNoiseHiLevel0;
	runtime->tnr_param_in.noiseHiLv[0][2] = runtime->tnr_noise_model_attr.RNoiseHiLevel0;
	runtime->tnr_param_in.noiseHiLv[1][0] = runtime->tnr_noise_model_attr.BNoiseHiLevel1;
	runtime->tnr_param_in.noiseHiLv[1][1] = runtime->tnr_noise_model_attr.GNoiseHiLevel1;
	runtime->tnr_param_in.noiseHiLv[1][2] = runtime->tnr_noise_model_attr.RNoiseHiLevel1;
	runtime->tnr_param_in.BrightnessNoiseLevelLE[0] = runtime->tnr_attr.BrightnessNoiseLevelLE;
	runtime->tnr_param_in.BrightnessNoiseLevelLE[1] = runtime->tnr_attr.BrightnessNoiseLevelSE;
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_tnr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_tnr_main(
		(struct tnr_param_in *)&runtime->tnr_param_in,
		(struct tnr_param_out *)&runtime->tnr_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_tnr_ctrl_postprocess(VI_PIPE ViPipe)
{
	// ISP_CTX_S *pstIspCtx = NULL;
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_tnr_config *tnr_cfg =
		(struct cvi_vip_isp_tnr_config *)&(post_addr->tun_cfg[tun_idx].tnr_cfg);

	const ISP_TNR_ATTR_S *tnr_attr = NULL;
	const ISP_TNR_GHOST_ATTR_S *tnr_ghost_attr = NULL;
	const ISP_TNR_MT_PRT_ATTR_S *tnr_mt_prt_attr = NULL;

	isp_tnr_ctrl_get_tnr_attr(ViPipe, &tnr_attr);
	isp_tnr_ctrl_get_tnr_ghost_attr(ViPipe, &tnr_ghost_attr);
	isp_tnr_ctrl_get_tnr_mt_prt_attr(ViPipe, &tnr_mt_prt_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		tnr_cfg->update = 0;
	} else {
		tnr_cfg->update = 1;
		tnr_cfg->manr_enable = tnr_attr->Enable && !runtime->is_module_bypass;
		tnr_cfg->rgbmap_w_bit = runtime->tnr_attr.MtDetectUnit;
		tnr_cfg->rgbmap_h_bit = runtime->tnr_attr.MtDetectUnit;
		tnr_cfg->mh_wgt = runtime->tnr_ghost_attr.MotionHistoryStr;
		CVI_U8 MtFltCoef[9] = {3, 4, 3, 4, 4, 4, 3, 4, 3};

		memcpy(tnr_cfg->lpf, MtFltCoef, sizeof(CVI_U8) * 9);
		tnr_cfg->map_gain = runtime->tnr_attr.TnrStrength0;

		tnr_cfg->map_thd_l = runtime->tnr_attr.MapThdLow0;
		tnr_cfg->map_thd_h = runtime->tnr_attr.MapThdHigh0;
		tnr_cfg->uv_rounding_type_sel = 1;
		tnr_cfg->history_sel_0 = tnr_attr->TnrMtMode;
		tnr_cfg->history_sel_1 = tnr_attr->YnrCnrSharpenMtMode;
		tnr_cfg->history_sel_3 = tnr_attr->PreSharpenMtMode;
		tnr_cfg->tdnr_debug_sel = tnr_attr->TuningMode;
		tnr_cfg->luma_adapt_lut_slope_2 = runtime->tnr_param_out.L2mSlope0[2];
		tnr_cfg->med_enable = runtime->tnr_attr.MtFiltMode;
		tnr_cfg->med_wgt = runtime->tnr_attr.MtFiltWgt;
		tnr_cfg->mtluma_mode = runtime->tnr_luma_motion_attr.MtLumaMode;
		tnr_cfg->avg_mode_write = runtime->avg_mode_write;
		tnr_cfg->drop_mode_write = runtime->drop_mode_write;
		tnr_cfg->tdnr_pixel_lp = tnr_mt_prt_attr->LowMtLowPassEnable;
		tnr_cfg->tdnr_comp_gain_enable = tnr_attr->CompGainEnable;
		tnr_cfg->tdnr_ee_comp_gain = runtime->tnr_param_out.SharpenCompGain;

		struct cvi_isp_tnr_tun_cfg *cfg_0 = &(tnr_cfg->tnr_cfg);

		cfg_0->REG_0C.bits.MMAP_0_LUMA_ADAPT_LUT_IN_0 = runtime->tnr_luma_motion_attr.L2mIn0[0];
		cfg_0->REG_0C.bits.MMAP_0_LUMA_ADAPT_LUT_IN_1 = runtime->tnr_luma_motion_attr.L2mIn0[1];
		cfg_0->REG_10.bits.MMAP_0_LUMA_ADAPT_LUT_IN_2 = runtime->tnr_luma_motion_attr.L2mIn0[2];
		cfg_0->REG_10.bits.MMAP_0_LUMA_ADAPT_LUT_IN_3 = runtime->tnr_luma_motion_attr.L2mIn0[3];
		cfg_0->REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_0 = runtime->tnr_luma_motion_attr.L2mOut0[0];
		cfg_0->REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_1 = runtime->tnr_luma_motion_attr.L2mOut0[1];
		cfg_0->REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_2 = runtime->tnr_luma_motion_attr.L2mOut0[2];
		cfg_0->REG_14.bits.MMAP_0_LUMA_ADAPT_LUT_OUT_3 = runtime->tnr_luma_motion_attr.L2mOut0[3];
		cfg_0->REG_18.bits.MMAP_0_LUMA_ADAPT_LUT_SLOPE_0 = runtime->tnr_param_out.L2mSlope0[0];
		cfg_0->REG_18.bits.MMAP_0_LUMA_ADAPT_LUT_SLOPE_1 = runtime->tnr_param_out.L2mSlope0[1];

		struct cvi_isp_tnr_tun_1_cfg *cfg_1 = &(tnr_cfg->tnr_1_cfg);

		cfg_1->REG_4C.bits.MMAP_1_LUMA_ADAPT_LUT_IN_0 = runtime->tnr_luma_motion_attr.L2mIn1[0];
		cfg_1->REG_4C.bits.MMAP_1_LUMA_ADAPT_LUT_IN_1 = runtime->tnr_luma_motion_attr.L2mIn1[1];
		cfg_1->REG_50.bits.MMAP_1_LUMA_ADAPT_LUT_IN_2 = runtime->tnr_luma_motion_attr.L2mIn1[2];
		cfg_1->REG_50.bits.MMAP_1_LUMA_ADAPT_LUT_IN_3 = runtime->tnr_luma_motion_attr.L2mIn1[3];
		cfg_1->REG_54.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_0 = runtime->tnr_luma_motion_attr.L2mOut1[0];
		cfg_1->REG_54.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_1 = runtime->tnr_luma_motion_attr.L2mOut1[1];
		cfg_1->REG_54.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_2 = runtime->tnr_luma_motion_attr.L2mOut1[2];
		cfg_1->REG_54.bits.MMAP_1_LUMA_ADAPT_LUT_OUT_3 = runtime->tnr_luma_motion_attr.L2mOut1[3];
		cfg_1->REG_58.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_0 = runtime->tnr_param_out.L2mSlope1[0];
		cfg_1->REG_58.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_1 = runtime->tnr_param_out.L2mSlope1[1];
		cfg_1->REG_5C.bits.MMAP_1_LUMA_ADAPT_LUT_SLOPE_2 = runtime->tnr_param_out.L2mSlope1[2];

		struct cvi_isp_tnr_tun_2_cfg *cfg_2 = &(tnr_cfg->tnr_2_cfg);

		cfg_2->REG_70.bits.MMAP_0_GAIN_RATIO_R = runtime->GainCompensateRatio[ISP_CHANNEL_LE][ISP_BAYER_CHN_R];
		cfg_2->REG_70.bits.MMAP_0_GAIN_RATIO_G = runtime->GainCompensateRatio[ISP_CHANNEL_LE][ISP_BAYER_CHN_GR];
		cfg_2->REG_74.bits.MMAP_0_GAIN_RATIO_B = runtime->GainCompensateRatio[ISP_CHANNEL_LE][ISP_BAYER_CHN_B];
		cfg_2->REG_78.bits.MMAP_0_NS_SLOPE_R = runtime->tnr_param_out.slope[0][2];
		cfg_2->REG_78.bits.MMAP_0_NS_SLOPE_G = runtime->tnr_param_out.slope[0][1];
		cfg_2->REG_7C.bits.MMAP_0_NS_SLOPE_B = runtime->tnr_param_out.slope[0][0];
		cfg_2->REG_80.bits.MMAP_0_NS_LUMA_TH0_R = runtime->tnr_param_out.luma[0][2];
		cfg_2->REG_80.bits.MMAP_0_NS_LUMA_TH0_G = runtime->tnr_param_out.luma[0][1];
		cfg_2->REG_84.bits.MMAP_0_NS_LUMA_TH0_B = runtime->tnr_param_out.luma[0][0];
		cfg_2->REG_84.bits.MMAP_0_NS_LOW_OFFSET_R = runtime->tnr_param_out.low[0][2];
		cfg_2->REG_88.bits.MMAP_0_NS_LOW_OFFSET_G = runtime->tnr_param_out.low[0][1];
		cfg_2->REG_88.bits.MMAP_0_NS_LOW_OFFSET_B = runtime->tnr_param_out.low[0][0];
		cfg_2->REG_8C.bits.MMAP_0_NS_HIGH_OFFSET_R = runtime->tnr_param_out.high[0][2];
		cfg_2->REG_8C.bits.MMAP_0_NS_HIGH_OFFSET_G = runtime->tnr_param_out.high[0][1];
		cfg_2->REG_90.bits.MMAP_0_NS_HIGH_OFFSET_B = runtime->tnr_param_out.high[0][0];

		struct cvi_isp_tnr_tun_3_cfg *cfg_3 = &(tnr_cfg->tnr_3_cfg);

		cfg_3->REG_A0.bits.MMAP_1_GAIN_RATIO_R = runtime->GainCompensateRatio[ISP_CHANNEL_SE][ISP_BAYER_CHN_R];
		cfg_3->REG_A0.bits.MMAP_1_GAIN_RATIO_G = runtime->GainCompensateRatio[ISP_CHANNEL_SE][ISP_BAYER_CHN_GR];
		cfg_3->REG_A4.bits.MMAP_1_GAIN_RATIO_B = runtime->GainCompensateRatio[ISP_CHANNEL_SE][ISP_BAYER_CHN_B];
		cfg_3->REG_A8.bits.MMAP_1_NS_SLOPE_R = runtime->tnr_param_out.slope[1][2];
		cfg_3->REG_A8.bits.MMAP_1_NS_SLOPE_G = runtime->tnr_param_out.slope[1][1];
		cfg_3->REG_AC.bits.MMAP_1_NS_SLOPE_B = runtime->tnr_param_out.slope[1][0];
		cfg_3->REG_B0.bits.MMAP_1_NS_LUMA_TH0_R = runtime->tnr_param_out.luma[1][2];
		cfg_3->REG_B0.bits.MMAP_1_NS_LUMA_TH0_G = runtime->tnr_param_out.luma[1][1];
		cfg_3->REG_B4.bits.MMAP_1_NS_LUMA_TH0_B = runtime->tnr_param_out.luma[1][0];
		cfg_3->REG_B4.bits.MMAP_1_NS_LOW_OFFSET_R = runtime->tnr_param_out.low[1][2];
		cfg_3->REG_B8.bits.MMAP_1_NS_LOW_OFFSET_G = runtime->tnr_param_out.low[1][1];
		cfg_3->REG_B8.bits.MMAP_1_NS_LOW_OFFSET_B = runtime->tnr_param_out.low[1][0];
		cfg_3->REG_BC.bits.MMAP_1_NS_HIGH_OFFSET_R = runtime->tnr_param_out.high[1][2];
		cfg_3->REG_BC.bits.MMAP_1_NS_HIGH_OFFSET_G = runtime->tnr_param_out.high[1][1];
		cfg_3->REG_C0.bits.MMAP_1_NS_HIGH_OFFSET_B = runtime->tnr_param_out.high[1][0];

		struct cvi_isp_tnr_tun_4_cfg *cfg_4 = &(tnr_cfg->tnr_4_cfg);

		cfg_4->REG_13.bits.REG_3DNR_Y_LUT_IN_0 = runtime->tnr_mt_prt_attr.LowMtPrtInY[0];
		cfg_4->REG_13.bits.REG_3DNR_Y_LUT_IN_1 = runtime->tnr_mt_prt_attr.LowMtPrtInY[1];
		cfg_4->REG_13.bits.REG_3DNR_Y_LUT_IN_2 = runtime->tnr_mt_prt_attr.LowMtPrtInY[2];
		cfg_4->REG_13.bits.REG_3DNR_Y_LUT_IN_3 = runtime->tnr_mt_prt_attr.LowMtPrtInY[3];
		cfg_4->REG_14.bits.REG_3DNR_Y_LUT_OUT_0 = runtime->tnr_mt_prt_attr.LowMtPrtOutY[0];
		cfg_4->REG_14.bits.REG_3DNR_Y_LUT_OUT_1 = runtime->tnr_mt_prt_attr.LowMtPrtOutY[1];
		cfg_4->REG_14.bits.REG_3DNR_Y_LUT_OUT_2 = runtime->tnr_mt_prt_attr.LowMtPrtOutY[2];
		cfg_4->REG_14.bits.REG_3DNR_Y_LUT_OUT_3 = runtime->tnr_mt_prt_attr.LowMtPrtOutY[3];
		cfg_4->REG_15.bits.REG_3DNR_Y_LUT_SLOPE_0 = runtime->tnr_param_out.LowMtPrtSlopeY[0];
		cfg_4->REG_15.bits.REG_3DNR_Y_LUT_SLOPE_1 = runtime->tnr_param_out.LowMtPrtSlopeY[1];
		cfg_4->REG_16.bits.REG_3DNR_Y_LUT_SLOPE_2 = runtime->tnr_param_out.LowMtPrtSlopeY[2];
		cfg_4->REG_16.bits.MOTION_SEL = tnr_mt_prt_attr->LowMtPrtEn;
		cfg_4->REG_16.bits.REG_3DNR_Y_BETA_MAX = runtime->tnr_mt_prt_attr.LowMtPrtLevelY;
		cfg_4->REG_17.bits.REG_3DNR_U_LUT_IN_0 = runtime->tnr_mt_prt_attr.LowMtPrtInU[0];
		cfg_4->REG_17.bits.REG_3DNR_U_LUT_IN_1 = runtime->tnr_mt_prt_attr.LowMtPrtInU[1];
		cfg_4->REG_17.bits.REG_3DNR_U_LUT_IN_2 = runtime->tnr_mt_prt_attr.LowMtPrtInU[2];
		cfg_4->REG_17.bits.REG_3DNR_U_LUT_IN_3 = runtime->tnr_mt_prt_attr.LowMtPrtInU[3];
		cfg_4->REG_18.bits.REG_3DNR_U_LUT_OUT_0 = runtime->tnr_mt_prt_attr.LowMtPrtOutU[0];
		cfg_4->REG_18.bits.REG_3DNR_U_LUT_OUT_1 = runtime->tnr_mt_prt_attr.LowMtPrtOutU[1];
		cfg_4->REG_18.bits.REG_3DNR_U_LUT_OUT_2 = runtime->tnr_mt_prt_attr.LowMtPrtOutU[2];
		cfg_4->REG_18.bits.REG_3DNR_U_LUT_OUT_3 = runtime->tnr_mt_prt_attr.LowMtPrtOutU[3];
		cfg_4->REG_19.bits.REG_3DNR_U_LUT_SLOPE_0 = runtime->tnr_param_out.LowMtPrtSlopeU[0];
		cfg_4->REG_19.bits.REG_3DNR_U_LUT_SLOPE_1 = runtime->tnr_param_out.LowMtPrtSlopeU[1];
		cfg_4->REG_20.bits.REG_3DNR_U_LUT_SLOPE_2 = runtime->tnr_param_out.LowMtPrtSlopeU[2];
		cfg_4->REG_20.bits.REG_3DNR_U_BETA_MAX = runtime->tnr_mt_prt_attr.LowMtPrtLevelU;
		cfg_4->REG_21.bits.REG_3DNR_V_LUT_IN_0 = runtime->tnr_mt_prt_attr.LowMtPrtInV[0];
		cfg_4->REG_21.bits.REG_3DNR_V_LUT_IN_1 = runtime->tnr_mt_prt_attr.LowMtPrtInV[1];
		cfg_4->REG_21.bits.REG_3DNR_V_LUT_IN_2 = runtime->tnr_mt_prt_attr.LowMtPrtInV[2];
		cfg_4->REG_21.bits.REG_3DNR_V_LUT_IN_3 = runtime->tnr_mt_prt_attr.LowMtPrtInV[3];
		cfg_4->REG_22.bits.REG_3DNR_V_LUT_OUT_0 = runtime->tnr_mt_prt_attr.LowMtPrtOutV[0];
		cfg_4->REG_22.bits.REG_3DNR_V_LUT_OUT_1 = runtime->tnr_mt_prt_attr.LowMtPrtOutV[1];
		cfg_4->REG_22.bits.REG_3DNR_V_LUT_OUT_2 = runtime->tnr_mt_prt_attr.LowMtPrtOutV[2];
		cfg_4->REG_22.bits.REG_3DNR_V_LUT_OUT_3 = runtime->tnr_mt_prt_attr.LowMtPrtOutV[3];
		cfg_4->REG_23.bits.REG_3DNR_V_LUT_SLOPE_0 = runtime->tnr_param_out.LowMtPrtSlopeV[0];
		cfg_4->REG_23.bits.REG_3DNR_V_LUT_SLOPE_1 = runtime->tnr_param_out.LowMtPrtSlopeV[1];
		cfg_4->REG_24.bits.REG_3DNR_V_LUT_SLOPE_2 = runtime->tnr_param_out.LowMtPrtSlopeV[2];
		cfg_4->REG_24.bits.REG_3DNR_V_BETA_MAX = runtime->tnr_mt_prt_attr.LowMtPrtLevelV;
		cfg_4->REG_25.bits.REG_3DNR_MOTION_Y_LUT_IN_0 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrIn[0];
		cfg_4->REG_25.bits.REG_3DNR_MOTION_Y_LUT_IN_1 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrIn[1];
		cfg_4->REG_25.bits.REG_3DNR_MOTION_Y_LUT_IN_2 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrIn[2];
		cfg_4->REG_25.bits.REG_3DNR_MOTION_Y_LUT_IN_3 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrIn[3];
		cfg_4->REG_26.bits.REG_3DNR_MOTION_Y_LUT_OUT_0 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrOut[0];
		cfg_4->REG_26.bits.REG_3DNR_MOTION_Y_LUT_OUT_1 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrOut[1];
		cfg_4->REG_26.bits.REG_3DNR_MOTION_Y_LUT_OUT_2 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrOut[2];
		cfg_4->REG_26.bits.REG_3DNR_MOTION_Y_LUT_OUT_3 = runtime->tnr_motion_adapt_attr.AdaptNrLumaStrOut[3];
		cfg_4->REG_27.bits.REG_3DNR_MOTION_Y_LUT_SLOPE_0 = runtime->tnr_param_out.AdaptNrLumaSlope[0];
		cfg_4->REG_27.bits.REG_3DNR_MOTION_Y_LUT_SLOPE_1 = runtime->tnr_param_out.AdaptNrLumaSlope[1];
		cfg_4->REG_28.bits.REG_3DNR_MOTION_Y_LUT_SLOPE_2 = runtime->tnr_param_out.AdaptNrLumaSlope[2];
		cfg_4->REG_28.bits.REG_3DNR_MOTION_C_LUT_SLOPE_0 = runtime->tnr_param_out.AdaptNrChromaSlope[0];
		cfg_4->REG_29.bits.REG_3DNR_MOTION_C_LUT_SLOPE_1 = runtime->tnr_param_out.AdaptNrChromaSlope[1];
		cfg_4->REG_29.bits.REG_3DNR_MOTION_C_LUT_SLOPE_2 = runtime->tnr_param_out.AdaptNrChromaSlope[2];
		cfg_4->REG_30.bits.REG_3DNR_MOTION_C_LUT_IN_0 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrIn[0];
		cfg_4->REG_30.bits.REG_3DNR_MOTION_C_LUT_IN_1 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrIn[1];
		cfg_4->REG_30.bits.REG_3DNR_MOTION_C_LUT_IN_2 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrIn[2];
		cfg_4->REG_30.bits.REG_3DNR_MOTION_C_LUT_IN_3 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrIn[3];
		cfg_4->REG_31.bits.REG_3DNR_MOTION_C_LUT_OUT_0 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrOut[0];
		cfg_4->REG_31.bits.REG_3DNR_MOTION_C_LUT_OUT_1 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrOut[1];
		cfg_4->REG_31.bits.REG_3DNR_MOTION_C_LUT_OUT_2 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrOut[2];
		cfg_4->REG_31.bits.REG_3DNR_MOTION_C_LUT_OUT_3 = runtime->tnr_motion_adapt_attr.AdaptNrChromaStrOut[3];

		struct cvi_isp_tnr_tun_5_cfg *cfg_5 = &(tnr_cfg->tnr_5_cfg);

		cfg_5->REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_0 = runtime->tnr_ghost_attr.PrvMotion0[0];
		cfg_5->REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_1 = runtime->tnr_ghost_attr.PrvMotion0[1];
		cfg_5->REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_2 = runtime->tnr_ghost_attr.PrvMotion0[2];
		cfg_5->REG_20.bits.MMAP_0_IIR_PRTCT_LUT_IN_3 = runtime->tnr_ghost_attr.PrvMotion0[3];
		cfg_5->REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_0 = runtime->tnr_ghost_attr.PrtctWgt0[0];
		cfg_5->REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_1 = runtime->tnr_ghost_attr.PrtctWgt0[1];
		cfg_5->REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_2 = runtime->tnr_ghost_attr.PrtctWgt0[2];
		cfg_5->REG_24.bits.MMAP_0_IIR_PRTCT_LUT_OUT_3 = runtime->tnr_ghost_attr.PrtctWgt0[3];
		cfg_5->REG_28.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_0 = runtime->tnr_param_out.PrtctSlope[0];
		cfg_5->REG_28.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_1 = runtime->tnr_param_out.PrtctSlope[1];
		cfg_5->REG_2C.bits.MMAP_0_IIR_PRTCT_LUT_SLOPE_2 = runtime->tnr_param_out.PrtctSlope[2];

		struct cvi_isp_tnr_tun_6_cfg *cfg_6 = &(tnr_cfg->tnr_6_cfg);

		// cfg_6->REG_80.bits.REG_3DNR_EE_COMP_GAIN = runtime->tnr_param_out.SharpenCompGain;
		cfg_6->REG_84.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_00 = runtime->tnr_param_out.LumaCompGain[0];
		cfg_6->REG_84.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_01 = runtime->tnr_param_out.LumaCompGain[1];
		cfg_6->REG_88.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_02 = runtime->tnr_param_out.LumaCompGain[2];
		cfg_6->REG_88.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_03 = runtime->tnr_param_out.LumaCompGain[3];
		cfg_6->REG_8C.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_04 = runtime->tnr_param_out.LumaCompGain[4];
		cfg_6->REG_8C.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_05 = runtime->tnr_param_out.LumaCompGain[5];
		cfg_6->REG_90.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_06 = runtime->tnr_param_out.LumaCompGain[6];
		cfg_6->REG_90.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_07 = runtime->tnr_param_out.LumaCompGain[7];
		cfg_6->REG_94.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_08 = runtime->tnr_param_out.LumaCompGain[8];
		cfg_6->REG_94.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_09 = runtime->tnr_param_out.LumaCompGain[9];
		cfg_6->REG_98.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_10 = runtime->tnr_param_out.LumaCompGain[10];
		cfg_6->REG_98.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_11 = runtime->tnr_param_out.LumaCompGain[11];
		cfg_6->REG_9C.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_12 = runtime->tnr_param_out.LumaCompGain[12];
		cfg_6->REG_9C.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_13 = runtime->tnr_param_out.LumaCompGain[13];
		cfg_6->REG_A0.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_14 = runtime->tnr_param_out.LumaCompGain[14];
		cfg_6->REG_A0.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_15 = runtime->tnr_param_out.LumaCompGain[15];
		cfg_6->REG_A4.bits.REG_3DNR_LUMA_COMP_GAIN_LUT_16 = runtime->tnr_param_out.LumaCompGain[16];
		cfg_6->REG_A8.bits.REG_3DNR_LSC_CENTERX = runtime->tnr_param_out.TnrLscCenterX;
		cfg_6->REG_A8.bits.REG_3DNR_LSC_CENTERY = runtime->tnr_param_out.TnrLscCenterY;
		cfg_6->REG_AC.bits.REG_3DNR_LSC_NORM = runtime->tnr_param_out.TnrLscNorm;
		cfg_6->REG_AC.bits.REG_3DNR_LSC_DY_GAIN = runtime->tnr_param_out.TnrLscDYGain;
		cfg_6->REG_B0.bits.REG_3DNR_LSC_COMP_GAIN_LUT_00 = runtime->tnr_param_out.TnrLscCompGain[0];
		cfg_6->REG_B0.bits.REG_3DNR_LSC_COMP_GAIN_LUT_01 = runtime->tnr_param_out.TnrLscCompGain[1];
		cfg_6->REG_B4.bits.REG_3DNR_LSC_COMP_GAIN_LUT_02 = runtime->tnr_param_out.TnrLscCompGain[2];
		cfg_6->REG_B4.bits.REG_3DNR_LSC_COMP_GAIN_LUT_03 = runtime->tnr_param_out.TnrLscCompGain[3];
		cfg_6->REG_B8.bits.REG_3DNR_LSC_COMP_GAIN_LUT_04 = runtime->tnr_param_out.TnrLscCompGain[4];
		cfg_6->REG_B8.bits.REG_3DNR_LSC_COMP_GAIN_LUT_05 = runtime->tnr_param_out.TnrLscCompGain[5];
		cfg_6->REG_BC.bits.REG_3DNR_LSC_COMP_GAIN_LUT_06 = runtime->tnr_param_out.TnrLscCompGain[6];
		cfg_6->REG_BC.bits.REG_3DNR_LSC_COMP_GAIN_LUT_07 = runtime->tnr_param_out.TnrLscCompGain[7];
		cfg_6->REG_C0.bits.REG_3DNR_LSC_COMP_GAIN_LUT_08 = runtime->tnr_param_out.TnrLscCompGain[8];
		cfg_6->REG_C0.bits.REG_3DNR_LSC_COMP_GAIN_LUT_09 = runtime->tnr_param_out.TnrLscCompGain[9];
		cfg_6->REG_C4.bits.REG_3DNR_LSC_COMP_GAIN_LUT_10 = runtime->tnr_param_out.TnrLscCompGain[10];
		cfg_6->REG_C4.bits.REG_3DNR_LSC_COMP_GAIN_LUT_11 = runtime->tnr_param_out.TnrLscCompGain[11];
		cfg_6->REG_C8.bits.REG_3DNR_LSC_COMP_GAIN_LUT_12 = runtime->tnr_param_out.TnrLscCompGain[12];
		cfg_6->REG_C8.bits.REG_3DNR_LSC_COMP_GAIN_LUT_13 = runtime->tnr_param_out.TnrLscCompGain[13];
		cfg_6->REG_CC.bits.REG_3DNR_LSC_COMP_GAIN_LUT_14 = runtime->tnr_param_out.TnrLscCompGain[14];
		cfg_6->REG_CC.bits.REG_3DNR_LSC_COMP_GAIN_LUT_15 = runtime->tnr_param_out.TnrLscCompGain[15];
		cfg_6->REG_D0.bits.REG_3DNR_LSC_COMP_GAIN_LUT_16 = runtime->tnr_param_out.TnrLscCompGain[16];
		cfg_6->REG_D4.bits.REG_3DNR_Y_GAIN_LUT_IN_0 = runtime->tnr_mt_prt_attr.LowMtPrtAdvIn[0];
		cfg_6->REG_D4.bits.REG_3DNR_Y_GAIN_LUT_IN_1 = runtime->tnr_mt_prt_attr.LowMtPrtAdvIn[1];
		cfg_6->REG_D4.bits.REG_3DNR_Y_GAIN_LUT_IN_2 = runtime->tnr_mt_prt_attr.LowMtPrtAdvIn[2];
		cfg_6->REG_D4.bits.REG_3DNR_Y_GAIN_LUT_IN_3 = runtime->tnr_mt_prt_attr.LowMtPrtAdvIn[3];
		cfg_6->REG_D8.bits.REG_3DNR_Y_GAIN_LUT_OUT_0 = runtime->tnr_mt_prt_attr.LowMtPrtAdvOut[0];
		cfg_6->REG_D8.bits.REG_3DNR_Y_GAIN_LUT_OUT_1 = runtime->tnr_mt_prt_attr.LowMtPrtAdvOut[1];
		cfg_6->REG_D8.bits.REG_3DNR_Y_GAIN_LUT_OUT_2 = runtime->tnr_mt_prt_attr.LowMtPrtAdvOut[2];
		cfg_6->REG_D8.bits.REG_3DNR_Y_GAIN_LUT_OUT_3 = runtime->tnr_mt_prt_attr.LowMtPrtAdvOut[3];
		cfg_6->REG_DC.bits.REG_3DNR_Y_GAIN_LUT_SLOPE_0 = runtime->tnr_param_out.LowMtPrtAdvSlope[0];
		cfg_6->REG_DC.bits.REG_3DNR_Y_GAIN_LUT_SLOPE_1 = runtime->tnr_param_out.LowMtPrtAdvSlope[1];
		cfg_6->REG_E0.bits.REG_3DNR_Y_GAIN_LUT_SLOPE_2 = runtime->tnr_param_out.LowMtPrtAdvSlope[2];
		cfg_6->REG_E0.bits.REG_3DNR_Y_MOTION_MAX =
		cfg_6->REG_E0.bits.REG_3DNR_C_MOTION_MAX = tnr_mt_prt_attr->LowMtPrtAdvMax;
		cfg_6->REG_E4.bits.MOT_DEBUG_LUT_IN_0 = tnr_mt_prt_attr->LowMtPrtAdvDebugIn[0];
		cfg_6->REG_E4.bits.MOT_DEBUG_LUT_IN_1 = tnr_mt_prt_attr->LowMtPrtAdvDebugIn[1];
		cfg_6->REG_E4.bits.MOT_DEBUG_LUT_IN_2 = tnr_mt_prt_attr->LowMtPrtAdvDebugIn[2];
		cfg_6->REG_E4.bits.MOT_DEBUG_LUT_IN_3 = tnr_mt_prt_attr->LowMtPrtAdvDebugIn[3];
		cfg_6->REG_E8.bits.MOT_DEBUG_LUT_OUT_0 = tnr_mt_prt_attr->LowMtPrtAdvDebugOut[0];
		cfg_6->REG_E8.bits.MOT_DEBUG_LUT_OUT_1 = tnr_mt_prt_attr->LowMtPrtAdvDebugOut[1];
		cfg_6->REG_E8.bits.MOT_DEBUG_LUT_OUT_2 = tnr_mt_prt_attr->LowMtPrtAdvDebugOut[2];
		cfg_6->REG_E8.bits.MOT_DEBUG_LUT_OUT_3 = tnr_mt_prt_attr->LowMtPrtAdvDebugOut[3];
		cfg_6->REG_EC.bits.MOT_DEBUG_LUT_SLOPE_0 = runtime->tnr_param_out.LowMtPrtAdvDebugSlope[0];
		cfg_6->REG_EC.bits.MOT_DEBUG_LUT_SLOPE_1 = runtime->tnr_param_out.LowMtPrtAdvDebugSlope[1];
		cfg_6->REG_F0.bits.MOT_DEBUG_LUT_SLOPE_2 = runtime->tnr_param_out.LowMtPrtAdvDebugSlope[2];
		cfg_6->REG_F0.bits.MOT_DEBUG_SWITCH = tnr_mt_prt_attr->LowMtPrtAdvDebugMode;
		cfg_6->REG_F0.bits.REG_3DNR_Y_PIX_GAIN_ENABLE =
		cfg_6->REG_F0.bits.REG_3DNR_C_PIX_GAIN_ENABLE = tnr_mt_prt_attr->LowMtPrtAdvLumaEnable;
		cfg_6->REG_F0.bits.REG_3DNR_PIX_GAIN_MODE = tnr_mt_prt_attr->LowMtPrtAdvMode;

		struct cvi_isp_tnr_tun_7_cfg *cfg_7 = &(tnr_cfg->tnr_7_cfg);

		cfg_7->REG_100.bits.MMAP_LSC_CENTERX = runtime->tnr_param_out.MtLscCenterX;
		cfg_7->REG_100.bits.MMAP_LSC_CENTERY = runtime->tnr_param_out.MtLscCenterY;
		cfg_7->REG_104.bits.MMAP_LSC_NORM = runtime->tnr_param_out.MtLscNorm;
		cfg_7->REG_104.bits.MMAP_LSC_DY_GAIN = runtime->tnr_param_out.MtLscDYGain;
		cfg_7->REG_108.bits.MMAP_LSC_COMP_GAIN_LUT_00 = runtime->tnr_param_out.MtLscCompGain[0];
		cfg_7->REG_108.bits.MMAP_LSC_COMP_GAIN_LUT_01 = runtime->tnr_param_out.MtLscCompGain[1];
		cfg_7->REG_10C.bits.MMAP_LSC_COMP_GAIN_LUT_02 = runtime->tnr_param_out.MtLscCompGain[2];
		cfg_7->REG_10C.bits.MMAP_LSC_COMP_GAIN_LUT_03 = runtime->tnr_param_out.MtLscCompGain[3];
		cfg_7->REG_110.bits.MMAP_LSC_COMP_GAIN_LUT_04 = runtime->tnr_param_out.MtLscCompGain[4];
		cfg_7->REG_110.bits.MMAP_LSC_COMP_GAIN_LUT_05 = runtime->tnr_param_out.MtLscCompGain[5];
		cfg_7->REG_114.bits.MMAP_LSC_COMP_GAIN_LUT_06 = runtime->tnr_param_out.MtLscCompGain[6];
		cfg_7->REG_114.bits.MMAP_LSC_COMP_GAIN_LUT_07 = runtime->tnr_param_out.MtLscCompGain[7];
		cfg_7->REG_118.bits.MMAP_LSC_COMP_GAIN_LUT_08 = runtime->tnr_param_out.MtLscCompGain[8];
		cfg_7->REG_118.bits.MMAP_LSC_COMP_GAIN_LUT_09 = runtime->tnr_param_out.MtLscCompGain[9];
		cfg_7->REG_11C.bits.MMAP_LSC_COMP_GAIN_LUT_10 = runtime->tnr_param_out.MtLscCompGain[10];
		cfg_7->REG_11C.bits.MMAP_LSC_COMP_GAIN_LUT_11 = runtime->tnr_param_out.MtLscCompGain[11];
		cfg_7->REG_120.bits.MMAP_LSC_COMP_GAIN_LUT_12 = runtime->tnr_param_out.MtLscCompGain[12];
		cfg_7->REG_120.bits.MMAP_LSC_COMP_GAIN_LUT_13 = runtime->tnr_param_out.MtLscCompGain[13];
		cfg_7->REG_124.bits.MMAP_LSC_COMP_GAIN_LUT_14 = runtime->tnr_param_out.MtLscCompGain[14];
		cfg_7->REG_124.bits.MMAP_LSC_COMP_GAIN_LUT_15 = runtime->tnr_param_out.MtLscCompGain[15];
		cfg_7->REG_128.bits.MMAP_LSC_COMP_GAIN_LUT_16 = runtime->tnr_param_out.MtLscCompGain[16];

		isp_tnr_ctrl_postprocess_compatible(ViPipe);
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_tnr_ctrl_postprocess_compatible(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

#ifdef CHIP_ARCH_CV182X
	CVI_U32 scene = 0;

	isp_get_scene_info(ViPipe, &scene);

	if (scene != FE_ON_BE_ON_POST_ON_SC)
		return ret;

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_tnr_config *tnr_cfg =
		(struct cvi_vip_isp_tnr_config *)&(post_addr->tun_cfg[tun_idx].tnr_cfg);

	// rgbmap_size should only be 3 due to hw limitation on-the-fly mode in cv182x
	tnr_cfg->rgbmap_w_bit = 3;
	tnr_cfg->rgbmap_h_bit = 3;
#endif // CHIP_ARCH_CV182X

	UNUSED(ViPipe);

	return ret;
}

static CVI_S32 set_tnr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

		const ISP_TNR_ATTR_S *tnr_attr = NULL;

		const ISP_TNR_MT_PRT_ATTR_S *tnr_mt_prt_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_tnr_ctrl_get_tnr_attr(ViPipe, &tnr_attr);
		isp_tnr_ctrl_get_tnr_mt_prt_attr(ViPipe, &tnr_mt_prt_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->TNREnable = tnr_attr->Enable;
		pProcST->TNRisManualMode = tnr_attr->enOpType;

		pProcST->TNRMtDetectUnit = runtime->tnr_attr.MtDetectUnit;
		memcpy(pProcST->TNRMtDetectUnitList, runtime->au8MtDetectUnit,
			sizeof(CVI_U8) * ISP_AUTO_ISO_STRENGTH_NUM);
		//manual or auto
		pProcST->TNRBrightnessNoiseLevelLE = runtime->tnr_attr.BrightnessNoiseLevelLE;
		pProcST->TNRBrightnessNoiseLevelSE = runtime->tnr_attr.BrightnessNoiseLevelSE;
		pProcST->TNRRNoiseLevel0 = runtime->tnr_noise_model_attr.RNoiseLevel0;
		pProcST->TNRRNoiseHiLevel0 = runtime->tnr_noise_model_attr.RNoiseHiLevel0;
		pProcST->TNRGNoiseLevel0 = runtime->tnr_noise_model_attr.GNoiseLevel0;
		pProcST->TNRGNoiseHiLevel0 = runtime->tnr_noise_model_attr.GNoiseHiLevel0;
		pProcST->TNRBNoiseLevel0 = runtime->tnr_noise_model_attr.BNoiseLevel0;
		pProcST->TNRBNoiseHiLevel0 = runtime->tnr_noise_model_attr.BNoiseHiLevel0;
		pProcST->TNRRNoiseLevel1 = runtime->tnr_noise_model_attr.RNoiseLevel1;
		pProcST->TNRRNoiseHiLevel1 = runtime->tnr_noise_model_attr.RNoiseHiLevel1;
		pProcST->TNRGNoiseLevel1 = runtime->tnr_noise_model_attr.GNoiseLevel1;
		pProcST->TNRGNoiseHiLevel1 = runtime->tnr_noise_model_attr.GNoiseHiLevel1;
		pProcST->TNRBNoiseLevel1 = runtime->tnr_noise_model_attr.BNoiseLevel1;
		pProcST->TNRBNoiseHiLevel1 = runtime->tnr_noise_model_attr.BNoiseHiLevel1;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_tnr_ctrl_runtime  *_get_tnr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_tnr_ctrl_check_tnr_attr_valid(const ISP_TNR_ATTR_S *pstTNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstTNRAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstTNRAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstTNRAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstTNRAttr, TuningMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstTNRAttr, TnrMtMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstTNRAttr, YnrCnrSharpenMtMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstTNRAttr, PreSharpenMtMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstTNRAttr, ChromaScalingDownMode, 0x0, 0x3);
	// CHECK_VALID_CONST(pstTNRAttr, CompGainEnable, CVI_FALSE, CVI_TRUE);

	// CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, TnrStrength0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, MapThdLow0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, MapThdHigh0, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, MtDetectUnit, 0x0, 0x5);
	CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, BrightnessNoiseLevelLE, 0x1, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, BrightnessNoiseLevelSE, 0x1, 0x3ff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, MtFiltMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_1D(pstTNRAttr, MtFiltWgt, 0x0, 0x100);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_check_tnr_noise_model_attr_valid(const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, RNoiseLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, GNoiseLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, BNoiseLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, RNoiseLevel1, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, GNoiseLevel1, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, BNoiseLevel1, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, RNoiseHiLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, GNoiseHiLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, BNoiseHiLevel0, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, RNoiseHiLevel1, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, GNoiseHiLevel1, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRNoiseModelAttr, BNoiseHiLevel1, 0x0, 0xff);

	UNUSED(pstTNRNoiseModelAttr);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_check_tnr_luma_motion_attr_valid(const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_AUTO_ISO_2D(pstTNRLumaMotionAttr, L2mIn0, 4, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_2D(pstTNRLumaMotionAttr, L2mOut0, 4, 0x0, 0x3f);
	CHECK_VALID_AUTO_ISO_2D(pstTNRLumaMotionAttr, L2mIn1, 4, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_2D(pstTNRLumaMotionAttr, L2mOut1, 4, 0x0, 0x3f);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRLumaMotionAttr, MtLumaMode, CVI_FALSE, CVI_TRUE);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_check_tnr_ghost_attr_valid(const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_2D(pstTNRGhostAttr, PrvMotion0, 4, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstTNRGhostAttr, PrtctWgt0, 4, 0x0, 0xf);
	CHECK_VALID_AUTO_ISO_1D(pstTNRGhostAttr, MotionHistoryStr, 0x0, 0xf);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_check_tnr_mt_prt_attr_valid(const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstTNRMtPrtAttr, LowMtPrtEn, CVI_FALSE, CVI_TRUE);

	// CHECK_VALID_AUTO_ISO_1D(pstTNRMtPrtAttr, LowMtPrtLevelY, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRMtPrtAttr, LowMtPrtLevelU, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstTNRMtPrtAttr, LowMtPrtLevelV, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtInY, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtInU, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtInV, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtOutY, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtOutU, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtOutV, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtAdvIn, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMtPrtAttr, LowMtPrtAdvOut, 4, 0x0, 0xff);

	UNUSED(pstTNRMtPrtAttr);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_check_tnr_motion_adapt_attr_valid(const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_2D(pstTNRMotionAdaptAttr, AdaptNrLumaStrIn, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMotionAdaptAttr, AdaptNrLumaStrOut, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMotionAdaptAttr, AdaptNrChromaStrIn, 4, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstTNRMotionAdaptAttr, AdaptNrChromaStrOut, 4, 0x0, 0xff);

	UNUSED(pstTNRMotionAdaptAttr);

	return ret;
}

static CVI_S32 isp_tnr_ctrl_set_tnr_mt_prt_attr_compatible(VI_PIPE ViPipe, ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

#ifdef CHIP_ARCH_CV182X
	CVI_U8 rounding_protect = 128 * 1.1;

	for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
		pstTNRMtPrtAttr->stManual.LowMtPrtInU[idx] =
			MAX(pstTNRMtPrtAttr->stManual.LowMtPrtInU[idx], 1);
		pstTNRMtPrtAttr->stManual.LowMtPrtInV[idx] =
			MAX(pstTNRMtPrtAttr->stManual.LowMtPrtInV[idx], 1);
	}

	for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
		CVI_U8 in;
		CVI_U8 val;

		in = pstTNRMtPrtAttr->stManual.LowMtPrtInU[idx];
		val = (rounding_protect + in - 1) / in;
		pstTNRMtPrtAttr->stManual.LowMtPrtOutU[idx] =
			MAX(pstTNRMtPrtAttr->stManual.LowMtPrtOutU[idx], val);

		in = pstTNRMtPrtAttr->stManual.LowMtPrtInV[idx];
		val = (rounding_protect + in - 1) / in;
		pstTNRMtPrtAttr->stManual.LowMtPrtOutV[idx] =
			MAX(pstTNRMtPrtAttr->stManual.LowMtPrtOutV[idx], val);
	}

	for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
		for (CVI_U32 iso = 0 ; iso < ISP_AUTO_ISO_STRENGTH_NUM ; iso++) {
			pstTNRMtPrtAttr->stAuto.LowMtPrtInU[idx][iso] =
				MAX(pstTNRMtPrtAttr->stAuto.LowMtPrtInU[idx][iso], 1);
			pstTNRMtPrtAttr->stAuto.LowMtPrtInV[idx][iso] =
				MAX(pstTNRMtPrtAttr->stAuto.LowMtPrtInV[idx][iso], 1);
		}
	}

	for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
		CVI_U8 in;
		CVI_U8 val;

		for (CVI_U32 iso = 0 ; iso < ISP_AUTO_ISO_STRENGTH_NUM ; iso++) {
			in = pstTNRMtPrtAttr->stAuto.LowMtPrtInU[idx][iso];
			val = (rounding_protect + in - 1) / in;
			pstTNRMtPrtAttr->stAuto.LowMtPrtOutU[idx][iso] =
				MAX(pstTNRMtPrtAttr->stAuto.LowMtPrtOutU[idx][iso], val);

			in = pstTNRMtPrtAttr->stAuto.LowMtPrtInV[idx][iso];
			val = (rounding_protect + in - 1) / in;
			pstTNRMtPrtAttr->stAuto.LowMtPrtOutV[idx][iso] =
				MAX(pstTNRMtPrtAttr->stAuto.LowMtPrtOutV[idx][iso], val);
		}
	}
#endif // CHIP_ARCH_CV182X

	UNUSED(ViPipe);
	UNUSED(pstTNRMtPrtAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_tnr_ctrl_get_tnr_internal_attr(VI_PIPE ViPipe, ISP_TNR_INTER_ATTR_S *pstTNRInterAttr)
{
	if (pstTNRInterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*pstTNRInterAttr = runtime->tnr_internal_attr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_internal_attr(VI_PIPE ViPipe, const ISP_TNR_INTER_ATTR_S *pstTNRInterAttr)
{
	if (pstTNRInterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->tnr_internal_attr = *pstTNRInterAttr;
	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_attr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S **pstTNRAttr)
{
	if (pstTNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);

	*pstTNRAttr = &shared_buffer->stTNRAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_attr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S *pstTNRAttr)
{
	if (pstTNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_attr_valid(pstTNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRAttr, sizeof(*pstTNRAttr));

	runtime->tnr_internal_attr.bUpdateMotionUnit = CVI_TRUE;
	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_noise_model_attr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S **pstTNRNoiseModelAttr)
{
	if (pstTNRNoiseModelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);
	*pstTNRNoiseModelAttr = &shared_buffer->stTNRNoiseModelAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_noise_model_attr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr)
{
	if (pstTNRNoiseModelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_noise_model_attr_valid(pstTNRNoiseModelAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_NOISE_MODEL_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_noise_model_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRNoiseModelAttr, sizeof(*pstTNRNoiseModelAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_luma_motion_attr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S **pstTNRLumaMotionAttr)
{
	if (pstTNRLumaMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);
	*pstTNRLumaMotionAttr = &shared_buffer->stTNRLumaMotionAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_luma_motion_attr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr)
{
	if (pstTNRLumaMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_luma_motion_attr_valid(pstTNRLumaMotionAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_LUMA_MOTION_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_luma_motion_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRLumaMotionAttr, sizeof(*pstTNRLumaMotionAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_ghost_attr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S **pstTNRGhostAttr)
{
	if (pstTNRGhostAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);
	*pstTNRGhostAttr = &shared_buffer->stTNRGhostAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_ghost_attr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr)
{
	if (pstTNRGhostAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_ghost_attr_valid(pstTNRGhostAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_GHOST_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_ghost_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRGhostAttr, sizeof(*pstTNRGhostAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_mt_prt_attr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S **pstTNRMtPrtAttr)
{
	if (pstTNRMtPrtAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);
	*pstTNRMtPrtAttr = &shared_buffer->stTNRMtPrtAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_mt_prt_attr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr)
{
	if (pstTNRMtPrtAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_mt_prt_attr_valid(pstTNRMtPrtAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_MT_PRT_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_mt_prt_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRMtPrtAttr, sizeof(*pstTNRMtPrtAttr));

	isp_tnr_ctrl_set_tnr_mt_prt_attr_compatible(ViPipe, (ISP_TNR_MT_PRT_ATTR_S *)p);

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tnr_ctrl_get_tnr_motion_adapt_attr(VI_PIPE ViPipe,
			const ISP_TNR_MOTION_ADAPT_ATTR_S **pstTNRMotionAdaptAttr)
{
	if (pstTNRMotionAdaptAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_3DNR, (CVI_VOID *) &shared_buffer);
	*pstTNRMotionAdaptAttr = &shared_buffer->stTNRMotionAdaptAttr;

	return ret;
}

CVI_S32 isp_tnr_ctrl_set_tnr_motion_adapt_attr(VI_PIPE ViPipe, const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr)
{
	if (pstTNRMotionAdaptAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tnr_ctrl_runtime *runtime = _get_tnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_tnr_ctrl_check_tnr_motion_adapt_attr_valid(pstTNRMotionAdaptAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_TNR_MOTION_ADAPT_ATTR_S *p = CVI_NULL;

	isp_tnr_ctrl_get_tnr_motion_adapt_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstTNRMotionAdaptAttr, sizeof(*pstTNRMotionAdaptAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

