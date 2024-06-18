/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_motion_ctrl.c
 * Description:
 *
 */

#include "cvi_sys.h"
#include "cvi_base.h"

#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"

#include "isp_sts_ctrl.h"

#include "isp_motion_ctrl.h"
#include "isp_mgr_buf.h"
#include "isp_interpolate.h"

const struct isp_module_ctrl motion_mod = {
	.init = isp_motion_ctrl_init,
	.uninit = isp_motion_ctrl_uninit,
	.suspend = isp_motion_ctrl_suspend,
	.resume = isp_motion_ctrl_resume,
	.ctrl = isp_motion_ctrl_ctrl
};

static CVI_S32 isp_motion_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_motion_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_motion_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_motion_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_motion_ctrl_check_motion_attr_valid(const ISP_VC_ATTR_S *pstMotionAttr);

static struct isp_motion_ctrl_runtime *_get_motion_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_motion_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_motion_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_motion_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_motion_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_motion_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_motion_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassMotion;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassMotion = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_motion_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_motion_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_motion_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_motion_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	return ret;
}

static CVI_S32 isp_motion_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);
	ISP_CTX_S *pstIspCtx = NULL;
	const ISP_VC_ATTR_S *motion_attr = NULL;
	ISP_MOTION_STATS_INFO *motionInfo = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	isp_motion_ctrl_get_motion_attr(ViPipe, &motion_attr);
	isp_sts_ctrl_get_motion_sts(ViPipe, &motionInfo);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(motion_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (runtime->is_module_bypass)
		return ret;

	struct motion_param_in *ptParamIn = &(runtime->motion_param_in);

	ptParamIn->frameCnt = motionInfo->frameCnt;
	ptParamIn->gridWidth = motionInfo->gridWidth;
	ptParamIn->gridHeight = motionInfo->gridHeight;
	ptParamIn->motionStsBufSize = motionInfo->motionStsBufSize;
	ptParamIn->motionMapAddr = ISP_PTR_CAST_PTR(motionInfo->motionStsData);
	ptParamIn->imageWidth = pstIspCtx->stSysRect.u32Width;
	ptParamIn->imageHeight = pstIspCtx->stSysRect.u32Height;
	ptParamIn->motionThreshold = INTERPOLATE_LINEAR(ViPipe, INTPLT_POST_ISO, motion_attr->MotionThreshold);
	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

static CVI_S32 isp_motion_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	//ALGO
	isp_algo_motion_main(
		(struct motion_param_in *)&runtime->motion_param_in,
		(struct motion_param_out *)&runtime->motion_param_out);

//	printf("motionLevel : %d, FrameCnt : %d\n",
//		runtime->motion_param_out.motionLevel, runtime->motion_param_out.frameCnt);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_motion_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);
	struct motion_param_out *ptParamOut = &(runtime->motion_param_out);
	struct mlv_info m_i;

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update && !runtime->is_module_bypass) {
		m_i.sensor_num	= ViPipe;
		m_i.frm_num		= ptParamOut->frameCnt;
		m_i.mlv			= ptParamOut->motionLevel;

		CVI_VI_SetMotionLV(m_i);
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static struct isp_motion_ctrl_runtime *_get_motion_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_motion_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MOTION, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_motion_ctrl_check_motion_attr_valid(const ISP_VC_ATTR_S *pstMotionAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_ARRAY_1D(pstMotionAttr, MotionThreshold, ISP_AUTO_ISO_STRENGTH_NUM, 0x0, 0xff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_motion_ctrl_get_motion_attr(VI_PIPE ViPipe, const ISP_VC_ATTR_S **pstMotionAttr)
{
	if (pstMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_motion_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_MOTION, (CVI_VOID *) &shared_buffer);
	*pstMotionAttr = &shared_buffer->stMotionAttr;

	return ret;
}

CVI_S32 isp_motion_ctrl_set_motion_attr(VI_PIPE ViPipe, const ISP_VC_ATTR_S *pstMotionAttr)
{
	if (pstMotionAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_motion_ctrl_runtime *runtime = _get_motion_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_motion_ctrl_check_motion_attr_valid(pstMotionAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_VC_ATTR_S *p = CVI_NULL;

	isp_motion_ctrl_get_motion_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstMotionAttr, sizeof(*pstMotionAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

