/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_drc_ctrl.c
 * Description:
 *
 */

#include <stdlib.h>
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"
#include "cvi_ae.h"
#include "isp_3a.h"
#include "isp_sts_ctrl.h"
#include "isp_lut.h"

#include "isp_drc_ctrl.h"
#include "isp_gamma_ctrl.h"
#include "isp_fswdr_ctrl.h"
#include "isp_mgr_buf.h"
#if defined(__CV180X__)
#include "cvi_sys.h"
#endif

#ifndef ENABLE_ISP_C906L
// #define MAX_GAMMA_LUT_VALUE			(4095)

const struct isp_module_ctrl drc_mod = {
	.init = isp_drc_ctrl_init,
	.uninit = isp_drc_ctrl_uninit,
	.suspend = isp_drc_ctrl_suspend,
	.resume = isp_drc_ctrl_resume,
	.ctrl = isp_drc_ctrl_ctrl
};

static CVI_S32 isp_drc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_drc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_drc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_drc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_drc_ctrl_buf_init(VI_PIPE ViPipe);
static CVI_S32 isp_drc_ctrl_buf_uninit(VI_PIPE ViPipe);

static CVI_S32 set_drc_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_drc_ctrl_check_drc_attr_valid(const ISP_DRC_ATTR_S *pstdrcAttr);
static CVI_S32 isp_drc_ctrl_set_drc_attr_compatible(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr);
static CVI_S32 isp_drc_ctrl_modify_rgbgamma_for_drc_tuning_mode(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr);
static CVI_S32 isp_drc_ctrl_ready(VI_PIPE ViPipe);

static struct isp_drc_ctrl_runtime  *_get_drc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_drc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->mapped = CVI_FALSE;
	runtime->initialized = CVI_TRUE;
	runtime->reset_iir = CVI_TRUE;
	runtime->modify_rgbgamma_for_drc_tuning = CVI_FALSE;
	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	runtime->pLocalBuffer = CVI_NULL;

	isp_drc_ctrl_buf_init(ViPipe);
	isp_algo_drc_init();

	return ret;
}

CVI_S32 isp_drc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	isp_algo_drc_uninit();
	isp_drc_ctrl_buf_uninit(ViPipe);

	return ret;
}

CVI_S32 isp_drc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_drc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_drc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_drc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDrc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDrc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

#ifndef ARCH_RTOS_CV181X
CVI_S32 isp_drc_ctrl_buf_init(VI_PIPE ViPipe)
{
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 u32BufferSize, u32BuffOffset;

	u32BufferSize = (LTM_G_CURVE_TOTAL_NODE_NUM * sizeof(CVI_U32))	// preGlobalLut
		+ (LTM_DARK_CURVE_NODE_NUM * sizeof(CVI_U32))				// preDarkLut
		+ (LTM_BRIGHT_CURVE_NODE_NUM * sizeof(CVI_U32));			// preBritLut

	runtime->pLocalBuffer = ISP_PTR_CAST_PTR(ISP_CALLOC(u32BufferSize, sizeof(CVI_U8)));
	if (runtime->pLocalBuffer == CVI_NULL) {
		ISP_LOG_ERR("Allocate [%d] DRC internal buffer fail, size=%d\n", ViPipe,
			u32BufferSize);
		return CVI_FAILURE;
	}

	u32BuffOffset = 0;

	runtime->preGlobalLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_G_CURVE_TOTAL_NODE_NUM * sizeof(CVI_U32));

	runtime->preDarkLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_DARK_CURVE_NODE_NUM * sizeof(CVI_U32));

	runtime->preBritLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_BRIGHT_CURVE_NODE_NUM * sizeof(CVI_U32));

	runtime->mapped = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_drc_ctrl_buf_uninit(VI_PIPE ViPipe)
{
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preGlobalLut = CVI_NULL;
	runtime->preDarkLut = CVI_NULL;
	runtime->preBritLut = CVI_NULL;

	runtime->mapped = CVI_FALSE;

	if (runtime->pLocalBuffer != CVI_NULL) {
		free(ISP_PTR_CAST_U8(runtime->pLocalBuffer));
		runtime->pLocalBuffer = CVI_NULL;
	}

	return CVI_SUCCESS;
}
#else
CVI_S32 isp_drc_ctrl_buf_init(VI_PIPE ViPipe)
{
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

#define __DRC_CRTL_BUF_SIZE ((LTM_G_CURVE_TOTAL_NODE_NUM * sizeof(CVI_U32)) +\
	(LTM_DARK_CURVE_NODE_NUM * sizeof(CVI_U32)) +\
	 (LTM_BRIGHT_CURVE_NODE_NUM * sizeof(CVI_U32)))

	static CVI_U8 buf[__DRC_CRTL_BUF_SIZE];

	memset(buf, 0, __DRC_CRTL_BUF_SIZE);
	runtime->pLocalBuffer = ISP_PTR_CAST_PTR(buf);

	CVI_U32 u32BuffOffset = 0;

	runtime->preGlobalLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_G_CURVE_TOTAL_NODE_NUM * sizeof(CVI_U32));

	runtime->preDarkLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_DARK_CURVE_NODE_NUM * sizeof(CVI_U32));

	runtime->preBritLut = ISP_PTR_CAST_PTR((ISP_PTR_CAST_U8(runtime->pLocalBuffer) + u32BuffOffset));
	u32BuffOffset += (LTM_BRIGHT_CURVE_NODE_NUM * sizeof(CVI_U32));

	runtime->mapped = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_drc_ctrl_buf_uninit(VI_PIPE ViPipe)
{
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preGlobalLut = CVI_NULL;
	runtime->preDarkLut = CVI_NULL;
	runtime->preBritLut = CVI_NULL;

	runtime->mapped = CVI_FALSE;

	if (runtime->pLocalBuffer != CVI_NULL) {
		runtime->pLocalBuffer = CVI_NULL;
	}

	return CVI_SUCCESS;
}
#endif

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_drc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_drc_ctrl_ready(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_drc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_drc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_drc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_drc_proc_info(ViPipe);

	return ret;
}

/* run in rtos, save stack */
static CVI_U32 _getExpRatioMax(VI_PIPE ViPipe)
{
	CVI_U32 u32ExpRatioMax;
	ISP_WDR_EXPOSURE_ATTR_S *wdrExposureAttr;

	wdrExposureAttr = (ISP_WDR_EXPOSURE_ATTR_S *) malloc(sizeof(ISP_WDR_EXPOSURE_ATTR_S));
	CVI_ISP_GetWDRExposureAttr(ViPipe, wdrExposureAttr);
	u32ExpRatioMax = wdrExposureAttr->u32ExpRatioMax;
	free(wdrExposureAttr);

	return u32ExpRatioMax;
}

static CVI_S32 isp_drc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DRC_ATTR_S *drc_attr = NULL;
	const ISP_FSWDR_ATTR_S *fswdr_attr = NULL;

	isp_drc_ctrl_get_drc_attr(ViPipe, &drc_attr);
	isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &fswdr_attr);

	int iir_average_frame = drc_attr->ToneCurveSmooth;

	if (drc_attr->Enable && !runtime->is_module_bypass) {
		if (runtime->reset_iir) {
			iir_average_frame = 0;
			runtime->reset_iir = CVI_FALSE;
		}
	} else {
		runtime->reset_iir = CVI_TRUE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(drc_attr->UpdateInterval, 1);
	CVI_U8 frame_delay = (intvl < iir_average_frame) ? 1 : (intvl / MAX(iir_average_frame, 1));

	is_preprocess_update =
		((runtime->preprocess_updated) ||
		 ((algoResult->u32FrameIdx % frame_delay) == 0) ||
		 (runtime->lv != (CVI_S16)(algoResult->currentLV/100)));

	runtime->lv = (CVI_S16)(algoResult->currentLV/100);

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	#if 0
	if (!drc_attr->Enable || runtime->is_module_bypass)
		return ret;
	#endif

	runtime->process_updated = CVI_TRUE;
	if (drc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(drc_attr, TargetYScale);
		MANUAL(drc_attr, HdrStrength);
		MANUAL(drc_attr, DEAdaptPercentile);
		MANUAL(drc_attr, DEAdaptTargetGain);
		MANUAL(drc_attr, DEAdaptGainUB);
		MANUAL(drc_attr, DEAdaptGainLB);
		MANUAL(drc_attr, BritInflectPtLuma);
		MANUAL(drc_attr, BritContrastLow);
		MANUAL(drc_attr, BritContrastHigh);
		MANUAL(drc_attr, SdrTargetY);
		MANUAL(drc_attr, SdrTargetYGain);
		MANUAL(drc_attr, SdrGlobalToneStr);
		MANUAL(drc_attr, SdrDEAdaptPercentile);
		MANUAL(drc_attr, SdrDEAdaptTargetGain);
		MANUAL(drc_attr, SdrDEAdaptGainLB);
		MANUAL(drc_attr, SdrDEAdaptGainUB);
		MANUAL(drc_attr, SdrBritInflectPtLuma);
		MANUAL(drc_attr, SdrBritContrastLow);
		MANUAL(drc_attr, SdrBritContrastHigh);
		MANUAL(drc_attr, TotalGain);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(drc_attr, TargetYScale, INTPLT_LV);
		AUTO(drc_attr, HdrStrength, INTPLT_LV);
		AUTO(drc_attr, DEAdaptPercentile, INTPLT_LV);
		AUTO(drc_attr, DEAdaptTargetGain, INTPLT_LV);
		AUTO(drc_attr, DEAdaptGainUB, INTPLT_LV);
		AUTO(drc_attr, DEAdaptGainLB, INTPLT_LV);
		AUTO(drc_attr, BritInflectPtLuma, INTPLT_LV);
		AUTO(drc_attr, BritContrastLow, INTPLT_LV);
		AUTO(drc_attr, BritContrastHigh, INTPLT_LV);
		AUTO(drc_attr, SdrTargetY, INTPLT_LV);
		AUTO(drc_attr, SdrTargetYGain, INTPLT_LV);
		AUTO(drc_attr, SdrGlobalToneStr, INTPLT_LV);
		AUTO(drc_attr, SdrDEAdaptPercentile, INTPLT_LV);
		AUTO(drc_attr, SdrDEAdaptTargetGain, INTPLT_LV);
		AUTO(drc_attr, SdrDEAdaptGainLB, INTPLT_LV);
		AUTO(drc_attr, SdrDEAdaptGainUB, INTPLT_LV);
		AUTO(drc_attr, SdrBritInflectPtLuma, INTPLT_LV);
		AUTO(drc_attr, SdrBritContrastLow, INTPLT_LV);
		AUTO(drc_attr, SdrBritContrastHigh, INTPLT_LV);
		AUTO(drc_attr, TotalGain, INTPLT_POST_ISO);

		#undef AUTO
	}

	if (!drc_attr->Enable || runtime->is_module_bypass) {
		return ret;
	}

	ISP_AE_STATISTICS_COMPAT_S *ae_sts;

	isp_sts_ctrl_get_ae_sts(ViPipe, &ae_sts);

	runtime->drc_param_in.fswdr_en = !pstIspCtx->wdrLinearMode;
	runtime->drc_param_in.exposure_ratio = (CVI_FLOAT)algoResult->au32ExpRatio[0] / 64;
	if (runtime->drc_param_in.exposure_ratio == 0) {
		runtime->drc_param_in.exposure_ratio = 1;
	}
	runtime->drc_param_in.ToneCurveSelect = drc_attr->ToneCurveSelect;
	memcpy(runtime->drc_param_in.au16CurveUserDefine, drc_attr->CurveUserDefine,
		sizeof(CVI_U16) * DRC_GLOBAL_USER_DEFINE_NUM);
	memcpy(runtime->drc_param_in.au16DarkUserDefine, drc_attr->DarkUserDefine,
		sizeof(CVI_U16) * DRC_DARK_USER_DEFINE_NUM);
	memcpy(runtime->drc_param_in.au16BrightUserDefine, drc_attr->BrightUserDefine,
		sizeof(CVI_U16) * DRC_BRIGHT_USER_DEFINE_NUM);

	runtime->drc_param_in.imgWidth = pstIspCtx->stSysRect.u32Width;
	runtime->drc_param_in.imgHeight = pstIspCtx->stSysRect.u32Height;

	struct fswdr_info	fswdr_info;

	isp_fswdr_ctrl_get_fswdr_info(ViPipe, &fswdr_info);

	runtime->drc_param_in.u16WDRCombineMinWeight = fswdr_info.WDRCombineMinWeight;
	runtime->drc_param_in.WDRCombineShortThr = fswdr_info.WDRCombineShortThr;
	runtime->drc_param_in.WDRCombineLongThr = fswdr_info.WDRCombineLongThr;

	runtime->drc_param_in.targetYScale = runtime->drc_attr.TargetYScale;
	runtime->drc_param_in.average_frame = iir_average_frame;

	runtime->drc_param_in.preWdrBinNum = runtime->drc_param_out.reg_ltm_wdr_bin_num;
	runtime->drc_param_in.HdrStrength = runtime->drc_attr.HdrStrength;
	runtime->drc_param_in.dbgLevel = drc_attr->DRCMu[ISP_TEST_PARAM_DRC_DEBUGLOG_LEVEL];
	runtime->drc_param_in.frameCnt = pstIspCtx->frameCnt;
	runtime->drc_param_in.ToneParserMaxRatio = _getExpRatioMax(ViPipe);
	runtime->drc_param_in.ViPipe = ViPipe;

	CVI_FLOAT fBritMapStr, fSdrBritMapStr;

	fBritMapStr = (128.0f - (CVI_FLOAT)drc_attr->BritMapStr) / 4.0;
	if (fBritMapStr < 1.0) {
		fBritMapStr = 1.0;
	}

	runtime->drc_param_in.stBritToneParam.fBritMapStr = fBritMapStr;
	runtime->drc_param_in.BritMapStr = MINMAX(drc_attr->BritMapStr, 0, 100);
	runtime->drc_param_in.stBritToneParam.u8BritInflectPtLuma = runtime->drc_attr.BritInflectPtLuma;
	runtime->drc_param_in.stBritToneParam.u8BritContrastLow = runtime->drc_attr.BritContrastLow;
	runtime->drc_param_in.stBritToneParam.u8BritContrastHigh = runtime->drc_attr.BritContrastHigh;

	runtime->drc_param_in.stBritToneParam.u8SdrTargetYGainMode = drc_attr->SdrTargetYGainMode;
	runtime->drc_param_in.stBritToneParam.u8SdrTargetY = runtime->drc_attr.SdrTargetY;
	runtime->drc_param_in.stBritToneParam.u8SdrTargetYGain = runtime->drc_attr.SdrTargetYGain;
	runtime->drc_param_in.stBritToneParam.u16SdrGlobalToneStr = runtime->drc_attr.SdrGlobalToneStr;

	fSdrBritMapStr = (128.0f - (CVI_FLOAT)drc_attr->SdrBritMapStr) / 4.0;
	if (fSdrBritMapStr < 1.0) {
		fSdrBritMapStr = 1.0;
	}

	runtime->drc_param_in.stBritToneParam.fSdrBritMapStr = fSdrBritMapStr;
	runtime->drc_param_in.SdrBritMapStr = MINMAX(drc_attr->SdrBritMapStr, 0, 100);
	runtime->drc_param_in.stBritToneParam.u8SdrBritInflectPtLuma = runtime->drc_attr.SdrBritInflectPtLuma;
	runtime->drc_param_in.stBritToneParam.u8SdrBritContrastLow = runtime->drc_attr.SdrBritContrastLow;
	runtime->drc_param_in.stBritToneParam.u8SdrBritContrastHigh = runtime->drc_attr.SdrBritContrastHigh;

	CVI_FLOAT fDarkMapStr, fSdrDarkMapStr;

	fDarkMapStr = (CVI_FLOAT)drc_attr->DarkMapStr / 4.0;
	if (fDarkMapStr < 1.0) {
		fDarkMapStr = 1.0;
	}

	runtime->drc_param_in.stDarkToneParam.fDarkMapStr = fDarkMapStr;
	runtime->drc_param_in.DarkMapStr = MINMAX(drc_attr->DarkMapStr, 0, 100);

	fSdrDarkMapStr = (CVI_FLOAT)drc_attr->SdrDarkMapStr / 4.0;
	if (fSdrDarkMapStr < 1.0) {
		fSdrDarkMapStr = 1.0;
	}

	runtime->drc_param_in.stDarkToneParam.fSdrDarkMapStr = fSdrDarkMapStr;
	runtime->drc_param_in.SdrDarkMapStr = MINMAX(drc_attr->SdrDarkMapStr, 0, 100);

	runtime->drc_param_in.u8DEAdaptPercentile = runtime->drc_attr.DEAdaptPercentile;
	runtime->drc_param_in.u8DEAdaptTargetGain = runtime->drc_attr.DEAdaptTargetGain;
	runtime->drc_param_in.u8DEAdaptGainLB = runtime->drc_attr.DEAdaptGainLB;
	runtime->drc_param_in.u8DEAdaptGainUB = runtime->drc_attr.DEAdaptGainUB;
	runtime->drc_param_in.u8SdrDEAdaptPercentile = runtime->drc_attr.SdrDEAdaptPercentile;
	runtime->drc_param_in.u8SdrDEAdaptTargetGain = runtime->drc_attr.SdrDEAdaptTargetGain;
	runtime->drc_param_in.u8SdrDEAdaptGainLB = runtime->drc_attr.SdrDEAdaptGainLB;
	runtime->drc_param_in.u8SdrDEAdaptGainUB = runtime->drc_attr.SdrDEAdaptGainUB;

	memcpy(runtime->drc_param_in.SEHistogram, ae_sts->aeStat1[0].au32HistogramMemArray[ISP_CHANNEL_SE],
		sizeof(runtime->drc_param_in.SEHistogram));
	memcpy(runtime->drc_param_in.LEHistogram, ae_sts->aeStat1[0].au32HistogramMemArray[ISP_CHANNEL_LE],
		sizeof(runtime->drc_param_in.LEHistogram));

	runtime->drc_param_in.pu32PreGlobalLut = runtime->preGlobalLut;
	runtime->drc_param_in.pu32PreDarkLut = runtime->preDarkLut;
	runtime->drc_param_in.pu32PreBritLut = runtime->preBritLut;

	for (CVI_U32 i = 0 ; i < 4 ; i++) {
		runtime->drc_param_in.DetailEnhanceMtIn[i] = drc_attr->DetailEnhanceMtIn[i];
		runtime->drc_param_in.DetailEnhanceMtOut[i] = drc_attr->DetailEnhanceMtOut[i];
	}

	runtime->drc_param_in.dbg_182x_sim_enable = drc_attr->dbg_182x_sim_enable;


	// ParamOut
	if (pstIspCtx->wdrLinearMode != runtime->ldrc_mode) {
		runtime->ldrc_mode = pstIspCtx->wdrLinearMode;
		ISP_LOG_DEBUG("Set linear enhance(%d)\n", pstIspCtx->wdrLinearMode);
		S_EXT_CTRLS_VALUE(VI_IOCTL_HDR_DETAIL_EN, pstIspCtx->wdrLinearMode, CVI_NULL);
	}

	return ret;
}

static CVI_S32 isp_drc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_drc_main(
		(struct drc_param_in *)&runtime->drc_param_in,
		(struct drc_param_out *)&runtime->drc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_drc_ctrl_postprocess(VI_PIPE ViPipe)
{
#ifndef ARCH_RTOS_CV181X
enum drc_tuning_toneCurve {
	DRC_TUNING_GLOBAL_CURVE,
	DRC_TUNING_DARK_CURVE,
	DRC_TUNING_BRIT_CURVE,
	DRC_TUNING_NUM
};

	static FILE*fdList[DRC_TUNING_NUM] = { NULL, NULL, NULL };
#endif

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);
	CVI_U32 rng = 5;

#if defined(__CV181X__)
	ISP_CTX_S * pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
#endif

#if defined(__CV180X__)
	CVI_U32 chip_ver;

	CVI_SYS_GetChipVersion(&chip_ver);
#endif

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_drc_config *drc_cfg =
		(struct cvi_vip_isp_drc_config *)&(post_addr->tun_cfg[tun_idx].drc_cfg);

	const ISP_DRC_ATTR_S *drc_attr = NULL;

	isp_drc_ctrl_get_drc_attr(ViPipe, &drc_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		drc_cfg->update = 0;
	} else {
		drc_cfg->update = 1;
		drc_cfg->ltm_enable = drc_attr->Enable;
		drc_cfg->dark_enh_en =
		drc_cfg->brit_enh_en = drc_attr->LocalToneEn;
		drc_cfg->dbg_mode = drc_attr->TuningMode;
		drc_cfg->dark_tone_wgt_refine_en =
		drc_cfg->brit_tone_wgt_refine_en = drc_attr->LocalToneRefineEn;

	#if 0
		memcpy(drc_cfg->dark_lut, out->reg_ltm_dark_lut,
			sizeof(CVI_U16) * LTM_DARK_CURVE_NODE_NUM);
		memcpy(drc_cfg->brit_lut, out->reg_ltm_brit_lut,
			sizeof(CVI_U16) * LTM_BRIGHT_CURVE_NODE_NUM);
		memcpy(drc_cfg->deflt_lut, out->reg_ltm_deflt_lut,
			sizeof(CVI_U16) * (LTM_G_CURVE_TOTAL_NODE_NUM));
	#else
		// TODO@ST Fix tone curve data type from u32 to u16
		for (int i = 0 ; i < LTM_G_CURVE_TOTAL_NODE_NUM ; i++)
			drc_cfg->global_lut[i] = runtime->drc_param_out.reg_ltm_deflt_lut[i];
		for (int i = 0 ; i < LTM_DARK_CURVE_NODE_NUM ; i++)
			drc_cfg->dark_lut[i] = runtime->drc_param_out.reg_ltm_dark_lut[i];
		for (int i = 0 ; i < LTM_BRIGHT_CURVE_NODE_NUM ; i++)
			drc_cfg->brit_lut[i] = runtime->drc_param_out.reg_ltm_brit_lut[i];
#if defined(__CV180X__)
#define RAW_BIT_DEPTH					(16)
#define DRC_OUT_MAX_VAL					((1 << RAW_BIT_DEPTH) - 1)
		//note: fix CV180X rtl bug
		//patch for not use global/bright tone node256
		if (chip_ver == E_CHIPVERSION_U01) {
			if (drc_attr->ToneCurveSelect != 0) {
				drc_cfg->global_lut[LTM_DARK_CURVE_NODE_NUM - 1 - 1] = DRC_OUT_MAX_VAL;
				drc_cfg->brit_lut[LTM_DARK_CURVE_NODE_NUM - 1 - 1] = DRC_OUT_MAX_VAL;
			}
		}
#endif
	#endif

		//debug setting
		if (drc_attr->DRCMu[31] == 1) {
			//disable bright tone
			drc_cfg->dark_enh_en = 0;
		} else if (drc_attr->DRCMu[31] == 2) {
			//disable dark tone
			drc_cfg->brit_enh_en = 0;
		}
#ifndef ARCH_RTOS_CV181X
		//dump curve
		CVI_U32 tunIdx = drc_attr->DRCMu[30];

		if (tunIdx != 0 && tunIdx <= 3) {
			tunIdx -= 1;
			if (fdList[tunIdx] == NULL) {
				char fileName[100] = {0};

				snprintf(fileName, 100, "/mnt/data/curve%d.txt", tunIdx);
				printf("start dump curve to %s!!\n", fileName);
				fdList[tunIdx] = fopen(fileName, "w");
			}
			if (fdList[DRC_TUNING_GLOBAL_CURVE]) {
				for (int u32Idx = 0; u32Idx < LTM_G_CURVE_TOTAL_NODE_NUM; ++u32Idx) {
					fprintf(fdList[DRC_TUNING_GLOBAL_CURVE], "%d,", drc_cfg->global_lut[u32Idx]);
					if (drc_cfg->global_lut[u32Idx] >= ((1 << 16) - 1))
						break;
				}
				fprintf(fdList[tunIdx], "\n-----\n");
			}
			if (fdList[DRC_TUNING_DARK_CURVE]) {
				for (int u32Idx = 0; u32Idx < LTM_DARK_CURVE_NODE_NUM; ++u32Idx) {
					fprintf(fdList[DRC_TUNING_DARK_CURVE], "%d,", drc_cfg->dark_lut[u32Idx]);
					if (drc_cfg->dark_lut[u32Idx] >= ((1 << 12) - 1))
						break;
				}
				fprintf(fdList[tunIdx], "\n-----\n");
			}
			if (fdList[DRC_TUNING_BRIT_CURVE]) {
				for (int u32Idx = 0; u32Idx < LTM_BRIGHT_CURVE_NODE_NUM; ++u32Idx) {
					fprintf(fdList[DRC_TUNING_BRIT_CURVE], "%d,", drc_cfg->brit_lut[u32Idx]);
					if (drc_cfg->brit_lut[u32Idx] >= ((1 << 16) - 1))
						break;
				}
				fprintf(fdList[tunIdx], "\n-----\n");
			}
		} else {
			for (CVI_U8 i = 0; i < DRC_TUNING_NUM; ++i) {
				if (fdList[i] != NULL) {
					printf("stop dump curve to /mnt/data/curve%d.txt!!\n", i);
					fclose(fdList[i]);
					fdList[i] = NULL;
				}
			}
		}
#endif
		//show lmap directly
		if (drc_attr->DRCMu[29] == 1 || drc_attr->DRCMu[29] == 2) {
			rng = 0;
			runtime->drc_param_out.reg_ltm_be_dist_wgt[0] = 1;
			runtime->drc_param_out.reg_ltm_de_dist_wgt[0] = 1;
			runtime->drc_param_out.reg_ltm_de_strth_dshft = 0;
			runtime->drc_param_out.reg_ltm_be_strth_dshft = 0;
			runtime->drc_param_out.reg_ltm_de_strth_gain = 1024;
			runtime->drc_param_out.reg_ltm_be_strth_gain = 1024;
			drc_cfg->dbg_mode = drc_attr->DRCMu[29];
		}

		//rgbgamma set bypass
		if (drc_attr->DRCMu[28] == 1) {
			isp_gamma_ctrl_set_rgbgamma_curve(ViPipe, CURVE_BYPASS);
		} else {
			isp_gamma_ctrl_set_rgbgamma_curve(ViPipe, CURVE_DEFAULT);
		}

#if defined(__CV181X__)
		if (pstIspCtx->u8SnsWDRMode <= WDR_MODE_QUDRA) {
			//In linearMode
			//hdr_paatern = 0 -> bnr ok, refineWeight ng
			//hdr_pattern = 1 -> bnr ng, refineWeight ok
			//we use drcMu[27] control bnr/refineWeight how to work
			if (drc_attr->DRCMu[27] <= 18) {
				if (runtime->lv <= (CVI_S16)(drc_attr->DRCMu[27])) {
					drc_cfg->hdr_pattern = 0;
				} else {
					drc_cfg->hdr_pattern = 1;
				}
			}
		}
#endif

		drc_cfg->lmap_enable = drc_attr->LocalToneEn;
		drc_cfg->lmap_w_bit = drc_attr->CoarseFltScale;
		drc_cfg->lmap_h_bit = drc_attr->CoarseFltScale;
		drc_cfg->lmap_thd_l = 0;
		drc_cfg->lmap_thd_h = 255;
		drc_cfg->lmap_y_mode = 1;
		drc_cfg->de_strth_dshft = runtime->drc_param_out.reg_ltm_de_strth_dshft;
		drc_cfg->de_strth_gain = runtime->drc_param_out.reg_ltm_de_strth_gain;
		drc_cfg->be_strth_dshft = runtime->drc_param_out.reg_ltm_be_strth_dshft;
		drc_cfg->be_strth_gain = runtime->drc_param_out.reg_ltm_be_strth_gain;

		struct cvi_isp_drc_tun_1_cfg *cfg_1 = &(drc_cfg->drc_1_cfg);

		cfg_1->REG_H90.bits.LTM_V_BLEND_THD_L = 0;
		cfg_1->REG_H90.bits.LTM_V_BLEND_THD_H = 1023;
#if defined(__CV180X__)
		//use Y as Luma
		if (chip_ver == E_CHIPVERSION_U01) {
			cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MIN = 0;
			cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MAX = 0;
		} else {
			cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MIN = 256;
			cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MAX = 256;
		}
		//note: fix CV180X rtl bug
		//patch for not use global/bright tone node256
#else
		//use maxRGB as Luma
		cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MIN = 256;
		cfg_1->REG_H94.bits.LTM_V_BLEND_WGT_MAX = 256;
#endif
		cfg_1->REG_H98.bits.LTM_V_BLEND_WGT_SLOPE = 0;

		cfg_1->REG_H9C.bits.LTM_DE_LMAP_LUT_IN_0 = runtime->drc_param_out.reg_ltm_de_lmap_lut_in[0];
		cfg_1->REG_H9C.bits.LTM_DE_LMAP_LUT_IN_1 = runtime->drc_param_out.reg_ltm_de_lmap_lut_in[1];
		cfg_1->REG_H9C.bits.LTM_DE_LMAP_LUT_IN_2 = runtime->drc_param_out.reg_ltm_de_lmap_lut_in[2];
		cfg_1->REG_H9C.bits.LTM_DE_LMAP_LUT_IN_3 = runtime->drc_param_out.reg_ltm_de_lmap_lut_in[3];
		cfg_1->REG_HA0.bits.LTM_DE_LMAP_LUT_OUT_0 = runtime->drc_param_out.reg_ltm_de_lmap_lut_out[0];
		cfg_1->REG_HA0.bits.LTM_DE_LMAP_LUT_OUT_1 = runtime->drc_param_out.reg_ltm_de_lmap_lut_out[1];
		cfg_1->REG_HA0.bits.LTM_DE_LMAP_LUT_OUT_2 = runtime->drc_param_out.reg_ltm_de_lmap_lut_out[2];
		cfg_1->REG_HA0.bits.LTM_DE_LMAP_LUT_OUT_3 = runtime->drc_param_out.reg_ltm_de_lmap_lut_out[3];
		cfg_1->REG_HA4.bits.LTM_DE_LMAP_LUT_SLOPE_0 = runtime->drc_param_out.reg_ltm_de_lmap_lut_slope[0];
		cfg_1->REG_HA4.bits.LTM_DE_LMAP_LUT_SLOPE_1 = runtime->drc_param_out.reg_ltm_de_lmap_lut_slope[1];
		cfg_1->REG_HA8.bits.LTM_DE_LMAP_LUT_SLOPE_2 = runtime->drc_param_out.reg_ltm_de_lmap_lut_slope[2];
		cfg_1->REG_HAC.bits.LTM_BE_LMAP_LUT_IN_0 = runtime->drc_param_out.reg_ltm_be_lmap_lut_in[0];
		cfg_1->REG_HAC.bits.LTM_BE_LMAP_LUT_IN_1 = runtime->drc_param_out.reg_ltm_be_lmap_lut_in[1];
		cfg_1->REG_HAC.bits.LTM_BE_LMAP_LUT_IN_2 = runtime->drc_param_out.reg_ltm_be_lmap_lut_in[2];
		cfg_1->REG_HAC.bits.LTM_BE_LMAP_LUT_IN_3 = runtime->drc_param_out.reg_ltm_be_lmap_lut_in[3];
		cfg_1->REG_HB0.bits.LTM_BE_LMAP_LUT_OUT_0 = runtime->drc_param_out.reg_ltm_be_lmap_lut_out[0];
		cfg_1->REG_HB0.bits.LTM_BE_LMAP_LUT_OUT_1 = runtime->drc_param_out.reg_ltm_be_lmap_lut_out[1];
		cfg_1->REG_HB0.bits.LTM_BE_LMAP_LUT_OUT_2 = runtime->drc_param_out.reg_ltm_be_lmap_lut_out[2];
		cfg_1->REG_HB0.bits.LTM_BE_LMAP_LUT_OUT_3 = runtime->drc_param_out.reg_ltm_be_lmap_lut_out[3];
		cfg_1->REG_HB4.bits.LTM_BE_LMAP_LUT_SLOPE_0 = runtime->drc_param_out.reg_ltm_be_lmap_lut_slope[0];
		cfg_1->REG_HB4.bits.LTM_BE_LMAP_LUT_SLOPE_1 = runtime->drc_param_out.reg_ltm_be_lmap_lut_slope[1];
		cfg_1->REG_HB8.bits.LTM_BE_LMAP_LUT_SLOPE_2 = runtime->drc_param_out.reg_ltm_be_lmap_lut_slope[2];

		cfg_1->REG_HBC.bits.LTM_EE_MOTION_LUT_IN_0 = drc_attr->DetailEnhanceMtIn[0];
		cfg_1->REG_HBC.bits.LTM_EE_MOTION_LUT_IN_1 = drc_attr->DetailEnhanceMtIn[1];
		cfg_1->REG_HBC.bits.LTM_EE_MOTION_LUT_IN_2 = drc_attr->DetailEnhanceMtIn[2];
		cfg_1->REG_HBC.bits.LTM_EE_MOTION_LUT_IN_3 = drc_attr->DetailEnhanceMtIn[3];
		cfg_1->REG_HC0.bits.LTM_EE_MOTION_LUT_OUT_0 = drc_attr->DetailEnhanceMtOut[0];
		cfg_1->REG_HC0.bits.LTM_EE_MOTION_LUT_OUT_1 = drc_attr->DetailEnhanceMtOut[1];
		cfg_1->REG_HC4.bits.LTM_EE_MOTION_LUT_OUT_2 = drc_attr->DetailEnhanceMtOut[2];
		cfg_1->REG_HC4.bits.LTM_EE_MOTION_LUT_OUT_3 = drc_attr->DetailEnhanceMtOut[3];
		cfg_1->REG_HC8.bits.LTM_EE_MOTION_LUT_SLOPE_0 = runtime->drc_param_out.DetailEnhanceMtSlope[0];
		cfg_1->REG_HC8.bits.LTM_EE_MOTION_LUT_SLOPE_1 = runtime->drc_param_out.DetailEnhanceMtSlope[1];
		cfg_1->REG_HCC.bits.LTM_EE_MOTION_LUT_SLOPE_2 = runtime->drc_param_out.DetailEnhanceMtSlope[2];

		cfg_1->REG_HD0.bits.LTM_EE_TOTAL_OSHTTHRD = drc_attr->OverShootThd;
		cfg_1->REG_HD0.bits.LTM_EE_TOTAL_USHTTHRD = drc_attr->UnderShootThd;
		cfg_1->REG_HD0.bits.LTM_EE_SHTCTRL_OSHTGAIN = drc_attr->OverShootGain;
		cfg_1->REG_HD0.bits.LTM_EE_SHTCTRL_USHTGAIN = drc_attr->UnderShootGain;
		cfg_1->REG_HD4.bits.LTM_EE_TOTAL_OSHTTHRD_CLP = drc_attr->OverShootThdMax;
		cfg_1->REG_HD4.bits.LTM_EE_TOTAL_USHTTHRD_CLP = drc_attr->UnderShootThdMin;
		cfg_1->REG_HD8.bits.LTM_EE_UPPER_BOUND_LEFT_DIFF = drc_attr->SoftClampUB;
		cfg_1->REG_HD8.bits.LTM_EE_LOWER_BOUND_RIGHT_DIFF = drc_attr->SoftClampLB;
		cfg_1->REG_HDC.bits.LTM_EE_MIN_Y = 3;

		struct cvi_isp_drc_tun_2_cfg *cfg_2 = &(drc_cfg->drc_2_cfg);
		cfg_2->REG_H14.bits.LTM_BE_RNG = rng;
		cfg_2->REG_H14.bits.LTM_DE_RNG = rng;
		cfg_2->REG_H18.bits.LTM_BRI_IN_THD_L = 0;
		cfg_2->REG_H18.bits.LTM_BRI_IN_THD_H = 255;
		cfg_2->REG_H18.bits.LTM_BRI_OUT_THD_L = 0;
		cfg_2->REG_H1C.bits.LTM_BRI_OUT_THD_H = 256;
		cfg_2->REG_H1C.bits.LTM_BRI_IN_GAIN_SLOP = 257;
		cfg_2->REG_H20.bits.LTM_DAR_IN_THD_L = 0;
		cfg_2->REG_H20.bits.LTM_DAR_IN_THD_H = 255;
		cfg_2->REG_H20.bits.LTM_DAR_OUT_THD_L = 256;
		cfg_2->REG_H24.bits.LTM_DAR_OUT_THD_H = 0;
		cfg_2->REG_H24.bits.LTM_DAR_IN_GAIN_SLOP = -257;
		cfg_2->REG_H28.bits.LTM_B_CURVE_QUAN_BIT = runtime->drc_param_out.reg_ltm_b_curve_quan_bit;
		cfg_2->REG_H28.bits.LTM_G_CURVE_1_QUAN_BIT = runtime->drc_param_out.reg_ltm_g_curve_1_quan_bit;
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_00 = runtime->drc_param_out.reg_ltm_be_dist_wgt[0];
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_01 = runtime->drc_param_out.reg_ltm_be_dist_wgt[1];
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_02 = runtime->drc_param_out.reg_ltm_be_dist_wgt[2];
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_03 = runtime->drc_param_out.reg_ltm_be_dist_wgt[3];
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_04 = runtime->drc_param_out.reg_ltm_be_dist_wgt[4];
		cfg_2->REG_H2C.bits.LTM_BE_DIST_WGT_05 = runtime->drc_param_out.reg_ltm_be_dist_wgt[5];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_00 = runtime->drc_param_out.reg_ltm_de_dist_wgt[0];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_01 = runtime->drc_param_out.reg_ltm_de_dist_wgt[1];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_02 = runtime->drc_param_out.reg_ltm_de_dist_wgt[2];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_03 = runtime->drc_param_out.reg_ltm_de_dist_wgt[3];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_04 = runtime->drc_param_out.reg_ltm_de_dist_wgt[4];
		cfg_2->REG_H30.bits.LTM_DE_DIST_WGT_05 = runtime->drc_param_out.reg_ltm_de_dist_wgt[5];

		struct cvi_isp_drc_tun_3_cfg *cfg_3 = &(drc_cfg->drc_3_cfg);

		cfg_3->REG_H64.bits.LTM_EE_ENABLE = drc_attr->DetailEnhanceEn;
		cfg_3->REG_H64.bits.LTM_EE_TOTAL_GAIN = runtime->drc_attr.TotalGain;
		cfg_3->REG_H64.bits.LTM_EE_LUMA_GAIN_ENABLE = 1;
		cfg_3->REG_H64.bits.LTM_EE_LUMA_MODE = 0;
		cfg_3->REG_H64.bits.LTM_EE_SOFT_CLAMP_ENABLE = drc_attr->SoftClampEnable;
		cfg_3->REG_H68.bits.LTM_EE_LUMA_GAIN_LUT_00 = drc_attr->LumaGain[0];
		cfg_3->REG_H68.bits.LTM_EE_LUMA_GAIN_LUT_01 = drc_attr->LumaGain[1];
		cfg_3->REG_H68.bits.LTM_EE_LUMA_GAIN_LUT_02 = drc_attr->LumaGain[2];
		cfg_3->REG_H68.bits.LTM_EE_LUMA_GAIN_LUT_03 = drc_attr->LumaGain[3];
		cfg_3->REG_H6C.bits.LTM_EE_LUMA_GAIN_LUT_04 = drc_attr->LumaGain[4];
		cfg_3->REG_H6C.bits.LTM_EE_LUMA_GAIN_LUT_05 = drc_attr->LumaGain[5];
		cfg_3->REG_H6C.bits.LTM_EE_LUMA_GAIN_LUT_06 = drc_attr->LumaGain[6];
		cfg_3->REG_H6C.bits.LTM_EE_LUMA_GAIN_LUT_07 = drc_attr->LumaGain[7];
		cfg_3->REG_H70.bits.LTM_EE_LUMA_GAIN_LUT_08 = drc_attr->LumaGain[8];
		cfg_3->REG_H70.bits.LTM_EE_LUMA_GAIN_LUT_09 = drc_attr->LumaGain[9];
		cfg_3->REG_H70.bits.LTM_EE_LUMA_GAIN_LUT_10 = drc_attr->LumaGain[10];
		cfg_3->REG_H70.bits.LTM_EE_LUMA_GAIN_LUT_11 = drc_attr->LumaGain[11];
		cfg_3->REG_H74.bits.LTM_EE_LUMA_GAIN_LUT_12 = drc_attr->LumaGain[12];
		cfg_3->REG_H74.bits.LTM_EE_LUMA_GAIN_LUT_13 = drc_attr->LumaGain[13];
		cfg_3->REG_H74.bits.LTM_EE_LUMA_GAIN_LUT_14 = drc_attr->LumaGain[14];
		cfg_3->REG_H74.bits.LTM_EE_LUMA_GAIN_LUT_15 = drc_attr->LumaGain[15];
		cfg_3->REG_H78.bits.LTM_EE_LUMA_GAIN_LUT_16 = drc_attr->LumaGain[16];
		cfg_3->REG_H78.bits.LTM_EE_LUMA_GAIN_LUT_17 = drc_attr->LumaGain[17];
		cfg_3->REG_H78.bits.LTM_EE_LUMA_GAIN_LUT_18 = drc_attr->LumaGain[18];
		cfg_3->REG_H78.bits.LTM_EE_LUMA_GAIN_LUT_19 = drc_attr->LumaGain[19];
		cfg_3->REG_H7C.bits.LTM_EE_LUMA_GAIN_LUT_20 = drc_attr->LumaGain[20];
		cfg_3->REG_H7C.bits.LTM_EE_LUMA_GAIN_LUT_21 = drc_attr->LumaGain[21];
		cfg_3->REG_H7C.bits.LTM_EE_LUMA_GAIN_LUT_22 = drc_attr->LumaGain[22];
		cfg_3->REG_H7C.bits.LTM_EE_LUMA_GAIN_LUT_23 = drc_attr->LumaGain[23];
		cfg_3->REG_H80.bits.LTM_EE_LUMA_GAIN_LUT_24 = drc_attr->LumaGain[24];
		cfg_3->REG_H80.bits.LTM_EE_LUMA_GAIN_LUT_25 = drc_attr->LumaGain[25];
		cfg_3->REG_H80.bits.LTM_EE_LUMA_GAIN_LUT_26 = drc_attr->LumaGain[26];
		cfg_3->REG_H80.bits.LTM_EE_LUMA_GAIN_LUT_27 = drc_attr->LumaGain[27];
		cfg_3->REG_H84.bits.LTM_EE_LUMA_GAIN_LUT_28 = drc_attr->LumaGain[28];
		cfg_3->REG_H84.bits.LTM_EE_LUMA_GAIN_LUT_29 = drc_attr->LumaGain[29];
		cfg_3->REG_H84.bits.LTM_EE_LUMA_GAIN_LUT_30 = drc_attr->LumaGain[30];
		cfg_3->REG_H84.bits.LTM_EE_LUMA_GAIN_LUT_31 = drc_attr->LumaGain[31];
		cfg_3->REG_H88.bits.LTM_EE_LUMA_GAIN_LUT_32 = drc_attr->LumaGain[32];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_drc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

		const ISP_DRC_ATTR_S *drc_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_drc_ctrl_get_drc_attr(ViPipe, &drc_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DRCEnable = drc_attr->Enable;
		pProcST->DRCisManualMode = (CVI_BOOL)drc_attr->enOpType;
		pProcST->DRCToneCurveSmooth = drc_attr->ToneCurveSmooth;
		pProcST->DRCHdrStrength = runtime->drc_attr.HdrStrength;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static CVI_S32 isp_drc_ctrl_ready(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (!runtime->initialized || !runtime->mapped)
		ret = CVI_FAILURE;

	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_drc_ctrl_runtime  *_get_drc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_drc_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DRC, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_drc_ctrl_check_drc_attr_valid(const ISP_DRC_ATTR_S *pstDRCAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDRCAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDRCAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDRCAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstDRCAttr, TuningMode, 0x0, 0x7);
	// CHECK_VALID_CONST(pstDRCAttr, LocalToneEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstDRCAttr, LocalToneRefineEn, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDRCAttr, ToneCurveSelect, 0x0, 0x1);
	// CHECK_VALID_ARRAY_1D(pstDRCAttr, CurveUserDefine, DRC_GLOBAL_USER_DEFINE_NUM, 0x0, 0xffff);
	// CHECK_VALID_ARRAY_1D(pstDRCAttr, DarkUserDefine, DRC_DARK_USER_DEFINE_NUM, 0x0, 0xffff);
	// CHECK_VALID_ARRAY_1D(pstDRCAttr, BrightUserDefine, DRC_BRIGHT_USER_DEFINE_NUM, 0x0, 0xffff);
	CHECK_VALID_CONST(pstDRCAttr, ToneCurveSmooth, 0x0, 0x1f4);
	CHECK_VALID_CONST(pstDRCAttr, CoarseFltScale, 0x3, 0x6);
	CHECK_VALID_CONST(pstDRCAttr, SdrTargetYGainMode, 0x0, 0x1);
	// CHECK_VALID_CONST(pstDRCAttr, DetailEnhanceEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_ARRAY_1D(pstDRCAttr, LumaGain, 33, 0x0, 0xff);
	// CHECK_VALID_ARRAY_1D(pstDRCAttr, DetailEnhanceMtIn, 4, 0x0, 0xff);
	CHECK_VALID_ARRAY_1D(pstDRCAttr, DetailEnhanceMtOut, 4, 0x0, 0x100);
	// CHECK_VALID_CONST(pstDRCAttr, OverShootThd, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDRCAttr, UnderShootThd, 0x0, 0xff);
	CHECK_VALID_CONST(pstDRCAttr, OverShootGain, 0x0, 0x3f);
	CHECK_VALID_CONST(pstDRCAttr, UnderShootGain, 0x0, 0x3f);
	// CHECK_VALID_CONST(pstDRCAttr, OverShootThdMax, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDRCAttr, UnderShootThdMin, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDRCAttr, SoftClampEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstDRCAttr, SoftClampUB, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDRCAttr, SoftClampLB, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDRCAttr, dbg_182x_sim_enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDRCAttr, DarkMapStr, 0x0, 0x80);
	CHECK_VALID_CONST(pstDRCAttr, BritMapStr, 0x0, 0x80);
	CHECK_VALID_CONST(pstDRCAttr, SdrDarkMapStr, 0x0, 0x80);
	CHECK_VALID_CONST(pstDRCAttr, SdrBritMapStr, 0x0, 0x80);


	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, TargetYScale, 0x0, 0x800);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, HdrStrength, 0x0, 0x100);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, DEAdaptPercentile, 0x0, 0x19);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, DEAdaptTargetGain, 0x1, 0x60);
	// CHECK_VALID_AUTO_LV_1D(pstDRCAttr, DEAdaptGainUB, 0x1, 0xff);
	// CHECK_VALID_AUTO_LV_1D(pstDRCAttr, DEAdaptGainLB, 0x1, 0xff);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, BritInflectPtLuma, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, BritContrastLow, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, BritContrastHigh, 0x0, 0x64);
	// CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrTargetY, 0x0, 0xff);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrTargetYGain, 0x20, 0x80);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrGlobalToneStr, 0x0, 0x100);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrDEAdaptPercentile, 0x0, 0x19);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrDEAdaptTargetGain, 0x1, 0x40);
	// CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrDEAdaptGainLB, 0x1, 0xff);
	// CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrDEAdaptGainUB, 0x1, 0xff);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrBritInflectPtLuma, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrBritContrastLow, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstDRCAttr, SdrBritContrastHigh, 0x0, 0x64);
	CHECK_VALID_AUTO_ISO_1D(pstDRCAttr, TotalGain, 0x0, 0xff);

	return ret;
}

static CVI_S32 isp_drc_ctrl_set_drc_attr_compatible(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

#ifdef CHIP_ARCH_CV182X
	// For U01/U02 compatible (CV182X only)
	uint32_t chip_ver;

	CVI_SYS_GetChipVersion(&chip_ver);

	if (chip_ver == CVIU01) {
		pstDRCAttr->DetailEnhanceEnable = CVI_FALSE;
	} else if (chip_ver == CVIU02) {
		pstDRCAttr->DetailEnhanceEnable = CVI_TRUE;
	}
#endif // CHIP_ARCH_CV182X

	UNUSED(ViPipe);
	UNUSED(pstDRCAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_drc_ctrl_get_drc_attr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S **pstDRCAttr)
{
	if (pstDRCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DRC, (CVI_VOID *) &shared_buffer);
	*pstDRCAttr = &shared_buffer->stDRCAttr;

	return ret;
}

static CVI_S32 isp_drc_ctrl_modify_rgbgamma_for_drc_tuning_mode(VI_PIPE ViPipe, ISP_DRC_ATTR_S *pstDRCAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_BOOL bModifyRGBGamma = CVI_FALSE;

	bModifyRGBGamma = (pstDRCAttr->Enable)
		&& ((pstDRCAttr->TuningMode == 1) || (pstDRCAttr->TuningMode == 2));

	if (runtime->modify_rgbgamma_for_drc_tuning != bModifyRGBGamma) {
		runtime->modify_rgbgamma_for_drc_tuning = bModifyRGBGamma;

		isp_gamma_ctrl_set_rgbgamma_curve(ViPipe,
			(runtime->modify_rgbgamma_for_drc_tuning) ? CURVE_DRC_TUNING_CURVE : CURVE_DEFAULT);
	}

	return ret;
}

CVI_S32 isp_drc_ctrl_set_drc_attr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S *pstDRCAttr)
{
	if (pstDRCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_drc_ctrl_check_drc_attr_valid(pstDRCAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DRC_ATTR_S *p = CVI_NULL;

	isp_drc_ctrl_get_drc_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDRCAttr, sizeof(*pstDRCAttr));

	isp_drc_ctrl_set_drc_attr_compatible(ViPipe, (ISP_DRC_ATTR_S *) p);

	isp_drc_ctrl_modify_rgbgamma_for_drc_tuning_mode(ViPipe, (ISP_DRC_ATTR_S *) p);

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_drc_ctrl_get_algo_info(VI_PIPE ViPipe, struct drc_algo_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_drc_ctrl_runtime *runtime = _get_drc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	info->globalToneBinNum = runtime->drc_param_out.ltm_global_tone_bin_num;
	info->globalToneSeStep = runtime->drc_param_out.ltm_global_tone_bin_se_step;
	info->globalToneResult = runtime->drc_param_out.reg_ltm_deflt_lut;
	info->darkToneResult = runtime->drc_param_out.reg_ltm_dark_lut;
	info->brightToneResult = runtime->drc_param_out.reg_ltm_brit_lut;

	info->drc_g_curve_1_quan_bit = runtime->drc_param_out.reg_ltm_g_curve_1_quan_bit;

	return ret;
}

