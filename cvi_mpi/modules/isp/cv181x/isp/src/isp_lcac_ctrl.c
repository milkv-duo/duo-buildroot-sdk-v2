/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_lcac_ctrl.c
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
#include "isp_sts_ctrl.h"

#include "isp_lcac_ctrl.h"
#include "isp_mgr_buf.h"
#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl lcac_mod = {
	.init = isp_lcac_ctrl_init,
	.uninit = isp_lcac_ctrl_uninit,
	.suspend = isp_lcac_ctrl_suspend,
	.resume = isp_lcac_ctrl_resume,
	.ctrl = isp_lcac_ctrl_ctrl
};

static CVI_S32 isp_lcac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_lcac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_lcac_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_lcac_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_lcac_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_lcac_ctrl_check_lcac_attr_valid(const ISP_LCAC_ATTR_S *pstLCACAttr);

static struct isp_lcac_ctrl_runtime *_get_lcac_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_lcac_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 u32AlgoMemorySize;

	isp_algo_lcac_get_internal_memory(&u32AlgoMemorySize);

	// printf("LCAC algo. internal memory : %d\n", u32AlgoMemorySize);

	if (runtime->pvAlgoMemory != CVI_NULL) {
		free(ISP_PTR_CAST_VOID(runtime->pvAlgoMemory));
		runtime->pvAlgoMemory = CVI_NULL;
	}
	runtime->pvAlgoMemory = ISP_PTR_CAST_PTR(malloc(u32AlgoMemorySize));
	if (runtime->pvAlgoMemory == CVI_NULL) {
		ISP_LOG_WARNING("Allocate memory for LCAC algo. (%d) fail\n", ViPipe);
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	runtime->lcac_param_in.pvIntMemory = runtime->pvAlgoMemory;

	isp_algo_lcac_init();

	return ret;
}

CVI_S32 isp_lcac_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	isp_algo_lcac_uninit();

	if (runtime->pvAlgoMemory != CVI_NULL) {
		free(ISP_PTR_CAST_VOID(runtime->pvAlgoMemory));
		runtime->pvAlgoMemory = CVI_NULL;
	}

	UNUSED(runtime);

	return ret;
}

CVI_S32 isp_lcac_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_lcac_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_lcac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_lcac_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassLcac;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassLcac = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_lcac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_lcac_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_lcac_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_lcac_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_lcac_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_lcac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	ISP_LOG_DEBUG("+\n");
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);
	const ISP_LCAC_ATTR_S *lcac_attr = NULL;
	CVI_S32 ret = CVI_SUCCESS;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_lcac_ctrl_get_lcac_attr(ViPipe, &lcac_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(lcac_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!lcac_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (lcac_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(lcac_attr, DePurpleCrGain);
		MANUAL(lcac_attr, DePurpleCbGain);
		MANUAL(lcac_attr, DePurepleCrWgt0);
		MANUAL(lcac_attr, DePurepleCbWgt0);
		MANUAL(lcac_attr, DePurepleCrWgt1);
		MANUAL(lcac_attr, DePurepleCbWgt1);
		MANUAL(lcac_attr, EdgeCoringBase);
		MANUAL(lcac_attr, EdgeCoringAdv);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(lcac_attr, DePurpleCrGain, INTPLT_POST_ISO);
		AUTO(lcac_attr, DePurpleCbGain, INTPLT_POST_ISO);
		AUTO(lcac_attr, DePurepleCrWgt0, INTPLT_POST_ISO);
		AUTO(lcac_attr, DePurepleCbWgt0, INTPLT_POST_ISO);
		AUTO(lcac_attr, DePurepleCrWgt1, INTPLT_POST_ISO);
		AUTO(lcac_attr, DePurepleCbWgt1, INTPLT_POST_ISO);
		AUTO(lcac_attr, EdgeCoringBase, INTPLT_POST_ISO);
		AUTO(lcac_attr, EdgeCoringAdv, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	struct lcac_param_in *ptParamIn = &(runtime->lcac_param_in);

	ptParamIn->u8FilterTypeBase = lcac_attr->FilterTypeBase;
	ptParamIn->u8FilterTypeAdv = lcac_attr->FilterTypeAdv;
	ptParamIn->u8EdgeWgtBase_Wgt = lcac_attr->EdgeWgtBase.Wgt;
	ptParamIn->u8EdgeWgtBase_Sigma = lcac_attr->EdgeWgtBase.Sigma;
	ptParamIn->u8EdgeWgtAdv_Wgt = lcac_attr->EdgeWgtAdv.Wgt;
	ptParamIn->u8EdgeWgtAdv_Sigma = lcac_attr->EdgeWgtAdv.Sigma;
	ptParamIn->pvIntMemory = runtime->pvAlgoMemory;

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_lcac_ctrl_process(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_lcac_main(&(runtime->lcac_param_in), &(runtime->lcac_param_out));

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_lcac_ctrl_postprocess(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_lcac_config *lcac_cfg =
		(struct cvi_vip_isp_lcac_config *)&(post_addr->tun_cfg[tun_idx].lcac_cfg);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		lcac_cfg->update = 0;
	} else {
		struct lcac_param_out *plcac_param = &(runtime->lcac_param_out);
		const ISP_LCAC_ATTR_S *lcac_attr = NULL;
		CVI_U8 *pu8Lut = CVI_NULL;

		isp_lcac_ctrl_get_lcac_attr(ViPipe, &lcac_attr);

		lcac_cfg->update = 1;
		lcac_cfg->enable = lcac_attr->Enable && !runtime->is_module_bypass;

		lcac_cfg->out_sel = lcac_attr->TuningMode;
		lcac_cfg->lti_luma_lut_32 = plcac_param->au8LTILumaLut[32];
		lcac_cfg->fcf_luma_lut_32 = plcac_param->au8FCFLumaLut[32];

		struct cvi_isp_lcac_tun_cfg *lcac_tun_cfg = &(lcac_cfg->lcac_cfg);

		lcac_tun_cfg->REG04.bits.LCAC_LTI_STR_R1 = runtime->lcac_attr.DePurpleCrGain;		//12
		lcac_tun_cfg->REG04.bits.LCAC_LTI_STR_B1 = runtime->lcac_attr.DePurpleCbGain;		//12
		lcac_tun_cfg->REG08.bits.LCAC_LTI_STR_R2_LE = lcac_attr->DePurpleCrStr0;	//7
		lcac_tun_cfg->REG08.bits.LCAC_LTI_STR_B2_LE = lcac_attr->DePurpleCbStr0;	//7
		lcac_tun_cfg->REG08.bits.LCAC_LTI_WGT_R_LE = runtime->lcac_attr.DePurepleCrWgt0;	//7
		lcac_tun_cfg->REG08.bits.LCAC_LTI_WGT_B_LE = runtime->lcac_attr.DePurepleCbWgt0;	//7
		lcac_tun_cfg->REG0C.bits.LCAC_LTI_STR_R2_SE = lcac_attr->DePurpleCrStr1;	//7
		lcac_tun_cfg->REG0C.bits.LCAC_LTI_STR_B2_SE = lcac_attr->DePurpleCbStr1;	//7
		lcac_tun_cfg->REG0C.bits.LCAC_LTI_WGT_R_SE = runtime->lcac_attr.DePurepleCrWgt1;	//7
		lcac_tun_cfg->REG0C.bits.LCAC_LTI_WGT_B_SE = runtime->lcac_attr.DePurepleCbWgt1;	//7
		lcac_tun_cfg->REG10.bits.LCAC_LTI_KERNEL_R0 = plcac_param->as16LTIFilterKernel[0];	//10
		lcac_tun_cfg->REG10.bits.LCAC_LTI_KERNEL_R1 = plcac_param->as16LTIFilterKernel[1];	//10
		lcac_tun_cfg->REG14.bits.LCAC_LTI_KERNEL_R2 = plcac_param->as16LTIFilterKernel[2];	//10
		lcac_tun_cfg->REG14.bits.LCAC_LTI_KERNEL_B0 = plcac_param->as16LTIFilterKernel[0];	//10
		lcac_tun_cfg->REG18.bits.LCAC_LTI_KERNEL_B1 = plcac_param->as16LTIFilterKernel[1];	//10
		lcac_tun_cfg->REG18.bits.LCAC_LTI_KERNEL_B2 = plcac_param->as16LTIFilterKernel[2];	//10
		lcac_tun_cfg->REG1C.bits.LCAC_LTI_EDGE_SCALE_R_LE = lcac_attr->EdgeGainBase0;	//6
		lcac_tun_cfg->REG1C.bits.LCAC_LTI_EDGE_SCALE_G_LE = lcac_attr->EdgeGainBase0;	//6
		lcac_tun_cfg->REG1C.bits.LCAC_LTI_EDGE_SCALE_B_LE = lcac_attr->EdgeGainBase0;	//6
		lcac_tun_cfg->REG20.bits.LCAC_LTI_EDGE_SCALE_R_SE = lcac_attr->EdgeGainBase1;	//6
		lcac_tun_cfg->REG20.bits.LCAC_LTI_EDGE_SCALE_G_SE = lcac_attr->EdgeGainBase1;	//6
		lcac_tun_cfg->REG20.bits.LCAC_LTI_EDGE_SCALE_B_SE = lcac_attr->EdgeGainBase1;	//6
		lcac_tun_cfg->REG24.bits.LCAC_LTI_EDGE_CORING_R = runtime->lcac_attr.EdgeCoringBase;	//8
		lcac_tun_cfg->REG24.bits.LCAC_LTI_EDGE_CORING_G = runtime->lcac_attr.EdgeCoringBase;	//8
		lcac_tun_cfg->REG24.bits.LCAC_LTI_EDGE_CORING_B = runtime->lcac_attr.EdgeCoringBase;	//8
		lcac_tun_cfg->REG28.bits.LCAC_LTI_WGT_MAX_R = lcac_attr->DePurpleStrMaxBase;	//8
		lcac_tun_cfg->REG28.bits.LCAC_LTI_WGT_MIN_R = lcac_attr->DePurpleStrMinBase;	//8
		lcac_tun_cfg->REG28.bits.LCAC_LTI_WGT_MAX_B = lcac_attr->DePurpleStrMaxBase;	//8
		lcac_tun_cfg->REG28.bits.LCAC_LTI_WGT_MIN_B = lcac_attr->DePurpleStrMinBase;	//8
		lcac_tun_cfg->REG2C.bits.LCAC_LTI_VAR_WGT_R = lcac_attr->EdgeStrWgtBase;	//5
		lcac_tun_cfg->REG2C.bits.LCAC_LTI_VAR_WGT_B = lcac_attr->EdgeStrWgtBase;	//5
		lcac_tun_cfg->REG2C.bits.LCAC_FILTER_SCALE = lcac_attr->FilterScaleAdv;	//4
		lcac_tun_cfg->REG2C.bits.LCAC_FCF_LUMA_BLEND_WGT = lcac_attr->LumaWgt;	//7
		lcac_tun_cfg->REG30.bits.LCAC_FCF_EDGE_SCALE_R_LE = lcac_attr->EdgeGainAdv0;	//6
		lcac_tun_cfg->REG30.bits.LCAC_FCF_EDGE_SCALE_G_LE = lcac_attr->EdgeGainAdv0;	//6
		lcac_tun_cfg->REG30.bits.LCAC_FCF_EDGE_SCALE_B_LE = lcac_attr->EdgeGainAdv0;	//6
		lcac_tun_cfg->REG30.bits.LCAC_FCF_EDGE_SCALE_Y_LE = lcac_attr->EdgeGainAdv0;	//6
		lcac_tun_cfg->REG34.bits.LCAC_FCF_EDGE_SCALE_R_SE = lcac_attr->EdgeGainAdv1;	//6
		lcac_tun_cfg->REG34.bits.LCAC_FCF_EDGE_SCALE_G_SE = lcac_attr->EdgeGainAdv1;	//6
		lcac_tun_cfg->REG34.bits.LCAC_FCF_EDGE_SCALE_B_SE = lcac_attr->EdgeGainAdv1;	//6
		lcac_tun_cfg->REG34.bits.LCAC_FCF_EDGE_SCALE_Y_SE = lcac_attr->EdgeGainAdv1;	//6
		lcac_tun_cfg->REG38.bits.LCAC_FCF_EDGE_CORING_R = runtime->lcac_attr.EdgeCoringAdv;	//8
		lcac_tun_cfg->REG38.bits.LCAC_FCF_EDGE_CORING_G = runtime->lcac_attr.EdgeCoringAdv;	//8
		lcac_tun_cfg->REG38.bits.LCAC_FCF_EDGE_CORING_B = runtime->lcac_attr.EdgeCoringAdv;	//8
		lcac_tun_cfg->REG38.bits.LCAC_FCF_EDGE_CORING_Y = runtime->lcac_attr.EdgeCoringAdv;	//8
		lcac_tun_cfg->REG3C.bits.LCAC_FCF_WGT_MAX_R = lcac_attr->DePurpleStrMaxAdv;	//8
		lcac_tun_cfg->REG3C.bits.LCAC_FCF_WGT_MIN_R = lcac_attr->DePurpleStrMinAdv;	//8
		lcac_tun_cfg->REG3C.bits.LCAC_FCF_WGT_MAX_B = lcac_attr->DePurpleStrMaxAdv;	//8
		lcac_tun_cfg->REG3C.bits.LCAC_FCF_WGT_MIN_B = lcac_attr->DePurpleStrMinAdv;	//8
		lcac_tun_cfg->REG40.bits.LCAC_FCF_VAR_WGT_R = (16 - lcac_attr->EdgeStrWgtAdvG) / 2;	//5
		lcac_tun_cfg->REG40.bits.LCAC_FCF_VAR_WGT_G = lcac_attr->EdgeStrWgtAdvG;	//5
		lcac_tun_cfg->REG40.bits.LCAC_FCF_VAR_WGT_B = (16 - lcac_attr->EdgeStrWgtAdvG) / 2;	//5
		lcac_tun_cfg->REG44.bits.LCAC_FCF_FILTER_KERNEL_00 = plcac_param->au8FCFFilterKernel[0];	//7
		lcac_tun_cfg->REG44.bits.LCAC_FCF_FILTER_KERNEL_01 = plcac_param->au8FCFFilterKernel[1];	//7
		lcac_tun_cfg->REG44.bits.LCAC_FCF_FILTER_KERNEL_02 = plcac_param->au8FCFFilterKernel[2];	//7
		lcac_tun_cfg->REG44.bits.LCAC_FCF_FILTER_KERNEL_03 = plcac_param->au8FCFFilterKernel[3];	//7
		lcac_tun_cfg->REG48.bits.LCAC_LUMA_KERNEL_00 = 3;	//7
		lcac_tun_cfg->REG48.bits.LCAC_LUMA_KERNEL_01 = 5;	//7
		lcac_tun_cfg->REG48.bits.LCAC_LUMA_KERNEL_02 = 8;	//7
		lcac_tun_cfg->REG48.bits.LCAC_LUMA_KERNEL_03 = 10;	//7
		lcac_tun_cfg->REG4C.bits.LCAC_FCF_FILTER_KERNEL_04 = plcac_param->au8FCFFilterKernel[4];	//7
		lcac_tun_cfg->REG4C.bits.LCAC_LUMA_KERNEL_04 = 14;	//7

		struct cvi_isp_lcac_2_tun_cfg *lcac_2_tun_cfg = &(lcac_cfg->lcac_2_cfg);

		pu8Lut = plcac_param->au8LTILumaLut;
		lcac_2_tun_cfg->REG50.bits.LCAC_LTI_LUMA_LUT_00 = pu8Lut[0];	//8
		lcac_2_tun_cfg->REG50.bits.LCAC_LTI_LUMA_LUT_01 = pu8Lut[1];	//8
		lcac_2_tun_cfg->REG50.bits.LCAC_LTI_LUMA_LUT_02 = pu8Lut[2];	//8
		lcac_2_tun_cfg->REG50.bits.LCAC_LTI_LUMA_LUT_03 = pu8Lut[3];	//8
		lcac_2_tun_cfg->REG54.bits.LCAC_LTI_LUMA_LUT_04 = pu8Lut[4];	//8
		lcac_2_tun_cfg->REG54.bits.LCAC_LTI_LUMA_LUT_05 = pu8Lut[5];	//8
		lcac_2_tun_cfg->REG54.bits.LCAC_LTI_LUMA_LUT_06 = pu8Lut[6];	//8
		lcac_2_tun_cfg->REG54.bits.LCAC_LTI_LUMA_LUT_07 = pu8Lut[7];	//8
		lcac_2_tun_cfg->REG58.bits.LCAC_LTI_LUMA_LUT_08 = pu8Lut[8];	//8
		lcac_2_tun_cfg->REG58.bits.LCAC_LTI_LUMA_LUT_09 = pu8Lut[9];	//8
		lcac_2_tun_cfg->REG58.bits.LCAC_LTI_LUMA_LUT_10 = pu8Lut[10];	//8
		lcac_2_tun_cfg->REG58.bits.LCAC_LTI_LUMA_LUT_11 = pu8Lut[11];	//8
		lcac_2_tun_cfg->REG5C.bits.LCAC_LTI_LUMA_LUT_12 = pu8Lut[12];	//8
		lcac_2_tun_cfg->REG5C.bits.LCAC_LTI_LUMA_LUT_13 = pu8Lut[13];	//8
		lcac_2_tun_cfg->REG5C.bits.LCAC_LTI_LUMA_LUT_14 = pu8Lut[14];	//8
		lcac_2_tun_cfg->REG5C.bits.LCAC_LTI_LUMA_LUT_15 = pu8Lut[15];	//8
		lcac_2_tun_cfg->REG60.bits.LCAC_LTI_LUMA_LUT_16 = pu8Lut[16];	//8
		lcac_2_tun_cfg->REG60.bits.LCAC_LTI_LUMA_LUT_17 = pu8Lut[17];	//8
		lcac_2_tun_cfg->REG60.bits.LCAC_LTI_LUMA_LUT_18 = pu8Lut[18];	//8
		lcac_2_tun_cfg->REG60.bits.LCAC_LTI_LUMA_LUT_19 = pu8Lut[19];	//8
		lcac_2_tun_cfg->REG64.bits.LCAC_LTI_LUMA_LUT_20 = pu8Lut[20];	//8
		lcac_2_tun_cfg->REG64.bits.LCAC_LTI_LUMA_LUT_21 = pu8Lut[21];	//8
		lcac_2_tun_cfg->REG64.bits.LCAC_LTI_LUMA_LUT_22 = pu8Lut[22];	//8
		lcac_2_tun_cfg->REG64.bits.LCAC_LTI_LUMA_LUT_23 = pu8Lut[23];	//8
		lcac_2_tun_cfg->REG68.bits.LCAC_LTI_LUMA_LUT_24 = pu8Lut[24];	//8
		lcac_2_tun_cfg->REG68.bits.LCAC_LTI_LUMA_LUT_25 = pu8Lut[25];	//8
		lcac_2_tun_cfg->REG68.bits.LCAC_LTI_LUMA_LUT_26 = pu8Lut[26];	//8
		lcac_2_tun_cfg->REG68.bits.LCAC_LTI_LUMA_LUT_27 = pu8Lut[27];	//8
		lcac_2_tun_cfg->REG6C.bits.LCAC_LTI_LUMA_LUT_28 = pu8Lut[28];	//8
		lcac_2_tun_cfg->REG6C.bits.LCAC_LTI_LUMA_LUT_29 = pu8Lut[29];	//8
		lcac_2_tun_cfg->REG6C.bits.LCAC_LTI_LUMA_LUT_30 = pu8Lut[30];	//8
		lcac_2_tun_cfg->REG6C.bits.LCAC_LTI_LUMA_LUT_31 = pu8Lut[31];	//8

		struct cvi_isp_lcac_3_tun_cfg *lcac_3_tun_cfg = &(lcac_cfg->lcac_3_cfg);

		pu8Lut = plcac_param->au8FCFLumaLut;
		lcac_3_tun_cfg->REG70.bits.LCAC_FCF_LUMA_LUT_00 = pu8Lut[0];	//8
		lcac_3_tun_cfg->REG70.bits.LCAC_FCF_LUMA_LUT_01 = pu8Lut[1];	//8
		lcac_3_tun_cfg->REG70.bits.LCAC_FCF_LUMA_LUT_02 = pu8Lut[2];	//8
		lcac_3_tun_cfg->REG70.bits.LCAC_FCF_LUMA_LUT_03 = pu8Lut[3];	//8
		lcac_3_tun_cfg->REG74.bits.LCAC_FCF_LUMA_LUT_04 = pu8Lut[4];	//8
		lcac_3_tun_cfg->REG74.bits.LCAC_FCF_LUMA_LUT_05 = pu8Lut[5];	//8
		lcac_3_tun_cfg->REG74.bits.LCAC_FCF_LUMA_LUT_06 = pu8Lut[6];	//8
		lcac_3_tun_cfg->REG74.bits.LCAC_FCF_LUMA_LUT_07 = pu8Lut[7];	//8
		lcac_3_tun_cfg->REG78.bits.LCAC_FCF_LUMA_LUT_08 = pu8Lut[8];	//8
		lcac_3_tun_cfg->REG78.bits.LCAC_FCF_LUMA_LUT_09 = pu8Lut[9];	//8
		lcac_3_tun_cfg->REG78.bits.LCAC_FCF_LUMA_LUT_10 = pu8Lut[10];	//8
		lcac_3_tun_cfg->REG78.bits.LCAC_FCF_LUMA_LUT_11 = pu8Lut[11];	//8
		lcac_3_tun_cfg->REG7C.bits.LCAC_FCF_LUMA_LUT_12 = pu8Lut[12];	//8
		lcac_3_tun_cfg->REG7C.bits.LCAC_FCF_LUMA_LUT_13 = pu8Lut[13];	//8
		lcac_3_tun_cfg->REG7C.bits.LCAC_FCF_LUMA_LUT_14 = pu8Lut[14];	//8
		lcac_3_tun_cfg->REG7C.bits.LCAC_FCF_LUMA_LUT_15 = pu8Lut[15];	//8
		lcac_3_tun_cfg->REG80.bits.LCAC_FCF_LUMA_LUT_16 = pu8Lut[16];	//8
		lcac_3_tun_cfg->REG80.bits.LCAC_FCF_LUMA_LUT_17 = pu8Lut[17];	//8
		lcac_3_tun_cfg->REG80.bits.LCAC_FCF_LUMA_LUT_18 = pu8Lut[18];	//8
		lcac_3_tun_cfg->REG80.bits.LCAC_FCF_LUMA_LUT_19 = pu8Lut[19];	//8
		lcac_3_tun_cfg->REG84.bits.LCAC_FCF_LUMA_LUT_20 = pu8Lut[20];	//8
		lcac_3_tun_cfg->REG84.bits.LCAC_FCF_LUMA_LUT_21 = pu8Lut[21];	//8
		lcac_3_tun_cfg->REG84.bits.LCAC_FCF_LUMA_LUT_22 = pu8Lut[22];	//8
		lcac_3_tun_cfg->REG84.bits.LCAC_FCF_LUMA_LUT_23 = pu8Lut[23];	//8
		lcac_3_tun_cfg->REG88.bits.LCAC_FCF_LUMA_LUT_24 = pu8Lut[24];	//8
		lcac_3_tun_cfg->REG88.bits.LCAC_FCF_LUMA_LUT_25 = pu8Lut[25];	//8
		lcac_3_tun_cfg->REG88.bits.LCAC_FCF_LUMA_LUT_26 = pu8Lut[26];	//8
		lcac_3_tun_cfg->REG88.bits.LCAC_FCF_LUMA_LUT_27 = pu8Lut[27];	//8
		lcac_3_tun_cfg->REG8C.bits.LCAC_FCF_LUMA_LUT_28 = pu8Lut[28];	//8
		lcac_3_tun_cfg->REG8C.bits.LCAC_FCF_LUMA_LUT_29 = pu8Lut[29];	//8
		lcac_3_tun_cfg->REG8C.bits.LCAC_FCF_LUMA_LUT_30 = pu8Lut[30];	//8
		lcac_3_tun_cfg->REG8C.bits.LCAC_FCF_LUMA_LUT_31 = pu8Lut[31];	//8
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_lcac_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		const ISP_LCAC_ATTR_S * lcac_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_lcac_ctrl_get_lcac_attr(ViPipe, &lcac_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->LCACEnable = lcac_attr->Enable;
		//manual or auto
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_lcac_ctrl_runtime *_get_lcac_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isViPipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isViPipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_lcac_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LCAC, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_lcac_ctrl_check_lcac_attr_valid(const ISP_LCAC_ATTR_S *pstLCACAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstLCACAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstLCACAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstLCACAttr, UpdateInterval, 1, 0xFF);
	CHECK_VALID_CONST(pstLCACAttr, TuningMode, 0, 0x6);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleCrStr0, 0, 0x40);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleCbStr0, 0, 0x40);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleCrStr1, 0, 0x40);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleCbStr1, 0, 0x40);
	CHECK_VALID_CONST(pstLCACAttr, FilterTypeBase, 0, 0x3);
	CHECK_VALID_CONST(pstLCACAttr, EdgeGainBase0, 0, 0x1C);
	CHECK_VALID_CONST(pstLCACAttr, EdgeGainBase1, 0, 0x23);
	CHECK_VALID_CONST(pstLCACAttr, EdgeStrWgtBase, 0, 0x10);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleStrMaxBase, 0, 0x80);
	CHECK_VALID_CONST(pstLCACAttr, DePurpleStrMinBase, 0, 0x80);
	CHECK_VALID_CONST(pstLCACAttr, FilterScaleAdv, 0, 0xF);
	CHECK_VALID_CONST(pstLCACAttr, LumaWgt, 0, 0x40);
	CHECK_VALID_CONST(pstLCACAttr, FilterTypeAdv, 0, 0x5);
	CHECK_VALID_CONST(pstLCACAttr, EdgeGainAdv0, 0, 0x1C);
	CHECK_VALID_CONST(pstLCACAttr, EdgeGainAdv1, 0, 0x23);
	CHECK_VALID_CONST(pstLCACAttr, EdgeStrWgtAdvG, 0, 0x10);
	// CHECK_VALID_CONST(pstLCACAttr, DePurpleStrMaxAdv, 0, 0xFF);
	// CHECK_VALID_CONST(pstLCACAttr, DePurpleStrMinAdv, 0, 0xFF);
	CHECK_VALID_CONST(pstLCACAttr, EdgeWgtBase.Wgt, 0, 0x80);
	CHECK_VALID_CONST(pstLCACAttr, EdgeWgtBase.Sigma, 0x1, 0xFF);
	CHECK_VALID_CONST(pstLCACAttr, EdgeWgtAdv.Wgt, 0, 0x80);
	CHECK_VALID_CONST(pstLCACAttr, EdgeWgtAdv.Sigma, 0x1, 0xFF);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurpleCrGain, 0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurpleCbGain, 0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurepleCrWgt0, 0, 0x40);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurepleCbWgt0, 0, 0x40);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurepleCrWgt1, 0, 0x40);
	CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, DePurepleCbWgt1, 0, 0x40);
	// CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, EdgeCoringBase, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLCACAttr, EdgeCoringAdv, 0, 0xFF);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_lcac_ctrl_get_lcac_attr(VI_PIPE ViPipe, const ISP_LCAC_ATTR_S **pstLCACAttr)
{
	ISP_LOG_DEBUG("+\n");
	if (pstLCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LCAC, (CVI_VOID *) &shared_buffer);
	*pstLCACAttr = &shared_buffer->stLCACAttr;

	return ret;
}

CVI_S32 isp_lcac_ctrl_set_lcac_attr(VI_PIPE ViPipe, const ISP_LCAC_ATTR_S *pstLCACAttr)
{
	ISP_LOG_DEBUG("+\n");
	if (pstLCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lcac_ctrl_runtime *runtime = _get_lcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_lcac_ctrl_check_lcac_attr_valid(pstLCACAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_LCAC_ATTR_S *p = CVI_NULL;

	isp_lcac_ctrl_get_lcac_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstLCACAttr, sizeof(ISP_LCAC_ATTR_S));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

