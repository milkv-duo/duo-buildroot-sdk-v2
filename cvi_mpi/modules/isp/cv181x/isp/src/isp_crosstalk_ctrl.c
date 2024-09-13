/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_crosstalk_ctrl.c
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

#include "isp_crosstalk_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl crosstalk_mod = {
	.init = isp_crosstalk_ctrl_init,
	.uninit = isp_crosstalk_ctrl_uninit,
	.suspend = isp_crosstalk_ctrl_suspend,
	.resume = isp_crosstalk_ctrl_resume,
	.ctrl = isp_crosstalk_ctrl_ctrl
};


static CVI_S32 isp_crosstalk_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_crosstalk_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_crosstalk_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_crosstalk_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_crosstalk_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_crosstalk_ctrl_check_crosstalk_attr_valid(const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr);

static struct isp_crosstalk_ctrl_runtime  *_get_crosstalk_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_crosstalk_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_crosstalk_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_crosstalk_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_crosstalk_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_crosstalk_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_crosstalk_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCrosstalk;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCrosstalk = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_crosstalk_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_crosstalk_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_crosstalk_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_crosstalk_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_crosstalk_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_crosstalk_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CROSSTALK_ATTR_S *crosstalk_attr = NULL;

	isp_crosstalk_ctrl_get_crosstalk_attr(ViPipe, &crosstalk_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(crosstalk_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!crosstalk_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (crosstalk_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(crosstalk_attr, Strength);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(crosstalk_attr, Strength, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

#if 0
static CVI_S32 isp_crosstalk_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_crosstalk_main(
		(struct crosstalk_param_in *)&runtime->crosstalk_param_in,
		(struct crosstalk_param_out *)&runtime->crosstalk_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_crosstalk_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ge_config *ge_cfg[2] = {
		(struct cvi_vip_isp_ge_config *)&(pre_be_addr->tun_cfg[tun_idx].ge_cfg[0]),
		(struct cvi_vip_isp_ge_config *)&(pre_be_addr->tun_cfg[tun_idx].ge_cfg[1]),
	};

	const ISP_CROSSTALK_ATTR_S *crosstalk_attr = NULL;

	isp_crosstalk_ctrl_get_crosstalk_attr(ViPipe, &crosstalk_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		for (CVI_U32 idx = 0 ; idx < 2 ; idx++) {
			ge_cfg[idx]->inst = idx;
			ge_cfg[idx]->update = 0;
		}
	} else {
		runtime->ge_cfg.update = 1;
		// runtime->crosstalk_cfg.inst;
		runtime->ge_cfg.enable = crosstalk_attr->Enable && !runtime->is_module_bypass;

		struct cvi_isp_ge_tun_cfg *cfg_ge  = (struct cvi_isp_ge_tun_cfg *)&runtime->ge_cfg.ge_cfg;

		//TODO@ST Check formula
		cfg_ge->DPC_10.bits.GE_STRENGTH = runtime->crosstalk_attr.Strength;
		cfg_ge->DPC_10.bits.GE_COMBINEWEIGHT = 4;
		cfg_ge->DPC_11.bits.GE_THRE1 = crosstalk_attr->GrGbDiffThreSec[0];
		cfg_ge->DPC_11.bits.GE_THRE2 = crosstalk_attr->GrGbDiffThreSec[1];
		cfg_ge->DPC_12.bits.GE_THRE3 = crosstalk_attr->GrGbDiffThreSec[2];
		cfg_ge->DPC_12.bits.GE_THRE4 = crosstalk_attr->GrGbDiffThreSec[3];
		cfg_ge->DPC_13.bits.GE_THRE11 = crosstalk_attr->FlatThre[0];
		cfg_ge->DPC_13.bits.GE_THRE21 = crosstalk_attr->FlatThre[1];
		cfg_ge->DPC_14.bits.GE_THRE31 = crosstalk_attr->FlatThre[2];
		cfg_ge->DPC_14.bits.GE_THRE41 = crosstalk_attr->FlatThre[3];
		cfg_ge->DPC_15.bits.GE_THRE12 = 128;
		cfg_ge->DPC_15.bits.GE_THRE22 = 192;
		cfg_ge->DPC_16.bits.GE_THRE32 = 224;
		cfg_ge->DPC_16.bits.GE_THRE42 = 256;

		ISP_CTX_S *pstIspCtx = NULL;
		CVI_BOOL *map_pipe_to_enable;
		CVI_BOOL map_pipe_to_enable_sdr[2] = {1, 0};
		CVI_BOOL map_pipe_to_enable_wdr[2] = {1, 1};

		ISP_GET_CTX(ViPipe, pstIspCtx);
		if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
			map_pipe_to_enable = map_pipe_to_enable_wdr;
		else
			map_pipe_to_enable = map_pipe_to_enable_sdr;
#if !defined(__CV180X__)
		for (CVI_U32 idx = 0 ; idx < 2 ; idx++) {
#else
		for (CVI_U32 idx = 0 ; idx < 1 ; idx++) {
#endif
			runtime->ge_cfg.inst = idx;
			runtime->ge_cfg.enable = runtime->ge_cfg.enable && map_pipe_to_enable[idx];
			memcpy(ge_cfg[idx], &(runtime->ge_cfg), sizeof(struct cvi_vip_isp_ge_config));
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_crosstalk_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_CROSSTALK_ATTR_S *crosstalk_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_crosstalk_ctrl_get_crosstalk_attr(ViPipe, &crosstalk_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->CrosstalkEnable = crosstalk_attr->Enable;
		pProcST->CrosstalkisManualMode = crosstalk_attr->enOpType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_crosstalk_ctrl_runtime  *_get_crosstalk_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_crosstalk_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CROSSTALK, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_crosstalk_ctrl_check_crosstalk_attr_valid(const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCrosstalkAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCrosstalkAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCrosstalkAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_ARRAY_1D(pstCrosstalkAttr, GrGbDiffThreSec, 4, 0x0, 0xfff);
	CHECK_VALID_ARRAY_1D(pstCrosstalkAttr, FlatThre, 4, 0x0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstCrosstalkAttr, Strength, 0x0, 0x100);

	return ret;
}
//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_crosstalk_ctrl_get_crosstalk_attr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S **pstCrosstalkAttr)
{
	if (pstCrosstalkAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_crosstalk_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CROSSTALK, (CVI_VOID *) &shared_buffer);
	*pstCrosstalkAttr = &shared_buffer->stCrosstalkAttr;

	return ret;
}

CVI_S32 isp_crosstalk_ctrl_set_crosstalk_attr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr)
{
	if (pstCrosstalkAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_crosstalk_ctrl_runtime *runtime = _get_crosstalk_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_crosstalk_ctrl_check_crosstalk_attr_valid(pstCrosstalkAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CROSSTALK_ATTR_S *p = CVI_NULL;

	isp_crosstalk_ctrl_get_crosstalk_attr(ViPipe, &p);
	memcpy((void *)p, pstCrosstalkAttr, sizeof(*pstCrosstalkAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

