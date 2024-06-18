/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_csc_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_csc_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl csc_mod = {
	.init = isp_csc_ctrl_init,
	.uninit = isp_csc_ctrl_uninit,
	.suspend = isp_csc_ctrl_suspend,
	.resume = isp_csc_ctrl_resume,
	.ctrl = isp_csc_ctrl_ctrl
};


static CVI_S32 isp_csc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_csc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_csc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_csc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_csc_ctrl_check_csc_attr_valid(const ISP_CSC_ATTR_S *pstCSCAttr);

static CVI_S32 set_csc_proc_info(VI_PIPE ViPipe);

static struct isp_csc_ctrl_runtime  *_get_csc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_csc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_csc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_csc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_csc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_csc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_csc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCsc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCsc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_csc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_csc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_csc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_csc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_csc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_csc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CSC_ATTR_S *csc_attr = NULL;

	isp_csc_ctrl_get_csc_attr(ViPipe, &csc_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(csc_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	if (csc_attr->Enable && !runtime->is_module_bypass) {
		runtime->csc_param_in.eCscColorGamut = csc_attr->enColorGamut;
		runtime->csc_param_in.u8hue = csc_attr->Hue;
		runtime->csc_param_in.u8luma = csc_attr->Luma;
		runtime->csc_param_in.u8contrast = csc_attr->Contrast;
		runtime->csc_param_in.u8saturation = csc_attr->Saturation;
		for (CVI_U8 idx = 0; idx < CSC_MATRIX_SIZE; ++idx)
			runtime->csc_param_in.s16userCscCoef[idx] = csc_attr->stUserMatrx.userCscCoef[idx];
		for (CVI_U8 idx = 0; idx < CSC_OFFSET_SIZE; ++idx)
			runtime->csc_param_in.s16userCscOffset[idx] = csc_attr->stUserMatrx.userCscOffset[idx];
	} else {
		runtime->csc_param_in.eCscColorGamut = ISP_CSC_COLORGAMUT_BT601;
		runtime->csc_param_in.u8hue = 50;
		runtime->csc_param_in.u8luma = 50;
		runtime->csc_param_in.u8contrast = 50;
		runtime->csc_param_in.u8saturation = 50;
	}

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_csc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_csc_main(
		(struct csc_param_in *)&runtime->csc_param_in,
		(struct csc_param_out *)&runtime->csc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_csc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update) {
		// Set to runtime
		runtime->csc_cfg.update = 1;
		runtime->csc_cfg.enable = 1;//alway convert rgb -> yuv

		for (CVI_U8 i = 0; i < CSC_MATRIX_SIZE; ++i) {
			runtime->csc_cfg.coeff[i] = runtime->csc_param_out.s16cscCoef[i];
		}
		for (CVI_U8 i = 0; i < CSC_OFFSET_SIZE; ++i) {
			runtime->csc_cfg.offset[i] = runtime->csc_param_out.s16cscOffset[i];
		}

		// Set to driver
		struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
		CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

		struct cvi_vip_isp_csc_config *csc_cfg =
			(struct cvi_vip_isp_csc_config *)&(post_addr->tun_cfg[tun_idx].csc_cfg);

		memcpy(csc_cfg, &runtime->csc_cfg, sizeof(struct cvi_vip_isp_csc_config));
	}
	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_csc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_csc_ctrl_runtime  *_get_csc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_csc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CSC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_csc_ctrl_check_csc_attr_valid(const ISP_CSC_ATTR_S *pstCSCAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCSCAttr, Enable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstCSCAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstCSCAttr, enColorGamut, 0x0, ISP_CSC_COLORGAMUT_NUM);
	CHECK_VALID_CONST(pstCSCAttr, Hue, 0x0, 0x64);
	CHECK_VALID_CONST(pstCSCAttr, Luma, 0x0, 0x64);
	CHECK_VALID_CONST(pstCSCAttr, Contrast, 0x0, 0x64);
	CHECK_VALID_CONST(pstCSCAttr, Saturation, 0x0, 0x64);
	CHECK_VALID_ARRAY_1D(pstCSCAttr, stUserMatrx.userCscCoef, CSC_MATRIX_SIZE, -0x2000, 0x1FFF);
	CHECK_VALID_ARRAY_1D(pstCSCAttr, stUserMatrx.userCscOffset, CSC_OFFSET_SIZE, -0x100, 0xFF);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_csc_ctrl_get_csc_attr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S **pstCSCAttr)
{
	if (pstCSCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_csc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CSC, (CVI_VOID *) &shared_buffer);
	*pstCSCAttr = &shared_buffer->stCSCAttr;

	return ret;
}

CVI_S32 isp_csc_ctrl_set_csc_attr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S *pstCSCAttr)
{
	if (pstCSCAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_csc_ctrl_runtime *runtime = _get_csc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_csc_ctrl_check_csc_attr_valid(pstCSCAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CSC_ATTR_S *p = CVI_NULL;

	isp_csc_ctrl_get_csc_attr(ViPipe, &p);
	memcpy((void *)p, pstCSCAttr, sizeof(*pstCSCAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

