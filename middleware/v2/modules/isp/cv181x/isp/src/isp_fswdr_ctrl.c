/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_fswdr_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"
#include "isp_mw_compat.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"
#include "cvi_ae.h"

#include "isp_fswdr_ctrl.h"
#include "isp_mgr_buf.h"

//#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl fswdr_mod = {
	.init = isp_fswdr_ctrl_init,
	.uninit = isp_fswdr_ctrl_uninit,
	.suspend = isp_fswdr_ctrl_suspend,
	.resume = isp_fswdr_ctrl_resume,
	.ctrl = isp_fswdr_ctrl_ctrl
};

static CVI_S32 isp_fswdr_ctrl_pre_fe_eof(VI_PIPE ViPipe);
static CVI_S32 isp_fswdr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_fswdr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_fswdr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_fswdr_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 isp_fswdr_ctrl_mmap(VI_PIPE ViPipe);
static CVI_S32 isp_fswdr_ctrl_munmap(VI_PIPE ViPipe);
static CVI_S32 isp_fswdr_ctrl_ready(VI_PIPE ViPipe);
static CVI_S32 set_fswdr_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_fswdr_ctrl_check_fswdr_attr_valid(const ISP_FSWDR_ATTR_S *pstfswdrAttr);

static struct isp_fswdr_ctrl_runtime  *_get_fswdr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_fswdr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (!runtime->is_init) {
		runtime->mapped = CVI_FALSE;
		runtime->initialized = CVI_TRUE;
		runtime->preprocess_updated = CVI_TRUE;
		runtime->process_updated = CVI_FALSE;
		runtime->postprocess_updated = CVI_FALSE;
		runtime->is_module_bypass = CVI_FALSE;
	}

#ifndef ARCH_RTOS_CV181X
#ifdef ENABLE_ISP_C906L
	runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
	runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
#else
	runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_MASTER;
	runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_MASTER;
#endif
#else
	if (!runtime->is_init) {
		runtime->run_ctrl_mark.pre_sof_run_ctrl = CTRL_RUN_IN_SLAVE;
		runtime->run_ctrl_mark.pre_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
		runtime->run_ctrl_mark.post_eof_run_ctrl = CTRL_RUN_IN_SLAVE;
	}
#endif

	runtime->is_init = CVI_TRUE;

	return ret;
}

CVI_S32 isp_fswdr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	isp_fswdr_ctrl_munmap(ViPipe);

	runtime->is_init = CVI_FALSE;

	return ret;
}

CVI_S32 isp_fswdr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_fswdr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_fswdr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_PREFE_EOF:
#ifndef ARCH_RTOS_CV181X
		if (runtime->run_ctrl_mark.pre_sof_run_ctrl == CTRL_RUN_IN_SLAVE) {
			break;
		}
#else
		if (runtime->run_ctrl_mark.pre_sof_run_ctrl == CTRL_RUN_IN_MASTER) {
			break;
		}
#endif
		isp_fswdr_ctrl_pre_fe_eof(ViPipe);
		break;
	case MOD_CMD_POST_EOF:
#ifndef ARCH_RTOS_CV181X
		if (runtime->run_ctrl_mark.post_eof_run_ctrl == CTRL_RUN_IN_SLAVE) {
			break;
		}
#else
		if (runtime->run_ctrl_mark.post_eof_run_ctrl == CTRL_RUN_IN_MASTER) {
			break;
		}
#endif
		isp_fswdr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassFusion;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassFusion = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

CVI_S32 isp_fswdr_ctrl_set_run_ctrl_mark(VI_PIPE ViPipe, const struct isp_module_run_ctrl_mark *mark)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->run_ctrl_mark = *mark;

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_fswdr_ctrl_pre_fe_eof(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->mapped == CVI_FALSE) {
		isp_fswdr_ctrl_mmap(ViPipe);
		runtime->mapped = CVI_TRUE;
	}

	return ret;
}

static CVI_S32 isp_fswdr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_fswdr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_fswdr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_fswdr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_fswdr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_fswdr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_FSWDR_ATTR_S *fswdr_attr = NULL;

	isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &fswdr_attr);

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(fswdr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	pstIspCtx->wdrLinearMode = (!fswdr_attr->Enable || runtime->is_module_bypass);
	CVI_ISP_SetWDRLEOnly(ViPipe, (!fswdr_attr->Enable || runtime->is_module_bypass));

	// No need to update parameters if disable. Because its meaningless
	#if 0
	if (!fswdr_attr->Enable || runtime->is_module_bypass)
		return ret;
	#endif

	if (fswdr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(fswdr_attr, WDRCombineLongThr);
		MANUAL(fswdr_attr, WDRCombineShortThr);
		MANUAL(fswdr_attr, WDRCombineMaxWeight);
		MANUAL(fswdr_attr, WDRCombineMinWeight);
		for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
			MANUAL(fswdr_attr, WDRMtIn[idx]);
			MANUAL(fswdr_attr, WDRMtOut[idx]);
		}
		MANUAL(fswdr_attr, WDRLongWgt);
		MANUAL(fswdr_attr, WDRCombineSNRAwareToleranceLevel);
		MANUAL(fswdr_attr, MergeModeAlpha);
		MANUAL(fswdr_attr, WDRMotionCombineLongThr);
		MANUAL(fswdr_attr, WDRMotionCombineShortThr);
		MANUAL(fswdr_attr, WDRMotionCombineMinWeight);
		MANUAL(fswdr_attr, WDRMotionCombineMaxWeight);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(fswdr_attr, WDRCombineLongThr, INTPLT_LV);
		AUTO(fswdr_attr, WDRCombineShortThr, INTPLT_LV);
		AUTO(fswdr_attr, WDRCombineMaxWeight, INTPLT_LV);
		AUTO(fswdr_attr, WDRCombineMinWeight, INTPLT_LV);
		for (CVI_U32 idx = 0 ; idx < 4 ; idx++) {
			AUTO(fswdr_attr, WDRMtIn[idx], INTPLT_LV);
			AUTO(fswdr_attr, WDRMtOut[idx], INTPLT_LV);
		}
		AUTO(fswdr_attr, WDRLongWgt, INTPLT_LV);
		AUTO(fswdr_attr, WDRCombineSNRAwareToleranceLevel, INTPLT_LV);
		AUTO(fswdr_attr, MergeModeAlpha, INTPLT_LV);
		AUTO(fswdr_attr, WDRMotionCombineLongThr, INTPLT_LV);
		AUTO(fswdr_attr, WDRMotionCombineShortThr, INTPLT_LV);
		AUTO(fswdr_attr, WDRMotionCombineMinWeight, INTPLT_LV);
		AUTO(fswdr_attr, WDRMotionCombineMaxWeight, INTPLT_LV);

		#undef AUTO
	}

	if (!fswdr_attr->Enable || runtime->is_module_bypass) {
		// runtime->fswdr_attr.MergeMode = 1;
		runtime->fswdr_attr.MergeModeAlpha = 0;
	}

	runtime->le_in_sel = 0;
	runtime->se_in_sel = 0;

	if (!fswdr_attr->Enable || runtime->is_module_bypass) {
		runtime->out_sel = 0;
		runtime->exp_ratio = 64;
	} else {
		if (fswdr_attr->TuningMode == 1 || fswdr_attr->TuningMode == 2) {
			//fix rtl error
			runtime->out_sel = 0;
			runtime->exp_ratio = 64;
			if (fswdr_attr->TuningMode == 1)
				runtime->se_in_sel = 1;
			else
				runtime->le_in_sel = 1;
		} else {
			runtime->out_sel = fswdr_attr->TuningMode;
			runtime->exp_ratio = algoResult->au32ExpRatio[0];
		}
	}

	// ParamIn
	ISP_CMOS_NOISE_CALIBRATION_S np = {0};

	CVI_ISP_GetNoiseProfileAttr(ViPipe, &np);

	runtime->fswdr_param_in.exposure_ratio = (CVI_FLOAT)algoResult->au32ExpRatio[0] / 64;
	runtime->fswdr_param_in.WDRCombineLongThr = runtime->fswdr_attr.WDRCombineLongThr;
	runtime->fswdr_param_in.WDRCombineShortThr = runtime->fswdr_attr.WDRCombineShortThr;
	runtime->fswdr_param_in.WDRCombineMinWeight = runtime->fswdr_attr.WDRCombineMinWeight;
	runtime->fswdr_param_in.WDRCombineMaxWeight = runtime->fswdr_attr.WDRCombineMaxWeight;
	if (isp_fswdr_ctrl_ready(ViPipe) != CVI_SUCCESS) {
		runtime->fswdr_param_in.WDRFsCalPxlNum = 0;
		runtime->fswdr_param_in.WDRPxlDiffSumR = 0;
		runtime->fswdr_param_in.WDRPxlDiffSumG = 0;
		runtime->fswdr_param_in.WDRPxlDiffSumB = 0;
	} else {
		// struct cvi_vip_isp_fswdr_report *report = runtime->fswdr_mem.vir_addr;

		runtime->fswdr_param_in.WDRFsCalPxlNum = 0;//report->cal_pix_num;
		runtime->fswdr_param_in.WDRPxlDiffSumR = 0;//report->diff_sum_r;
		runtime->fswdr_param_in.WDRPxlDiffSumG = 0;//report->diff_sum_g;
		runtime->fswdr_param_in.WDRPxlDiffSumB = 0;//report->diff_sum_b;
	}
	memcpy(runtime->fswdr_param_in.WDRMtIn, runtime->fswdr_attr.WDRMtIn, sizeof(CVI_U8) * 4);
	memcpy(runtime->fswdr_param_in.WDRMtOut, runtime->fswdr_attr.WDRMtOut, sizeof(CVI_U16) * 4);

	runtime->fswdr_param_in.WDRCombineSNRAwareEn = fswdr_attr->WDRCombineSNRAwareEn;
	runtime->fswdr_param_in.WDRCombineSNRAwareSmoothLevel = fswdr_attr->WDRCombineSNRAwareSmoothLevel;
	runtime->fswdr_param_in.WDRCombineSNRAwareLowThr = fswdr_attr->WDRCombineSNRAwareLowThr;
	runtime->fswdr_param_in.WDRCombineSNRAwareHighThr = fswdr_attr->WDRCombineSNRAwareHighThr;
	runtime->fswdr_param_in.WDRCombineSNRAwareToleranceLevel = runtime->fswdr_attr.WDRCombineSNRAwareToleranceLevel;

	runtime->fswdr_param_in.DarkToneRefinedThrL = fswdr_attr->DarkToneRefinedThrL;
	runtime->fswdr_param_in.DarkToneRefinedThrH = fswdr_attr->DarkToneRefinedThrH;
	runtime->fswdr_param_in.DarkToneRefinedMaxWeight = fswdr_attr->DarkToneRefinedMaxWeight;
	runtime->fswdr_param_in.DarkToneRefinedMinWeight = fswdr_attr->DarkToneRefinedMinWeight;

	runtime->fswdr_param_in.BrightToneRefinedThrL = fswdr_attr->BrightToneRefinedThrL;
	runtime->fswdr_param_in.BrightToneRefinedThrH = fswdr_attr->BrightToneRefinedThrH;
	runtime->fswdr_param_in.BrightToneRefinedMaxWeight = fswdr_attr->BrightToneRefinedMaxWeight;
	runtime->fswdr_param_in.BrightToneRefinedMinWeight = fswdr_attr->BrightToneRefinedMinWeight;

	runtime->fswdr_param_in.WDRMotionFusionMode = fswdr_attr->WDRMotionFusionMode;

	runtime->fswdr_param_in.WDRMotionCombineLongThr = runtime->fswdr_attr.WDRMotionCombineLongThr;
	runtime->fswdr_param_in.WDRMotionCombineShortThr = runtime->fswdr_attr.WDRMotionCombineShortThr;
	runtime->fswdr_param_in.WDRMotionCombineMinWeight = runtime->fswdr_attr.WDRMotionCombineMinWeight;
	runtime->fswdr_param_in.WDRMotionCombineMaxWeight = runtime->fswdr_attr.WDRMotionCombineMaxWeight;

	runtime->fswdr_param_in.iso = algoResult->u32PostIso;
	runtime->fswdr_param_in.digital_gain = algoResult->u32IspPostDgain;
	runtime->fswdr_param_in.wb_r_gain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_R];
	runtime->fswdr_param_in.wb_b_gain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_B];

	runtime->fswdr_param_in.pNoiseCalibration = ISP_PTR_CAST_PTR(&np);
	runtime->fswdr_param_in.WDRMinWeightSNRIIR = runtime->fswdr_param_out.WDRMinWeightSNRIIR;

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_fswdr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;
#ifndef ENABLE_ISP_C906L
	ret = isp_algo_fswdr_main(
		(struct fswdr_param_in *)&runtime->fswdr_param_in,
		(struct fswdr_param_out *)&runtime->fswdr_param_out
	);
#endif
	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_fswdr_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);
	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_fswdr_config *fswdr_cfg =
		(struct cvi_vip_isp_fswdr_config *)&(post_addr->tun_cfg[tun_idx].fswdr_cfg);

	const ISP_FSWDR_ATTR_S *fswdr_attr = NULL;

	isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &fswdr_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		fswdr_cfg->update = 0;
	} else {
		if (pstIspCtx->u8SnsWDRMode > WDR_MODE_QUDRA) {
			fswdr_cfg->update = 1;
		} else {
			//linearMode do not update drc param
			fswdr_cfg->update = 0;
		}
		fswdr_cfg->enable = fswdr_attr->Enable && !runtime->is_module_bypass;
		fswdr_cfg->mc_enable = fswdr_attr->MotionCompEnable;
		fswdr_cfg->dc_mode = fswdr_attr->WDRDCMode;
		fswdr_cfg->luma_mode = fswdr_attr->WDRLumaMode;
		fswdr_cfg->lmap_guide_dc_mode = fswdr_attr->LocalToneRefinedDCMode;
		fswdr_cfg->lmap_guide_luma_mode = fswdr_attr->LocalToneRefinedLumaMode;
		fswdr_cfg->s_max = runtime->fswdr_param_out.ShortMaxVal;
		fswdr_cfg->fusion_type = fswdr_attr->WDRType;
		fswdr_cfg->fusion_lwgt = runtime->fswdr_attr.WDRLongWgt;
		fswdr_cfg->motion_ls_mode = runtime->fswdr_param_out.MergeMtLsMode;
		fswdr_cfg->mmap_mrg_mode = runtime->fswdr_param_out.MergeMode;
		fswdr_cfg->mmap_1_enable = runtime->fswdr_param_out.MergeMtMaxMode;
		fswdr_cfg->motion_ls_sel = 0;
		fswdr_cfg->mmap_mrg_alph = runtime->fswdr_attr.MergeModeAlpha;
		fswdr_cfg->history_sel_2 = fswdr_attr->MtMode;
		fswdr_cfg->mmap_v_thd_l = runtime->fswdr_attr.WDRMotionCombineLongThr;
		fswdr_cfg->mmap_v_thd_h = runtime->fswdr_attr.WDRMotionCombineShortThr;
		fswdr_cfg->mmap_v_wgt_min = runtime->fswdr_attr.WDRMotionCombineMinWeight;
		fswdr_cfg->mmap_v_wgt_max = runtime->fswdr_attr.WDRMotionCombineMaxWeight;
		fswdr_cfg->mmap_v_wgt_slp = runtime->fswdr_param_out.WDRMotionCombineSlope;
		fswdr_cfg->le_in_sel = runtime->le_in_sel;
		fswdr_cfg->se_in_sel = runtime->se_in_sel;

		struct cvi_isp_fswdr_tun_cfg *cfg_0 = &(fswdr_cfg->fswdr_cfg);

		cfg_0->FS_SE_GAIN.bits.FS_LS_GAIN = runtime->exp_ratio;
		cfg_0->FS_SE_GAIN.bits.FS_OUT_SEL = runtime->out_sel;
		cfg_0->FS_LUMA_THD.bits.FS_LUMA_THD_L = runtime->fswdr_attr.WDRCombineLongThr;
		cfg_0->FS_LUMA_THD.bits.FS_LUMA_THD_H = runtime->fswdr_attr.WDRCombineShortThr;
		cfg_0->FS_WGT.bits.FS_WGT_MAX = runtime->fswdr_attr.WDRCombineMaxWeight;
		cfg_0->FS_WGT.bits.FS_WGT_MIN = runtime->fswdr_param_out.WDRCombineMinWeight;
		cfg_0->FS_WGT_SLOPE.bits.FS_WGT_SLP = runtime->fswdr_param_out.WDRCombineSlope;

		struct cvi_isp_fswdr_tun_2_cfg *cfg_2 = &(fswdr_cfg->fswdr_2_cfg);

		cfg_2->FS_MOTION_LUT_IN.bits.FS_MOTION_LUT_IN_0 = runtime->fswdr_attr.WDRMtIn[0];
		cfg_2->FS_MOTION_LUT_IN.bits.FS_MOTION_LUT_IN_1 = runtime->fswdr_attr.WDRMtIn[1];
		cfg_2->FS_MOTION_LUT_IN.bits.FS_MOTION_LUT_IN_2 = runtime->fswdr_attr.WDRMtIn[2];
		cfg_2->FS_MOTION_LUT_IN.bits.FS_MOTION_LUT_IN_3 = runtime->fswdr_attr.WDRMtIn[3];
		cfg_2->FS_MOTION_LUT_OUT_0.bits.FS_MOTION_LUT_OUT_0 = runtime->fswdr_attr.WDRMtOut[0];
		cfg_2->FS_MOTION_LUT_OUT_0.bits.FS_MOTION_LUT_OUT_1 = runtime->fswdr_attr.WDRMtOut[1];
		cfg_2->FS_MOTION_LUT_OUT_1.bits.FS_MOTION_LUT_OUT_2 = runtime->fswdr_attr.WDRMtOut[2];
		cfg_2->FS_MOTION_LUT_OUT_1.bits.FS_MOTION_LUT_OUT_3 = runtime->fswdr_attr.WDRMtOut[3];
		cfg_2->FS_MOTION_LUT_SLOPE_0.bits.FS_MOTION_LUT_SLOPE_0 = runtime->fswdr_param_out.WDRMtSlope[0];
		cfg_2->FS_MOTION_LUT_SLOPE_0.bits.FS_MOTION_LUT_SLOPE_1 = runtime->fswdr_param_out.WDRMtSlope[1];
		cfg_2->FS_MOTION_LUT_SLOPE_1.bits.FS_MOTION_LUT_SLOPE_2 = runtime->fswdr_param_out.WDRMtSlope[2];

		struct cvi_isp_fswdr_tun_3_cfg *cfg_3 = &(fswdr_cfg->fswdr_3_cfg);

		cfg_3->FS_CALIB_CTRL_0.bits.FS_CALIB_LUMA_LOW_TH = 3300;
		cfg_3->FS_CALIB_CTRL_0.bits.FS_CALIB_LUMA_HIGH_TH = 3900;
		cfg_3->FS_CALIB_CTRL_1.bits.FS_CALIB_DIF_TH = 0;
		cfg_3->FS_SE_FIX_OFFSET_0.bits.FS_SE_FIX_OFFSET_R = 0;
		cfg_3->FS_SE_FIX_OFFSET_1.bits.FS_SE_FIX_OFFSET_G = 0;
		cfg_3->FS_SE_FIX_OFFSET_2.bits.FS_SE_FIX_OFFSET_B = 0;
		cfg_3->FS_CALIB_OUT_0.bits.FS_CAL_PXL_NUM = 0;
		cfg_3->FS_CALIB_OUT_1.bits.FS_PXL_DIFF_SUM_R = 0;
		cfg_3->FS_CALIB_OUT_2.bits.FS_PXL_DIFF_SUM_G = 0;
		cfg_3->FS_CALIB_OUT_3.bits.FS_PXL_DIFF_SUM_B = 0;
		cfg_3->FS_LMAP_DARK_THD.bits.FS_LMAP_GUIDE_DARK_THD_L = fswdr_attr->DarkToneRefinedThrL;
		cfg_3->FS_LMAP_DARK_THD.bits.FS_LMAP_GUIDE_DARK_THD_H = fswdr_attr->DarkToneRefinedThrH;
		cfg_3->FS_LMAP_DARK_WGT.bits.FS_LMAP_GUIDE_DARK_WGT_L = fswdr_attr->DarkToneRefinedMaxWeight;
		cfg_3->FS_LMAP_DARK_WGT.bits.FS_LMAP_GUIDE_DARK_WGT_H = fswdr_attr->DarkToneRefinedMinWeight;
		cfg_3->FS_LMAP_DARK_WGT_SLOPE.bits.FS_LMAP_GUIDE_DARK_WGT_SLP =
			runtime->fswdr_param_out.DarkToneRefinedSlope;
		cfg_3->FS_LMAP_BRIT_THD.bits.FS_LMAP_GUIDE_BRIT_THD_L = fswdr_attr->BrightToneRefinedThrL;
		cfg_3->FS_LMAP_BRIT_THD.bits.FS_LMAP_GUIDE_BRIT_THD_H = fswdr_attr->BrightToneRefinedThrH;
		cfg_3->FS_LMAP_BRIT_WGT.bits.FS_LMAP_GUIDE_BRIT_WGT_L = fswdr_attr->BrightToneRefinedMaxWeight;
		cfg_3->FS_LMAP_BRIT_WGT.bits.FS_LMAP_GUIDE_BRIT_WGT_H = fswdr_attr->BrightToneRefinedMinWeight;
		cfg_3->FS_LMAP_BRIT_WGT_SLOPE.bits.FS_LMAP_GUIDE_BRIT_WGT_SLP =
			runtime->fswdr_param_out.BrightToneRefinedSlope;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_fswdr_ctrl_mmap(VI_PIPE ViPipe)
{
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifndef ARCH_RTOS_CV181X
	struct cvi_vip_memblock *memblk = &(runtime->fswdr_mem);

	#define _mmap(va, pa, size) do {\
		if (size)\
			va = MMAP_COMPAT(pa, size); \
	} while (0)

	memblk->raw_num = ViPipe;

	G_EXT_CTRLS_PTR(VI_IOCTL_GET_FSWDR_PHY_BUF, memblk);

	_mmap(memblk->vir_addr, memblk->phy_addr, memblk->size);

	#undef _mmap
#endif
	return CVI_SUCCESS;
}

static CVI_S32 isp_fswdr_ctrl_munmap(VI_PIPE ViPipe)
{
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifndef ARCH_RTOS_CV181X
	struct cvi_vip_memblock *memblk = &(runtime->fswdr_mem);

	#define _munmap(va, size) do {\
		if (size)\
			MUNMAP_COMPAT(va, size); \
	} while (0)

	_munmap(memblk->vir_addr, memblk->size);

	#undef _munmap
#endif
	return CVI_SUCCESS;
}

static CVI_S32 set_fswdr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

		const ISP_FSWDR_ATTR_S *fswdr_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &fswdr_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->FSWDREnable = fswdr_attr->Enable;
		pProcST->FSWDRisManualMode = fswdr_attr->enOpType;
		pProcST->FSWDRWDRCombineSNRAwareEn = fswdr_attr->WDRCombineSNRAwareEn;
		pProcST->FSWDRWDRCombineSNRAwareLowThr = fswdr_attr->WDRCombineSNRAwareLowThr;
		pProcST->FSWDRWDRCombineSNRAwareHighThr = fswdr_attr->WDRCombineSNRAwareHighThr;
		pProcST->FSWDRWDRCombineSNRAwareSmoothLevel = fswdr_attr->WDRCombineSNRAwareSmoothLevel;
		//manual or auto
		pProcST->FSWDRWDRCombineSNRAwareToleranceLevel =
			runtime->fswdr_param_in.WDRCombineSNRAwareToleranceLevel;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static CVI_S32 isp_fswdr_ctrl_ready(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (!runtime->initialized || !runtime->mapped)
		ret = CVI_FAILURE;

	return ret;
}
//#endif // ENABLE_ISP_C906L

static struct isp_fswdr_ctrl_runtime  *_get_fswdr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_fswdr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_FUSION, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_fswdr_ctrl_check_fswdr_attr_valid(const ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CVI_BOOL CHECK_VALID_CONST(pstFSWDRAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstFSWDRAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstFSWDRAttr, UpdateInterval, 0x1, 0xFF);
	// CHECK_VALID_CONST(pstFSWDRAttr, MotionCompEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstFSWDRAttr, TuningMode, 0x0, 0x9);
	// CHECK_VALID_CONST(pstFSWDRAttr, WDRDCMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstFSWDRAttr, WDRLumaMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstFSWDRAttr, WDRType, 0x0, 0x2);
	// CHECK_VALID_CONST(pstFSWDRAttr, WDRCombineSNRAwareEn, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstFSWDRAttr, WDRCombineSNRAwareLowThr, 0x0, 0xffff);
	// CHECK_VALID_CONST(pstFSWDRAttr, WDRCombineSNRAwareHighThr, 0x0, 0xffff);
	CHECK_VALID_CONST(pstFSWDRAttr, WDRCombineSNRAwareSmoothLevel, 0x1, 0xbb8);
	// CHECK_VALID_CONST(pstFSWDRAttr, LocalToneRefinedDCMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstFSWDRAttr, LocalToneRefinedLumaMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstFSWDRAttr, DarkToneRefinedThrL, 0x0, 0xfff);
	CHECK_VALID_CONST(pstFSWDRAttr, DarkToneRefinedThrH, 0x0, 0xfff);
	CHECK_VALID_CONST(pstFSWDRAttr, DarkToneRefinedMaxWeight, 0x0, 0x100);
	CHECK_VALID_CONST(pstFSWDRAttr, DarkToneRefinedMinWeight, 0x0, 0x100);
	CHECK_VALID_CONST(pstFSWDRAttr, BrightToneRefinedThrL, 0x0, 0xfff);
	CHECK_VALID_CONST(pstFSWDRAttr, BrightToneRefinedThrH, 0x0, 0xfff);
	CHECK_VALID_CONST(pstFSWDRAttr, BrightToneRefinedMaxWeight, 0x0, 0x100);
	CHECK_VALID_CONST(pstFSWDRAttr, BrightToneRefinedMinWeight, 0x0, 0x100);
	CHECK_VALID_CONST(pstFSWDRAttr, WDRMotionFusionMode, 0x0, 0x3);
	// CHECK_VALID_CONST(pstFSWDRAttr, MtMode, CVI_FALSE, CVI_TRUE);

	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRCombineLongThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRCombineShortThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRCombineMaxWeight, 0x0, 0x100);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRCombineMinWeight, 0x0, 0x100);
	// CHECK_VALID_AUTO_LV_2D(pstFSWDRAttr, WDRMtIn, 4, 0x0, 0xff);
	CHECK_VALID_AUTO_LV_2D(pstFSWDRAttr, WDRMtOut, 4, 0x0, 0x100);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRLongWgt, 0x0, 0x100);
	// CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRCombineSNRAwareToleranceLevel, 0x0, 0xff);
	// CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, MergeModeAlpha, 0x0, 0xff);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRMotionCombineLongThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRMotionCombineShortThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRMotionCombineMinWeight, 0x0, 0x100);
	CHECK_VALID_AUTO_LV_1D(pstFSWDRAttr, WDRMotionCombineMaxWeight, 0x0, 0x100);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_fswdr_ctrl_get_fswdr_attr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S **pstFSWDRAttr)
{
	if (pstFSWDRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_FUSION, (CVI_VOID *) &shared_buffer);
	*pstFSWDRAttr = &shared_buffer->stFSWDRAttr;

	return ret;
}

CVI_S32 isp_fswdr_ctrl_set_fswdr_attr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S *pstFSWDRAttr)
{
	if (pstFSWDRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_fswdr_ctrl_check_fswdr_attr_valid(pstFSWDRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_FSWDR_ATTR_S *p = CVI_NULL;

	isp_fswdr_ctrl_get_fswdr_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstFSWDRAttr, sizeof(*pstFSWDRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_fswdr_ctrl_get_fswdr_info(VI_PIPE ViPipe, struct fswdr_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_fswdr_ctrl_runtime *runtime = _get_fswdr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	info->WDRCombineLongThr = runtime->fswdr_attr.WDRCombineLongThr;
	info->WDRCombineShortThr = runtime->fswdr_attr.WDRCombineShortThr;
	info->WDRCombineMinWeight = runtime->fswdr_attr.WDRCombineMinWeight;

	return ret;
}


