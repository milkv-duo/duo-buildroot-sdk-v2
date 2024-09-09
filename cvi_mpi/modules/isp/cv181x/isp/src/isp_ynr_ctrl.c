/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ynr_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"
#include "cvi_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_ynr_ctrl.h"
#include "isp_mgr_buf.h"
#include "isp_ccm_ctrl.h"
#include "isp_gamma_ctrl.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl ynr_mod = {
	.init = isp_ynr_ctrl_init,
	.uninit = isp_ynr_ctrl_uninit,
	.suspend = isp_ynr_ctrl_suspend,
	.resume = isp_ynr_ctrl_resume,
	.ctrl = isp_ynr_ctrl_ctrl
};


static CVI_S32 isp_ynr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ynr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ynr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ynr_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_ynr_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_ynr_ctrl_check_ynr_attr_valid(const ISP_YNR_ATTR_S *pstYNRAttr);
static CVI_S32 isp_ynr_ctrl_check_ynr_filter_attr_valid(const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr);
static CVI_S32 isp_ynr_ctrl_check_ynr_motion_attr_valid(const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr);
static CVI_BOOL is_value_in_array(CVI_S32 value, CVI_S32 *array, CVI_U32 length);
static struct isp_ynr_ctrl_runtime  *_get_ynr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ynr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_ynr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ynr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ynr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ynr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ynr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassYnr;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassYnr = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ynr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ynr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ynr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ynr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ynr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ynr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_YNR_ATTR_S *ynr_attr = NULL;
	const ISP_YNR_FILTER_ATTR_S *ynr_filter_attr = NULL;
	const ISP_YNR_MOTION_NR_ATTR_S *ynr_motion_attr = NULL;

	isp_ynr_ctrl_get_ynr_attr(ViPipe, &ynr_attr);
	isp_ynr_ctrl_get_ynr_filter_attr(ViPipe, &ynr_filter_attr);
	isp_ynr_ctrl_get_ynr_motion_attr(ViPipe, &ynr_motion_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ynr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ynr_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (ynr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(ynr_attr, WindowType);
		MANUAL(ynr_attr, DetailSmoothMode);
		MANUAL(ynr_attr, NoiseSuppressStr);
		MANUAL(ynr_attr, FilterType);
		MANUAL(ynr_attr, NoiseCoringMax);
		MANUAL(ynr_attr, NoiseCoringBase);
		MANUAL(ynr_attr, NoiseCoringAdv);

		MANUAL(ynr_filter_attr, VarThr);
		MANUAL(ynr_filter_attr, CoringWgtLF);
		MANUAL(ynr_filter_attr, CoringWgtHF);
		MANUAL(ynr_filter_attr, NonDirFiltStr);
		MANUAL(ynr_filter_attr, VhDirFiltStr);
		MANUAL(ynr_filter_attr, AaDirFiltStr);
		MANUAL(ynr_filter_attr, CoringWgtMax);
		MANUAL(ynr_filter_attr, FilterMode);

		MANUAL(ynr_motion_attr, MotionCoringWgtMax);
		for (CVI_U32 idx = 0 ; idx < 16 ; idx++) {
			MANUAL(ynr_motion_attr, MotionYnrLut[idx]);
			MANUAL(ynr_motion_attr, MotionCoringWgt[idx]);
		}
		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(ynr_attr, WindowType, INTPLT_POST_ISO);
		AUTO(ynr_attr, DetailSmoothMode, INTPLT_POST_ISO);
		AUTO(ynr_attr, NoiseSuppressStr, INTPLT_POST_ISO);
		AUTO(ynr_attr, FilterType, INTPLT_POST_ISO);
		AUTO(ynr_attr, NoiseCoringMax, INTPLT_POST_ISO);
		AUTO(ynr_attr, NoiseCoringBase, INTPLT_POST_ISO);
		AUTO(ynr_attr, NoiseCoringAdv, INTPLT_POST_ISO);

		AUTO(ynr_filter_attr, VarThr, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, CoringWgtLF, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, CoringWgtHF, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, NonDirFiltStr, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, VhDirFiltStr, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, AaDirFiltStr, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, CoringWgtMax, INTPLT_POST_ISO);
		AUTO(ynr_filter_attr, FilterMode, INTPLT_POST_ISO);

		AUTO(ynr_motion_attr, MotionCoringWgtMax, INTPLT_POST_ISO);
		for (CVI_U32 idx = 0 ; idx < 16 ; idx++) {
			AUTO(ynr_motion_attr, MotionYnrLut[idx], INTPLT_POST_ISO);
			AUTO(ynr_motion_attr, MotionCoringWgt[idx], INTPLT_POST_ISO);
		}
		#undef AUTO
	}

	struct ccm_info ccm_info;
	CVI_U8 noiseCoringLuma[6] = {0, 16, 32, 64, 128, 255};

	// ParamIn
	runtime->ynr_param_in.CoringParamEnable = ynr_attr->CoringParamEnable;
	runtime->ynr_param_in.isWdrMode = IS_2to1_WDR_MODE(algoResult->enFSWDRMode);
	runtime->ynr_param_in.u32ExpRatio = algoResult->au32ExpRatio[0];
	for (CVI_U32 idx = 0 ; idx < YNR_CORING_NUM ; idx++) {
		runtime->ynr_param_in.NoiseCoringBaseLuma[idx] = noiseCoringLuma[idx];
		runtime->ynr_param_in.NoiseCoringBaseOffset[idx] = runtime->ynr_attr.NoiseCoringBase;
		runtime->ynr_param_in.NoiseCoringAdvLuma[idx] = noiseCoringLuma[idx];
		runtime->ynr_param_in.NoiseCoringAdvOffset[idx] = runtime->ynr_attr.NoiseCoringAdv;
	}
	CVI_ISP_GetNoiseProfileAttr(ViPipe, &runtime->ynr_param_in.np);
	runtime->ynr_param_in.iso = algoResult->u32PostNpIso;
	runtime->ynr_param_in.ispdgain = algoResult->u32IspPostDgain;
	runtime->ynr_param_in.wb_rgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_R];
	runtime->ynr_param_in.wb_bgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_B];
	isp_ccm_ctrl_get_ccm_info(ViPipe, &ccm_info);
	for (CVI_U32 i = 0 ; i < 9 ; i++)
		runtime->ynr_param_in.CCM[i] = ccm_info.CCM[i] / 1024.0;

	isp_gamma_ctrl_get_real_gamma_lut(ViPipe, (CVI_U16 **) &runtime->ynr_param_in.GammaLUT);

	runtime->ynr_param_in.window_type = runtime->ynr_attr.WindowType;
	runtime->ynr_param_in.filter_type = runtime->ynr_attr.FilterType;
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_ynr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_ynr_main(
		(struct ynr_param_in *)&runtime->ynr_param_in,
		(struct ynr_param_out *)&runtime->ynr_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_ynr_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ynr_config *ynr_cfg =
		(struct cvi_vip_isp_ynr_config *)&(post_addr->tun_cfg[tun_idx].ynr_cfg);

	const ISP_YNR_ATTR_S *ynr_attr = NULL;

	isp_ynr_ctrl_get_ynr_attr(ViPipe, &ynr_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		ynr_cfg->update = 0;
	} else {
		ynr_cfg->update = 1;
		ynr_cfg->enable = ynr_attr->Enable && !runtime->is_module_bypass;
		ynr_cfg->out_sel = ynr_attr->TuningMode;
		ynr_cfg->weight_intra_0 = runtime->ynr_param_out.kernel.reg_ynr_weight_intra_0;
		ynr_cfg->weight_intra_1 = runtime->ynr_param_out.kernel.reg_ynr_weight_intra_1;
		ynr_cfg->weight_intra_2 = runtime->ynr_param_out.kernel.reg_ynr_weight_intra_2;
		ynr_cfg->weight_norm_1 = runtime->ynr_param_out.kernel.reg_ynr_weight_norm_intra1;
		ynr_cfg->weight_norm_2 = runtime->ynr_param_out.kernel.reg_ynr_weight_norm_intra2;
		ynr_cfg->res_max = runtime->ynr_filter_attr.CoringWgtMax;
		ynr_cfg->res_motion_max = runtime->ynr_motion_attr.MotionCoringWgtMax;
		ynr_cfg->motion_ns_clip_max = runtime->ynr_attr.NoiseCoringMax;
		memcpy(ynr_cfg->weight_lut_h, runtime->ynr_param_out.weight_lut, sizeof(CVI_U8) * 64);

		struct cvi_isp_ynr_tun_1_cfg *cfg_1 = &(ynr_cfg->ynr_1_cfg);

		cfg_1->NS0_LUMA_TH_00.bits.YNR_NS0_LUMA_TH_00 = runtime->ynr_param_out.NoiseCoringBaseLuma[0];
		cfg_1->NS0_LUMA_TH_01.bits.YNR_NS0_LUMA_TH_01 = runtime->ynr_param_out.NoiseCoringBaseLuma[1];
		cfg_1->NS0_LUMA_TH_02.bits.YNR_NS0_LUMA_TH_02 = runtime->ynr_param_out.NoiseCoringBaseLuma[2];
		cfg_1->NS0_LUMA_TH_03.bits.YNR_NS0_LUMA_TH_03 = runtime->ynr_param_out.NoiseCoringBaseLuma[3];
		cfg_1->NS0_LUMA_TH_04.bits.YNR_NS0_LUMA_TH_04 = runtime->ynr_param_out.NoiseCoringBaseLuma[4];
		cfg_1->NS0_LUMA_TH_05.bits.YNR_NS0_LUMA_TH_05 = runtime->ynr_param_out.NoiseCoringBaseLuma[5];
		cfg_1->NS0_SLOPE_00.bits.YNR_NS0_SLOPE_00 = runtime->ynr_param_out.NpSlopeBase[0];
		cfg_1->NS0_SLOPE_01.bits.YNR_NS0_SLOPE_01 = runtime->ynr_param_out.NpSlopeBase[1];
		cfg_1->NS0_SLOPE_02.bits.YNR_NS0_SLOPE_02 = runtime->ynr_param_out.NpSlopeBase[2];
		cfg_1->NS0_SLOPE_03.bits.YNR_NS0_SLOPE_03 = runtime->ynr_param_out.NpSlopeBase[3];
		cfg_1->NS0_SLOPE_04.bits.YNR_NS0_SLOPE_04 = runtime->ynr_param_out.NpSlopeBase[4];
		cfg_1->NS0_OFFSET_00.bits.YNR_NS0_OFFSET_00 = runtime->ynr_param_out.NoiseCoringBaseOffset[0];
		cfg_1->NS0_OFFSET_01.bits.YNR_NS0_OFFSET_01 = runtime->ynr_param_out.NoiseCoringBaseOffset[1];
		cfg_1->NS0_OFFSET_02.bits.YNR_NS0_OFFSET_02 = runtime->ynr_param_out.NoiseCoringBaseOffset[2];
		cfg_1->NS0_OFFSET_03.bits.YNR_NS0_OFFSET_03 = runtime->ynr_param_out.NoiseCoringBaseOffset[3];
		cfg_1->NS0_OFFSET_04.bits.YNR_NS0_OFFSET_04 = runtime->ynr_param_out.NoiseCoringBaseOffset[4];
		cfg_1->NS0_OFFSET_05.bits.YNR_NS0_OFFSET_05 = runtime->ynr_param_out.NoiseCoringBaseOffset[5];

		cfg_1->NS1_LUMA_TH_00.bits.YNR_NS1_LUMA_TH_00 = runtime->ynr_param_out.NoiseCoringAdvLuma[0];
		cfg_1->NS1_LUMA_TH_01.bits.YNR_NS1_LUMA_TH_01 = runtime->ynr_param_out.NoiseCoringAdvLuma[1];
		cfg_1->NS1_LUMA_TH_02.bits.YNR_NS1_LUMA_TH_02 = runtime->ynr_param_out.NoiseCoringAdvLuma[2];
		cfg_1->NS1_LUMA_TH_03.bits.YNR_NS1_LUMA_TH_03 = runtime->ynr_param_out.NoiseCoringAdvLuma[3];
		cfg_1->NS1_LUMA_TH_04.bits.YNR_NS1_LUMA_TH_04 = runtime->ynr_param_out.NoiseCoringAdvLuma[4];
		cfg_1->NS1_LUMA_TH_05.bits.YNR_NS1_LUMA_TH_05 = runtime->ynr_param_out.NoiseCoringAdvLuma[5];
		cfg_1->NS1_SLOPE_00.bits.YNR_NS1_SLOPE_00 = runtime->ynr_param_out.NpSlopeAdvance[0];
		cfg_1->NS1_SLOPE_01.bits.YNR_NS1_SLOPE_01 = runtime->ynr_param_out.NpSlopeAdvance[1];
		cfg_1->NS1_SLOPE_02.bits.YNR_NS1_SLOPE_02 = runtime->ynr_param_out.NpSlopeAdvance[2];
		cfg_1->NS1_SLOPE_03.bits.YNR_NS1_SLOPE_03 = runtime->ynr_param_out.NpSlopeAdvance[3];
		cfg_1->NS1_SLOPE_04.bits.YNR_NS1_SLOPE_04 = runtime->ynr_param_out.NpSlopeAdvance[4];
		cfg_1->NS1_OFFSET_00.bits.YNR_NS1_OFFSET_00 = runtime->ynr_param_out.NoiseCoringAdvOffset[0];
		cfg_1->NS1_OFFSET_01.bits.YNR_NS1_OFFSET_01 = runtime->ynr_param_out.NoiseCoringAdvOffset[1];
		cfg_1->NS1_OFFSET_02.bits.YNR_NS1_OFFSET_02 = runtime->ynr_param_out.NoiseCoringAdvOffset[2];
		cfg_1->NS1_OFFSET_03.bits.YNR_NS1_OFFSET_03 = runtime->ynr_param_out.NoiseCoringAdvOffset[3];
		cfg_1->NS1_OFFSET_04.bits.YNR_NS1_OFFSET_04 = runtime->ynr_param_out.NoiseCoringAdvOffset[4];
		cfg_1->NS1_OFFSET_05.bits.YNR_NS1_OFFSET_05 = runtime->ynr_param_out.NoiseCoringAdvOffset[5];
		cfg_1->NS_GAIN.bits.YNR_NS_GAIN = runtime->ynr_attr.NoiseSuppressStr;

		struct cvi_isp_ynr_tun_2_cfg *cfg_2 = &(ynr_cfg->ynr_2_cfg);

		cfg_2->MOTION_LUT_00.bits.YNR_MOTION_LUT_00 = runtime->ynr_motion_attr.MotionYnrLut[0];
		cfg_2->MOTION_LUT_01.bits.YNR_MOTION_LUT_01 = runtime->ynr_motion_attr.MotionYnrLut[1];
		cfg_2->MOTION_LUT_02.bits.YNR_MOTION_LUT_02 = runtime->ynr_motion_attr.MotionYnrLut[2];
		cfg_2->MOTION_LUT_03.bits.YNR_MOTION_LUT_03 = runtime->ynr_motion_attr.MotionYnrLut[3];
		cfg_2->MOTION_LUT_04.bits.YNR_MOTION_LUT_04 = runtime->ynr_motion_attr.MotionYnrLut[4];
		cfg_2->MOTION_LUT_05.bits.YNR_MOTION_LUT_05 = runtime->ynr_motion_attr.MotionYnrLut[5];
		cfg_2->MOTION_LUT_06.bits.YNR_MOTION_LUT_06 = runtime->ynr_motion_attr.MotionYnrLut[6];
		cfg_2->MOTION_LUT_07.bits.YNR_MOTION_LUT_07 = runtime->ynr_motion_attr.MotionYnrLut[7];
		cfg_2->MOTION_LUT_08.bits.YNR_MOTION_LUT_08 = runtime->ynr_motion_attr.MotionYnrLut[8];
		cfg_2->MOTION_LUT_09.bits.YNR_MOTION_LUT_09 = runtime->ynr_motion_attr.MotionYnrLut[9];
		cfg_2->MOTION_LUT_10.bits.YNR_MOTION_LUT_10 = runtime->ynr_motion_attr.MotionYnrLut[10];
		cfg_2->MOTION_LUT_11.bits.YNR_MOTION_LUT_11 = runtime->ynr_motion_attr.MotionYnrLut[11];
		cfg_2->MOTION_LUT_12.bits.YNR_MOTION_LUT_12 = runtime->ynr_motion_attr.MotionYnrLut[12];
		cfg_2->MOTION_LUT_13.bits.YNR_MOTION_LUT_13 = runtime->ynr_motion_attr.MotionYnrLut[13];
		cfg_2->MOTION_LUT_14.bits.YNR_MOTION_LUT_14 = runtime->ynr_motion_attr.MotionYnrLut[14];
		cfg_2->MOTION_LUT_15.bits.YNR_MOTION_LUT_15 = runtime->ynr_motion_attr.MotionYnrLut[15];

		struct cvi_isp_ynr_tun_3_cfg *cfg_3 = &(ynr_cfg->ynr_3_cfg);

		cfg_3->ALPHA_GAIN.bits.YNR_ALPHA_GAIN = runtime->ynr_filter_attr.FilterMode;
		cfg_3->VAR_TH.bits.YNR_VAR_TH = runtime->ynr_filter_attr.VarThr;
		cfg_3->WEIGHT_SM.bits.YNR_WEIGHT_SMOOTH = runtime->ynr_filter_attr.NonDirFiltStr;
		cfg_3->WEIGHT_V.bits.YNR_WEIGHT_V =
		cfg_3->WEIGHT_H.bits.YNR_WEIGHT_H = runtime->ynr_filter_attr.VhDirFiltStr;
		cfg_3->WEIGHT_D45.bits.YNR_WEIGHT_D45 =
		cfg_3->WEIGHT_D135.bits.YNR_WEIGHT_D135 = runtime->ynr_filter_attr.AaDirFiltStr;
		cfg_3->NEIGHBOR_MAX.bits.YNR_FLAG_NEIGHBOR_MAX_WEIGHT = runtime->ynr_attr.DetailSmoothMode;
		cfg_3->RES_K_SMOOTH.bits.YNR_RES_RATIO_K_SMOOTH = runtime->ynr_filter_attr.CoringWgtLF;
		cfg_3->RES_K_TEXTURE.bits.YNR_RES_RATIO_K_TEXTURE = runtime->ynr_filter_attr.CoringWgtHF;
		cfg_3->FILTER_MODE_EN.bits.YNR_FILTER_MODE_ENABLE = ynr_attr->FiltModeEnable;
		cfg_3->FILTER_MODE_ALPHA.bits.YNR_FILTER_MODE_ALPHA = ynr_attr->FiltMode;

		struct cvi_isp_ynr_tun_4_cfg *cfg_4 = &(ynr_cfg->ynr_4_cfg);

		cfg_4->RES_MOT_LUT_00.bits.YNR_RES_MOT_LUT_00 = runtime->ynr_motion_attr.MotionCoringWgt[0];
		cfg_4->RES_MOT_LUT_01.bits.YNR_RES_MOT_LUT_01 = runtime->ynr_motion_attr.MotionCoringWgt[1];
		cfg_4->RES_MOT_LUT_02.bits.YNR_RES_MOT_LUT_02 = runtime->ynr_motion_attr.MotionCoringWgt[2];
		cfg_4->RES_MOT_LUT_03.bits.YNR_RES_MOT_LUT_03 = runtime->ynr_motion_attr.MotionCoringWgt[3];
		cfg_4->RES_MOT_LUT_04.bits.YNR_RES_MOT_LUT_04 = runtime->ynr_motion_attr.MotionCoringWgt[4];
		cfg_4->RES_MOT_LUT_05.bits.YNR_RES_MOT_LUT_05 = runtime->ynr_motion_attr.MotionCoringWgt[5];
		cfg_4->RES_MOT_LUT_06.bits.YNR_RES_MOT_LUT_06 = runtime->ynr_motion_attr.MotionCoringWgt[6];
		cfg_4->RES_MOT_LUT_07.bits.YNR_RES_MOT_LUT_07 = runtime->ynr_motion_attr.MotionCoringWgt[7];
		cfg_4->RES_MOT_LUT_08.bits.YNR_RES_MOT_LUT_08 = runtime->ynr_motion_attr.MotionCoringWgt[8];
		cfg_4->RES_MOT_LUT_09.bits.YNR_RES_MOT_LUT_09 = runtime->ynr_motion_attr.MotionCoringWgt[9];
		cfg_4->RES_MOT_LUT_10.bits.YNR_RES_MOT_LUT_10 = runtime->ynr_motion_attr.MotionCoringWgt[10];
		cfg_4->RES_MOT_LUT_11.bits.YNR_RES_MOT_LUT_11 = runtime->ynr_motion_attr.MotionCoringWgt[11];
		cfg_4->RES_MOT_LUT_12.bits.YNR_RES_MOT_LUT_12 = runtime->ynr_motion_attr.MotionCoringWgt[12];
		cfg_4->RES_MOT_LUT_13.bits.YNR_RES_MOT_LUT_13 = runtime->ynr_motion_attr.MotionCoringWgt[13];
		cfg_4->RES_MOT_LUT_14.bits.YNR_RES_MOT_LUT_14 = runtime->ynr_motion_attr.MotionCoringWgt[14];
		cfg_4->RES_MOT_LUT_15.bits.YNR_RES_MOT_LUT_15 = runtime->ynr_motion_attr.MotionCoringWgt[15];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ynr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

		const ISP_YNR_ATTR_S *ynr_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_ynr_ctrl_get_ynr_attr(ViPipe, &ynr_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->YNREnable = ynr_attr->Enable;
		pProcST->YNRisManualMode = ynr_attr->enOpType;
		pProcST->YNRCoringParamEnable = ynr_attr->CoringParamEnable;
		//manual or auto
		pProcST->YNRWindowType = runtime->ynr_attr.WindowType;
		pProcST->YNRFilterType = runtime->ynr_attr.FilterType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}


#endif // ENABLE_ISP_C906L

static struct isp_ynr_ctrl_runtime  *_get_ynr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ynr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YNR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_BOOL is_value_in_array(CVI_S32 value, CVI_S32 *array, CVI_U32 length)
{
	CVI_U32 i;

	for (i = 0; i < length; i++)
		if (array[i] == value)
			break;

	return i != length;
}

static CVI_S32 isp_ynr_ctrl_check_ynr_attr_valid(const ISP_YNR_ATTR_S *pstYNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstYNRAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstYNRAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstYNRAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstYNRAttr, CoringParamEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstYNRAttr, FiltModeEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstYNRAttr, FiltMode, 0x0, 0x100);

	CVI_S32 TuningModeList[] = {8, 11, 12, 13, 14, 15};

	if (!is_value_in_array(pstYNRAttr->TuningMode, TuningModeList, ARRAY_SIZE(TuningModeList))) {
		ISP_LOG_WARNING("tuning moode only accept values in 8, 11, 12, 13, 14, 15\n");
		ret = CVI_FAILURE_ILLEGAL_PARAM;
	}

	CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, WindowType, 0x0, 0xb);
	CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, DetailSmoothMode, 0x0, 0x1);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, NoiseSuppressStr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, FilterType, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, NoiseCoringMax, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, NoiseCoringBase, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRAttr, NoiseCoringAdv, 0x0, 0xff);

	return ret;
}

static CVI_S32 isp_ynr_ctrl_check_ynr_filter_attr_valid(const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, VarThr, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, CoringWgtLF, 0x0, 0x100);
	CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, CoringWgtHF, 0x0, 0x100);
	CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, NonDirFiltStr, 0x0, 0x1f);
	CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, VhDirFiltStr, 0x0, 0x1f);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, AaDirFiltStr, 0x0, 0x1f);
	// CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, CoringWgtMax, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstYNRFilterAttr, FilterMode, 0x0, 0x3ff);

	return ret;
}

static CVI_S32 isp_ynr_ctrl_check_ynr_motion_attr_valid(const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_1D(pstYNRMotionNRAttr, MotionCoringWgtMax, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstYNRMotionNRAttr, MotionYnrLut, 16, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstYNRMotionNRAttr, MotionCoringWgt, 16, 0x0, 0x100);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ynr_ctrl_get_ynr_attr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S **pstYNRAttr)
{
	if (pstYNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ynr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YNR, (CVI_VOID *) &shared_buffer);
	*pstYNRAttr = &shared_buffer->stYNRAttr;

	return ret;
}

CVI_S32 isp_ynr_ctrl_set_ynr_attr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S *pstYNRAttr)
{
	if (pstYNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ynr_ctrl_check_ynr_attr_valid(pstYNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_YNR_ATTR_S *p = CVI_NULL;

	isp_ynr_ctrl_get_ynr_attr(ViPipe, &p);
	memcpy((void *)p, pstYNRAttr, sizeof(*pstYNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_ynr_ctrl_get_ynr_filter_attr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S **pstYNRFilterAttr)
{
	if (pstYNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ynr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YNR, (CVI_VOID *) &shared_buffer);
	*pstYNRFilterAttr = &shared_buffer->stYNRFilterAttr;

	return ret;
}

CVI_S32 isp_ynr_ctrl_set_ynr_filter_attr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr)
{
	if (pstYNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ynr_ctrl_check_ynr_filter_attr_valid(pstYNRFilterAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_YNR_FILTER_ATTR_S *p = CVI_NULL;

	isp_ynr_ctrl_get_ynr_filter_attr(ViPipe, &p);
	memcpy((void *)p, pstYNRFilterAttr, sizeof(*pstYNRFilterAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_ynr_ctrl_get_ynr_motion_attr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S **pstYNRMotionNRAttr)
{
	if (pstYNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ynr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YNR, (CVI_VOID *) &shared_buffer);
	*pstYNRMotionNRAttr = &shared_buffer->stYNRMotionNRAttr;

	return ret;
}

CVI_S32 isp_ynr_ctrl_set_ynr_motion_attr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr)
{
	if (pstYNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ynr_ctrl_check_ynr_motion_attr_valid(pstYNRMotionNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_YNR_MOTION_NR_ATTR_S *p = CVI_NULL;

	isp_ynr_ctrl_get_ynr_motion_attr(ViPipe, &p);
	memcpy((void *)p, pstYNRMotionNRAttr, sizeof(*pstYNRMotionNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_ynr_ctrl_get_ynr_info(VI_PIPE ViPipe, struct ynr_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ynr_ctrl_runtime *runtime = _get_ynr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	for (CVI_U32 i = 0 ; i < YNR_NP_NODE_SIZE ; i++) {
		info->NoiseCoringBaseLuma[i] = runtime->ynr_param_out.NoiseCoringBaseLuma[i];
		info->NoiseCoringBaseOffset[i] = runtime->ynr_param_out.NoiseCoringBaseOffset[i];
	}
	for (CVI_U32 i = 0 ; i < YNR_NP_NODE_SIZE - 1 ; i++) {
		info->NpSlopeBase[i] = runtime->ynr_param_out.NpSlopeBase[i];
	}

	return ret;
}

