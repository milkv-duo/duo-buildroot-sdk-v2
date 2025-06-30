/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: isp_lblc_ctrl.c
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

#include "isp_lblc_ctrl.h"
#include "isp_mgr_buf.h"

const struct isp_module_ctrl lblc_mod = {
	.init = isp_lblc_ctrl_init,
	.uninit = isp_lblc_ctrl_uninit,
	.suspend = isp_lblc_ctrl_suspend,
	.resume = isp_lblc_ctrl_resume,
	.ctrl = isp_lblc_ctrl_ctrl
};

static CVI_S32 isp_lblc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_lblc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_lblc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_lblc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 set_lblc_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_lblc_ctrl_check_lblc_attr_valid(const ISP_LBLC_ATTR_S *pstLblcAttr);
static CVI_S32 isp_lblc_ctrl_check_lblc_lut_attr_valid(
	const ISP_LBLC_LUT_ATTR_S *pstLblcLutAttr);

static struct isp_lblc_ctrl_runtime  *_get_lblc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_lblc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_lblc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_lblc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_lblc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_lblc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_lblc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassLblc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassLblc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_lblc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_lblc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_lblc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_lblc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_lblc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_lblc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_LBLC_ATTR_S *lblc_attr = NULL;
	const ISP_LBLC_LUT_ATTR_S *lblc_lut_attr = NULL;

	isp_lblc_ctrl_get_lblc_attr(ViPipe, &lblc_attr);
	isp_lblc_ctrl_get_lblc_lut_attr(ViPipe, &lblc_lut_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(lblc_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));
	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	 //No need to update parameters if disable. Because its meaningless
	if (!lblc_attr->enable || runtime->is_module_bypass)
		return ret;

	if (lblc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(lblc_attr, strength);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(lblc_attr, strength, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	runtime->lblc_param_in.iso = algoResult->u32PostIso;
	runtime->lblc_param_in.iso_tbl_size = lblc_lut_attr->size;

	for (CVI_U32 i = 0 ; i < ISP_LBLC_ISO_SIZE; i++) {
		runtime->lblc_param_in.iso_tbl[i] = lblc_lut_attr->lblcLut[i].iso;
		runtime->lblc_param_in.lblc_lut_r[i] = ISP_PTR_CAST_PTR(lblc_lut_attr->lblcLut[i].lblcOffsetR);
		runtime->lblc_param_in.lblc_lut_gr[i] = ISP_PTR_CAST_PTR(lblc_lut_attr->lblcLut[i].lblcOffsetGr);
		runtime->lblc_param_in.lblc_lut_gb[i] = ISP_PTR_CAST_PTR(lblc_lut_attr->lblcLut[i].lblcOffsetGb);
		runtime->lblc_param_in.lblc_lut_b[i] = ISP_PTR_CAST_PTR(lblc_lut_attr->lblcLut[i].lblcOffsetB);
	}

	runtime->lblc_param_in.strength = runtime->lblc_attr.strength;

	// ParamOut
	runtime->lblc_param_out.lblc_lut_r = ISP_PTR_CAST_PTR(runtime->lblc_lut_attr.lblcOffsetR);
	runtime->lblc_param_out.lblc_lut_gr = ISP_PTR_CAST_PTR(runtime->lblc_lut_attr.lblcOffsetGr);
	runtime->lblc_param_out.lblc_lut_gb = ISP_PTR_CAST_PTR(runtime->lblc_lut_attr.lblcOffsetGb);
	runtime->lblc_param_out.lblc_lut_b = ISP_PTR_CAST_PTR(runtime->lblc_lut_attr.lblcOffsetB);
	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_lblc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_lblc_main(
		(struct lblc_param_in *)&runtime->lblc_param_in,
		(struct lblc_param_out *)&runtime->lblc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_lblc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {

	} else {

	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_lblc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

static struct isp_lblc_ctrl_runtime  *_get_lblc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_lblc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LBLC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_lblc_ctrl_check_lblc_attr_valid(const ISP_LBLC_ATTR_S *pstLblcAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstLblcAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	CHECK_VALID_AUTO_ISO_1D(pstLblcAttr, strength, 0x0, 0xfff);

	return ret;
}

static CVI_S32 isp_lblc_ctrl_check_lblc_lut_attr_valid(
	const ISP_LBLC_LUT_ATTR_S *pstLblcLutAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstLblcLutAttr, size, 0x1, 0x7);

	for (CVI_U32 tempIdx = 0; tempIdx < pstLblcLutAttr->size; ++tempIdx) {
		const ISP_LBLC_LUT_S *pstLblcLut
			= &(pstLblcLutAttr->lblcLut[tempIdx]);

		CHECK_VALID_CONST(pstLblcLut, iso, 0x64, 0x320000);
		CHECK_VALID_ARRAY_1D(pstLblcLut, lblcOffsetR, ISP_LBLC_GRID_POINTS, 0x0, 0xfff);
		CHECK_VALID_ARRAY_1D(pstLblcLut, lblcOffsetGr, ISP_LBLC_GRID_POINTS, 0x0, 0xfff);
		CHECK_VALID_ARRAY_1D(pstLblcLut, lblcOffsetGb, ISP_LBLC_GRID_POINTS, 0x0, 0xfff);
		CHECK_VALID_ARRAY_1D(pstLblcLut, lblcOffsetB, ISP_LBLC_GRID_POINTS, 0x0, 0xfff);
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_lblc_ctrl_get_lblc_attr(VI_PIPE ViPipe, const ISP_LBLC_ATTR_S **pstLblcAttr)
{
	if (pstLblcAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LBLC, (CVI_VOID *) &shared_buffer);

	*pstLblcAttr = &shared_buffer->stLblcAttr;

	return ret;
}

CVI_S32 isp_lblc_ctrl_set_lblc_attr(VI_PIPE ViPipe, const ISP_LBLC_ATTR_S *pstLblcAttr)
{
	if (pstLblcAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_lblc_ctrl_check_lblc_attr_valid(pstLblcAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_LBLC_ATTR_S *p = CVI_NULL;

	isp_lblc_ctrl_get_lblc_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstLblcAttr, sizeof(*pstLblcAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_lblc_ctrl_get_lblc_lut_attr(VI_PIPE ViPipe,
			const ISP_LBLC_LUT_ATTR_S **pstLblcLutAttr)
{
	if (pstLblcLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_LBLC, (CVI_VOID *) &shared_buffer);

	*pstLblcLutAttr = &shared_buffer->stLblcLutAttr;

	return ret;
}

CVI_S32 isp_lblc_ctrl_set_lblc_lut_attr(VI_PIPE ViPipe
	, const ISP_LBLC_LUT_ATTR_S *pstLblcLutAttr)
{
	if (pstLblcLutAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_lblc_ctrl_check_lblc_lut_attr_valid(pstLblcLutAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_LBLC_LUT_ATTR_S *p = CVI_NULL;

	isp_lblc_ctrl_get_lblc_lut_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstLblcLutAttr, sizeof(*pstLblcLutAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_lblc_ctrl_get_lblc_info(VI_PIPE ViPipe, struct lblc_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_lblc_ctrl_runtime *runtime = _get_lblc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_LBLC_ATTR_S *lblc_attr = NULL;

	isp_lblc_ctrl_get_lblc_attr(ViPipe, &lblc_attr);

	info->lblcOffsetR = runtime->lblc_lut_attr.lblcOffsetR;
	info->lblcOffsetGr = runtime->lblc_lut_attr.lblcOffsetGr;
	info->lblcOffsetGb = runtime->lblc_lut_attr.lblcOffsetGb;
	info->lblcOffsetB = runtime->lblc_lut_attr.lblcOffsetB;

	if (!lblc_attr->enable || runtime->is_module_bypass) {
		memset(info->lblcOffsetR, 0, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memset(info->lblcOffsetGr, 0, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memset(info->lblcOffsetGb, 0, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
		memset(info->lblcOffsetB, 0, ISP_LBLC_GRID_POINTS * sizeof(CVI_U16));
	}

	return ret;
}

