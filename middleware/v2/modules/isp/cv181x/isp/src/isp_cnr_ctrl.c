/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_cnr_ctrl.c
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

#include "isp_cnr_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl cnr_mod = {
	.init = isp_cnr_ctrl_init,
	.uninit = isp_cnr_ctrl_uninit,
	.suspend = isp_cnr_ctrl_suspend,
	.resume = isp_cnr_ctrl_resume,
	.ctrl = isp_cnr_ctrl_ctrl
};


static CVI_S32 isp_cnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_cnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_cnr_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_cnr_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_cnr_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_cnr_ctrl_check_cnr_attr_valid(const ISP_CNR_ATTR_S *pstCNRAttr);
static CVI_S32 isp_cnr_ctrl_check_cnr_motion_attr_valid(const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr);

static struct isp_cnr_ctrl_runtime  *_get_cnr_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_cnr_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_cnr_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cnr_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cnr_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_cnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_cnr_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCnr;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCnr = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_cnr_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_cnr_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_cnr_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_cnr_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_cnr_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_cnr_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CNR_ATTR_S *cnr_attr = NULL;
	const ISP_CNR_MOTION_NR_ATTR_S *cnr_motion_attr = NULL;

	isp_cnr_ctrl_get_cnr_attr(ViPipe, &cnr_attr);
	isp_cnr_ctrl_get_cnr_motion_attr(ViPipe, &cnr_motion_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(cnr_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!cnr_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (cnr_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(cnr_attr, CnrStr);
		MANUAL(cnr_attr, NoiseSuppressStr);
		MANUAL(cnr_attr, NoiseSuppressGain);
		MANUAL(cnr_attr, FilterType);
		MANUAL(cnr_attr, MotionNrStr);
		MANUAL(cnr_attr, LumaWgt);
		MANUAL(cnr_attr, DetailSmoothMode);

		for (CVI_U32 idx = 0 ; idx < CNR_MOTION_LUT_NUM ; idx++) {
			MANUAL(cnr_motion_attr, MotionCnrCoringLut[idx]);
		}
		for (CVI_U32 idx = 0 ; idx < CNR_MOTION_LUT_NUM ; idx++) {
			MANUAL(cnr_motion_attr, MotionCnrStrLut[idx]);
		}

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(cnr_attr, CnrStr, INTPLT_POST_ISO);
		AUTO(cnr_attr, NoiseSuppressStr, INTPLT_POST_ISO);
		AUTO(cnr_attr, NoiseSuppressGain, INTPLT_POST_ISO);
		AUTO(cnr_attr, FilterType, INTPLT_POST_ISO);
		AUTO(cnr_attr, MotionNrStr, INTPLT_POST_ISO);
		AUTO(cnr_attr, LumaWgt, INTPLT_POST_ISO);
		AUTO(cnr_attr, DetailSmoothMode, INTPLT_POST_ISO);

		for (CVI_U32 idx = 0 ; idx < CNR_MOTION_LUT_NUM ; idx++) {
			AUTO(cnr_motion_attr, MotionCnrCoringLut[idx], INTPLT_POST_ISO);
		}
		for (CVI_U32 idx = 0 ; idx < CNR_MOTION_LUT_NUM ; idx++) {
			AUTO(cnr_motion_attr, MotionCnrStrLut[idx], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	// ParamIn
	runtime->cnr_param_in.filter_type = runtime->cnr_attr.FilterType;
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_cnr_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_cnr_main(
		(struct cnr_param_in *)&runtime->cnr_param_in,
		(struct cnr_param_out *)&runtime->cnr_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_cnr_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_cnr_config *cnr_cfg =
		(struct cvi_vip_isp_cnr_config *)&(post_addr->tun_cfg[tun_idx].cnr_cfg);

	const ISP_CNR_ATTR_S *cnr_attr = NULL;
	const ISP_CNR_MOTION_NR_ATTR_S *cnr_motion_attr = NULL;

	isp_cnr_ctrl_get_cnr_attr(ViPipe, &cnr_attr);
	isp_cnr_ctrl_get_cnr_motion_attr(ViPipe, &cnr_motion_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		cnr_cfg->update = 0;
	} else {
		cnr_cfg->update = 1;
		cnr_cfg->enable = cnr_attr->Enable && !runtime->is_module_bypass;
		cnr_cfg->strength_mode = runtime->cnr_attr.CnrStr;
		cnr_cfg->diff_shift_val = runtime->cnr_attr.NoiseSuppressStr;

		CVI_U8 val = 9 - runtime->cnr_attr.NoiseSuppressGain;

		cnr_cfg->diff_gain = MINMAX(val, 1, 8);
		cnr_cfg->ratio = runtime->cnr_attr.MotionNrStr;
		cnr_cfg->fusion_intensity_weight = runtime->cnr_attr.LumaWgt;
		cnr_cfg->flag_neighbor_max_weight = runtime->cnr_attr.DetailSmoothMode;

		memcpy(cnr_cfg->weight_lut_inter,
			runtime->cnr_param_out.weight_lut, sizeof(CVI_U8) * CNR_MOTION_LUT_NUM);
		cnr_cfg->motion_enable = cnr_motion_attr->MotionCnrEnable;
		memcpy(cnr_cfg->coring_motion_lut, runtime->cnr_motion_attr.MotionCnrCoringLut,
			sizeof(CVI_U8) * CNR_MOTION_LUT_NUM);
		memcpy(cnr_cfg->motion_lut,
			runtime->cnr_motion_attr.MotionCnrStrLut, sizeof(CVI_U8) * CNR_MOTION_LUT_NUM);
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_cnr_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

		const ISP_CNR_ATTR_S *cnr_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_cnr_ctrl_get_cnr_attr(ViPipe, &cnr_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->CNREnable = cnr_attr->Enable;
		pProcST->CNRisManualMode = cnr_attr->enOpType;
		//manual or auto
		pProcST->CNRNoiseSuppressGain = runtime->cnr_attr.NoiseSuppressGain;
		pProcST->CNRFilterType = runtime->cnr_attr.FilterType;
		pProcST->CNRMotionNrStr = runtime->cnr_attr.MotionNrStr;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_cnr_ctrl_runtime  *_get_cnr_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_cnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CNR, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_cnr_ctrl_check_cnr_attr_valid(const ISP_CNR_ATTR_S *pstCNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCNRAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCNRAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCNRAttr, UpdateInterval, 0, 0xff);

	// CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, CnrStr, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, NoiseSuppressStr, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, NoiseSuppressGain, 0x1, 0x8);
	CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, FilterType, 0x0, 0x1f);
	// CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, MotionNrStr, 0x0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, LumaWgt, 0x0, 0x8);
	CHECK_VALID_AUTO_ISO_1D(pstCNRAttr, DetailSmoothMode, 0x0, 0x1);

	return ret;
}

static CVI_S32 isp_cnr_ctrl_check_cnr_motion_attr_valid(const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCNRMotionNRAttr, MotionCnrEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_AUTO_ISO_2D(pstCNRMotionNRAttr, MotionCnrCoringLut, CNR_MOTION_LUT_NUM, 0x0, 0xff);
	// CHECK_VALID_AUTO_ISO_2D(pstCNRMotionNRAttr, MotionCnrStrLut, CNR_MOTION_LUT_NUM, 0x0, 0xff);

	UNUSED(pstCNRMotionNRAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_cnr_ctrl_get_cnr_attr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S **pstCNRAttr)
{
	if (pstCNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_cnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CNR, (CVI_VOID *) &shared_buffer);
	*pstCNRAttr = &shared_buffer->stCNRAttr;

	return ret;
}

CVI_S32 isp_cnr_ctrl_set_cnr_attr(VI_PIPE ViPipe, const ISP_CNR_ATTR_S *pstCNRAttr)
{
	if (pstCNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_cnr_ctrl_check_cnr_attr_valid(pstCNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CNR_ATTR_S *p = CVI_NULL;

	isp_cnr_ctrl_get_cnr_attr(ViPipe, &p);
	memcpy((void *)p, pstCNRAttr, sizeof(*pstCNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_cnr_ctrl_get_cnr_motion_attr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S **pstCNRMotionNRAttr)
{
	if (pstCNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_cnr_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CNR, (CVI_VOID *) &shared_buffer);
	*pstCNRMotionNRAttr = &shared_buffer->stCNRMotionNRAttr;

	return ret;
}

CVI_S32 isp_cnr_ctrl_set_cnr_motion_attr(VI_PIPE ViPipe, const ISP_CNR_MOTION_NR_ATTR_S *pstCNRMotionNRAttr)
{
	if (pstCNRMotionNRAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_cnr_ctrl_runtime *runtime = _get_cnr_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_cnr_ctrl_check_cnr_motion_attr_valid(pstCNRMotionNRAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CNR_MOTION_NR_ATTR_S *p = CVI_NULL;

	isp_cnr_ctrl_get_cnr_motion_attr(ViPipe, &p);
	memcpy((void *)p, pstCNRMotionNRAttr, sizeof(*pstCNRMotionNRAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

