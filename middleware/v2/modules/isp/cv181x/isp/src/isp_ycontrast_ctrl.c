/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ycontrast_ctrl.c
 * Description:
 *
 */

#include "cvi_type.h"

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_ycontrast_ctrl.h"

#include "isp_iir_api.h"
#include "isp_mgr_buf.h"

#define		DEFAULT_YCOURVE_IIR_SPEED		(50)

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl ycontrast_mod = {
	.init = isp_ycontrast_ctrl_init,
	.uninit = isp_ycontrast_ctrl_uninit,
	.suspend = isp_ycontrast_ctrl_suspend,
	.resume = isp_ycontrast_ctrl_resume,
	.ctrl = isp_ycontrast_ctrl_ctrl
};

static CVI_S32 isp_ycontrast_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ycontrast_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ycontrast_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ycontrast_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_ycontrast_ctrl_check_ycontrast_attr_valid(const ISP_YCONTRAST_ATTR_S *pstYContrastAttr);
static CVI_S32 set_ycontrast_proc_info(VI_PIPE ViPipe);

static struct isp_ycontrast_ctrl_runtime *_get_ycontrast_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ycontrast_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->bResetYCurveIIR = CVI_TRUE;
	runtime->u16YCurveIIRSpeed = DEFAULT_YCOURVE_IIR_SPEED;

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_ycontrast_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ycontrast_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ycontrast_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ycontrast_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ycontrast_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassYcontrast;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassYcontrast = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ycontrast_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	// ISP_BYPASS_CHECK(ispBlkBypass[ViPipe][ISP_IQ_BLOCK_LSC_M], ycontrast0);
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ycontrast_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ycontrast_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ycontrast_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ycontrast_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ycontrast_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_YCONTRAST_ATTR_S *ycontrast_attr = NULL;

	isp_ycontrast_ctrl_get_ycontrast_attr(ViPipe, &ycontrast_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ycontrast_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ycontrast_attr->Enable || runtime->is_module_bypass) {
		runtime->bResetYCurveIIR = CVI_TRUE;
		return ret;
	}

	if (ycontrast_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param_in, _param_out) \
		{ runtime->_param_out = _attr->stManual._param_in; }

		MANUAL(ycontrast_attr, ContrastLow, u8ContrastLow);
		MANUAL(ycontrast_attr, ContrastHigh, u8ContrastHigh);
		MANUAL(ycontrast_attr, CenterLuma, u8CenterLuma);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param_in, _param_out, type) \
		{ runtime->_param_out = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param_in); }

		AUTO(ycontrast_attr, ContrastLow, u8ContrastLow, INTPLT_LV);
		AUTO(ycontrast_attr, ContrastHigh, u8ContrastHigh, INTPLT_LV);
		AUTO(ycontrast_attr, CenterLuma, u8CenterLuma, INTPLT_LV);

		#undef AUTO
	}

	runtime->ycontrast_param_in.ContrastLow = runtime->u8ContrastLow;
	runtime->ycontrast_param_in.ContrastHigh = runtime->u8ContrastHigh;
	runtime->ycontrast_param_in.CenterLuma = runtime->u8CenterLuma;

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_ycontrast_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_ycontrast_main(
		(struct ycontrast_param_in *)&runtime->ycontrast_param_in,
		(struct ycontrast_param_out *)&runtime->ycontrast_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_ycontrast_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);
	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ycur_config *ycur_cfg =
		(struct cvi_vip_isp_ycur_config *)&(post_addr->tun_cfg[tun_idx].ycur_cfg);

	const ISP_YCONTRAST_ATTR_S *ycontrast_attr = NULL;

	TIIR_U8_Ctrl tIIRCtrl;

	tIIRCtrl.pu8LutIn = runtime->ycontrast_param_out.au8YCurve_Lut;
	tIIRCtrl.pu32LutHistory = runtime->au32YCurveLutIIRHist;
	tIIRCtrl.pu8LutOut = ycur_cfg->lut;
	tIIRCtrl.u16LutSize = YCURVE_LUT_ENTRY_NUM;
	tIIRCtrl.u16IIRWeight = runtime->u16YCurveIIRSpeed;

	if (runtime->bResetYCurveIIR) {
		tIIRCtrl.u16IIRWeight = 0;
		runtime->bResetYCurveIIR = CVI_FALSE;
	}

	isp_ycontrast_ctrl_get_ycontrast_attr(ViPipe, &ycontrast_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		ycur_cfg->update = 0;
	} else {
		ycur_cfg->update = 1;
		ycur_cfg->enable = ycontrast_attr->Enable && !runtime->is_module_bypass;

		IIR_U8_Once(&tIIRCtrl);

		ycur_cfg->lut_256 = runtime->ycontrast_param_out.u16YCurve_LastPoint;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ycontrast_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	UNUSED(ViPipe);
#if 0
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

		const ISP_YCONTRAST_ATTR_S *ycontrast_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_ycontrast_ctrl_get_ycontrast_attr(ViPipe, &ycontrast_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->YCURVEnable = ycontrast_attr->Enable && !runtime->is_module_bypass;
		pProcST->YCURVisManualMode = ycontrast_attr->enOpType;
	}
#endif //
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_ycontrast_ctrl_runtime *_get_ycontrast_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ycur_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YCONTRAST, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ycontrast_ctrl_check_ycontrast_attr_valid(const ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstYContrastAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstYContrastAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstYContrastAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_LV_1D(pstYContrastAttr, ContrastLow, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstYContrastAttr, ContrastHigh, 0x0, 0x64);
	CHECK_VALID_AUTO_LV_1D(pstYContrastAttr, CenterLuma, 0x0, 0x40);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ycontrast_ctrl_get_ycontrast_attr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S **pstYContrastAttr)
{
	if (pstYContrastAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycur_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YCONTRAST, (CVI_VOID *) &shared_buffer);
	*pstYContrastAttr = &shared_buffer->stYContrastAttr;

	return ret;
}

CVI_S32 isp_ycontrast_ctrl_set_ycontrast_attr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S *pstYContrastAttr)
{
	if (pstYContrastAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ycontrast_ctrl_runtime *runtime = _get_ycontrast_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ycontrast_ctrl_check_ycontrast_attr_valid(pstYContrastAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_YCONTRAST_ATTR_S *p = CVI_NULL;

	isp_ycontrast_ctrl_get_ycontrast_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstYContrastAttr, sizeof(*pstYContrastAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

