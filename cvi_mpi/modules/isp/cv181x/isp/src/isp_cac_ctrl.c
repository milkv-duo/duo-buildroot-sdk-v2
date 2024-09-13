/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_cac_ctrl.c
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

#include "isp_cac_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl cac_mod = {
	.init = isp_cac_ctrl_init,
	.uninit = isp_cac_ctrl_uninit,
	.suspend = isp_cac_ctrl_suspend,
	.resume = isp_cac_ctrl_resume,
	.ctrl = isp_cac_ctrl_ctrl
};


static CVI_S32 isp_cac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_cac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_cac_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_cac_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_cac_ctrl_check_cac_attr_valid(const ISP_CAC_ATTR_S *pstCACAttr);

static CVI_S32 set_cac_proc_info(VI_PIPE ViPipe);

static struct isp_cac_ctrl_runtime *_get_cac_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_cac_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_cac_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cac_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cac_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_cac_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCac;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCac = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_cac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_cac_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_cac_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_cac_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_cac_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_cac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CAC_ATTR_S *cac_attr = NULL;

	isp_cac_ctrl_get_cac_attr(ViPipe, &cac_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(cac_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!cac_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (cac_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(cac_attr, DePurpleStr);
		MANUAL(cac_attr, EdgeGlobalGain);
		MANUAL(cac_attr, EdgeCoring);
		MANUAL(cac_attr, EdgeStrMin);
		MANUAL(cac_attr, EdgeStrMax);
		MANUAL(cac_attr, DePurpleCbStr);
		MANUAL(cac_attr, DePurpleCrStr);
		MANUAL(cac_attr, DePurpleStrMaxRatio);
		MANUAL(cac_attr, DePurpleStrMinRatio);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(cac_attr, DePurpleStr, INTPLT_POST_ISO);
		AUTO(cac_attr, EdgeGlobalGain, INTPLT_POST_ISO);
		AUTO(cac_attr, EdgeCoring, INTPLT_POST_ISO);
		AUTO(cac_attr, EdgeStrMin, INTPLT_POST_ISO);
		AUTO(cac_attr, EdgeStrMax, INTPLT_POST_ISO);
		AUTO(cac_attr, DePurpleCbStr, INTPLT_POST_ISO);
		AUTO(cac_attr, DePurpleCrStr, INTPLT_POST_ISO);
		AUTO(cac_attr, DePurpleStrMaxRatio, INTPLT_POST_ISO);
		AUTO(cac_attr, DePurpleStrMinRatio, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	struct cac_param_in *ptParamIn = &(runtime->cac_param_in);

	ptParamIn->u8DePurpleCbStr = runtime->cac_attr.DePurpleCbStr;
	ptParamIn->u8DePurpleCrStr = runtime->cac_attr.DePurpleCrStr;
	ptParamIn->u8DePurpleStrMaxRatio = runtime->cac_attr.DePurpleStrMaxRatio;
	ptParamIn->u8DePurpleStrMinRatio = runtime->cac_attr.DePurpleStrMinRatio;
	memcpy(ptParamIn->au8EdgeGainIn, cac_attr->EdgeGainIn, 3 * sizeof(CVI_U8));
	memcpy(ptParamIn->au8EdgeGainOut, cac_attr->EdgeGainOut, 3 * sizeof(CVI_U8));

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_cac_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_cac_main(
		(struct cac_param_in *)&runtime->cac_param_in,
		(struct cac_param_out *)&runtime->cac_param_out
	);
	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_cac_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_cac_config *cac_cfg =
		(struct cvi_vip_isp_cac_config *)&(post_addr->tun_cfg[tun_idx].cac_cfg);

	const ISP_CAC_ATTR_S *cac_attr = NULL;

	isp_cac_ctrl_get_cac_attr(ViPipe, &cac_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		cac_cfg->update = 0;
	} else {
		struct cac_param_out *pcac_param = &(runtime->cac_param_out);

		cac_cfg->update = 1;
		cac_cfg->enable = cac_attr->Enable && !runtime->is_module_bypass;
		cac_cfg->out_sel = cac_attr->TuningMode;
		cac_cfg->purple_th = cac_attr->PurpleDetRange;
		cac_cfg->correct_strength = runtime->cac_attr.DePurpleStr;
		cac_cfg->purple_cb2 = cac_attr->PurpleCb2;
		cac_cfg->purple_cr2 = cac_attr->PurpleCr2;
		cac_cfg->purple_cb3 = cac_attr->PurpleCb3;
		cac_cfg->purple_cr3 = cac_attr->PurpleCr3;

		struct cvi_isp_cac_tun_cfg *cac_1_cfg = &(cac_cfg->cac_cfg);

		cac_1_cfg->CNR_PURPLE_CB.bits.CNR_PURPLE_CB = cac_attr->PurpleCb;
		cac_1_cfg->CNR_PURPLE_CB.bits.CNR_PURPLE_CR = cac_attr->PurpleCr;
		cac_1_cfg->CNR_GREEN_CB.bits.CNR_GREEN_CB = cac_attr->GreenCb;
		cac_1_cfg->CNR_GREEN_CB.bits.CNR_GREEN_CR = cac_attr->GreenCr;

		struct cvi_isp_cac_2_tun_cfg *cac_2_cfg = &(cac_cfg->cac_2_cfg);

		cac_2_cfg->CNR_EDGE_SCALE.bits.CNR_EDGE_SCALE = runtime->cac_attr.EdgeGlobalGain;
		cac_2_cfg->CNR_EDGE_SCALE.bits.CNR_EDGE_CORING = runtime->cac_attr.EdgeCoring;
		cac_2_cfg->CNR_EDGE_SCALE.bits.CNR_EDGE_MIN = runtime->cac_attr.EdgeStrMin;
		cac_2_cfg->CNR_EDGE_SCALE.bits.CNR_EDGE_MAX = runtime->cac_attr.EdgeStrMax;
		cac_2_cfg->CNR_EDGE_RATIO_SPEED.bits.CNR_RATIO_SPEED = 64;
		cac_2_cfg->CNR_EDGE_RATIO_SPEED.bits.CNR_CB_STR = pcac_param->u8CbStr;
		cac_2_cfg->CNR_EDGE_RATIO_SPEED.bits.CNR_CR_STR = pcac_param->u8CrStr;
		cac_2_cfg->CNR_DEPURPLE_WEIGHT_TH.bits.CNR_DEPURPLE_WEIGHT_TH = 255;
		cac_2_cfg->CNR_DEPURPLE_WEIGHT_TH.bits.CNR_DEPURPLE_STR_MIN_RATIO = pcac_param->u8DePurpleStrRatioMin;
		cac_2_cfg->CNR_DEPURPLE_WEIGHT_TH.bits.CNR_DEPURPLE_STR_MAX_RATIO = pcac_param->u8DePurpleStrRatioMax;

		struct cvi_isp_cac_3_tun_cfg *cac_3_cfg = &(cac_cfg->cac_3_cfg);
		CVI_U8 *pu8EdgeScaleLut = pcac_param->au8EdgeScaleLut;

		cac_3_cfg->CNR_EDGE_SCALE_LUT_0.bits.CNR_EDGE_SCALE_LUT_00 = pu8EdgeScaleLut[0];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_0.bits.CNR_EDGE_SCALE_LUT_01 = pu8EdgeScaleLut[1];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_0.bits.CNR_EDGE_SCALE_LUT_02 = pu8EdgeScaleLut[2];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_0.bits.CNR_EDGE_SCALE_LUT_03 = pu8EdgeScaleLut[3];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_4.bits.CNR_EDGE_SCALE_LUT_04 = pu8EdgeScaleLut[4];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_4.bits.CNR_EDGE_SCALE_LUT_05 = pu8EdgeScaleLut[5];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_4.bits.CNR_EDGE_SCALE_LUT_06 = pu8EdgeScaleLut[6];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_4.bits.CNR_EDGE_SCALE_LUT_07 = pu8EdgeScaleLut[7];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_8.bits.CNR_EDGE_SCALE_LUT_08 = pu8EdgeScaleLut[8];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_8.bits.CNR_EDGE_SCALE_LUT_09 = pu8EdgeScaleLut[9];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_8.bits.CNR_EDGE_SCALE_LUT_10 = pu8EdgeScaleLut[10];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_8.bits.CNR_EDGE_SCALE_LUT_11 = pu8EdgeScaleLut[11];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_12.bits.CNR_EDGE_SCALE_LUT_12 = pu8EdgeScaleLut[12];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_12.bits.CNR_EDGE_SCALE_LUT_13 = pu8EdgeScaleLut[13];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_12.bits.CNR_EDGE_SCALE_LUT_14 = pu8EdgeScaleLut[14];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_12.bits.CNR_EDGE_SCALE_LUT_15 = pu8EdgeScaleLut[15];
		cac_3_cfg->CNR_EDGE_SCALE_LUT_16.bits.CNR_EDGE_SCALE_LUT_16 = pu8EdgeScaleLut[16];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_cac_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_CAC_ATTR_S *cac_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_cac_ctrl_get_cac_attr(ViPipe, &cac_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->CACEnable = cac_attr->Enable;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_cac_ctrl_runtime *_get_cac_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_cac_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CAC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_cac_ctrl_check_cac_attr_valid(const ISP_CAC_ATTR_S *pstCACAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCACAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCACAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCACAttr, UpdateInterval, 1, 0xFF);
	CHECK_VALID_CONST(pstCACAttr, PurpleDetRange, 0, 0x80);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCb, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCr, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCb2, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCr2, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCb3, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, PurpleCr3, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, GreenCb, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstCACAttr, GreenCr, 0x0, 0xFF);
	CHECK_VALID_CONST(pstCACAttr, TuningMode, 0x0, 0x2);
	CHECK_VALID_ARRAY_1D(pstCACAttr, EdgeGainIn, 3, 0x0, 0x10);
	CHECK_VALID_ARRAY_1D(pstCACAttr, EdgeGainOut, 3, 0x0, 0x20);
	// CHECK_VALID_AUTO_ISO_1D(pstCACAttr, DePurpleStr, 0x0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstCACAttr, EdgeGlobalGain, 0x0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstCACAttr, EdgeCoring, 0x0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstCACAttr, EdgeStrMin, 0x0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstCACAttr, EdgeStrMax, 0x0, 0xFF);
	CHECK_VALID_AUTO_ISO_1D(pstCACAttr, DePurpleCbStr, 0x0, 0x8);
	CHECK_VALID_AUTO_ISO_1D(pstCACAttr, DePurpleCrStr, 0x0, 0x8);
	CHECK_VALID_AUTO_ISO_1D(pstCACAttr, DePurpleStrMaxRatio, 0x0, 0x40);
	CHECK_VALID_AUTO_ISO_1D(pstCACAttr, DePurpleStrMinRatio, 0x0, 0x40);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_cac_ctrl_get_cac_attr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S **pstCACAttr)
{
	if (pstCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_cac_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CAC, (CVI_VOID *) &shared_buffer);
	*pstCACAttr = &shared_buffer->stCACAttr;

	return ret;
}

CVI_S32 isp_cac_ctrl_set_cac_attr(VI_PIPE ViPipe, const ISP_CAC_ATTR_S *pstCACAttr)
{
	if (pstCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cac_ctrl_runtime *runtime = _get_cac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_cac_ctrl_check_cac_attr_valid(pstCACAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CAC_ATTR_S *p = CVI_NULL;

	isp_cac_ctrl_get_cac_attr(ViPipe, &p);
	memcpy((void *)p, pstCACAttr, sizeof(*pstCACAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

