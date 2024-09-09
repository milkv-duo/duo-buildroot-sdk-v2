/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mono_ctrl.c
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

#include "isp_mono_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl mono_mod = {
	.init = isp_mono_ctrl_init,
	.uninit = isp_mono_ctrl_uninit,
	.suspend = isp_mono_ctrl_suspend,
	.resume = isp_mono_ctrl_resume,
	.ctrl = isp_mono_ctrl_ctrl
};

static CVI_S32 isp_mono_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_mono_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_mono_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_mono_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_mono_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_mono_ctrl_check_mono_attr_valid(const ISP_MONO_ATTR_S *pstMonoAttr);

static struct isp_mono_ctrl_runtime  *_get_mono_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_mono_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_mono_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_mono_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_mono_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_mono_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_mono_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassMono;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassMono = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_mono_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_mono_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_mono_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_mono_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_mono_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_mono_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_MONO_ATTR_S *mono_attr = NULL;

	isp_mono_ctrl_get_mono_attr(ViPipe, &mono_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(mono_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!mono_attr->Enable || runtime->is_module_bypass)
		return ret;

	// ParamIn
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

#if 0
static CVI_S32 isp_mono_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_mono_main(
		(struct mono_param_in *)&runtime->mono_param_in,
		(struct mono_param_out *)&runtime->mono_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_mono_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_mono_config *mono_cfg =
		(struct cvi_vip_isp_mono_config *)&(post_addr->tun_cfg[tun_idx].mono_cfg);

	const ISP_MONO_ATTR_S *mono_attr = NULL;

	isp_mono_ctrl_get_mono_attr(ViPipe, &mono_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		mono_cfg->update = 0;
	} else {
		mono_cfg->update = 1;
		mono_cfg->force_mono_enable = mono_attr->Enable && !runtime->is_module_bypass;
	}

	runtime->postprocess_updated = CVI_FALSE;
	return ret;
}

static CVI_S32 set_mono_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_mono_ctrl_runtime  *_get_mono_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_mono_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MONO, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_mono_ctrl_check_mono_attr_valid(const ISP_MONO_ATTR_S *pstMonoAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(pstMonoAttr);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_mono_ctrl_get_mono_attr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S **pstMonoAttr)
{
	if (pstMonoAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MONO, (CVI_VOID *) &shared_buffer);
	*pstMonoAttr = &shared_buffer->stMonoAttr;

	return ret;
}

CVI_S32 isp_mono_ctrl_set_mono_attr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S *pstMonoAttr)
{
	if (pstMonoAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mono_ctrl_runtime *runtime = _get_mono_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_mono_ctrl_check_mono_attr_valid(pstMonoAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_MONO_ATTR_S *p = CVI_NULL;

	isp_mono_ctrl_get_mono_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstMonoAttr, sizeof(*pstMonoAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

