/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ldci_ctrl.c
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

#include "isp_ldci_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl ldci_mod = {
	.init = isp_ldci_ctrl_init,
	.uninit = isp_ldci_ctrl_uninit,
	.suspend = isp_ldci_ctrl_suspend,
	.resume = isp_ldci_ctrl_resume,
	.ctrl = isp_ldci_ctrl_ctrl
};


static CVI_S32 isp_ldci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ldci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ldci_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ldci_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_ldci_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_ldci_ctrl_check_ldci_attr_valid(const ISP_LDCI_ATTR_S *pstLDCIAttr);

static struct isp_ldci_ctrl_runtime *_get_ldci_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ldci_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	ISP_CTX_S * pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_U32 u32AlgoMemorySize;

	isp_algo_ldci_get_internal_memory(&u32AlgoMemorySize);

	// printf("LDCI algo. internal memory : %d\n", u32AlgoMemorySize);

	if (runtime->pvAlgoMemory != CVI_NULL) {
		free(ISP_PTR_CAST_VOID(runtime->pvAlgoMemory));
		runtime->pvAlgoMemory = CVI_NULL;
	}

	runtime->pvAlgoMemory = ISP_PTR_CAST_PTR(malloc(u32AlgoMemorySize));
	if (runtime->pvAlgoMemory == CVI_NULL) {
		ISP_LOG_WARNING("Allocate memory for LDCI algo. (%d) fail\n", ViPipe);
		return CVI_FAILURE;
	}

	isp_algo_ldci_get_init_coef(pstIspCtx->stSysRect.u32Width, pstIspCtx->stSysRect.u32Height,
		&(runtime->ldci_coef));

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	runtime->ldci_param_in.pvIntMemory = runtime->pvAlgoMemory;

	isp_algo_ldci_init();

	return ret;
}

CVI_S32 isp_ldci_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	isp_algo_ldci_uninit();

	if (runtime->pvAlgoMemory != CVI_NULL) {
		free(ISP_PTR_CAST_VOID(runtime->pvAlgoMemory));
		runtime->pvAlgoMemory = CVI_NULL;
	}

	return ret;
}

CVI_S32 isp_ldci_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ldci_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ldci_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ldci_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassLdci;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassLdci = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ldci_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ldci_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ldci_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ldci_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ldci_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ldci_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);
	const ISP_LDCI_ATTR_S *ldci_attr = NULL;
	CVI_S32 ret = CVI_SUCCESS;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_ldci_ctrl_get_ldci_attr(ViPipe, &ldci_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ldci_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ldci_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (ldci_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(ldci_attr, LdciStrength);
		MANUAL(ldci_attr, LdciRange);
		MANUAL(ldci_attr, TprCoef);
		MANUAL(ldci_attr, EdgeCoring);
		MANUAL(ldci_attr, LumaWgtMax);
		MANUAL(ldci_attr, LumaWgtMin);
		MANUAL(ldci_attr, VarMapMax);
		MANUAL(ldci_attr, VarMapMin);
		MANUAL(ldci_attr, UvGainMax);
		MANUAL(ldci_attr, UvGainMin);
		MANUAL(ldci_attr, BrightContrastHigh);
		MANUAL(ldci_attr, BrightContrastLow);
		MANUAL(ldci_attr, DarkContrastHigh);
		MANUAL(ldci_attr, DarkContrastLow);
		MANUAL(ldci_attr, LumaPosWgt.Wgt);
		MANUAL(ldci_attr, LumaPosWgt.Sigma);
		MANUAL(ldci_attr, LumaPosWgt.Mean);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(ldci_attr, LdciStrength, INTPLT_POST_ISO);
		AUTO(ldci_attr, LdciRange, INTPLT_POST_ISO);
		AUTO(ldci_attr, TprCoef, INTPLT_POST_ISO);
		AUTO(ldci_attr, EdgeCoring, INTPLT_POST_ISO);
		AUTO(ldci_attr, LumaWgtMax, INTPLT_POST_ISO);
		AUTO(ldci_attr, LumaWgtMin, INTPLT_POST_ISO);
		AUTO(ldci_attr, VarMapMax, INTPLT_POST_ISO);
		AUTO(ldci_attr, VarMapMin, INTPLT_POST_ISO);
		AUTO(ldci_attr, UvGainMax, INTPLT_POST_ISO);
		AUTO(ldci_attr, UvGainMin, INTPLT_POST_ISO);
		AUTO(ldci_attr, BrightContrastHigh, INTPLT_POST_ISO);
		AUTO(ldci_attr, BrightContrastLow, INTPLT_POST_ISO);
		AUTO(ldci_attr, DarkContrastHigh, INTPLT_POST_ISO);
		AUTO(ldci_attr, DarkContrastLow, INTPLT_POST_ISO);

		CVI_U8 au8PosWgt[ISP_AUTO_ISO_STRENGTH_NUM];
		CVI_U8 au8PosSigma[ISP_AUTO_ISO_STRENGTH_NUM];
		CVI_U8 au8PosMean[ISP_AUTO_ISO_STRENGTH_NUM];

		for (CVI_U32 u32IsoIdx = 0; u32IsoIdx < ISP_AUTO_ISO_STRENGTH_NUM; ++u32IsoIdx) {
			au8PosWgt[u32IsoIdx] = ldci_attr->stAuto.LumaPosWgt[u32IsoIdx].Wgt;
			au8PosSigma[u32IsoIdx] = ldci_attr->stAuto.LumaPosWgt[u32IsoIdx].Sigma;
			au8PosMean[u32IsoIdx] = ldci_attr->stAuto.LumaPosWgt[u32IsoIdx].Mean;
		}
		runtime->ldci_attr.LumaPosWgt.Wgt = INTERPOLATE_LINEAR(ViPipe, INTPLT_POST_ISO, au8PosWgt);
		runtime->ldci_attr.LumaPosWgt.Sigma = INTERPOLATE_LINEAR(ViPipe, INTPLT_POST_ISO, au8PosSigma);
		runtime->ldci_attr.LumaPosWgt.Mean = INTERPOLATE_LINEAR(ViPipe, INTPLT_POST_ISO, au8PosMean);

		#undef AUTO
	}

	// ParamIn
	struct ldci_param_in *ptParamIn = &(runtime->ldci_param_in);

	ptParamIn->u8GaussLPFSigma = ldci_attr->GaussLPFSigma;
	ptParamIn->u8BrightContrastHigh = runtime->ldci_attr.BrightContrastHigh;
	ptParamIn->u8BrightContrastLow = runtime->ldci_attr.BrightContrastLow;
	ptParamIn->u8DarkContrastHigh = runtime->ldci_attr.DarkContrastHigh;
	ptParamIn->u8DarkContrastLow = runtime->ldci_attr.DarkContrastLow;
	ptParamIn->u8LumaPosWgt_Wgt = runtime->ldci_attr.LumaPosWgt.Wgt;
	ptParamIn->u8LumaPosWgt_Sigma = runtime->ldci_attr.LumaPosWgt.Sigma;
	ptParamIn->u8LumaPosWgt_Mean = runtime->ldci_attr.LumaPosWgt.Mean;
	ptParamIn->pvIntMemory = runtime->pvAlgoMemory;

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_ldci_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_ldci_main(
		(struct ldci_param_in *)&runtime->ldci_param_in,
		(struct ldci_param_out *)&runtime->ldci_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_ldci_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ldci_config *ldci_cfg =
		(struct cvi_vip_isp_ldci_config *)&(post_addr->tun_cfg[tun_idx].ldci_cfg);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		ldci_cfg->update = 0;
	} else {
		struct ldci_init_coef *pldci_coef = &(runtime->ldci_coef);
		struct ldci_param_out *pldci_param = &(runtime->ldci_param_out);
		const ISP_LDCI_ATTR_S *ldci_attr = NULL;
		CVI_U16 *pu16Lut = CVI_NULL;
		CVI_U8 *pu8Lut = CVI_NULL;

		isp_ldci_ctrl_get_ldci_attr(ViPipe, &ldci_attr);

		ldci_cfg->update = 1;
		ldci_cfg->enable = ldci_attr->Enable && !runtime->is_module_bypass;

		ldci_cfg->stats_enable = ldci_cfg->enable;
		ldci_cfg->map_enable = ldci_cfg->enable;
		ldci_cfg->uv_gain_enable = ldci_cfg->enable;
		ldci_cfg->first_frame_enable = 0;				// for RTL verify

		ldci_cfg->image_size_div_by_16x12 = pldci_coef->u8ImageSizeDivBy16x12;
		ldci_cfg->strength = runtime->ldci_attr.LdciStrength;

		struct cvi_vip_isp_ldci_tun_1_cfg *ldci_1_cfg = &(ldci_cfg->ldci_1_cfg);

		ldci_1_cfg->LDCI_LUMA_WGT_MAX.bits.LDCI_LUMA_WGT_MAX = runtime->ldci_attr.LumaWgtMax;
		ldci_1_cfg->LDCI_LUMA_WGT_MAX.bits.LDCI_LUMA_WGT_MIN = runtime->ldci_attr.LumaWgtMin;
		ldci_1_cfg->LDCI_IDX_IIR_ALPHA.bits.LDCI_IDX_IIR_ALPHA = runtime->ldci_attr.TprCoef;
		ldci_1_cfg->LDCI_IDX_IIR_ALPHA.bits.LDCI_VAR_IIR_ALPHA = runtime->ldci_attr.TprCoef;
		ldci_1_cfg->LDCI_EDGE_SCALE.bits.LDCI_EDGE_SCALE = runtime->ldci_attr.LdciRange;
		ldci_1_cfg->LDCI_EDGE_SCALE.bits.LDCI_EDGE_CORING = runtime->ldci_attr.EdgeCoring;
		ldci_1_cfg->LDCI_EDGE_CLAMP.bits.LDCI_VAR_MAP_MAX = runtime->ldci_attr.VarMapMax;
		ldci_1_cfg->LDCI_EDGE_CLAMP.bits.LDCI_VAR_MAP_MIN = runtime->ldci_attr.VarMapMin;
		ldci_1_cfg->LDCI_IDX_FILTER_NORM.bits.LDCI_IDX_FILTER_NORM = pldci_param->u16IdxFilterNorm;
		ldci_1_cfg->LDCI_IDX_FILTER_NORM.bits.LDCI_VAR_FILTER_NORM = pldci_param->u16VarFilterNorm;
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_00 = pldci_param->au8ToneCurveIdx[0];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_01 = pldci_param->au8ToneCurveIdx[1];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_02 = pldci_param->au8ToneCurveIdx[2];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_03 = pldci_param->au8ToneCurveIdx[3];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_04 = pldci_param->au8ToneCurveIdx[4];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_05 = pldci_param->au8ToneCurveIdx[5];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_06 = pldci_param->au8ToneCurveIdx[6];
		ldci_1_cfg->LDCI_TONE_CURVE_IDX_00.bits.LDCI_TONE_CURVE_IDX_07 = pldci_param->au8ToneCurveIdx[7];

		struct cvi_vip_isp_ldci_tun_2_cfg *ldci_2_cfg = &(ldci_cfg->ldci_2_cfg);

		ldci_2_cfg->LDCI_BLK_SIZE_X.bits.LDCI_BLK_SIZE_X = pldci_coef->u16BlkSizeX;
		ldci_2_cfg->LDCI_BLK_SIZE_X.bits.LDCI_BLK_SIZE_Y = pldci_coef->u16BlkSizeY;
		ldci_2_cfg->LDCI_BLK_SIZE_X1.bits.LDCI_BLK_SIZE_X1 = pldci_coef->u16BlkSizeX1;
		ldci_2_cfg->LDCI_BLK_SIZE_X1.bits.LDCI_BLK_SIZE_Y1 = pldci_coef->u16BlkSizeY1;
		ldci_2_cfg->LDCI_SUBBLK_SIZE_X.bits.LDCI_SUBBLK_SIZE_X = pldci_coef->u16SubBlkSizeX;
		ldci_2_cfg->LDCI_SUBBLK_SIZE_X.bits.LDCI_SUBBLK_SIZE_Y = pldci_coef->u16SubBlkSizeY;
		ldci_2_cfg->LDCI_SUBBLK_SIZE_X1.bits.LDCI_SUBBLK_SIZE_X1 = pldci_coef->u16SubBlkSizeX1;
		ldci_2_cfg->LDCI_SUBBLK_SIZE_X1.bits.LDCI_SUBBLK_SIZE_Y1 = pldci_coef->u16SubBlkSizeY1;
		ldci_2_cfg->LDCI_INTERP_NORM_LR.bits.LDCI_INTERP_NORM_LR = pldci_coef->u16InterpNormLR;
		ldci_2_cfg->LDCI_INTERP_NORM_LR.bits.LDCI_INTERP_NORM_UD = pldci_coef->u16InterpNormUD;
		ldci_2_cfg->LDCI_SUB_INTERP_NORM_LR.bits.LDCI_SUB_INTERP_NORM_LR = pldci_coef->u16SubInterpNormLR;
		ldci_2_cfg->LDCI_SUB_INTERP_NORM_LR.bits.LDCI_SUB_INTERP_NORM_UD = pldci_coef->u16SubInterpNormUD;
		ldci_2_cfg->LDCI_MEAN_NORM_X.bits.LDCI_MEAN_NORM_X = pldci_coef->u16MeanNormX;
		ldci_2_cfg->LDCI_MEAN_NORM_X.bits.LDCI_MEAN_NORM_Y = pldci_coef->u16MeanNormY;
		ldci_2_cfg->LDCI_VAR_NORM_Y.bits.LDCI_VAR_NORM_Y = pldci_coef->u16VarNormY;
		ldci_2_cfg->LDCI_UV_GAIN_MAX.bits.LDCI_UV_GAIN_MAX = runtime->ldci_attr.UvGainMax;
		ldci_2_cfg->LDCI_UV_GAIN_MAX.bits.LDCI_UV_GAIN_MIN = runtime->ldci_attr.UvGainMin;

		struct cvi_vip_isp_ldci_tun_3_cfg *ldci_3_cfg = &(ldci_cfg->ldci_3_cfg);

		pu16Lut = pldci_param->au16IdxFilterLut;
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_00.bits.LDCI_IDX_FILTER_LUT_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_00.bits.LDCI_IDX_FILTER_LUT_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_02.bits.LDCI_IDX_FILTER_LUT_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_02.bits.LDCI_IDX_FILTER_LUT_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_04.bits.LDCI_IDX_FILTER_LUT_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_04.bits.LDCI_IDX_FILTER_LUT_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_06.bits.LDCI_IDX_FILTER_LUT_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_06.bits.LDCI_IDX_FILTER_LUT_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_08.bits.LDCI_IDX_FILTER_LUT_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_08.bits.LDCI_IDX_FILTER_LUT_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_10.bits.LDCI_IDX_FILTER_LUT_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_10.bits.LDCI_IDX_FILTER_LUT_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_12.bits.LDCI_IDX_FILTER_LUT_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_12.bits.LDCI_IDX_FILTER_LUT_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_IDX_FILTER_LUT_14.bits.LDCI_IDX_FILTER_LUT_14 = pu16Lut[14];

		ldci_3_cfg->LDCI_INTERP_NORM_LR1.bits.LDCI_INTERP_NORM_LR1 = pldci_coef->u16InterpNormLR1;
		ldci_3_cfg->LDCI_INTERP_NORM_LR1.bits.LDCI_INTERP_NORM_UD1 = pldci_coef->u16InterpNormUD1;
		ldci_3_cfg->LDCI_SUB_INTERP_NORM_LR1.bits.LDCI_SUB_INTERP_NORM_LR1 = pldci_coef->u16SubInterpNormLR1;
		ldci_3_cfg->LDCI_SUB_INTERP_NORM_LR1.bits.LDCI_SUB_INTERP_NORM_UD1 = pldci_coef->u16SubInterpNormUD1;

		pu16Lut = pldci_param->au16ToneCurveLut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_00.bits.LDCI_TONE_CURVE_LUT_00_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_00.bits.LDCI_TONE_CURVE_LUT_00_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_02.bits.LDCI_TONE_CURVE_LUT_00_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_02.bits.LDCI_TONE_CURVE_LUT_00_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_04.bits.LDCI_TONE_CURVE_LUT_00_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_04.bits.LDCI_TONE_CURVE_LUT_00_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_06.bits.LDCI_TONE_CURVE_LUT_00_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_06.bits.LDCI_TONE_CURVE_LUT_00_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_08.bits.LDCI_TONE_CURVE_LUT_00_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_08.bits.LDCI_TONE_CURVE_LUT_00_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_10.bits.LDCI_TONE_CURVE_LUT_00_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_10.bits.LDCI_TONE_CURVE_LUT_00_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_12.bits.LDCI_TONE_CURVE_LUT_00_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_12.bits.LDCI_TONE_CURVE_LUT_00_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_14.bits.LDCI_TONE_CURVE_LUT_00_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_00_14.bits.LDCI_TONE_CURVE_LUT_00_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_00.bits.LDCI_TONE_CURVE_LUT_01_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_00.bits.LDCI_TONE_CURVE_LUT_01_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_02.bits.LDCI_TONE_CURVE_LUT_01_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_02.bits.LDCI_TONE_CURVE_LUT_01_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_04.bits.LDCI_TONE_CURVE_LUT_01_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_04.bits.LDCI_TONE_CURVE_LUT_01_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_06.bits.LDCI_TONE_CURVE_LUT_01_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_06.bits.LDCI_TONE_CURVE_LUT_01_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_08.bits.LDCI_TONE_CURVE_LUT_01_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_08.bits.LDCI_TONE_CURVE_LUT_01_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_10.bits.LDCI_TONE_CURVE_LUT_01_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_10.bits.LDCI_TONE_CURVE_LUT_01_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_12.bits.LDCI_TONE_CURVE_LUT_01_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_12.bits.LDCI_TONE_CURVE_LUT_01_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_14.bits.LDCI_TONE_CURVE_LUT_01_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_01_14.bits.LDCI_TONE_CURVE_LUT_01_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_00.bits.LDCI_TONE_CURVE_LUT_02_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_00.bits.LDCI_TONE_CURVE_LUT_02_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_02.bits.LDCI_TONE_CURVE_LUT_02_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_02.bits.LDCI_TONE_CURVE_LUT_02_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_04.bits.LDCI_TONE_CURVE_LUT_02_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_04.bits.LDCI_TONE_CURVE_LUT_02_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_06.bits.LDCI_TONE_CURVE_LUT_02_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_06.bits.LDCI_TONE_CURVE_LUT_02_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_08.bits.LDCI_TONE_CURVE_LUT_02_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_08.bits.LDCI_TONE_CURVE_LUT_02_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_10.bits.LDCI_TONE_CURVE_LUT_02_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_10.bits.LDCI_TONE_CURVE_LUT_02_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_12.bits.LDCI_TONE_CURVE_LUT_02_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_12.bits.LDCI_TONE_CURVE_LUT_02_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_14.bits.LDCI_TONE_CURVE_LUT_02_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_02_14.bits.LDCI_TONE_CURVE_LUT_02_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_00.bits.LDCI_TONE_CURVE_LUT_03_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_00.bits.LDCI_TONE_CURVE_LUT_03_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_02.bits.LDCI_TONE_CURVE_LUT_03_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_02.bits.LDCI_TONE_CURVE_LUT_03_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_04.bits.LDCI_TONE_CURVE_LUT_03_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_04.bits.LDCI_TONE_CURVE_LUT_03_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_06.bits.LDCI_TONE_CURVE_LUT_03_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_06.bits.LDCI_TONE_CURVE_LUT_03_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_08.bits.LDCI_TONE_CURVE_LUT_03_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_08.bits.LDCI_TONE_CURVE_LUT_03_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_10.bits.LDCI_TONE_CURVE_LUT_03_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_10.bits.LDCI_TONE_CURVE_LUT_03_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_12.bits.LDCI_TONE_CURVE_LUT_03_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_12.bits.LDCI_TONE_CURVE_LUT_03_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_14.bits.LDCI_TONE_CURVE_LUT_03_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_03_14.bits.LDCI_TONE_CURVE_LUT_03_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_00.bits.LDCI_TONE_CURVE_LUT_04_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_00.bits.LDCI_TONE_CURVE_LUT_04_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_02.bits.LDCI_TONE_CURVE_LUT_04_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_02.bits.LDCI_TONE_CURVE_LUT_04_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_04.bits.LDCI_TONE_CURVE_LUT_04_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_04.bits.LDCI_TONE_CURVE_LUT_04_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_06.bits.LDCI_TONE_CURVE_LUT_04_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_06.bits.LDCI_TONE_CURVE_LUT_04_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_08.bits.LDCI_TONE_CURVE_LUT_04_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_08.bits.LDCI_TONE_CURVE_LUT_04_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_10.bits.LDCI_TONE_CURVE_LUT_04_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_10.bits.LDCI_TONE_CURVE_LUT_04_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_12.bits.LDCI_TONE_CURVE_LUT_04_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_12.bits.LDCI_TONE_CURVE_LUT_04_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_14.bits.LDCI_TONE_CURVE_LUT_04_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_04_14.bits.LDCI_TONE_CURVE_LUT_04_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_00.bits.LDCI_TONE_CURVE_LUT_05_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_00.bits.LDCI_TONE_CURVE_LUT_05_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_02.bits.LDCI_TONE_CURVE_LUT_05_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_02.bits.LDCI_TONE_CURVE_LUT_05_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_04.bits.LDCI_TONE_CURVE_LUT_05_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_04.bits.LDCI_TONE_CURVE_LUT_05_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_06.bits.LDCI_TONE_CURVE_LUT_05_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_06.bits.LDCI_TONE_CURVE_LUT_05_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_08.bits.LDCI_TONE_CURVE_LUT_05_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_08.bits.LDCI_TONE_CURVE_LUT_05_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_10.bits.LDCI_TONE_CURVE_LUT_05_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_10.bits.LDCI_TONE_CURVE_LUT_05_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_12.bits.LDCI_TONE_CURVE_LUT_05_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_12.bits.LDCI_TONE_CURVE_LUT_05_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_14.bits.LDCI_TONE_CURVE_LUT_05_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_05_14.bits.LDCI_TONE_CURVE_LUT_05_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_00.bits.LDCI_TONE_CURVE_LUT_06_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_00.bits.LDCI_TONE_CURVE_LUT_06_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_02.bits.LDCI_TONE_CURVE_LUT_06_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_02.bits.LDCI_TONE_CURVE_LUT_06_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_04.bits.LDCI_TONE_CURVE_LUT_06_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_04.bits.LDCI_TONE_CURVE_LUT_06_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_06.bits.LDCI_TONE_CURVE_LUT_06_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_06.bits.LDCI_TONE_CURVE_LUT_06_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_08.bits.LDCI_TONE_CURVE_LUT_06_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_08.bits.LDCI_TONE_CURVE_LUT_06_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_10.bits.LDCI_TONE_CURVE_LUT_06_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_10.bits.LDCI_TONE_CURVE_LUT_06_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_12.bits.LDCI_TONE_CURVE_LUT_06_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_12.bits.LDCI_TONE_CURVE_LUT_06_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_14.bits.LDCI_TONE_CURVE_LUT_06_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_06_14.bits.LDCI_TONE_CURVE_LUT_06_15 = pu16Lut[15];

		pu16Lut = pldci_param->au16ToneCurveLut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_00.bits.LDCI_TONE_CURVE_LUT_07_00 = pu16Lut[0];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_00.bits.LDCI_TONE_CURVE_LUT_07_01 = pu16Lut[1];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_02.bits.LDCI_TONE_CURVE_LUT_07_02 = pu16Lut[2];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_02.bits.LDCI_TONE_CURVE_LUT_07_03 = pu16Lut[3];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_04.bits.LDCI_TONE_CURVE_LUT_07_04 = pu16Lut[4];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_04.bits.LDCI_TONE_CURVE_LUT_07_05 = pu16Lut[5];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_06.bits.LDCI_TONE_CURVE_LUT_07_06 = pu16Lut[6];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_06.bits.LDCI_TONE_CURVE_LUT_07_07 = pu16Lut[7];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_08.bits.LDCI_TONE_CURVE_LUT_07_08 = pu16Lut[8];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_08.bits.LDCI_TONE_CURVE_LUT_07_09 = pu16Lut[9];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_10.bits.LDCI_TONE_CURVE_LUT_07_10 = pu16Lut[10];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_10.bits.LDCI_TONE_CURVE_LUT_07_11 = pu16Lut[11];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_12.bits.LDCI_TONE_CURVE_LUT_07_12 = pu16Lut[12];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_12.bits.LDCI_TONE_CURVE_LUT_07_13 = pu16Lut[13];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_14.bits.LDCI_TONE_CURVE_LUT_07_14 = pu16Lut[14];
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_07_14.bits.LDCI_TONE_CURVE_LUT_07_15 = pu16Lut[15];

		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_00.bits.LDCI_TONE_CURVE_LUT_P_00 = 0;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_00.bits.LDCI_TONE_CURVE_LUT_P_01 = 64;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_02.bits.LDCI_TONE_CURVE_LUT_P_02 = 128;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_02.bits.LDCI_TONE_CURVE_LUT_P_03 = 192;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_04.bits.LDCI_TONE_CURVE_LUT_P_04 = 256;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_04.bits.LDCI_TONE_CURVE_LUT_P_05 = 320;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_06.bits.LDCI_TONE_CURVE_LUT_P_06 = 384;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_06.bits.LDCI_TONE_CURVE_LUT_P_07 = 448;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_08.bits.LDCI_TONE_CURVE_LUT_P_08 = 512;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_08.bits.LDCI_TONE_CURVE_LUT_P_09 = 576;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_10.bits.LDCI_TONE_CURVE_LUT_P_10 = 640;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_10.bits.LDCI_TONE_CURVE_LUT_P_11 = 704;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_12.bits.LDCI_TONE_CURVE_LUT_P_12 = 768;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_12.bits.LDCI_TONE_CURVE_LUT_P_13 = 832;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_14.bits.LDCI_TONE_CURVE_LUT_P_14 = 896;
		ldci_3_cfg->LDCI_TONE_CURVE_LUT_P_14.bits.LDCI_TONE_CURVE_LUT_P_15 = 960;

		struct cvi_vip_isp_ldci_tun_4_cfg *ldci_4_cfg = &(ldci_cfg->ldci_4_cfg);

		pu8Lut = pldci_param->au8LumaWgtLut;
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_00.bits.LDCI_LUMA_WGT_LUT_00 = pu8Lut[0];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_00.bits.LDCI_LUMA_WGT_LUT_01 = pu8Lut[1];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_00.bits.LDCI_LUMA_WGT_LUT_02 = pu8Lut[2];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_00.bits.LDCI_LUMA_WGT_LUT_03 = pu8Lut[3];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_04.bits.LDCI_LUMA_WGT_LUT_04 = pu8Lut[4];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_04.bits.LDCI_LUMA_WGT_LUT_05 = pu8Lut[5];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_04.bits.LDCI_LUMA_WGT_LUT_06 = pu8Lut[6];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_04.bits.LDCI_LUMA_WGT_LUT_07 = pu8Lut[7];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_08.bits.LDCI_LUMA_WGT_LUT_08 = pu8Lut[8];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_08.bits.LDCI_LUMA_WGT_LUT_09 = pu8Lut[9];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_08.bits.LDCI_LUMA_WGT_LUT_10 = pu8Lut[10];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_08.bits.LDCI_LUMA_WGT_LUT_11 = pu8Lut[11];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_12.bits.LDCI_LUMA_WGT_LUT_12 = pu8Lut[12];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_12.bits.LDCI_LUMA_WGT_LUT_13 = pu8Lut[13];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_12.bits.LDCI_LUMA_WGT_LUT_14 = pu8Lut[14];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_12.bits.LDCI_LUMA_WGT_LUT_15 = pu8Lut[15];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_16.bits.LDCI_LUMA_WGT_LUT_16 = pu8Lut[16];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_16.bits.LDCI_LUMA_WGT_LUT_17 = pu8Lut[17];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_16.bits.LDCI_LUMA_WGT_LUT_18 = pu8Lut[18];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_16.bits.LDCI_LUMA_WGT_LUT_19 = pu8Lut[19];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_20.bits.LDCI_LUMA_WGT_LUT_20 = pu8Lut[20];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_20.bits.LDCI_LUMA_WGT_LUT_21 = pu8Lut[21];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_20.bits.LDCI_LUMA_WGT_LUT_22 = pu8Lut[22];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_20.bits.LDCI_LUMA_WGT_LUT_23 = pu8Lut[23];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_24.bits.LDCI_LUMA_WGT_LUT_24 = pu8Lut[24];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_24.bits.LDCI_LUMA_WGT_LUT_25 = pu8Lut[25];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_24.bits.LDCI_LUMA_WGT_LUT_26 = pu8Lut[26];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_24.bits.LDCI_LUMA_WGT_LUT_27 = pu8Lut[27];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_28.bits.LDCI_LUMA_WGT_LUT_28 = pu8Lut[28];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_28.bits.LDCI_LUMA_WGT_LUT_29 = pu8Lut[29];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_28.bits.LDCI_LUMA_WGT_LUT_30 = pu8Lut[30];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_28.bits.LDCI_LUMA_WGT_LUT_31 = pu8Lut[31];
		ldci_4_cfg->LDCI_LUMA_WGT_LUT_32.bits.LDCI_LUMA_WGT_LUT_32 = pu8Lut[32];

		struct cvi_vip_isp_ldci_tun_5_cfg *ldci_5_cfg = &(ldci_cfg->ldci_5_cfg);

		pu16Lut = pldci_param->au16VarFilterLut;
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_00.bits.LDCI_VAR_FILTER_LUT_00 = pu16Lut[0];
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_00.bits.LDCI_VAR_FILTER_LUT_01 = pu16Lut[1];
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_02.bits.LDCI_VAR_FILTER_LUT_02 = pu16Lut[2];
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_02.bits.LDCI_VAR_FILTER_LUT_03 = pu16Lut[3];
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_04.bits.LDCI_VAR_FILTER_LUT_04 = pu16Lut[4];
		ldci_5_cfg->LDCI_VAR_FILTER_LUT_04.bits.LDCI_VAR_FILTER_LUT_05 = pu16Lut[5];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ldci_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		const ISP_LDCI_ATTR_S * ldci_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_ldci_ctrl_get_ldci_attr(ViPipe, &ldci_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->LDCIEnable = ldci_attr->Enable;
		//manual or auto
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_ldci_ctrl_runtime *_get_ldci_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isViPipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isViPipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ldci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LDCI, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ldci_ctrl_check_ldci_attr_valid(const ISP_LDCI_ATTR_S *pstLDCIAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstLDCIAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstLDCIAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstLDCIAttr, UpdateInterval, 0, 0xFF);
	// CHECK_VALID_CONST(pstLDCIAttr, GaussLPFSigma, 0, 0xFF);

	CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, LdciStrength, 0, 0x100);
	CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, LdciRange, 0, 0x3FF);
	CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, TprCoef, 0, 0x3FF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, EdgeCoring, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, LumaWgtMax, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, LumaWgtMin, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, VarMapMax, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, VarMapMin, 0, 0xFF);
	CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, UvGainMax, 0, 0x7F);
	CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, UvGainMin, 0, 0x7F);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, BrightContrastHigh, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, BrightContrastLow, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, DarkContrastHigh, 0, 0xFF);
	// CHECK_VALID_AUTO_ISO_1D(pstLDCIAttr, DarkContrastLow, 0, 0xFF);

	CHECK_VALID_CONST(pstLDCIAttr, stManual.LumaPosWgt.Wgt, 0, 0x80);
	CHECK_VALID_CONST(pstLDCIAttr, stManual.LumaPosWgt.Sigma, 0x1, 0xFF);
	CHECK_VALID_CONST(pstLDCIAttr, stManual.LumaPosWgt.Mean, 0x1, 0xFF);
	for (CVI_U32 u32IsoIdx = 0; u32IsoIdx < ISP_AUTO_ISO_STRENGTH_NUM; ++u32IsoIdx) {
		CHECK_VALID_CONST(pstLDCIAttr, stAuto.LumaPosWgt[u32IsoIdx].Wgt, 0x1, 0x80);
		CHECK_VALID_CONST(pstLDCIAttr, stAuto.LumaPosWgt[u32IsoIdx].Sigma, 0x1, 0xFF);
		CHECK_VALID_CONST(pstLDCIAttr, stAuto.LumaPosWgt[u32IsoIdx].Mean, 0x1, 0xFF);
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ldci_ctrl_get_ldci_attr(VI_PIPE ViPipe, const ISP_LDCI_ATTR_S **pstLDCIAttr)
{
	if (pstLDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LDCI, (CVI_VOID *) &shared_buffer);

	*pstLDCIAttr = &shared_buffer->stLdciAttr;

	return ret;
}

CVI_S32 isp_ldci_ctrl_set_ldci_attr(VI_PIPE ViPipe, const ISP_LDCI_ATTR_S *pstLDCIAttr)
{
	if (pstLDCIAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ldci_ctrl_runtime *runtime = _get_ldci_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ldci_ctrl_check_ldci_attr_valid(pstLDCIAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_LDCI_ATTR_S *p = CVI_NULL;

	isp_ldci_ctrl_get_ldci_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstLDCIAttr, sizeof(ISP_LDCI_ATTR_S));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

