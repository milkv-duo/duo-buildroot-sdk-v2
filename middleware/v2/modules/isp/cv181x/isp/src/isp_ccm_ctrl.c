/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ccm_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_ccm_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
#define AWB_SATURATION_BASE		(0x80)

const struct isp_module_ctrl ccm_mod = {
	.init = isp_ccm_ctrl_init,
	.uninit = isp_ccm_ctrl_uninit,
	.suspend = isp_ccm_ctrl_suspend,
	.resume = isp_ccm_ctrl_resume,
	.ctrl = isp_ccm_ctrl_ctrl
};


static CVI_S32 isp_ccm_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ccm_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ccm_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ccm_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_ccm_ctrl_check_ccm_attr_valid(const ISP_CCM_ATTR_S *pstCCMAttr);
static CVI_S32 isp_ccm_ctrl_check_saturation_attr_valid(const ISP_SATURATION_ATTR_S *pstSaturationAttr);
static CVI_S32 isp_ccm_ctrl_check_ccm_saturation_attr_valid(const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr);

static CVI_S32 set_ccm_proc_info(VI_PIPE ViPipe);

static struct isp_ccm_ctrl_runtime  *_get_ccm_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ccm_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	runtime->ccm_param_out.CCM[ISP_CHANNEL_LE] =
		ISP_PTR_CAST_PTR(&(runtime->ccm_attr[ISP_CHANNEL_LE].stManual.CCM[0]));
	runtime->ccm_param_out.CCM[ISP_CHANNEL_SE] =
		ISP_PTR_CAST_PTR(&(runtime->ccm_attr[ISP_CHANNEL_SE].stManual.CCM[0]));

	return ret;
}

CVI_S32 isp_ccm_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ccm_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ccm_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ccm_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ccm_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCcm;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCcm = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ccm_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ccm_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ccm_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ccm_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ccm_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ccm_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);
	ISP_CTX_S *pstIspCtx = NULL;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CCM_ATTR_S *ccm_attr = NULL;
	const ISP_CCM_SATURATION_ATTR_S *ccm_saturation_attr = NULL;

	isp_ccm_ctrl_get_ccm_attr(ViPipe, &ccm_attr);
	isp_ccm_ctrl_get_ccm_saturation_attr(ViPipe, &ccm_saturation_attr);
	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ccm_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ccm_attr->Enable || runtime->is_module_bypass)
		return ret;

	memcpy(&(runtime->ccm_attr[ISP_CHANNEL_LE]), ccm_attr, sizeof(ISP_CCM_ATTR_S));
	memcpy(&(runtime->ccm_attr[ISP_CHANNEL_SE]), ccm_attr, sizeof(ISP_CCM_ATTR_S));

	ISP_SATURATION_ATTR_S *pSaturation;
	CVI_U8 u8AWBAdjSaturation;

	u8AWBAdjSaturation = pstIspCtx->stAwbResult.u8Saturation[ISP_CHANNEL_LE];
	pSaturation = &(runtime->saturation_attr[ISP_CHANNEL_LE]);
	if (u8AWBAdjSaturation == AWB_SATURATION_BASE) {
		pSaturation->stManual.Saturation = ccm_saturation_attr->stManual.SaturationLE;
		for (CVI_U32 idx = 0 ; idx < ISP_AUTO_ISO_STRENGTH_NUM ; idx++)
			pSaturation->stAuto.Saturation[idx] = ccm_saturation_attr->stAuto.SaturationLE[idx];
	} else {
		CVI_U16 u16Saturation;
		CVI_U8 u8Saturation;

		u16Saturation = (CVI_U16)ccm_saturation_attr->stManual.SaturationLE
			* (CVI_U16)u8AWBAdjSaturation / AWB_SATURATION_BASE;
		u8Saturation = (u16Saturation > 0xFF) ? 0xFF : (CVI_U8)u16Saturation;
		pSaturation->stManual.Saturation = u8Saturation;

		for (CVI_U32 idx = 0 ; idx < ISP_AUTO_ISO_STRENGTH_NUM ; idx++) {
			u16Saturation = (CVI_U16)ccm_saturation_attr->stAuto.SaturationLE[idx]
				* (CVI_U16)u8AWBAdjSaturation / AWB_SATURATION_BASE;
			u8Saturation = (u16Saturation > 0xFF) ? 0xFF : (CVI_U8)u16Saturation;
			pSaturation->stAuto.Saturation[idx] = u8Saturation;
		}
	}

	u8AWBAdjSaturation = pstIspCtx->stAwbResult.u8Saturation[ISP_CHANNEL_SE];
	pSaturation = &(runtime->saturation_attr[ISP_CHANNEL_SE]);
	if (u8AWBAdjSaturation == AWB_SATURATION_BASE) {
		pSaturation->stManual.Saturation = ccm_saturation_attr->stManual.SaturationSE;
		for (CVI_U32 idx = 0 ; idx < ISP_AUTO_ISO_STRENGTH_NUM ; idx++)
			pSaturation->stAuto.Saturation[idx] = ccm_saturation_attr->stAuto.SaturationSE[idx];
	} else {
		CVI_U16 u16Saturation;
		CVI_U8 u8Saturation;

		u16Saturation = (CVI_U16)ccm_saturation_attr->stManual.SaturationSE
			* (CVI_U16)u8AWBAdjSaturation / AWB_SATURATION_BASE;
		u8Saturation = (u16Saturation > 0xFF) ? 0xFF : (CVI_U8)u16Saturation;
		pSaturation->stManual.Saturation = u8Saturation;

		for (CVI_U32 idx = 0 ; idx < ISP_AUTO_ISO_STRENGTH_NUM ; idx++) {
			u16Saturation = (CVI_U16)ccm_saturation_attr->stAuto.SaturationSE[idx]
				* (CVI_U16)u8AWBAdjSaturation / AWB_SATURATION_BASE;
			u8Saturation = (u16Saturation > 0xFF) ? 0xFF : (CVI_U8)u16Saturation;
			pSaturation->stAuto.Saturation[idx] = u8Saturation;
		}
	}

	// ParamIn
	runtime->ccm_param_in.ccm_attr[ISP_CHANNEL_LE] =
		ISP_PTR_CAST_PTR(&(runtime->ccm_attr[ISP_CHANNEL_LE]));
	runtime->ccm_param_in.ccm_attr[ISP_CHANNEL_SE] =
		ISP_PTR_CAST_PTR(&(runtime->ccm_attr[ISP_CHANNEL_SE]));
	runtime->ccm_param_in.saturation_attr[ISP_CHANNEL_LE] =
		ISP_PTR_CAST_PTR(&(runtime->saturation_attr[ISP_CHANNEL_LE]));
	runtime->ccm_param_in.saturation_attr[ISP_CHANNEL_SE] =
		ISP_PTR_CAST_PTR(&(runtime->saturation_attr[ISP_CHANNEL_SE]));
	runtime->ccm_param_in.iso = algoResult->u32PostIso;
	runtime->ccm_param_in.color_temp[ISP_CHANNEL_LE] = algoResult->u32ColorTemp;
	runtime->ccm_param_in.color_temp[ISP_CHANNEL_SE] = algoResult->u32ColorTemp;

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_ccm_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_ccm_main(
		(struct ccm_param_in *)&runtime->ccm_param_in,
		(struct ccm_param_out *)&runtime->ccm_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_ccm_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CCM_ATTR_S *ccm_attr = NULL;

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ccm_config *ccm_cfg[2] = {
		(struct cvi_vip_isp_ccm_config *)&(post_addr->tun_cfg[tun_idx].ccm_cfg[0]),
		(struct cvi_vip_isp_ccm_config *)&(post_addr->tun_cfg[tun_idx].ccm_cfg[1]),
	};

	isp_ccm_ctrl_get_ccm_attr(ViPipe, &ccm_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		for (CVI_U8 i = 0; i < 2 ; i++) {
			ccm_cfg[i]->inst = i;
			ccm_cfg[i]->update = 0;
		}
	} else {
		for (ISP_CHANNEL_LIST_E idx = ISP_CHANNEL_LE ; idx < ISP_CHANNEL_MAX_NUM ; idx++) {
			runtime->ccm_cfg[idx].update = 1;
			runtime->ccm_cfg[idx].enable = ccm_attr->Enable && !runtime->is_module_bypass;
			//memcpy(runtime->ccm_cfg[idx].coef,
			//	&(ISP_PTR_CAST_S16(runtime->ccm_param_out.CCM[idx])[0]), sizeof(CVI_S16) * 9);
			memcpy(runtime->ccm_cfg[idx].coef,
				runtime->ccm_attr[idx].stManual.CCM, sizeof(CVI_S16) * 9);
		}

		ISP_CTX_S *pstIspCtx = NULL;
		CVI_BOOL *map_pipe_to_enable;

		struct cvi_vip_isp_ccm_config *tmp_ccm_cfg[2] = {
			&(runtime->ccm_cfg[ISP_CHANNEL_LE]),
			&(runtime->ccm_cfg[ISP_CHANNEL_SE]),
		};

		CVI_BOOL map_pipe_to_enable_sdr[2] = {1, 0};
		CVI_BOOL map_pipe_to_enable_wdr[2] = {1, 1};

		ISP_GET_CTX(ViPipe, pstIspCtx);

		if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
			map_pipe_to_enable = map_pipe_to_enable_wdr;
		else
			map_pipe_to_enable = map_pipe_to_enable_sdr;
#if !defined(__CV180X__)
		for (CVI_U8 i = 0; i < 2 ; i++) {
#else
		for (CVI_U8 i = 0; i < 1 ; i++) {
#endif
			tmp_ccm_cfg[i]->inst = i;
			memcpy(ccm_cfg[i], tmp_ccm_cfg[i], sizeof(struct cvi_vip_isp_ccm_config));
			ccm_cfg[i]->enable = tmp_ccm_cfg[i]->enable && map_pipe_to_enable[i]
									&& !runtime->is_module_bypass;
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ccm_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_CCM_ATTR_S *ccm_attr = NULL;

		const ISP_CCM_SATURATION_ATTR_S *ccm_saturation_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_ccm_ctrl_get_ccm_attr(ViPipe, &ccm_attr);
		isp_ccm_ctrl_get_ccm_saturation_attr(ViPipe, &ccm_saturation_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		// CCM
		//common
		pProcST->CCMEnable = ccm_attr->Enable;
		pProcST->CCMisManualMode = ccm_attr->enOpType;
		//manual or auto
		pProcST->CCMSatEnable = ccm_attr->stManual.SatEnable;
		memcpy(pProcST->CCMCCM, ccm_attr->stManual.CCM, sizeof(CVI_S16) * 9);

		pProcST->CCMISOActEnable = ccm_attr->stAuto.ISOActEnable;
		pProcST->CCMTempActEnable = ccm_attr->stAuto.TempActEnable;
		pProcST->CCMTabNum = ccm_attr->stAuto.CCMTabNum;
		memcpy(pProcST->CCMTab, ccm_attr->stAuto.CCMTab,
			pProcST->CCMTabNum * sizeof(ISP_COLORMATRIX_ATTR_S));

		if (ccm_attr->enOpType == OP_TYPE_MANUAL) {
			pProcST->SaturationLE[0] = ccm_saturation_attr->stManual.SaturationLE;
			pProcST->SaturationSE[0] = ccm_saturation_attr->stManual.SaturationSE;
		} else {
			memcpy(pProcST->SaturationLE, ccm_saturation_attr->stAuto.SaturationLE,
				sizeof(CVI_U8) * ISP_AUTO_ISO_STRENGTH_NUM);
			memcpy(pProcST->SaturationSE, ccm_saturation_attr->stAuto.SaturationSE,
				sizeof(CVI_U8) * ISP_AUTO_ISO_STRENGTH_NUM);
		}
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_ccm_ctrl_runtime  *_get_ccm_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ccm_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CCM, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ccm_ctrl_check_ccm_attr_valid(const ISP_CCM_ATTR_S *pstCCMAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCCMAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCCMAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCCMAttr, UpdateInterval, 0, 0xff);

	// Manual
	const ISP_CCM_MANUAL_ATTR_S *ptCCMManual = &(pstCCMAttr->stManual);

	// CHECK_VALID_CONST(ptCCMManual, SatEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_ARRAY_1D(ptCCMManual, CCM, 9, -8192, 8191);

	// Auto
	const ISP_CCM_AUTO_ATTR_S *ptCCMAuto = &(pstCCMAttr->stAuto);

	// CHECK_VALID_CONST(ptCCMAuto, ISOActEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(ptCCMAuto, TempActEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(ptCCMAuto, CCMTabNum, 0x3, 0x7);

	for (CVI_U32 tempIdx = 0; tempIdx < ptCCMAuto->CCMTabNum; ++tempIdx) {
		const ISP_COLORMATRIX_ATTR_S *pstColorMatrix = &(ptCCMAuto->CCMTab[tempIdx]);

		CHECK_VALID_CONST(pstColorMatrix, ColorTemp, 0x1f4, 0x7530);
		CHECK_VALID_ARRAY_1D(pstColorMatrix, CCM, 9, -8192, 8191);
	}

	return ret;
}

static CVI_S32 isp_ccm_ctrl_check_saturation_attr_valid(const ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstSaturationAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);

	return ret;
}

static CVI_S32 isp_ccm_ctrl_check_ccm_saturation_attr_valid(const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_1D(pstCCMSaturationAttr, SaturationLE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstCCMSaturationAttr, SaturationSE, 0x0, 0xff);

	UNUSED(pstCCMSaturationAttr);

	return ret;
}


//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ccm_ctrl_get_ccm_attr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S **pstCCMAttr)
{
	if (pstCCMAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ccm_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CCM, (CVI_VOID *) &shared_buffer);
	*pstCCMAttr = &shared_buffer->stCCMAttr;

	// CVI_ISP_PrintCCMAttr(pstCCMAttr);

	return ret;
}

CVI_S32 isp_ccm_ctrl_set_ccm_attr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S *pstCCMAttr)
{
	if (pstCCMAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ccm_ctrl_check_ccm_attr_valid(pstCCMAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CCM_ATTR_S *p = CVI_NULL;

	isp_ccm_ctrl_get_ccm_attr(ViPipe, &p);
	memcpy((void *)p, pstCCMAttr, sizeof(*pstCCMAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_ccm_ctrl_get_ccm_saturation_attr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S **pstCCMSaturationAttr)
{
	if (pstCCMSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ccm_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CCM, (CVI_VOID *) &shared_buffer);
	*pstCCMSaturationAttr = &shared_buffer->stCCMSaturationAttr;

	return ret;
}

CVI_S32 isp_ccm_ctrl_set_ccm_saturation_attr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr)
{
	if (pstCCMSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ccm_ctrl_check_ccm_saturation_attr_valid(pstCCMSaturationAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CCM_SATURATION_ATTR_S *p = CVI_NULL;

	isp_ccm_ctrl_get_ccm_saturation_attr(ViPipe, &p);
	memcpy((void *)p, pstCCMSaturationAttr, sizeof(*pstCCMSaturationAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_ccm_ctrl_get_saturation_attr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S **pstSaturationAttr)
{
	if (pstSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ccm_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CCM, (CVI_VOID *) &shared_buffer);
	*pstSaturationAttr = &shared_buffer->stSaturationAttr;

	return ret;
}

CVI_S32 isp_ccm_ctrl_set_saturation_attr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSaturationAttr)
{
	if (pstSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ccm_ctrl_check_saturation_attr_valid(pstSaturationAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_SATURATION_ATTR_S *p = CVI_NULL;

	isp_ccm_ctrl_get_saturation_attr(ViPipe, &p);
	memcpy((void *)p, pstSaturationAttr, sizeof(*pstSaturationAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_ccm_ctrl_get_ccm_info(VI_PIPE ViPipe, struct ccm_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ccm_ctrl_runtime *runtime = _get_ccm_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CCM_ATTR_S *ccm_attr = NULL;

	isp_ccm_ctrl_get_ccm_attr(ViPipe, &ccm_attr);
	info->ccm_en = ccm_attr->Enable;
	for (CVI_U32 i = 0 ; i < 9 ; i++) {
		//info->CCM[i] = ISP_PRT_CAST_U16(runtime->ccm_param_out.CCM[0])[i];
		info->CCM[i] = runtime->ccm_attr[ISP_CHANNEL_LE].stManual.CCM[i];
	}

	return ret;
}

