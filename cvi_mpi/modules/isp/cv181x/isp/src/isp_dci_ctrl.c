/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dci_ctrl.c
 * Description:
 *
 */

#include <math.h>

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "isp_dci_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl dci_mod = {
	.init = isp_dci_ctrl_init,
	.uninit = isp_dci_ctrl_uninit,
	.suspend = isp_dci_ctrl_suspend,
	.resume = isp_dci_ctrl_resume,
	.ctrl = isp_dci_ctrl_ctrl
};

static CVI_S32 isp_dci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dci_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dci_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_dci_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_dci_ctrl_check_dci_attr_valid(const ISP_DCI_ATTR_S *pstdciAttr);

static struct isp_dci_ctrl_runtime  *_get_dci_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dci_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	memset(runtime->pre_dci_data, 0, sizeof(CVI_U16) * DCI_BINS_NUM);
	memset(runtime->pre_dci_lut, 0, sizeof(CVI_U32) * DCI_BINS_NUM);
	memset(&runtime->dciStatsInfoBuf, 0, sizeof(ISP_DCI_STATISTICS_S));
	runtime->reset_iir = CVI_TRUE;

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	isp_algo_dci_init();

	return ret;
}

CVI_S32 isp_dci_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	isp_algo_dci_uninit();

	return ret;
}

CVI_S32 isp_dci_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dci_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dci_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dci_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDci;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDci = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dci_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dci_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dci_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_dci_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DCI_ATTR_S *dci_attr = NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);

	CVI_U32 speed = dci_attr->Speed;

	CVI_BOOL checkSkipDCIGenCurveStep = CVI_TRUE;

	if (dci_attr->Enable && !runtime->is_module_bypass) {
		if (runtime->reset_iir) {
			speed = 0;
			checkSkipDCIGenCurveStep = CVI_FALSE;
			runtime->reset_iir = CVI_FALSE;
		}
	} else {
		runtime->reset_iir = CVI_TRUE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(dci_attr->UpdateInterval, 1);
	CVI_U8 frame_delay = (intvl < speed) ? 1 : (intvl / MAX(speed, 1));

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % frame_delay) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!dci_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (dci_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(dci_attr, ContrastGain);
		MANUAL(dci_attr, BlcThr);
		MANUAL(dci_attr, WhtThr);
		MANUAL(dci_attr, BlcCtrl);
		MANUAL(dci_attr, WhtCtrl);
		MANUAL(dci_attr, DciGainMax);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(dci_attr, ContrastGain, INTPLT_POST_ISO);
		AUTO(dci_attr, BlcThr, INTPLT_POST_ISO);
		AUTO(dci_attr, WhtThr, INTPLT_POST_ISO);
		AUTO(dci_attr, BlcCtrl, INTPLT_POST_ISO);
		AUTO(dci_attr, WhtCtrl, INTPLT_POST_ISO);
		AUTO(dci_attr, DciGainMax, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	ISP_DCI_STATISTICS_S *dciStatsInfo;

	isp_sts_ctrl_get_dci_sts(ViPipe, &dciStatsInfo);

	runtime->dci_param_in.pHist = ISP_PTR_CAST_PTR(dciStatsInfo->hist);
	runtime->dci_param_in.pCntCurve = ISP_PTR_CAST_PTR(runtime->pre_dci_data);
	runtime->dci_param_in.pPreDCILut = ISP_PTR_CAST_PTR(runtime->pre_dci_lut);
	runtime->dci_param_in.dci_bins_num = DCI_BINS_NUM;
	runtime->dci_param_in.img_width = pstIspCtx->stSysRect.u32Width;
	runtime->dci_param_in.img_height = pstIspCtx->stSysRect.u32Height;
	runtime->dci_param_in.ContrastGain = runtime->dci_attr.ContrastGain;
	runtime->dci_param_in.DciStrength = dci_attr->DciStrength;
	runtime->dci_param_in.DciGamma = dci_attr->DciGamma;
	runtime->dci_param_in.DciOffset = dci_attr->DciOffset;
	runtime->dci_param_in.BlcThr = runtime->dci_attr.BlcThr;
	runtime->dci_param_in.WhtThr = runtime->dci_attr.WhtThr;
	runtime->dci_param_in.BlcCtrl = runtime->dci_attr.BlcCtrl;
	runtime->dci_param_in.WhtCtrl = runtime->dci_attr.WhtCtrl;
	runtime->dci_param_in.DciGainMax = runtime->dci_attr.DciGainMax;
	runtime->dci_param_in.Speed = speed;
	runtime->dci_param_in.ToleranceY = dci_attr->ToleranceY;
	runtime->dci_param_in.Method = dci_attr->Method;
	runtime->dci_param_in.bUpdateCurve = CVI_TRUE;

	// ParamOut
	runtime->dci_param_out.map_lut = ISP_PTR_CAST_PTR(runtime->map_lut);

	if (checkSkipDCIGenCurveStep) {
		CVI_U32 threshold = (0xFF - dci_attr->Sensitivity) << 4;

		CVI_U32 diff;

		runtime->dci_param_in.bUpdateCurve = CVI_FALSE;
		for (CVI_U32 i = 0 ; i < DCI_BINS_NUM; i++) {
			diff = abs(runtime->dciStatsInfoBuf.hist[i] - dciStatsInfo->hist[i]);
			if (diff > threshold) {
				memcpy(&runtime->dciStatsInfoBuf, dciStatsInfo, sizeof(*dciStatsInfo));
				runtime->dci_param_in.bUpdateCurve = CVI_TRUE;
				break;
			}
		}
	} else {
		memcpy(&runtime->dciStatsInfoBuf, dciStatsInfo, sizeof(*dciStatsInfo));
	}

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_dci_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_dci_main(
		(struct dci_param_in *)&runtime->dci_param_in,
		(struct dci_param_out *)&runtime->dci_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_dci_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_dci_config *dci_cfg =
		(struct cvi_vip_isp_dci_config *)&(post_addr->tun_cfg[tun_idx].dci_cfg);

	const ISP_DCI_ATTR_S *dci_attr = NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		dci_cfg->update = 0;
	} else {
		dci_cfg->update = 1;
		dci_cfg->enable = dci_attr->Enable && !runtime->is_module_bypass;
		dci_cfg->map_enable = dci_attr->Enable && !runtime->is_module_bypass;
		dci_cfg->demo_mode = dci_attr->TuningMode;

		memcpy(dci_cfg->map_lut, runtime->map_lut, sizeof(CVI_U16) * DCI_BINS_NUM);

		dci_cfg->per1sample_enable = 1;
		dci_cfg->hist_enable = 1;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_dci_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

		const ISP_DCI_ATTR_S *dci_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_dci_ctrl_get_dci_attr(ViPipe, &dci_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DCIEnable = dci_attr->Enable;
		pProcST->DCISpeed = dci_attr->Speed;
		pProcST->DCIDciStrength = dci_attr->DciStrength;
		pProcST->DCIisManualMode = (CVI_BOOL)dci_attr->enOpType;
		//manual or auto
		pProcST->DCIContrastGain = runtime->dci_param_in.ContrastGain;
		pProcST->DCIBlcThr = runtime->dci_param_in.BlcThr;
		pProcST->DCIWhtThr = runtime->dci_param_in.WhtThr;
		pProcST->DCIBlcCtrl = runtime->dci_param_in.BlcCtrl;
		pProcST->DCIWhtCtrl = runtime->dci_param_in.WhtCtrl;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_dci_ctrl_runtime *_get_dci_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DCI, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dci_ctrl_check_dci_attr_valid(const ISP_DCI_ATTR_S *pstDCIAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDCIAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDCIAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDCIAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstDCIAttr, TuningMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDCIAttr, Method, 0x0, 0x1);
	CHECK_VALID_CONST(pstDCIAttr, Speed, 0x0, 0x1f4);
	CHECK_VALID_CONST(pstDCIAttr, DciStrength, 0x0, 0x100);
	CHECK_VALID_CONST(pstDCIAttr, DciGamma, 0x64, 0x320);
	// CHECK_VALID_CONST(pstDCIAttr, DciOffset, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDCIAttr, ToleranceY, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDCIAttr, Sensitivity, 0x0, 0xff);

	CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, ContrastGain, 0, 0x100);
	// CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, BlcThr, 0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, WhtThr, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, BlcCtrl, 0, 0x200);
	CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, WhtCtrl, 0, 0x200);
	CHECK_VALID_AUTO_ISO_1D(pstDCIAttr, DciGainMax, 0, 0x100);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dci_ctrl_get_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S **pstDCIAttr)
{
	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DCI, (CVI_VOID *) &shared_buffer);

	*pstDCIAttr = &shared_buffer->stDciAttr;

	return ret;
}

CVI_S32 isp_dci_ctrl_set_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S *pstDCIAttr)
{
	if (pstDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dci_ctrl_runtime *runtime = _get_dci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dci_ctrl_check_dci_attr_valid(pstDCIAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DCI_ATTR_S *p = CVI_NULL;

	isp_dci_ctrl_get_dci_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDCIAttr, sizeof(*pstDCIAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}


