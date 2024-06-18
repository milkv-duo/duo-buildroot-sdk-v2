/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_bnr_ctrl.c
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
#include "cvi_isp.h"

#include "isp_bnr_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl bnr_mod = {
	.init = isp_bnr_ctrl_init,
	.uninit = isp_bnr_ctrl_uninit,
	.suspend = isp_bnr_ctrl_suspend,
	.resume = isp_bnr_ctrl_resume,
	.ctrl = isp_bnr_ctrl_ctrl
};

CVI_U8 lsc_gain_lut_default[32] = {
	32, 32, 32, 32, 32, 32, 32, 33, 33, 34, 34, 35, 36, 37, 38, 39,
	40, 41, 43, 44, 45, 47, 49, 51, 53, 55, 57, 59, 61, 64, 66, 69
};


static CVI_S32 isp_bnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_bnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_bnr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_bnr_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_bnr_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_bnr_ctrl_check_bnr_attr_valid(const ISP_NR_ATTR_S *pstNRAttr);
static CVI_S32 isp_bnr_ctrl_check_bnr_filter_attr_valid(const ISP_NR_FILTER_ATTR_S *pstNRFilterAttr);

static struct isp_bnr_ctrl_runtime  *_get_bnr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_bnr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_bnr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_bnr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_bnr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_bnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_bnr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassBnr;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassBnr = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_bnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bnr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_bnr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_bnr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_bnr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_bnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_NR_ATTR_S *bnr_attr = NULL;
	const ISP_NR_FILTER_ATTR_S *bnr_filter_attr = NULL;

	isp_bnr_ctrl_get_bnr_attr(ViPipe, &bnr_attr);
	isp_bnr_ctrl_get_bnr_filter_attr(ViPipe, &bnr_filter_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(bnr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!bnr_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (bnr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(bnr_attr, WindowType);
		MANUAL(bnr_attr, DetailSmoothMode);
		MANUAL(bnr_attr, NoiseSuppressStr);
		MANUAL(bnr_attr, FilterType);
		MANUAL(bnr_attr, NoiseSuppressStrMode);

		MANUAL(bnr_filter_attr, LumaStr[0]);
		MANUAL(bnr_filter_attr, LumaStr[1]);
		MANUAL(bnr_filter_attr, LumaStr[2]);
		MANUAL(bnr_filter_attr, LumaStr[3]);
		MANUAL(bnr_filter_attr, LumaStr[4]);
		MANUAL(bnr_filter_attr, LumaStr[5]);
		MANUAL(bnr_filter_attr, LumaStr[6]);
		MANUAL(bnr_filter_attr, LumaStr[7]);
		MANUAL(bnr_filter_attr, VarThr);
		MANUAL(bnr_filter_attr, CoringWgtLF);
		MANUAL(bnr_filter_attr, CoringWgtHF);
		MANUAL(bnr_filter_attr, NonDirFiltStr);
		MANUAL(bnr_filter_attr, VhDirFiltStr);
		MANUAL(bnr_filter_attr, AaDirFiltStr);
		MANUAL(bnr_filter_attr, NpSlopeR);
		MANUAL(bnr_filter_attr, NpSlopeGr);
		MANUAL(bnr_filter_attr, NpSlopeGb);
		MANUAL(bnr_filter_attr, NpSlopeB);
		MANUAL(bnr_filter_attr, NpLumaThrR);
		MANUAL(bnr_filter_attr, NpLumaThrGr);
		MANUAL(bnr_filter_attr, NpLumaThrGb);
		MANUAL(bnr_filter_attr, NpLumaThrB);
		MANUAL(bnr_filter_attr, NpLowOffsetR);
		MANUAL(bnr_filter_attr, NpLowOffsetGr);
		MANUAL(bnr_filter_attr, NpLowOffsetGb);
		MANUAL(bnr_filter_attr, NpLowOffsetB);
		MANUAL(bnr_filter_attr, NpHighOffsetR);
		MANUAL(bnr_filter_attr, NpHighOffsetGr);
		MANUAL(bnr_filter_attr, NpHighOffsetGb);
		MANUAL(bnr_filter_attr, NpHighOffsetB);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(bnr_attr, WindowType, INTPLT_POST_ISO);
		AUTO(bnr_attr, DetailSmoothMode, INTPLT_POST_ISO);
		AUTO(bnr_attr, NoiseSuppressStr, INTPLT_POST_ISO);
		AUTO(bnr_attr, FilterType, INTPLT_POST_ISO);
		AUTO(bnr_attr, NoiseSuppressStrMode, INTPLT_POST_ISO);

		AUTO(bnr_filter_attr, LumaStr[0], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[1], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[2], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[3], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[4], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[5], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[6], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, LumaStr[7], INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, VarThr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, CoringWgtLF, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, CoringWgtHF, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NonDirFiltStr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, VhDirFiltStr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, AaDirFiltStr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpSlopeR, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpSlopeGr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpSlopeGb, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpSlopeB, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLumaThrR, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLumaThrGr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLumaThrGb, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLumaThrB, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLowOffsetR, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLowOffsetGr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLowOffsetGb, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpLowOffsetB, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpHighOffsetR, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpHighOffsetGr, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpHighOffsetGb, INTPLT_POST_ISO);
		AUTO(bnr_filter_attr, NpHighOffsetB, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	CVI_ISP_GetNoiseProfileAttr(ViPipe, &runtime->bnr_param_in.np);
	runtime->bnr_param_in.iso = algoResult->u32PostNpIso;
	runtime->bnr_param_in.ispdgain = algoResult->u32IspPostDgain;
	runtime->bnr_param_in.wb_rgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_R];
	runtime->bnr_param_in.wb_bgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_B];

	runtime->bnr_param_in.window_type = runtime->bnr_attr.WindowType;
	runtime->bnr_param_in.filter_type = runtime->bnr_attr.FilterType;

	runtime->bnr_param_in.CoringParamEnable = bnr_attr->CoringParamEnable;

	runtime->bnr_param_in.NpSlope[ISP_BAYER_CHN_R] = runtime->bnr_filter_attr.NpSlopeR;
	runtime->bnr_param_in.NpSlope[ISP_BAYER_CHN_GR] = runtime->bnr_filter_attr.NpSlopeGr;
	runtime->bnr_param_in.NpSlope[ISP_BAYER_CHN_GB] = runtime->bnr_filter_attr.NpSlopeGb;
	runtime->bnr_param_in.NpSlope[ISP_BAYER_CHN_B] = runtime->bnr_filter_attr.NpSlopeB;

	runtime->bnr_param_in.NpLumaThr[ISP_BAYER_CHN_R] = runtime->bnr_filter_attr.NpLumaThrR;
	runtime->bnr_param_in.NpLumaThr[ISP_BAYER_CHN_GR] = runtime->bnr_filter_attr.NpLumaThrGr;
	runtime->bnr_param_in.NpLumaThr[ISP_BAYER_CHN_GB] = runtime->bnr_filter_attr.NpLumaThrGb;
	runtime->bnr_param_in.NpLumaThr[ISP_BAYER_CHN_B] = runtime->bnr_filter_attr.NpLumaThrB;

	runtime->bnr_param_in.NpLowOffset[ISP_BAYER_CHN_R] = runtime->bnr_filter_attr.NpLowOffsetR;
	runtime->bnr_param_in.NpLowOffset[ISP_BAYER_CHN_GR] = runtime->bnr_filter_attr.NpLowOffsetGr;
	runtime->bnr_param_in.NpLowOffset[ISP_BAYER_CHN_GB] = runtime->bnr_filter_attr.NpLowOffsetGb;
	runtime->bnr_param_in.NpLowOffset[ISP_BAYER_CHN_B] = runtime->bnr_filter_attr.NpLowOffsetB;

	runtime->bnr_param_in.NpHighOffset[ISP_BAYER_CHN_R] = runtime->bnr_filter_attr.NpHighOffsetR;
	runtime->bnr_param_in.NpHighOffset[ISP_BAYER_CHN_GR] = runtime->bnr_filter_attr.NpHighOffsetGr;
	runtime->bnr_param_in.NpHighOffset[ISP_BAYER_CHN_GB] = runtime->bnr_filter_attr.NpHighOffsetGb;
	runtime->bnr_param_in.NpHighOffset[ISP_BAYER_CHN_B] = runtime->bnr_filter_attr.NpHighOffsetB;
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_bnr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_bnr_main(
		(struct bnr_param_in *)&runtime->bnr_param_in,
		(struct bnr_param_out *)&runtime->bnr_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_bnr_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_bnr_config *bnr_cfg =
		(struct cvi_vip_isp_bnr_config *)&(post_addr->tun_cfg[tun_idx].bnr_cfg);

	const ISP_NR_ATTR_S *bnr_attr = NULL;
	const ISP_NR_FILTER_ATTR_S *bnr_filter_attr = NULL;

	isp_bnr_ctrl_get_bnr_attr(ViPipe, &bnr_attr);
	isp_bnr_ctrl_get_bnr_filter_attr(ViPipe, &bnr_filter_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		bnr_cfg->update = 0;
	} else {
		bnr_cfg->update = 1;
		bnr_cfg->enable = bnr_attr->Enable && !runtime->is_module_bypass;
		bnr_cfg->out_sel = bnr_filter_attr->TuningMode;
		memcpy(bnr_cfg->weight_lut, runtime->bnr_param_out.weight_lut, sizeof(CVI_U8) * 256);
		memcpy(bnr_cfg->intensity_sel, runtime->bnr_filter_attr.LumaStr, sizeof(CVI_U8) * 8);
		bnr_cfg->weight_intra_0 = runtime->bnr_param_out.kernel.reg_bnr_weight_intra_0;
		bnr_cfg->weight_intra_1 = runtime->bnr_param_out.kernel.reg_bnr_weight_intra_1;
		bnr_cfg->weight_intra_2 = runtime->bnr_param_out.kernel.reg_bnr_weight_intra_2;
		bnr_cfg->weight_norm_1 = runtime->bnr_param_out.kernel.reg_bnr_weight_norm_intra1;
		bnr_cfg->weight_norm_2 = runtime->bnr_param_out.kernel.reg_bnr_weight_norm_intra2;
		bnr_cfg->k_smooth = runtime->bnr_filter_attr.CoringWgtLF;
		bnr_cfg->k_texture = runtime->bnr_filter_attr.CoringWgtHF;

		struct cvi_isp_bnr_tun_1_cfg *cfg_1 = &(bnr_cfg->bnr_1_cfg);

		cfg_1->NS_LUMA_TH_R.bits.BNR_NS_LUMA_TH_R = runtime->bnr_param_out.NpLumaThr[ISP_BAYER_CHN_R];
		cfg_1->NS_SLOPE_R.bits.BNR_NS_SLOPE_R = runtime->bnr_param_out.NpSlope[ISP_BAYER_CHN_R];
		cfg_1->NS_OFFSET0_R.bits.BNR_NS_LOW_OFFSET_R = runtime->bnr_param_out.NpLowOffset[ISP_BAYER_CHN_R];
		cfg_1->NS_OFFSET1_R.bits.BNR_NS_HIGH_OFFSET_R = runtime->bnr_param_out.NpHighOffset[ISP_BAYER_CHN_R];
		cfg_1->NS_LUMA_TH_GR.bits.BNR_NS_LUMA_TH_GR = runtime->bnr_param_out.NpLumaThr[ISP_BAYER_CHN_GR];
		cfg_1->NS_SLOPE_GR.bits.BNR_NS_SLOPE_GR = runtime->bnr_param_out.NpSlope[ISP_BAYER_CHN_GR];
		cfg_1->NS_OFFSET0_GR.bits.BNR_NS_LOW_OFFSET_GR = runtime->bnr_param_out.NpLowOffset[ISP_BAYER_CHN_GR];
		cfg_1->NS_OFFSET1_GR.bits.BNR_NS_HIGH_OFFSET_GR = runtime->bnr_param_out.NpHighOffset[ISP_BAYER_CHN_GR];
		cfg_1->NS_LUMA_TH_GB.bits.BNR_NS_LUMA_TH_GB = runtime->bnr_param_out.NpLumaThr[ISP_BAYER_CHN_GB];
		cfg_1->NS_SLOPE_GB.bits.BNR_NS_SLOPE_GB = runtime->bnr_param_out.NpSlope[ISP_BAYER_CHN_GB];
		cfg_1->NS_OFFSET0_GB.bits.BNR_NS_LOW_OFFSET_GB = runtime->bnr_param_out.NpLowOffset[ISP_BAYER_CHN_GB];
		cfg_1->NS_OFFSET1_GB.bits.BNR_NS_HIGH_OFFSET_GB = runtime->bnr_param_out.NpHighOffset[ISP_BAYER_CHN_GB];
		cfg_1->NS_LUMA_TH_B.bits.BNR_NS_LUMA_TH_B = runtime->bnr_param_out.NpLumaThr[ISP_BAYER_CHN_B];
		cfg_1->NS_SLOPE_B.bits.BNR_NS_SLOPE_B = runtime->bnr_param_out.NpSlope[ISP_BAYER_CHN_B];
		cfg_1->NS_OFFSET0_B.bits.BNR_NS_LOW_OFFSET_B = runtime->bnr_param_out.NpLowOffset[ISP_BAYER_CHN_B];
		cfg_1->NS_OFFSET1_B.bits.BNR_NS_HIGH_OFFSET_B = runtime->bnr_param_out.NpHighOffset[ISP_BAYER_CHN_B];
		cfg_1->NS_GAIN.bits.BNR_NS_GAIN = runtime->bnr_attr.NoiseSuppressStr;
		cfg_1->STRENGTH_MODE.bits.BNR_STRENGTH_MODE = runtime->bnr_attr.NoiseSuppressStrMode;

		struct cvi_isp_bnr_tun_2_cfg *cfg_2 = &(bnr_cfg->bnr_2_cfg);

		cfg_2->VAR_TH.bits.BNR_VAR_TH = runtime->bnr_filter_attr.VarThr;
		cfg_2->WEIGHT_SM.bits.BNR_WEIGHT_SMOOTH = runtime->bnr_filter_attr.NonDirFiltStr;
		cfg_2->WEIGHT_V.bits.BNR_WEIGHT_V =
		cfg_2->WEIGHT_H.bits.BNR_WEIGHT_H = runtime->bnr_filter_attr.VhDirFiltStr;
		cfg_2->WEIGHT_D45.bits.BNR_WEIGHT_D45 =
		cfg_2->WEIGHT_D135.bits.BNR_WEIGHT_D135 = runtime->bnr_filter_attr.AaDirFiltStr;
		cfg_2->NEIGHBOR_MAX.bits.BNR_FLAG_NEIGHBOR_MAX = runtime->bnr_attr.DetailSmoothMode;
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_bnr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	/*BNR Save the value to debugInfoST start */
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		const ISP_NR_ATTR_S * bnr_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_bnr_ctrl_get_bnr_attr(ViPipe, &bnr_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->BNREnable = bnr_attr->Enable;
		pProcST->BNRisManualMode = bnr_attr->enOpType;
		//manual or auto
		pProcST->BNRWindowType = runtime->bnr_attr.WindowType;
		pProcST->BNRFilterType = runtime->bnr_attr.FilterType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_bnr_ctrl_runtime  *_get_bnr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_BNR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_bnr_ctrl_check_bnr_attr_valid(const ISP_NR_ATTR_S *pstNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstNRAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstNRAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstNRAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstNRAttr, CoringParamEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_1D(pstNRAttr, WindowType, 0x0, 0xb);
	CHECK_VALID_AUTO_ISO_1D(pstNRAttr, DetailSmoothMode, 0x0, 0x1);
	// CHECK_VALID_AUTO_ISO_1D(pstNRAttr, NoiseSuppressStr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstNRAttr, FilterType, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstNRAttr, NoiseSuppressStrMode, 0x0, 0xff);

	return ret;
}

static CVI_S32 isp_bnr_ctrl_check_bnr_filter_attr_valid(const ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstNRFilterAttr, TuningMode, 0x0, 0xf);
	CHECK_VALID_AUTO_ISO_2D(pstNRFilterAttr, LumaStr, 8, 0x0, 0x1f);
	// CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, VarThr, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, CoringWgtLF, 0x0, 0x100);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, CoringWgtHF, 0x0, 0x100);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NonDirFiltStr, 0x0, 0x1f);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, VhDirFiltStr, 0x0, 0x1f);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, AaDirFiltStr, 0x0, 0x1f);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpSlopeR, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpSlopeGr, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpSlopeGb, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpSlopeB, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLumaThrR, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLumaThrGr, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLumaThrGb, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLumaThrB, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLowOffsetR, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLowOffsetGr, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLowOffsetGb, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpLowOffsetB, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpHighOffsetR, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpHighOffsetGr, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpHighOffsetGb, 0x0, 0x3ff);
	CHECK_VALID_AUTO_ISO_1D(pstNRFilterAttr, NpHighOffsetB, 0x0, 0x3ff);

	return ret;
}


//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_bnr_ctrl_get_bnr_attr(VI_PIPE ViPipe, const ISP_NR_ATTR_S **pstNRAttr)
{
	if (pstNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_BNR, (CVI_VOID *) &shared_buffer);

	*pstNRAttr = &shared_buffer->stNRAttr;

	return ret;
}

CVI_S32 isp_bnr_ctrl_set_bnr_attr(VI_PIPE ViPipe, const ISP_NR_ATTR_S *pstNRAttr)
{
	if (pstNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_bnr_ctrl_check_bnr_attr_valid(pstNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_NR_ATTR_S *p = CVI_NULL;

	isp_bnr_ctrl_get_bnr_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstNRAttr, sizeof(*pstNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_bnr_ctrl_get_bnr_filter_attr(VI_PIPE ViPipe, const ISP_NR_FILTER_ATTR_S **pstNRFilterAttr)
{
	if (pstNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_BNR, (CVI_VOID *) &shared_buffer);

	*pstNRFilterAttr = &shared_buffer->stNRFilterAttr;

	return ret;
}

CVI_S32 isp_bnr_ctrl_set_bnr_filter_attr(VI_PIPE ViPipe, const ISP_NR_FILTER_ATTR_S *pstNRFilterAttr)
{
	if (pstNRFilterAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_bnr_ctrl_runtime *runtime = _get_bnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_bnr_ctrl_check_bnr_filter_attr_valid(pstNRFilterAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_NR_FILTER_ATTR_S *p = CVI_NULL;

	isp_bnr_ctrl_get_bnr_filter_attr(ViPipe, &p);
	memcpy((CVI_VOID *)p, pstNRFilterAttr, sizeof(*pstNRFilterAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}


