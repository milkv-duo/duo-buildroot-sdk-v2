/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_sharpen_ctrl.c
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

#include "isp_sharpen_ctrl.h"
#include "isp_mgr_buf.h"

#include "isp_algo_drc.h"
#include "isp_drc_ctrl.h"
#include "isp_ynr_ctrl.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl sharpen_mod = {
	.init = isp_sharpen_ctrl_init,
	.uninit = isp_sharpen_ctrl_uninit,
	.suspend = isp_sharpen_ctrl_suspend,
	.resume = isp_sharpen_ctrl_resume,
	.ctrl = isp_sharpen_ctrl_ctrl
};


static CVI_S32 isp_sharpen_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_sharpen_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_sharpen_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_sharpen_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_sharpen_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_sharpen_ctrl_check_sharpen_attr_valid(const ISP_SHARPEN_ATTR_S *pstsharpenAttr);

static struct isp_sharpen_ctrl_runtime  *_get_sharpen_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_sharpen_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_sharpen_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_sharpen_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_sharpen_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_sharpen_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_sharpen_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassYee;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassYee = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_sharpen_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_sharpen_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_sharpen_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_sharpen_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_sharpen_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_sharpen_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_SHARPEN_ATTR_S *sharpen_attr = NULL;

	isp_sharpen_ctrl_get_sharpen_attr(ViPipe, &sharpen_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(sharpen_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!sharpen_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (sharpen_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			MANUAL(sharpen_attr, LumaAdpGain[i]);
			MANUAL(sharpen_attr, DeltaAdpGain[i]);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(sharpen_attr, LumaCorLutIn[i]);
			MANUAL(sharpen_attr, LumaCorLutOut[i]);
			MANUAL(sharpen_attr, MotionCorLutIn[i]);
			MANUAL(sharpen_attr, MotionCorLutOut[i]);
			MANUAL(sharpen_attr, MotionCorWgtLutIn[i]);
			MANUAL(sharpen_attr, MotionCorWgtLutOut[i]);
		}

		MANUAL(sharpen_attr, GlobalGain);
		MANUAL(sharpen_attr, OverShootGain);
		MANUAL(sharpen_attr, UnderShootGain);
		MANUAL(sharpen_attr, HFBlendWgt);
		MANUAL(sharpen_attr, MFBlendWgt);
		MANUAL(sharpen_attr, OverShootThr);
		MANUAL(sharpen_attr, UnderShootThr);
		MANUAL(sharpen_attr, OverShootThrMax);
		MANUAL(sharpen_attr, UnderShootThrMin);

		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(sharpen_attr, MotionShtGainIn[i]);
			MANUAL(sharpen_attr, MotionShtGainOut[i]);
		}
		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			MANUAL(sharpen_attr, HueShtCtrl[i]);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(sharpen_attr, SatShtGainIn[i]);
			MANUAL(sharpen_attr, SatShtGainOut[i]);
		}

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			AUTO(sharpen_attr, LumaAdpGain[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, DeltaAdpGain[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(sharpen_attr, LumaCorLutIn[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, LumaCorLutOut[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, MotionCorLutIn[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, MotionCorLutOut[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, MotionCorWgtLutIn[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, MotionCorWgtLutOut[i], INTPLT_POST_ISO);
		}

		AUTO(sharpen_attr, GlobalGain, INTPLT_POST_ISO);
		AUTO(sharpen_attr, OverShootGain, INTPLT_POST_ISO);
		AUTO(sharpen_attr, UnderShootGain, INTPLT_POST_ISO);
		AUTO(sharpen_attr, HFBlendWgt, INTPLT_POST_ISO);
		AUTO(sharpen_attr, MFBlendWgt, INTPLT_POST_ISO);
		AUTO(sharpen_attr, OverShootThr, INTPLT_POST_ISO);
		AUTO(sharpen_attr, UnderShootThr, INTPLT_POST_ISO);
		AUTO(sharpen_attr, OverShootThrMax, INTPLT_POST_ISO);
		AUTO(sharpen_attr, UnderShootThrMin, INTPLT_POST_ISO);

		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(sharpen_attr, MotionShtGainIn[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, MotionShtGainOut[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			AUTO(sharpen_attr, HueShtCtrl[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(sharpen_attr, SatShtGainIn[i], INTPLT_POST_ISO);
			AUTO(sharpen_attr, SatShtGainOut[i], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
		runtime->sharpen_param_in.LumaCorLutIn[i] = runtime->sharpen_attr.LumaCorLutIn[i];
		runtime->sharpen_param_in.LumaCorLutOut[i] = runtime->sharpen_attr.LumaCorLutOut[i];
		runtime->sharpen_param_in.MotionCorLutIn[i] = runtime->sharpen_attr.MotionCorLutIn[i];
		runtime->sharpen_param_in.MotionCorLutOut[i] = runtime->sharpen_attr.MotionCorLutOut[i];
		runtime->sharpen_param_in.MotionCorWgtLutIn[i] = runtime->sharpen_attr.MotionCorWgtLutIn[i];
		runtime->sharpen_param_in.MotionCorWgtLutOut[i] = runtime->sharpen_attr.MotionCorWgtLutOut[i];
		runtime->sharpen_param_in.MotionShtGainIn[i] = runtime->sharpen_attr.MotionShtGainIn[i];
		runtime->sharpen_param_in.MotionShtGainOut[i] = runtime->sharpen_attr.MotionShtGainOut[i];
		runtime->sharpen_param_in.SatShtGainIn[i] = runtime->sharpen_attr.SatShtGainIn[i];
		runtime->sharpen_param_in.SatShtGainOut[i] = runtime->sharpen_attr.SatShtGainOut[i];
	}

	// ParamIn
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_sharpen_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_sharpen_main(
		(struct sharpen_param_in *)&runtime->sharpen_param_in,
		(struct sharpen_param_out *)&runtime->sharpen_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
static CVI_S32 isp_sharpen_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ee_config *sharpen_cfg =
		(struct cvi_vip_isp_ee_config *)&(post_addr->tun_cfg[tun_idx].ee_cfg);

	const ISP_SHARPEN_ATTR_S *sharpen_attr = NULL;

	isp_sharpen_ctrl_get_sharpen_attr(ViPipe, &sharpen_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		sharpen_cfg->update = 0;
	} else {
		sharpen_cfg->update = 1;
		sharpen_cfg->enable = sharpen_attr->Enable && !runtime->is_module_bypass;
		sharpen_cfg->dbg_mode = sharpen_attr->TuningMode;
		sharpen_cfg->total_coring = 1;
		sharpen_cfg->total_motion_coring = 8;
		sharpen_cfg->total_gain = runtime->sharpen_attr.GlobalGain;
		sharpen_cfg->total_oshtthrd = runtime->sharpen_attr.OverShootThr;
		sharpen_cfg->total_ushtthrd = runtime->sharpen_attr.UnderShootThr;
		sharpen_cfg->pre_proc_enable = sharpen_attr->NoiseSuppressEnable;
		sharpen_cfg->lumaref_lpf_en = 1;
		sharpen_cfg->luma_coring_en = 1;
		sharpen_cfg->luma_adptctrl_en = sharpen_attr->LumaAdpGainEn;
		sharpen_cfg->delta_adptctrl_en = sharpen_attr->DeltaAdpGainEn;
		sharpen_cfg->delta_adptctrl_shift = 1;
		sharpen_cfg->chromaref_lpf_en = 1;
		sharpen_cfg->chroma_adptctrl_en = sharpen_attr->SatShtCtrlEn;
		sharpen_cfg->mf_core_gain = 128;
		sharpen_cfg->hf_blend_wgt = runtime->sharpen_attr.HFBlendWgt;
		sharpen_cfg->mf_blend_wgt = runtime->sharpen_attr.MFBlendWgt;
		sharpen_cfg->soft_clamp_enable = sharpen_attr->SoftClampEnable;
		sharpen_cfg->upper_bound_left_diff = sharpen_attr->SoftClampUB;
		sharpen_cfg->lower_bound_right_diff = sharpen_attr->SoftClampLB;
		memcpy(sharpen_cfg->luma_adptctrl_lut, runtime->sharpen_attr.LumaAdpGain, sizeof(CVI_U8) * 33);
		memcpy(sharpen_cfg->delta_adptctrl_lut, runtime->sharpen_attr.DeltaAdpGain, sizeof(CVI_U8) * 33);
		memcpy(sharpen_cfg->chroma_adptctrl_lut, runtime->sharpen_attr.HueShtCtrl, sizeof(CVI_U8) * 33);

		struct cvi_isp_ee_tun_1_cfg *cfg_1 = &(sharpen_cfg->ee_1_cfg);

		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_0 = runtime->sharpen_attr.LumaCorLutIn[0];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_1 = runtime->sharpen_attr.LumaCorLutIn[1];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_2 = runtime->sharpen_attr.LumaCorLutIn[2];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_3 = runtime->sharpen_attr.LumaCorLutIn[3];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_0 = runtime->sharpen_attr.LumaCorLutOut[0];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_1 = runtime->sharpen_attr.LumaCorLutOut[1];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_2 = runtime->sharpen_attr.LumaCorLutOut[2];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_3 = runtime->sharpen_attr.LumaCorLutOut[3];
		cfg_1->REG_AC.bits.EE_LUMA_CORING_LUT_SLOPE_0 = runtime->sharpen_param_out.LumaCorLutSlope[0];
		cfg_1->REG_AC.bits.EE_LUMA_CORING_LUT_SLOPE_1 = runtime->sharpen_param_out.LumaCorLutSlope[1];
		cfg_1->REG_B0.bits.EE_LUMA_CORING_LUT_SLOPE_2 = runtime->sharpen_param_out.LumaCorLutSlope[2];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_0 = runtime->sharpen_attr.MotionCorLutIn[0];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_1 = runtime->sharpen_attr.MotionCorLutIn[1];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_2 = runtime->sharpen_attr.MotionCorLutIn[2];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_3 = runtime->sharpen_attr.MotionCorLutIn[3];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_0 = runtime->sharpen_attr.MotionCorLutOut[0];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_1 = runtime->sharpen_attr.MotionCorLutOut[1];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_2 = runtime->sharpen_attr.MotionCorLutOut[2];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_3 = runtime->sharpen_attr.MotionCorLutOut[3];
		cfg_1->REG_BC.bits.EE_MOTION_CORING_LUT_SLOPE_0 = runtime->sharpen_param_out.MotionCorLutSlope[0];
		cfg_1->REG_BC.bits.EE_MOTION_CORING_LUT_SLOPE_1 = runtime->sharpen_param_out.MotionCorLutSlope[1];
		cfg_1->REG_C0.bits.EE_MOTION_CORING_LUT_SLOPE_2 = runtime->sharpen_param_out.MotionCorLutSlope[2];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_0 = runtime->sharpen_attr.MotionCorWgtLutIn[0];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_1 = runtime->sharpen_attr.MotionCorWgtLutIn[1];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_2 = runtime->sharpen_attr.MotionCorWgtLutIn[2];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_3 = runtime->sharpen_attr.MotionCorWgtLutIn[3];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_0 = runtime->sharpen_attr.MotionCorWgtLutOut[0];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_1 = runtime->sharpen_attr.MotionCorWgtLutOut[1];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_2 = runtime->sharpen_attr.MotionCorWgtLutOut[2];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_3 = runtime->sharpen_attr.MotionCorWgtLutOut[3];
		cfg_1->REG_HCC.bits.EE_MCORE_GAIN_LUT_SLOPE_0 = runtime->sharpen_param_out.MotionCorWgtLutSlope[0];
		cfg_1->REG_HCC.bits.EE_MCORE_GAIN_LUT_SLOPE_1 = runtime->sharpen_param_out.MotionCorWgtLutSlope[1];
		cfg_1->REG_HD0.bits.EE_MCORE_GAIN_LUT_SLOPE_2 = runtime->sharpen_param_out.MotionCorWgtLutSlope[2];

		struct cvi_isp_ee_tun_2_cfg *cfg_2 = &(sharpen_cfg->ee_2_cfg);

		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_0 = runtime->sharpen_attr.SatShtGainIn[0];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_1 = runtime->sharpen_attr.SatShtGainIn[1];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_2 = runtime->sharpen_attr.SatShtGainIn[2];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_3 = runtime->sharpen_attr.SatShtGainIn[3];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_0 = runtime->sharpen_attr.SatShtGainOut[0];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_1 = runtime->sharpen_attr.SatShtGainOut[1];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_2 = runtime->sharpen_attr.SatShtGainOut[2];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_3 = runtime->sharpen_attr.SatShtGainOut[3];
		cfg_2->REG_1A4.bits.EE_CHROMA_AMP_LUT_SLOPE_0 = runtime->sharpen_param_out.SatShtGainSlope[0];
		cfg_2->REG_1A4.bits.EE_CHROMA_AMP_LUT_SLOPE_1 = runtime->sharpen_param_out.SatShtGainSlope[1];
		cfg_2->REG_1A8.bits.EE_CHROMA_AMP_LUT_SLOPE_2 = runtime->sharpen_param_out.SatShtGainSlope[2];

		struct cvi_isp_ee_tun_3_cfg *cfg_3 = &(sharpen_cfg->ee_3_cfg);

		cfg_3->REG_1C4.bits.EE_SHTCTRL_OSHTGAIN = runtime->sharpen_attr.OverShootGain;
		cfg_3->REG_1C4.bits.EE_SHTCTRL_USHTGAIN = runtime->sharpen_attr.UnderShootGain;
		cfg_3->REG_1C8.bits.EE_TOTAL_OSHTTHRD_CLP = runtime->sharpen_attr.OverShootThrMax;
		cfg_3->REG_1C8.bits.EE_TOTAL_USHTTHRD_CLP = runtime->sharpen_attr.UnderShootThrMin;
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_0 = runtime->sharpen_attr.MotionShtGainIn[0];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_1 = runtime->sharpen_attr.MotionShtGainIn[1];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_2 = runtime->sharpen_attr.MotionShtGainIn[2];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_3 = runtime->sharpen_attr.MotionShtGainIn[3];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_0 = runtime->sharpen_attr.MotionShtGainOut[0];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_1 = runtime->sharpen_attr.MotionShtGainOut[1];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_2 = runtime->sharpen_attr.MotionShtGainOut[2];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_3 = runtime->sharpen_attr.MotionShtGainOut[3];
		cfg_3->REG_1D4.bits.EE_MOTION_LUT_SLOPE_0 = runtime->sharpen_param_out.MotionShtGainSlope[0];
		cfg_3->REG_1D4.bits.EE_MOTION_LUT_SLOPE_1 = runtime->sharpen_param_out.MotionShtGainSlope[1];
		cfg_3->REG_1D8.bits.EE_MOTION_LUT_SLOPE_2 = runtime->sharpen_param_out.MotionShtGainSlope[2];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_sharpen_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

		const ISP_SHARPEN_ATTR_S *sharpen_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_sharpen_ctrl_get_sharpen_attr(ViPipe, &sharpen_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->SharpenEnable = sharpen_attr->Enable;
		pProcST->SharpenisManualMode = sharpen_attr->enOpType;

	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_sharpen_ctrl_runtime  *_get_sharpen_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_sharpen_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YEE, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_sharpen_ctrl_check_sharpen_attr_valid(const ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstSharpenAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstSharpenAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstSharpenAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstSharpenAttr, TuningMode, 0x0, 0xb);
	// CHECK_VALID_CONST(pstSharpenAttr, LumaAdpGainEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstSharpenAttr, DeltaAdpGainEn, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, DeltaAdpGain, SHARPEN_LUT_NUM, 0x0, 0x3f);
	// CHECK_VALID_CONST(pstSharpenAttr, NoiseSuppressEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstSharpenAttr, SatShtCtrlEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstSharpenAttr, SoftClampEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstSharpenAttr, SoftClampUB, 0x0, 0xff);
	// CHECK_VALID_CONST(pstSharpenAttr, SoftClampLB, 0x0, 0xff);

	CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, LumaAdpGain, SHARPEN_LUT_NUM, 0x0, 0x3f);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, LumaCorLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, LumaCorLutOut, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionCorLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionCorLutOut, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionCorWgtLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionCorWgtLutOut, EE_LUT_NODE, 0x0, 0xff);

	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, GlobalGain, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, OverShootGain, 0x0, 0x3f);
	CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, UnderShootGain, 0x0, 0x3f);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, HFBlendWgt, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, MFBlendWgt, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, OverShootThr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, UnderShootThr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, OverShootThrMax, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstSharpenAttr, UnderShootThrMin, 0x0, 0xff);

	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionShtGainIn, EE_LUT_NODE, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, MotionShtGainOut, EE_LUT_NODE, 0x0, 0x80);

	CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, HueShtCtrl, 4, 0x0, 0x3f);

	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, SatShtGainIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstSharpenAttr, SatShtGainOut, EE_LUT_NODE, 0x0, 0xff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_sharpen_ctrl_get_sharpen_attr(VI_PIPE ViPipe, const ISP_SHARPEN_ATTR_S **pstSharpenAttr)
{
	if (pstSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_sharpen_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_YEE, (CVI_VOID *) &shared_buffer);

	*pstSharpenAttr = &shared_buffer->stSharpenAttr;

	return ret;
}

CVI_S32 isp_sharpen_ctrl_set_sharpen_attr(VI_PIPE ViPipe, const ISP_SHARPEN_ATTR_S *pstSharpenAttr)
{
	if (pstSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sharpen_ctrl_runtime *runtime = _get_sharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_sharpen_ctrl_check_sharpen_attr_valid(pstSharpenAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_SHARPEN_ATTR_S *p = CVI_NULL;

	isp_sharpen_ctrl_get_sharpen_attr(ViPipe, &p);
	memcpy((void *)p, pstSharpenAttr, sizeof(*pstSharpenAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

