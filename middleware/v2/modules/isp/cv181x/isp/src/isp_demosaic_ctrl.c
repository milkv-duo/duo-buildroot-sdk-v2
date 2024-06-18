/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_demosaic_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "cvi_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_demosaic_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
static CVI_U8 cfa_ghp_lut[32] =  {
	 8,  8,  8,  8, 10, 10, 10, 10, 12, 12, 12, 12, 14, 14, 14, 14,
	16, 16, 16, 16, 20, 20, 20, 20, 24, 24, 24, 24, 28, 28, 31, 31
};

const struct isp_module_ctrl demosaic_mod = {
	.init = isp_demosaic_ctrl_init,
	.uninit = isp_demosaic_ctrl_uninit,
	.suspend = isp_demosaic_ctrl_suspend,
	.resume = isp_demosaic_ctrl_resume,
	.ctrl = isp_demosaic_ctrl_ctrl
};

static CVI_S32 isp_demosaic_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_demosaic_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_demosaic_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_demosaic_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_demosaic_ctrl_check_demosaic_attr_valid(
	const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr);
static CVI_S32 isp_demosaic_ctrl_check_demosaic_demoire_attr_valid(
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr);

static CVI_S32 set_demosaic_proc_info(VI_PIPE ViPipe);

static struct isp_demosaic_ctrl_runtime  *_get_demosaic_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_demosaic_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_demosaic_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_demosaic_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_demosaic_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_demosaic_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_demosaic_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDemosaic;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDemosaic = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_demosaic_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_demosaic_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_demosaic_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_demosaic_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_demosaic_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_demosaic_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DEMOSAIC_ATTR_S *demosaic_attr = NULL;
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *demosaic_demoire_attr = NULL;

	isp_demosaic_ctrl_get_demosaic_attr(ViPipe, &demosaic_attr);
	isp_demosaic_ctrl_get_demosaic_demoire_attr(ViPipe, &demosaic_demoire_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(demosaic_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!demosaic_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (demosaic_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		// Demosaic
		MANUAL(demosaic_attr, CoarseEdgeThr);
		MANUAL(demosaic_attr, CoarseStr);
		MANUAL(demosaic_attr, FineEdgeThr);
		MANUAL(demosaic_attr, FineStr);
		MANUAL(demosaic_attr, RbSigLumaThd);
		MANUAL(demosaic_attr, FilterMode);

		// DemosaicDemoire
		MANUAL(demosaic_demoire_attr, AntiFalseColorStr);
		for (CVI_U32 i = 0 ; i < 2 ; i++) {
			MANUAL(demosaic_demoire_attr, SatGainIn[i]);
			MANUAL(demosaic_demoire_attr, SatGainOut[i]);
			MANUAL(demosaic_demoire_attr, ProtectColorGainIn[i]);
			MANUAL(demosaic_demoire_attr, ProtectColorGainOut[i]);
			MANUAL(demosaic_demoire_attr, EdgeGainIn[i]);
			MANUAL(demosaic_demoire_attr, EdgeGainOut[i]);
			MANUAL(demosaic_demoire_attr, DetailGainIn[i]);
			MANUAL(demosaic_demoire_attr, DetailGaintOut[i]);
		}
		MANUAL(demosaic_demoire_attr, UserDefineProtectColor1);
		MANUAL(demosaic_demoire_attr, UserDefineProtectColor2);
		MANUAL(demosaic_demoire_attr, UserDefineProtectColor3);
		MANUAL(demosaic_demoire_attr, DetailDetectLumaStr);
		MANUAL(demosaic_demoire_attr, DetailSmoothStr);
		MANUAL(demosaic_demoire_attr, DetailWgtThr);
		MANUAL(demosaic_demoire_attr, DetailWgtMin);
		MANUAL(demosaic_demoire_attr, DetailWgtMax);
		MANUAL(demosaic_demoire_attr, DetailWgtSlope);
		MANUAL(demosaic_demoire_attr, EdgeWgtNp);
		MANUAL(demosaic_demoire_attr, EdgeWgtThr);
		MANUAL(demosaic_demoire_attr, EdgeWgtMin);
		MANUAL(demosaic_demoire_attr, EdgeWgtMax);
		MANUAL(demosaic_demoire_attr, EdgeWgtSlope);
		MANUAL(demosaic_demoire_attr, DetailSmoothMapTh);
		MANUAL(demosaic_demoire_attr, DetailSmoothMapMin);
		MANUAL(demosaic_demoire_attr, DetailSmoothMapMax);
		MANUAL(demosaic_demoire_attr, DetailSmoothMapSlope);
		MANUAL(demosaic_demoire_attr, LumaWgt);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		// Demosaic
		AUTO(demosaic_attr, CoarseEdgeThr, INTPLT_POST_ISO);
		AUTO(demosaic_attr, CoarseStr, INTPLT_POST_ISO);
		AUTO(demosaic_attr, FineEdgeThr, INTPLT_POST_ISO);
		AUTO(demosaic_attr, FineStr, INTPLT_POST_ISO);
		AUTO(demosaic_attr, RbSigLumaThd, INTPLT_POST_ISO);
		AUTO(demosaic_attr, FilterMode, INTPLT_POST_ISO);

		// DemosaicDemoire
		AUTO(demosaic_demoire_attr, AntiFalseColorStr, INTPLT_POST_ISO);
		for (CVI_U32 i = 0 ; i < 2 ; i++) {
			AUTO(demosaic_demoire_attr, SatGainIn[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, SatGainOut[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, ProtectColorGainIn[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, ProtectColorGainOut[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, EdgeGainIn[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, EdgeGainOut[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, DetailGainIn[i], INTPLT_POST_ISO);
			AUTO(demosaic_demoire_attr, DetailGaintOut[i], INTPLT_POST_ISO);
		}
		AUTO(demosaic_demoire_attr, UserDefineProtectColor1, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, UserDefineProtectColor2, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, UserDefineProtectColor3, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailDetectLumaStr, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailSmoothStr, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailWgtThr, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailWgtMin, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailWgtMax, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailWgtSlope, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, EdgeWgtNp, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, EdgeWgtThr, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, EdgeWgtMin, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, EdgeWgtMax, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, EdgeWgtSlope, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailSmoothMapTh, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailSmoothMapMin, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailSmoothMapMax, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, DetailSmoothMapSlope, INTPLT_POST_ISO);
		AUTO(demosaic_demoire_attr, LumaWgt, INTPLT_POST_ISO);

		#undef AUTO
	}

	for (CVI_U32 i = 0 ; i < 2 ; i++) {
		runtime->demosaic_param_in.SatGainIn[i] = runtime->demosaic_demoire_attr.SatGainIn[i];
		runtime->demosaic_param_in.SatGainOut[i] = runtime->demosaic_demoire_attr.SatGainOut[i];
		runtime->demosaic_param_in.ProtectColorGainIn[i] =
			runtime->demosaic_demoire_attr.ProtectColorGainIn[i];
		runtime->demosaic_param_in.ProtectColorGainOut[i] =
			runtime->demosaic_demoire_attr.ProtectColorGainOut[i];
		runtime->demosaic_param_in.EdgeGainIn[i] = runtime->demosaic_demoire_attr.EdgeGainIn[i];
		runtime->demosaic_param_in.EdgeGainOut[i] = runtime->demosaic_demoire_attr.EdgeGainOut[i];
		runtime->demosaic_param_in.DetailGainIn[i] = runtime->demosaic_demoire_attr.DetailGainIn[i];
		runtime->demosaic_param_in.DetailGaintOut[i] = runtime->demosaic_demoire_attr.DetailGaintOut[i];
	}

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_demosaic_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_demosaic_main(
		(struct demosaic_param_in *)&runtime->demosaic_param_in,
		(struct demosaic_param_out *)&runtime->demosaic_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_demosaic_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_demosiac_config *demosaic_cfg = {
		(struct cvi_vip_isp_demosiac_config *)&(post_addr->tun_cfg[tun_idx].demosiac_cfg)
	};

	const ISP_DEMOSAIC_ATTR_S *demosaic_attr = NULL;
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *demosaic_demoire_attr = NULL;

	isp_demosaic_ctrl_get_demosaic_attr(ViPipe, &demosaic_attr);
	isp_demosaic_ctrl_get_demosaic_demoire_attr(ViPipe, &demosaic_demoire_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		demosaic_cfg->update = 0;
	} else {
		demosaic_cfg->update = 1;
		demosaic_cfg->cfa_enable = demosaic_attr->Enable && !runtime->is_module_bypass;
		//reg_cfa_edgee_thd = reg_cfa_edgee_thd2
		demosaic_cfg->cfa_edgee_thd2 = runtime->demosaic_attr.CoarseEdgeThr;
		demosaic_cfg->cfa_out_sel = demosaic_attr->TuningMode;
		demosaic_cfg->cfa_force_dir_enable = 0;
		demosaic_cfg->cfa_force_dir_sel = 0;
		demosaic_cfg->cfa_rbsig_luma_thd = runtime->demosaic_attr.RbSigLumaThd;
		memcpy(demosaic_cfg->cfa_ghp_lut, cfa_ghp_lut, sizeof(cfa_ghp_lut));
		demosaic_cfg->cfa_ymoire_enable = demosaic_demoire_attr->DetailSmoothEnable;
		demosaic_cfg->cfa_ymoire_dc_w = runtime->demosaic_demoire_attr.DetailSmoothStr;
		demosaic_cfg->cfa_ymoire_lpf_w = runtime->demosaic_demoire_attr.LumaWgt;

		struct cvi_isp_demosiac_tun_cfg *cfg_0 = &(demosaic_cfg->demosiac_cfg);

		cfg_0->REG_0C.bits.CFA_EDGEE_THD = runtime->demosaic_attr.CoarseEdgeThr;
		cfg_0->REG_0C.bits.CFA_SIGE_THD = runtime->demosaic_attr.FineEdgeThr;
		cfg_0->REG_10.bits.CFA_GSIG_TOL =
		cfg_0->REG_10.bits.CFA_RBSIG_TOL = runtime->demosaic_attr.FineStr;
		cfg_0->REG_14.bits.CFA_EDGE_TOL = runtime->demosaic_attr.CoarseStr;
		cfg_0->REG_18.bits.CFA_GHP_THD = 3580;	// TODO check this with Alvin
		cfg_0->REG_1C.bits.CFA_RB_VT_ENABLE = demosaic_attr->RbVtEnable;

		struct cvi_isp_demosiac_tun_1_cfg *cfg_1 = &(demosaic_cfg->demosiac_1_cfg);

		cfg_1->REG_120.bits.CFA_CMOIRE_ENABLE = demosaic_demoire_attr->AntiFalseColorEnable;
		cfg_1->REG_120.bits.CFA_CMOIRE_STRTH = runtime->demosaic_demoire_attr.AntiFalseColorStr;
		cfg_1->REG_124.bits.CFA_CMOIRE_SAT_X0 = runtime->demosaic_demoire_attr.SatGainIn[0];
		cfg_1->REG_124.bits.CFA_CMOIRE_SAT_Y0 = runtime->demosaic_demoire_attr.SatGainOut[0];
		cfg_1->REG_128.bits.CFA_CMOIRE_SAT_SLP0 = runtime->demosaic_param_out.SatGainSlope;
		cfg_1->REG_12C.bits.CFA_CMOIRE_SAT_X1 = runtime->demosaic_demoire_attr.SatGainIn[1];
		cfg_1->REG_12C.bits.CFA_CMOIRE_SAT_Y1 = runtime->demosaic_demoire_attr.SatGainOut[1];
		cfg_1->REG_130.bits.CFA_CMOIRE_PTCLR_X0 = runtime->demosaic_demoire_attr.ProtectColorGainIn[0];
		cfg_1->REG_130.bits.CFA_CMOIRE_PTCLR_Y0 = runtime->demosaic_demoire_attr.ProtectColorGainOut[0];
		cfg_1->REG_134.bits.CFA_CMOIRE_PTCLR_SLP0 = runtime->demosaic_param_out.ProtectColorGainSlope;
		cfg_1->REG_138.bits.CFA_CMOIRE_PTCLR_X1 = runtime->demosaic_demoire_attr.ProtectColorGainIn[1];
		cfg_1->REG_138.bits.CFA_CMOIRE_PTCLR_Y1 = runtime->demosaic_demoire_attr.ProtectColorGainOut[1];
		cfg_1->REG_13C.bits.CFA_CMOIRE_PROTCLR1_ENABLE =
		cfg_1->REG_13C.bits.CFA_CMOIRE_PROTCLR2_ENABLE =
		cfg_1->REG_13C.bits.CFA_CMOIRE_PROTCLR3_ENABLE = demosaic_demoire_attr->ProtectColorEnable;
		cfg_1->REG_140.bits.CFA_CMOIRE_PROTCLR1 = runtime->demosaic_demoire_attr.UserDefineProtectColor1;
		cfg_1->REG_140.bits.CFA_CMOIRE_PROTCLR2 = runtime->demosaic_demoire_attr.UserDefineProtectColor2;
		cfg_1->REG_144.bits.CFA_CMOIRE_PROTCLR3 = runtime->demosaic_demoire_attr.UserDefineProtectColor3;
		cfg_1->REG_144.bits.CFA_CMOIRE_PD_X0 = runtime->demosaic_demoire_attr.EdgeGainIn[0];
		cfg_1->REG_148.bits.CFA_CMOIRE_PD_Y0 = runtime->demosaic_demoire_attr.EdgeGainOut[0];
		cfg_1->REG_14C.bits.CFA_CMOIRE_PD_SLP0 = runtime->demosaic_param_out.EdgeGainSlope;
		cfg_1->REG_150.bits.CFA_CMOIRE_PD_X1 = runtime->demosaic_demoire_attr.EdgeGainIn[1];
		cfg_1->REG_150.bits.CFA_CMOIRE_PD_Y1 = runtime->demosaic_demoire_attr.EdgeGainOut[1];
		cfg_1->REG_154.bits.CFA_CMOIRE_EDGE_X0 = runtime->demosaic_demoire_attr.DetailGainIn[0];
		cfg_1->REG_154.bits.CFA_CMOIRE_EDGE_Y0 = runtime->demosaic_demoire_attr.DetailGaintOut[0];
		cfg_1->REG_158.bits.CFA_CMOIRE_EDGE_SLP0 = runtime->demosaic_param_out.DetailGainSlope;
		cfg_1->REG_15C.bits.CFA_CMOIRE_EDGE_X1 = runtime->demosaic_demoire_attr.DetailGainIn[1];
		cfg_1->REG_15C.bits.CFA_CMOIRE_EDGE_Y1 = runtime->demosaic_demoire_attr.DetailGaintOut[1];
		cfg_1->REG_160.bits.CFA_CMOIRE_LUMAGAIN_ENABLE = demosaic_demoire_attr->DetailDetectLumaEnable;
		cfg_1->REG_164.bits.CFA_CMOIRE_LUMATG = runtime->demosaic_demoire_attr.DetailDetectLumaStr;
		cfg_1->REG_168.bits.CFA_CMOIRE_EDGE_D0C0 = 308;
		cfg_1->REG_168.bits.CFA_CMOIRE_EDGE_D0C1 = -205;
		cfg_1->REG_16C.bits.CFA_CMOIRE_EDGE_D0C2 = 51;
		cfg_1->REG_16C.bits.CFA_CMOIRE_EDGE_D45C0 = 492;
		cfg_1->REG_170.bits.CFA_CMOIRE_EDGE_D45C1 = 492;
		cfg_1->REG_170.bits.CFA_CMOIRE_EDGE_D45C2 = 84;
		cfg_1->REG_174.bits.CFA_CMOIRE_EDGE_D45C3 = -112;
		cfg_1->REG_174.bits.CFA_CMOIRE_EDGE_D45C4 = -98;
		cfg_1->REG_178.bits.CFA_CMOIRE_EDGE_D45C5 = -201;
		cfg_1->REG_178.bits.CFA_CMOIRE_EDGE_D45C6 = -201;
		cfg_1->REG_17C.bits.CFA_CMOIRE_EDGE_D45C7 = 92;
		cfg_1->REG_17C.bits.CFA_CMOIRE_EDGE_D45C8 = 18;
		cfg_1->REG_180.bits._CFA_SHPN_ENABLE = 1;
		cfg_1->REG_180.bits._CFA_SHPN_PRE_PROC_ENABLE = 1;
		cfg_1->REG_180.bits._CFA_SHPN_MIN_Y = 3;
		cfg_1->REG_184.bits._CFA_SHPN_MIN_GAIN = 512;
		cfg_1->REG_184.bits._CFA_SHPN_MAX_GAIN = 2048;
		cfg_1->REG_188.bits.CFA_SHPN_MF_CORE_GAIN = 128;
		cfg_1->REG_188.bits.CFA_SHPN_HF_BLEND_WGT = 64;
		cfg_1->REG_188.bits.CFA_SHPN_MF_BLEND_WGT = 64;
		cfg_1->REG_18C.bits.CFA_SHPN_CORE_VALUE = 16;

		struct cvi_isp_demosiac_tun_2_cfg *cfg_2 = &(demosaic_cfg->demosiac_2_cfg);

		cfg_2->REG_90.bits.CFA_YMOIRE_REF_MAXG_ONLY = demosaic_demoire_attr->DetailMode;
		cfg_2->REG_90.bits.CFA_YMOIRE_NP = runtime->demosaic_demoire_attr.EdgeWgtNp;
		cfg_2->REG_94.bits.CFA_YMOIRE_DETAIL_TH = runtime->demosaic_demoire_attr.DetailWgtThr;
		cfg_2->REG_94.bits.CFA_YMOIRE_DETAIL_LOW = runtime->demosaic_demoire_attr.DetailWgtMin;
		cfg_2->REG_98.bits.CFA_YMOIRE_DETAIL_HIGH = runtime->demosaic_demoire_attr.DetailWgtMax;
		cfg_2->REG_98.bits.CFA_YMOIRE_DETAIL_SLOPE = runtime->demosaic_demoire_attr.DetailWgtSlope;
		cfg_2->REG_9C.bits.CFA_YMOIRE_EDGE_TH = runtime->demosaic_demoire_attr.EdgeWgtThr;
		cfg_2->REG_9C.bits.CFA_YMOIRE_EDGE_LOW = runtime->demosaic_demoire_attr.EdgeWgtMin;
		cfg_2->REG_A0.bits.CFA_YMOIRE_EDGE_HIGH = runtime->demosaic_demoire_attr.EdgeWgtMax;
		cfg_2->REG_A0.bits.CFA_YMOIRE_EDGE_SLOPE = runtime->demosaic_demoire_attr.EdgeWgtSlope;
		cfg_2->REG_A4.bits.CFA_YMOIRE_LUT_TH = runtime->demosaic_demoire_attr.DetailSmoothMapTh;
		cfg_2->REG_A4.bits.CFA_YMOIRE_LUT_LOW = runtime->demosaic_demoire_attr.DetailSmoothMapMin;
		cfg_2->REG_A8.bits.CFA_YMOIRE_LUT_HIGH = runtime->demosaic_demoire_attr.DetailSmoothMapMax;
		cfg_2->REG_A8.bits.CFA_YMOIRE_LUT_SLOPE = runtime->demosaic_demoire_attr.DetailSmoothMapSlope;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_demosaic_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_DEMOSAIC_ATTR_S *demosaic_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_demosaic_ctrl_get_demosaic_attr(ViPipe, &demosaic_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DemosaicEnable = demosaic_attr->Enable;
		pProcST->DemosaicisManualMode = demosaic_attr->enOpType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_demosaic_ctrl_runtime  *_get_demosaic_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_demosaic_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DEMOSAIC, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_demosaic_ctrl_check_demosaic_attr_valid(const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDemosaicAttr, Enable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstDemosaicAttr, TuningMode, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDemosaicAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDemosaicAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstDemosaicAttr, CoarseEdgeThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstDemosaicAttr, CoarseStr, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstDemosaicAttr, FineEdgeThr, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstDemosaicAttr, FineStr, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstDemosaicAttr, FilterMode, 0x0, 0x1);

	return ret;
}

static CVI_S32 isp_demosaic_ctrl_check_demosaic_demoire_attr_valid(
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_AUTO_ISO_1D(pstDemosaicDemoireAttr, DetailSmoothEnable, 0x0, 0x1);
	// CHECK_VALID_AUTO_ISO_1D(pstDemosaicDemoireAttr, DetailSmoothStr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstDemosaicDemoireAttr, EdgeWgtStr, 0x0, 0xff);

	UNUSED(pstDemosaicDemoireAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_demosaic_ctrl_get_demosaic_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_ATTR_S **pstDemosaicAttr)
{
	if (pstDemosaicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DEMOSAIC, (CVI_VOID *) &shared_buffer);
	*pstDemosaicAttr = &shared_buffer->stDemosaicAttr;

	return ret;
}

CVI_S32 isp_demosaic_ctrl_set_demosaic_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr)
{
	if (pstDemosaicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_demosaic_ctrl_check_demosaic_attr_valid(pstDemosaicAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DEMOSAIC_ATTR_S *p = CVI_NULL;

	isp_demosaic_ctrl_get_demosaic_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDemosaicAttr, sizeof(*pstDemosaicAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_demosaic_ctrl_get_demosaic_demoire_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S **pstDemosaicDemoireAttr)
{
	if (pstDemosaicDemoireAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DEMOSAIC, (CVI_VOID *) &shared_buffer);
	*pstDemosaicDemoireAttr = &shared_buffer->stDemoireAttr;

	return ret;
}

CVI_S32 isp_demosaic_ctrl_set_demosaic_demoire_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr)
{
	if (pstDemosaicDemoireAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_demosaic_ctrl_runtime *runtime = _get_demosaic_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_demosaic_ctrl_check_demosaic_demoire_attr_valid(pstDemosaicDemoireAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *p = CVI_NULL;

	isp_demosaic_ctrl_get_demosaic_demoire_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDemosaicDemoireAttr, sizeof(*pstDemosaicDemoireAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}


