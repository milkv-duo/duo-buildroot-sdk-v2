/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ca2_ctrl.c
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
#include "isp_sts_ctrl.h"

#include "isp_ca2_ctrl.h"
#include "isp_mgr_buf.h"

#include "isp_ccm_ctrl.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl ca2_mod = {
	.init = isp_ca2_ctrl_init,
	.uninit = isp_ca2_ctrl_uninit,
	.suspend = isp_ca2_ctrl_suspend,
	.resume = isp_ca2_ctrl_resume,
	.ctrl = isp_ca2_ctrl_ctrl
};


static CVI_S32 isp_ca2_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ca2_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ca2_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ca2_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_ca2_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_ca2_ctrl_check_ca2_attr_valid(const ISP_CA2_ATTR_S *pstCA2Attr);

static struct isp_ca2_ctrl_runtime  *_get_ca2_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ca2_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_ca2_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	UNUSED(ViPipe);
	return ret;
}

CVI_S32 isp_ca2_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ca2_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ca2_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ca2_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCa2;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCa2 = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ca2_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ca2_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ca2_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_ca2_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ca2_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ca2_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CA2_ATTR_S *ca2_attr = NULL;

	isp_ca2_ctrl_get_ca2_attr(ViPipe, &ca2_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ca2_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ca2_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (ca2_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		for (CVI_U32 idx = 0 ; idx < CA_LITE_NODE ; idx++) {
			MANUAL(ca2_attr, Ca2In[idx]);
			MANUAL(ca2_attr, Ca2Out[idx]);
		}

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		for (CVI_U32 idx = 0 ; idx < CA_LITE_NODE ; idx++) {
			AUTO(ca2_attr, Ca2In[idx], INTPLT_POST_ISO);
			AUTO(ca2_attr, Ca2Out[idx], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	// ParamIn
	runtime->ca2_param_in.Ca2In = ISP_PTR_CAST_PTR(runtime->ca2_attr.Ca2In);
	runtime->ca2_param_in.Ca2Out = ISP_PTR_CAST_PTR(runtime->ca2_attr.Ca2Out);
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_ca2_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_ca2_main(
		(struct ca2_param_in *)&runtime->ca2_param_in,
		(struct ca2_param_out *)&runtime->ca2_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_ca2_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ca2_config *ca2_cfg =
		(struct cvi_vip_isp_ca2_config *)&(post_addr->tun_cfg[tun_idx].ca2_cfg);

	const ISP_CA2_ATTR_S *ca2_attr = NULL;

	isp_ca2_ctrl_get_ca2_attr(ViPipe, &ca2_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		ca2_cfg->update = 0;
	} else {
		ca2_cfg->update = 1;
		ca2_cfg->enable = ca2_attr->Enable && !runtime->is_module_bypass;

		// memcpy(ca2_cfg->lut_in, runtime->ca2_attr.Ca2In, sizeof(CVI_U8) * CA_LITE_NODE);
		for (CVI_U32 i = 0 ; i < CA_LITE_NODE ; i++)
			ca2_cfg->lut_in[i] = runtime->ca2_attr.Ca2In[i];
		memcpy(ca2_cfg->lut_out, runtime->ca2_attr.Ca2Out, sizeof(CVI_U16) * CA_LITE_NODE);
		memcpy(ca2_cfg->lut_slp, runtime->ca2_param_out.Ca2Slope, sizeof(CVI_U16) * (CA_LITE_NODE-1));
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ca2_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#if 0
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

		const ISP_CA_ATTR_S *ca_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_ca_ctrl_get_ca_attr(ViPipe, &ca_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DCIEnable = DCI.Enable;
		pProcST->DCISpeed = DCI.Speed;
		pProcST->DCIDciStrength = DCI.DciStrength;
		pProcST->DCIisManualMode = (CVI_BOOL)DCI.enOpType;
		//manual or auto
		pProcST->DCIContrastGain = ContrastGain;
		pProcST->DCIBlcThr = BlcThr;
		pProcST->DCIWhtThr = WhtThr;
		pProcST->DCIBlcCtrl = BlcCtrl;
		pProcST->DCIWhtCtrl = WhtCtrl;
	}
#endif
	UNUSED(ViPipe);
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_ca2_ctrl_runtime  *_get_ca2_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ca2_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CA2, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ca2_ctrl_check_ca2_attr_valid(const ISP_CA2_ATTR_S *pstCA2Attr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCA2Attr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCA2Attr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCA2Attr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_ISO_2D(pstCA2Attr, Ca2In, CA_LITE_NODE, 0, 0xC0);
	CHECK_VALID_AUTO_ISO_2D(pstCA2Attr, Ca2Out, CA_LITE_NODE, 0, 0x7FF);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ca2_ctrl_get_ca2_attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S **pstCA2Attr)
{
	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ca2_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CA2, (CVI_VOID *) &shared_buffer);
	*pstCA2Attr = &shared_buffer->stCA2Attr;

	return ret;
}

CVI_S32 isp_ca2_ctrl_set_ca2_attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S *pstCA2Attr)
{
	if (pstCA2Attr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca2_ctrl_runtime *runtime = _get_ca2_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ca2_ctrl_check_ca2_attr_valid(pstCA2Attr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CA2_ATTR_S *p = CVI_NULL;

	isp_ca2_ctrl_get_ca2_attr(ViPipe, &p);
	memcpy((void *)p, pstCA2Attr, sizeof(*pstCA2Attr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

