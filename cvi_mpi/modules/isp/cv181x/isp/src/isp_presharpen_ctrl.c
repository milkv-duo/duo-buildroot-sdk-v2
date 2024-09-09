/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_presharpen_ctrl.c
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

#include "isp_presharpen_ctrl.h"
#include "isp_mgr_buf.h"

#include "isp_algo_drc.h"
#include "isp_drc_ctrl.h"
#include "isp_ynr_ctrl.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl presharpen_mod = {
	.init = isp_presharpen_ctrl_init,
	.uninit = isp_presharpen_ctrl_uninit,
	.suspend = isp_presharpen_ctrl_suspend,
	.resume = isp_presharpen_ctrl_resume,
	.ctrl = isp_presharpen_ctrl_ctrl
};


static CVI_S32 isp_presharpen_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_presharpen_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_presharpen_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_presharpen_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_presharpen_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_presharpen_ctrl_check_presharpen_attr_valid(const ISP_PRESHARPEN_ATTR_S *pstpresharpenAttr);

static struct isp_presharpen_ctrl_runtime  *_get_presharpen_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_presharpen_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_presharpen_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_presharpen_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_presharpen_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_presharpen_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_presharpen_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassPreyee;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassPreyee = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_presharpen_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_presharpen_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_presharpen_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_presharpen_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_presharpen_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_presharpen_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_PRESHARPEN_ATTR_S *presharpen_attr = NULL;

	isp_presharpen_ctrl_get_presharpen_attr(ViPipe, &presharpen_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(presharpen_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!presharpen_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (presharpen_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			MANUAL(presharpen_attr, LumaAdpGain[i]);
			MANUAL(presharpen_attr, DeltaAdpGain[i]);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(presharpen_attr, LumaCorLutIn[i]);
			MANUAL(presharpen_attr, LumaCorLutOut[i]);
			MANUAL(presharpen_attr, MotionCorLutIn[i]);
			MANUAL(presharpen_attr, MotionCorLutOut[i]);
			MANUAL(presharpen_attr, MotionCorWgtLutIn[i]);
			MANUAL(presharpen_attr, MotionCorWgtLutOut[i]);
		}

		MANUAL(presharpen_attr, GlobalGain);
		MANUAL(presharpen_attr, OverShootGain);
		MANUAL(presharpen_attr, UnderShootGain);
		MANUAL(presharpen_attr, HFBlendWgt);
		MANUAL(presharpen_attr, MFBlendWgt);
		MANUAL(presharpen_attr, OverShootThr);
		MANUAL(presharpen_attr, UnderShootThr);
		MANUAL(presharpen_attr, OverShootThrMax);
		MANUAL(presharpen_attr, UnderShootThrMin);

		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(presharpen_attr, MotionShtGainIn[i]);
			MANUAL(presharpen_attr, MotionShtGainOut[i]);
		}
		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			MANUAL(presharpen_attr, HueShtCtrl[i]);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			MANUAL(presharpen_attr, SatShtGainIn[i]);
			MANUAL(presharpen_attr, SatShtGainOut[i]);
		}

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			AUTO(presharpen_attr, LumaAdpGain[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, DeltaAdpGain[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(presharpen_attr, LumaCorLutIn[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, LumaCorLutOut[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, MotionCorLutIn[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, MotionCorLutOut[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, MotionCorWgtLutIn[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, MotionCorWgtLutOut[i], INTPLT_POST_ISO);
		}

		AUTO(presharpen_attr, GlobalGain, INTPLT_POST_ISO);
		AUTO(presharpen_attr, OverShootGain, INTPLT_POST_ISO);
		AUTO(presharpen_attr, UnderShootGain, INTPLT_POST_ISO);
		AUTO(presharpen_attr, HFBlendWgt, INTPLT_POST_ISO);
		AUTO(presharpen_attr, MFBlendWgt, INTPLT_POST_ISO);
		AUTO(presharpen_attr, OverShootThr, INTPLT_POST_ISO);
		AUTO(presharpen_attr, UnderShootThr, INTPLT_POST_ISO);
		AUTO(presharpen_attr, OverShootThrMax, INTPLT_POST_ISO);
		AUTO(presharpen_attr, UnderShootThrMin, INTPLT_POST_ISO);

		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(presharpen_attr, MotionShtGainIn[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, MotionShtGainOut[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
			AUTO(presharpen_attr, HueShtCtrl[i], INTPLT_POST_ISO);
		}
		for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
			AUTO(presharpen_attr, SatShtGainIn[i], INTPLT_POST_ISO);
			AUTO(presharpen_attr, SatShtGainOut[i], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	for (CVI_U32 i = 0 ; i < EE_LUT_NODE ; i++) {
		runtime->presharpen_param_in.LumaCorLutIn[i] = runtime->presharpen_attr.LumaCorLutIn[i];
		runtime->presharpen_param_in.LumaCorLutOut[i] = runtime->presharpen_attr.LumaCorLutOut[i];
		runtime->presharpen_param_in.MotionCorLutIn[i] = runtime->presharpen_attr.MotionCorLutIn[i];
		runtime->presharpen_param_in.MotionCorLutOut[i] = runtime->presharpen_attr.MotionCorLutOut[i];
		runtime->presharpen_param_in.MotionCorWgtLutIn[i] = runtime->presharpen_attr.MotionCorWgtLutIn[i];
		runtime->presharpen_param_in.MotionCorWgtLutOut[i] = runtime->presharpen_attr.MotionCorWgtLutOut[i];
		runtime->presharpen_param_in.MotionShtGainIn[i] = runtime->presharpen_attr.MotionShtGainIn[i];
		runtime->presharpen_param_in.MotionShtGainOut[i] = runtime->presharpen_attr.MotionShtGainOut[i];
		runtime->presharpen_param_in.SatShtGainIn[i] = runtime->presharpen_attr.SatShtGainIn[i];
		runtime->presharpen_param_in.SatShtGainOut[i] = runtime->presharpen_attr.SatShtGainOut[i];
	}

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_presharpen_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_presharpen_main(
		(struct presharpen_param_in *)&runtime->presharpen_param_in,
		(struct presharpen_param_out *)&runtime->presharpen_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_presharpen_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_pre_ee_config *presharpen_cfg =
		(struct cvi_vip_isp_pre_ee_config *)&(post_addr->tun_cfg[tun_idx].pre_ee_cfg);

	const ISP_PRESHARPEN_ATTR_S *presharpen_attr = NULL;

	isp_presharpen_ctrl_get_presharpen_attr(ViPipe, &presharpen_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		presharpen_cfg->update = 0;
	} else {
		presharpen_cfg->update = 1;
		presharpen_cfg->enable = presharpen_attr->Enable && !runtime->is_module_bypass;
		presharpen_cfg->dbg_mode = presharpen_attr->TuningMode;
		presharpen_cfg->total_coring = 1;
		presharpen_cfg->total_motion_coring = 8;
		presharpen_cfg->total_gain = runtime->presharpen_attr.GlobalGain;
		presharpen_cfg->total_oshtthrd = runtime->presharpen_attr.OverShootThr;
		presharpen_cfg->total_ushtthrd = runtime->presharpen_attr.UnderShootThr;
		presharpen_cfg->pre_proc_enable = presharpen_attr->NoiseSuppressEnable;
		presharpen_cfg->lumaref_lpf_en = 1;
		presharpen_cfg->luma_coring_en = 1;
		presharpen_cfg->luma_adptctrl_en = presharpen_attr->LumaAdpGainEn;
		presharpen_cfg->delta_adptctrl_en = presharpen_attr->DeltaAdpGainEn;
		presharpen_cfg->delta_adptctrl_shift = 1;
		presharpen_cfg->chromaref_lpf_en = 1;
		presharpen_cfg->chroma_adptctrl_en = presharpen_attr->SatShtCtrlEn;
		presharpen_cfg->mf_core_gain = 128;
		presharpen_cfg->hf_blend_wgt = runtime->presharpen_attr.HFBlendWgt;
		presharpen_cfg->mf_blend_wgt = runtime->presharpen_attr.MFBlendWgt;
		presharpen_cfg->soft_clamp_enable = presharpen_attr->SoftClampEnable;
		presharpen_cfg->upper_bound_left_diff = presharpen_attr->SoftClampUB;
		presharpen_cfg->lower_bound_right_diff = presharpen_attr->SoftClampLB;
		memcpy(presharpen_cfg->luma_adptctrl_lut, runtime->presharpen_attr.LumaAdpGain, sizeof(CVI_U8) * 33);
		memcpy(presharpen_cfg->delta_adptctrl_lut, runtime->presharpen_attr.DeltaAdpGain, sizeof(CVI_U8) * 33);
		memcpy(presharpen_cfg->chroma_adptctrl_lut, runtime->presharpen_attr.HueShtCtrl, sizeof(CVI_U8) * 33);

		struct cvi_isp_ee_tun_1_cfg *cfg_1 = &(presharpen_cfg->pre_ee_1_cfg);

		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_0 = runtime->presharpen_attr.LumaCorLutIn[0];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_1 = runtime->presharpen_attr.LumaCorLutIn[1];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_2 = runtime->presharpen_attr.LumaCorLutIn[2];
		cfg_1->REG_A4.bits.EE_LUMA_CORING_LUT_IN_3 = runtime->presharpen_attr.LumaCorLutIn[3];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_0 = runtime->presharpen_attr.LumaCorLutOut[0];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_1 = runtime->presharpen_attr.LumaCorLutOut[1];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_2 = runtime->presharpen_attr.LumaCorLutOut[2];
		cfg_1->REG_A8.bits.EE_LUMA_CORING_LUT_OUT_3 = runtime->presharpen_attr.LumaCorLutOut[3];
		cfg_1->REG_AC.bits.EE_LUMA_CORING_LUT_SLOPE_0 = runtime->presharpen_param_out.LumaCorLutSlope[0];
		cfg_1->REG_AC.bits.EE_LUMA_CORING_LUT_SLOPE_1 = runtime->presharpen_param_out.LumaCorLutSlope[1];
		cfg_1->REG_B0.bits.EE_LUMA_CORING_LUT_SLOPE_2 = runtime->presharpen_param_out.LumaCorLutSlope[2];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_0 = runtime->presharpen_attr.MotionCorLutIn[0];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_1 = runtime->presharpen_attr.MotionCorLutIn[1];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_2 = runtime->presharpen_attr.MotionCorLutIn[2];
		cfg_1->REG_B4.bits.EE_MOTION_CORING_LUT_IN_3 = runtime->presharpen_attr.MotionCorLutIn[3];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_0 = runtime->presharpen_attr.MotionCorLutOut[0];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_1 = runtime->presharpen_attr.MotionCorLutOut[1];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_2 = runtime->presharpen_attr.MotionCorLutOut[2];
		cfg_1->REG_B8.bits.EE_MOTION_CORING_LUT_OUT_3 = runtime->presharpen_attr.MotionCorLutOut[3];
		cfg_1->REG_BC.bits.EE_MOTION_CORING_LUT_SLOPE_0 = runtime->presharpen_param_out.MotionCorLutSlope[0];
		cfg_1->REG_BC.bits.EE_MOTION_CORING_LUT_SLOPE_1 = runtime->presharpen_param_out.MotionCorLutSlope[1];
		cfg_1->REG_C0.bits.EE_MOTION_CORING_LUT_SLOPE_2 = runtime->presharpen_param_out.MotionCorLutSlope[2];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_0 = runtime->presharpen_attr.MotionCorWgtLutIn[0];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_1 = runtime->presharpen_attr.MotionCorWgtLutIn[1];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_2 = runtime->presharpen_attr.MotionCorWgtLutIn[2];
		cfg_1->REG_C4.bits.EE_MCORE_GAIN_LUT_IN_3 = runtime->presharpen_attr.MotionCorWgtLutIn[3];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_0 = runtime->presharpen_attr.MotionCorWgtLutOut[0];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_1 = runtime->presharpen_attr.MotionCorWgtLutOut[1];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_2 = runtime->presharpen_attr.MotionCorWgtLutOut[2];
		cfg_1->REG_C8.bits.EE_MCORE_GAIN_LUT_OUT_3 = runtime->presharpen_attr.MotionCorWgtLutOut[3];
		cfg_1->REG_HCC.bits.EE_MCORE_GAIN_LUT_SLOPE_0 = runtime->presharpen_param_out.MotionCorWgtLutSlope[0];
		cfg_1->REG_HCC.bits.EE_MCORE_GAIN_LUT_SLOPE_1 = runtime->presharpen_param_out.MotionCorWgtLutSlope[1];
		cfg_1->REG_HD0.bits.EE_MCORE_GAIN_LUT_SLOPE_2 = runtime->presharpen_param_out.MotionCorWgtLutSlope[2];

		struct cvi_isp_ee_tun_2_cfg *cfg_2 = &(presharpen_cfg->pre_ee_2_cfg);

		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_0 = runtime->presharpen_attr.SatShtGainIn[0];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_1 = runtime->presharpen_attr.SatShtGainIn[1];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_2 = runtime->presharpen_attr.SatShtGainIn[2];
		cfg_2->REG_19C.bits.EE_CHROMA_AMP_LUT_IN_3 = runtime->presharpen_attr.SatShtGainIn[3];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_0 = runtime->presharpen_attr.SatShtGainOut[0];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_1 = runtime->presharpen_attr.SatShtGainOut[1];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_2 = runtime->presharpen_attr.SatShtGainOut[2];
		cfg_2->REG_1A0.bits.EE_CHROMA_AMP_LUT_OUT_3 = runtime->presharpen_attr.SatShtGainOut[3];
		cfg_2->REG_1A4.bits.EE_CHROMA_AMP_LUT_SLOPE_0 = runtime->presharpen_param_out.SatShtGainSlope[0];
		cfg_2->REG_1A4.bits.EE_CHROMA_AMP_LUT_SLOPE_1 = runtime->presharpen_param_out.SatShtGainSlope[1];
		cfg_2->REG_1A8.bits.EE_CHROMA_AMP_LUT_SLOPE_2 = runtime->presharpen_param_out.SatShtGainSlope[2];

		struct cvi_isp_ee_tun_3_cfg *cfg_3 = &(presharpen_cfg->pre_ee_3_cfg);

		cfg_3->REG_1C4.bits.EE_SHTCTRL_OSHTGAIN = runtime->presharpen_attr.OverShootGain;
		cfg_3->REG_1C4.bits.EE_SHTCTRL_USHTGAIN = runtime->presharpen_attr.UnderShootGain;
		cfg_3->REG_1C8.bits.EE_TOTAL_OSHTTHRD_CLP = runtime->presharpen_attr.OverShootThrMax;
		cfg_3->REG_1C8.bits.EE_TOTAL_USHTTHRD_CLP = runtime->presharpen_attr.UnderShootThrMin;
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_0 = runtime->presharpen_attr.MotionShtGainIn[0];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_1 = runtime->presharpen_attr.MotionShtGainIn[1];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_2 = runtime->presharpen_attr.MotionShtGainIn[2];
		cfg_3->REG_1CC.bits.EE_MOTION_LUT_IN_3 = runtime->presharpen_attr.MotionShtGainIn[3];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_0 = runtime->presharpen_attr.MotionShtGainOut[0];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_1 = runtime->presharpen_attr.MotionShtGainOut[1];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_2 = runtime->presharpen_attr.MotionShtGainOut[2];
		cfg_3->REG_1D0.bits.EE_MOTION_LUT_OUT_3 = runtime->presharpen_attr.MotionShtGainOut[3];
		cfg_3->REG_1D4.bits.EE_MOTION_LUT_SLOPE_0 = runtime->presharpen_param_out.MotionShtGainSlope[0];
		cfg_3->REG_1D4.bits.EE_MOTION_LUT_SLOPE_1 = runtime->presharpen_param_out.MotionShtGainSlope[1];
		cfg_3->REG_1D8.bits.EE_MOTION_LUT_SLOPE_2 = runtime->presharpen_param_out.MotionShtGainSlope[2];
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_presharpen_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	//if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
	//	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	//	const ISP_PRESHARPEN_ATTR_S *presharpen_attr = NULL;
	//	ISP_DEBUGINFO_PROC_S *pProcST = NULL;

	//	if (runtime == CVI_NULL) {
	//		return CVI_FAILURE;
	//	}

	//	isp_presharpen_ctrl_get_presharpen_attr(ViPipe, &presharpen_attr);
	//	ISP_GET_PROC_INFO(ViPipe, pProcST);

	//	//common
	//	pProcST->PreSharpenEnable = presharpen_attr->Enable && !runtime->is_module_bypass;
	//	pProcST->PreSharpenisManualMode = presharpen_attr->enOpType;
	//}

	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_presharpen_ctrl_runtime  *_get_presharpen_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_presharpen_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_PREYEE, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_presharpen_ctrl_check_presharpen_attr_valid(const ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstPreSharpenAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstPreSharpenAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstPreSharpenAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstPreSharpenAttr, TuningMode, 0x0, 0xb);
	// CHECK_VALID_CONST(pstPreSharpenAttr, LumaAdpGainEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstPreSharpenAttr, DeltaAdpGainEn, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, DeltaAdpGain, SHARPEN_LUT_NUM, 0x0, 0x3f);
	// CHECK_VALID_CONST(pstPreSharpenAttr, NoiseSuppressEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstPreSharpenAttr, SatShtCtrlEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstPreSharpenAttr, SoftClampEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstPreSharpenAttr, SoftClampUB, 0x0, 0xff);
	// CHECK_VALID_CONST(pstPreSharpenAttr, SoftClampLB, 0x0, 0xff);

	CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, LumaAdpGain, SHARPEN_LUT_NUM, 0x0, 0x3f);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, LumaCorLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, LumaCorLutOut, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionCorLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionCorLutOut, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionCorWgtLutIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionCorWgtLutOut, EE_LUT_NODE, 0x0, 0xff);

	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, GlobalGain, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, OverShootGain, 0x0, 0x3f);
	CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, UnderShootGain, 0x0, 0x3f);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, HFBlendWgt, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, MFBlendWgt, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, OverShootThr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, UnderShootThr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, OverShootThrMax, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstPreSharpenAttr, UnderShootThrMin, 0x0, 0xff);

	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionShtGainIn, EE_LUT_NODE, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, MotionShtGainOut, EE_LUT_NODE, 0x0, 0x80);

	CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, HueShtCtrl, 4, 0x0, 0x3f);

	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, SatShtGainIn, EE_LUT_NODE, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstPreSharpenAttr, SatShtGainOut, EE_LUT_NODE, 0x0, 0xff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_presharpen_ctrl_get_presharpen_attr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S **pstPreSharpenAttr)
{
	if (pstPreSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_presharpen_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_PREYEE, (CVI_VOID *) &shared_buffer);

	*pstPreSharpenAttr = &shared_buffer->stPreSharpenAttr;

	return ret;
}

CVI_S32 isp_presharpen_ctrl_set_presharpen_attr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr)
{
	if (pstPreSharpenAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_presharpen_ctrl_runtime *runtime = _get_presharpen_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_presharpen_ctrl_check_presharpen_attr_valid(pstPreSharpenAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_PRESHARPEN_ATTR_S *p = CVI_NULL;

	isp_presharpen_ctrl_get_presharpen_attr(ViPipe, &p);
	memcpy((void *)p, pstPreSharpenAttr, sizeof(*pstPreSharpenAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

