/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_rgbcac_ctrl.c
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

#include "isp_rgbcac_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl rgbcac_mod = {
	.init = isp_rgbcac_ctrl_init,
	.uninit = isp_rgbcac_ctrl_uninit,
	.suspend = isp_rgbcac_ctrl_suspend,
	.resume = isp_rgbcac_ctrl_resume,
	.ctrl = isp_rgbcac_ctrl_ctrl
};

static CVI_S32 isp_rgbcac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_rgbcac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_rgbcac_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_rgbcac_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_rgbcac_ctrl_check_rgbcac_attr_valid(const ISP_RGBCAC_ATTR_S *pstRGBCACAttr);

static CVI_S32 set_rgbcac_proc_info(VI_PIPE ViPipe);

static struct isp_rgbcac_ctrl_runtime *_get_rgbcac_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_rgbcac_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_rgbcac_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbcac_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbcac_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_rgbcac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_rgbcac_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassRgbcac;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassRgbcac = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_rgbcac_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_rgbcac_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_rgbcac_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_rgbcac_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_rgbcac_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_rgbcac_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_RGBCAC_ATTR_S *rgbcac_attr = NULL;

	isp_rgbcac_ctrl_get_rgbcac_attr(ViPipe, &rgbcac_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(rgbcac_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!rgbcac_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (rgbcac_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(rgbcac_attr, DePurpleStr0);
		MANUAL(rgbcac_attr, DePurpleStr1);
		MANUAL(rgbcac_attr, EdgeCoring);
		MANUAL(rgbcac_attr, DePurpleCrStr0);
		MANUAL(rgbcac_attr, DePurpleCbStr0);
		MANUAL(rgbcac_attr, DePurpleCrStr1);
		MANUAL(rgbcac_attr, DePurpleCbStr1);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(rgbcac_attr, DePurpleStr0, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, DePurpleStr1, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, EdgeCoring, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, DePurpleCrStr0, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, DePurpleCbStr0, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, DePurpleCrStr1, INTPLT_POST_ISO);
		AUTO(rgbcac_attr, DePurpleCbStr1, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	struct rgbcac_param_in *ptParamIn = &(runtime->rgbcac_param_in);

	ptParamIn->u8DePurpleStrMax0 = rgbcac_attr->DePurpleStrMax0;
	ptParamIn->u8DePurpleStrMin0 = rgbcac_attr->DePurpleStrMin0;
	ptParamIn->u8DePurpleStrMax1 = rgbcac_attr->DePurpleStrMax1;
	ptParamIn->u8DePurpleStrMin1 = rgbcac_attr->DePurpleStrMin1;
	memcpy(ptParamIn->au8EdgeGainIn, rgbcac_attr->EdgeGainIn, 3 * sizeof(CVI_U8));
	memcpy(ptParamIn->au8EdgeGainOut, rgbcac_attr->EdgeGainOut, 3 * sizeof(CVI_U8));

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_rgbcac_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_rgbcac_main(
		(struct rgbcac_param_in *)&runtime->rgbcac_param_in,
		(struct rgbcac_param_out *)&runtime->rgbcac_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_rgbcac_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_rgbcac_config *rgbcac_cfg =
		(struct cvi_vip_isp_rgbcac_config *)&(post_addr->tun_cfg[tun_idx].rgbcac_cfg);

	const ISP_RGBCAC_ATTR_S *rgbcac_attr = NULL;

	isp_rgbcac_ctrl_get_rgbcac_attr(ViPipe, &rgbcac_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		rgbcac_cfg->update = 0;
	} else {
		rgbcac_cfg->update = 1;
		//rgbcac_cfg->enable = rgbcac_attr->Enable && !runtime->is_module_bypass;
		rgbcac_cfg->enable = 1;
		rgbcac_cfg->out_sel = rgbcac_attr->TuningMode;

		struct cvi_isp_rgbcac_tun_cfg *rgbcac_tun_cfg = &(rgbcac_cfg->rgbcac_cfg);
		struct rgbcac_param_out *prgbcac_param = &(runtime->rgbcac_param_out);
		CVI_U8 *pu8EdgeScaleLut = prgbcac_param->au8EdgeScaleLut;

		rgbcac_tun_cfg->RGBCAC_PURPLE_TH.bits.RGBCAC_PURPLE_TH_LE = rgbcac_attr->PurpleDetRange0;
		rgbcac_tun_cfg->RGBCAC_PURPLE_TH.bits.RGBCAC_PURPLE_TH_SE = rgbcac_attr->PurpleDetRange1;
		rgbcac_tun_cfg->RGBCAC_PURPLE_TH.bits.RGBCAC_CORRECT_STRENGTH_LE = runtime->rgbcac_attr.DePurpleStr0;
		rgbcac_tun_cfg->RGBCAC_PURPLE_TH.bits.RGBCAC_CORRECT_STRENGTH_SE = runtime->rgbcac_attr.DePurpleStr1;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR.bits.RGBCAC_PURPLE_CB = rgbcac_attr->PurpleCb;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR.bits.RGBCAC_PURPLE_CR = rgbcac_attr->PurpleCr;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR2.bits.RGBCAC_PURPLE_CB2 = rgbcac_attr->PurpleCb2;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR2.bits.RGBCAC_PURPLE_CR2 = rgbcac_attr->PurpleCr2;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR3.bits.RGBCAC_PURPLE_CB3 = rgbcac_attr->PurpleCb3;
		rgbcac_tun_cfg->RGBCAC_PURPLE_CBCR3.bits.RGBCAC_PURPLE_CR3 = rgbcac_attr->PurpleCr3;
		rgbcac_tun_cfg->RGBCAC_GREEN_CBCR.bits.RGBCAC_GREEN_CB = rgbcac_attr->GreenCb;
		rgbcac_tun_cfg->RGBCAC_GREEN_CBCR.bits.RGBCAC_GREEN_CR = rgbcac_attr->GreenCr;
		rgbcac_tun_cfg->RGBCAC_EDGE_CORING.bits.RGBCAC_EDGE_CORING = runtime->rgbcac_attr.EdgeCoring;
		rgbcac_tun_cfg->RGBCAC_EDGE_CORING.bits.RGBCAC_EDGE_SCALE = rgbcac_attr->EdgeGlobalGain;
		rgbcac_tun_cfg->RGBCAC_DEPURPLE_STR_RATIO_MIN.bits.RGBCAC_DEPURPLE_STR_RATIO_MIN_LE
			= prgbcac_param->u16DePurpleStrRatioMin0;
		rgbcac_tun_cfg->RGBCAC_DEPURPLE_STR_RATIO_MIN.bits.RGBCAC_DEPURPLE_STR_RATIO_MIN_SE
			= prgbcac_param->u16DePurpleStrRatioMin1;
		rgbcac_tun_cfg->RGBCAC_DEPURPLE_STR_RATIO_MAX.bits.RGBCAC_DEPURPLE_STR_RATIO_MAX_LE
			= prgbcac_param->u16DePurpleStrRatioMax0;
		rgbcac_tun_cfg->RGBCAC_DEPURPLE_STR_RATIO_MAX.bits.RGBCAC_DEPURPLE_STR_RATIO_MAX_SE
			= prgbcac_param->u16DePurpleStrRatioMax1;
		if (rgbcac_attr->Enable && !runtime->is_module_bypass) {
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_00 = pu8EdgeScaleLut[0];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_01 = pu8EdgeScaleLut[1];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_02 = pu8EdgeScaleLut[2];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_03 = pu8EdgeScaleLut[3];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_04 = pu8EdgeScaleLut[4];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_05 = pu8EdgeScaleLut[5];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_06 = pu8EdgeScaleLut[6];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_07 = pu8EdgeScaleLut[7];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_08 = pu8EdgeScaleLut[8];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_09 = pu8EdgeScaleLut[9];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_10 = pu8EdgeScaleLut[10];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_11 = pu8EdgeScaleLut[11];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_12 = pu8EdgeScaleLut[12];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_13 = pu8EdgeScaleLut[13];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_14 = pu8EdgeScaleLut[14];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_15 = pu8EdgeScaleLut[15];
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT4.bits.RGBCAC_EDGE_WGT_LUT_16 = pu8EdgeScaleLut[16];
		} else {
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_00 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_01 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_02 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT0.bits.RGBCAC_EDGE_WGT_LUT_03 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_04 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_05 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_06 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT1.bits.RGBCAC_EDGE_WGT_LUT_07 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_08 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_09 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_10 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT2.bits.RGBCAC_EDGE_WGT_LUT_11 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_12 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_13 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_14 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT3.bits.RGBCAC_EDGE_WGT_LUT_15 = 0;
			rgbcac_tun_cfg->RGBCAC_EDGE_WGT_LUT4.bits.RGBCAC_EDGE_WGT_LUT_16 = 0;
		}
		rgbcac_tun_cfg->RGBCAC_LUMA.bits.RGBCAC_LUMA_SCALE = rgbcac_attr->LumaScale;
		rgbcac_tun_cfg->RGBCAC_LUMA.bits.RGBCAC_LUMA2 = rgbcac_attr->UserDefineLuma;
		rgbcac_tun_cfg->RGBCAC_LUMA_BLEND.bits.RGBCAC_LUMA_BLEND_WGT = rgbcac_attr->LumaBlendWgt;
		rgbcac_tun_cfg->RGBCAC_LUMA_BLEND.bits.RGBCAC_LUMA_BLEND_WGT2 = rgbcac_attr->LumaBlendWgt2;
		rgbcac_tun_cfg->RGBCAC_LUMA_BLEND.bits.RGBCAC_LUMA_BLEND_WGT3 = rgbcac_attr->LumaBlendWgt3;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER0.bits.RGBCAC_LUMA_FILTER_00 = 5;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER0.bits.RGBCAC_LUMA_FILTER_01 = 8;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER0.bits.RGBCAC_LUMA_FILTER_02 = 5;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER1.bits.RGBCAC_LUMA_FILTER_03 = 12;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER1.bits.RGBCAC_LUMA_FILTER_04 = 8;
		rgbcac_tun_cfg->RGBCAC_LUMA_FILTER1.bits.RGBCAC_LUMA_FILTER_05 = 5;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER0.bits.RGBCAC_VAR_FILTER_00 = 5;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER0.bits.RGBCAC_VAR_FILTER_01 = 8;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER0.bits.RGBCAC_VAR_FILTER_02 = 5;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER1.bits.RGBCAC_VAR_FILTER_03 = 12;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER1.bits.RGBCAC_VAR_FILTER_04 = 8;
		rgbcac_tun_cfg->RGBCAC_VAR_FILTER1.bits.RGBCAC_VAR_FILTER_05 = 5;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER0.bits.RGBCAC_CHROMA_FILTER_00 = 5;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER0.bits.RGBCAC_CHROMA_FILTER_01 = 8;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER0.bits.RGBCAC_CHROMA_FILTER_02 = 5;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER1.bits.RGBCAC_CHROMA_FILTER_03 = 12;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER1.bits.RGBCAC_CHROMA_FILTER_04 = 8;
		rgbcac_tun_cfg->RGBCAC_CHROMA_FILTER1.bits.RGBCAC_CHROMA_FILTER_05 = 5;
		rgbcac_tun_cfg->RGBCAC_CBCR_STR.bits.RGBCAC_CB_STR_LE = runtime->rgbcac_attr.DePurpleCbStr0;
		rgbcac_tun_cfg->RGBCAC_CBCR_STR.bits.RGBCAC_CR_STR_LE = runtime->rgbcac_attr.DePurpleCrStr0;
		rgbcac_tun_cfg->RGBCAC_CBCR_STR.bits.RGBCAC_CB_STR_SE = runtime->rgbcac_attr.DePurpleCbStr1;
		rgbcac_tun_cfg->RGBCAC_CBCR_STR.bits.RGBCAC_CR_STR_SE = runtime->rgbcac_attr.DePurpleCrStr1;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_rgbcac_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		const ISP_RGBCAC_ATTR_S * rgbcac_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_rgbcac_ctrl_get_rgbcac_attr(ViPipe, &rgbcac_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->RGBCACEnable = rgbcac_attr->Enable;
		//manual or auto
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_rgbcac_ctrl_runtime *_get_rgbcac_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_rgbcac_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_RGBCAC, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_rgbcac_ctrl_check_rgbcac_attr_valid(const ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstRGBCACAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstRGBCACAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstRGBCACAttr, UpdateInterval, 1, 0xFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleDetRange0, 0x0, 0x80);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleDetRange1, 0x0, 0x80);
	// CHECK_VALID_CONST(pstRGBCACAttr, DePurpleStrMax0, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstRGBCACAttr, DePurpleStrMin0, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstRGBCACAttr, DePurpleStrMax1, 0x0, 0xFF);
	// CHECK_VALID_CONST(pstRGBCACAttr, DePurpleStrMin1, 0x0, 0xFF);
	CHECK_VALID_CONST(pstRGBCACAttr, EdgeGlobalGain, 0x0, 0xFFF);
	CHECK_VALID_ARRAY_1D(pstRGBCACAttr, EdgeGainIn, 3, 0x0, 0xFFF);
	CHECK_VALID_ARRAY_1D(pstRGBCACAttr, EdgeGainOut, 3, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, LumaScale, 0x0, 0x7FF);
	CHECK_VALID_CONST(pstRGBCACAttr, UserDefineLuma, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, LumaBlendWgt, 0x0, 0x20);
	CHECK_VALID_CONST(pstRGBCACAttr, LumaBlendWgt2, 0x0, 0x20);
	CHECK_VALID_CONST(pstRGBCACAttr, LumaBlendWgt3, 0x0, 0x20);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCb, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCr, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCb2, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCr2, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCb3, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, PurpleCr3, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, GreenCb, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, GreenCr, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstRGBCACAttr, TuningMode, 0x0, 0x2);
	// CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleStr0, 0x0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleStr1, 0x0, 0xFF);
	CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, EdgeCoring, 0x0, 0xFFF);
	CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleCrStr0, 0x0, 0x10);
	CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleCbStr0, 0x0, 0x10);
	CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleCrStr1, 0x0, 0x10);
	CHECK_VALID_AUTO_ISO_1D(pstRGBCACAttr, DePurpleCbStr1, 0x0, 0x10);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_rgbcac_ctrl_get_rgbcac_attr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S **pstRGBCACAttr)
{
	if (pstRGBCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_RGBCAC, (CVI_VOID *) &shared_buffer);
	*pstRGBCACAttr = &shared_buffer->stRGBCACAttr;

	return ret;
}

CVI_S32 isp_rgbcac_ctrl_set_rgbcac_attr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S *pstRGBCACAttr)
{
	if (pstRGBCACAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_rgbcac_ctrl_runtime *runtime = _get_rgbcac_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_rgbcac_ctrl_check_rgbcac_attr_valid(pstRGBCACAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_RGBCAC_ATTR_S *p = CVI_NULL;

	isp_rgbcac_ctrl_get_rgbcac_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstRGBCACAttr, sizeof(*pstRGBCACAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

