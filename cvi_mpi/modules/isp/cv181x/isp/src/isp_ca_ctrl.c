/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ca_ctrl.c
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

#include "isp_ca_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl ca_mod = {
	.init = isp_ca_ctrl_init,
	.uninit = isp_ca_ctrl_uninit,
	.suspend = isp_ca_ctrl_suspend,
	.resume = isp_ca_ctrl_resume,
	.ctrl = isp_ca_ctrl_ctrl
};

#define CA_MIN(a, b)               ((a) < (b) ? (a) : (b))
static CVI_S32 isp_ca_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_ca_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_ca_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_ca_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_ca_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_ca_ctrl_check_ca_attr_valid(const ISP_CA_ATTR_S *pstCAAttr);

static struct isp_ca_ctrl_runtime  *_get_ca_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_ca_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_ca_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ca_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ca_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_ca_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_ca_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassCa;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassCa = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ca_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_ca_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_ca_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_ca_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_ca_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_ca_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CA_ATTR_S *ca_attr = NULL;

	isp_ca_ctrl_get_ca_attr(ViPipe, &ca_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(ca_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!ca_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (ca_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(ca_attr, ISORatio);
		for (CVI_U32 idx = 0 ; idx < CA_LUT_NUM ; idx++) {
			MANUAL(ca_attr, YRatioLut[idx]);
		}

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(ca_attr, ISORatio, INTPLT_POST_ISO);
		for (CVI_U32 idx = 0 ; idx < CA_LUT_NUM ; idx++) {
			AUTO(ca_attr, YRatioLut[idx], INTPLT_POST_ISO);
		}

		#undef AUTO
	}

	// ParamIn
	// ParamOut

	runtime->process_updated = CVI_TRUE;

	return ret;
}

#if 0
static CVI_S32 isp_ca_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	isp_algo_ca_main(
		(struct ca_param_in *)&runtime->ca_param_in,
		(struct ca_param_out *)&runtime->ca_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_U8 isp_ca_adjust_saturation_iir(CVI_U8 adjValue)
{
	static CVI_U8 curAdjVal;

	if (curAdjVal > adjValue) {
		curAdjVal--;
	} else if (curAdjVal < adjValue) {
		curAdjVal++;
	}

	return curAdjVal;
}

static CVI_S32 isp_ca_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS, i;
	CVI_U8 u8AdjustSat, u8CurSat;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);
	ISP_CTX_S *pstIspCtx = NULL;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
	ISP_GET_CTX(ViPipe, pstIspCtx);

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_cacp_config *cacp_cfg =
		(struct cvi_vip_isp_cacp_config *)&(post_addr->tun_cfg[tun_idx].cacp_cfg);

	const ISP_CA_ATTR_S *ca_attr = NULL;

	isp_ca_ctrl_get_ca_attr(ViPipe, &ca_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		cacp_cfg->update = 0;
	} else {
		cacp_cfg->update = 1;
		cacp_cfg->enable = ca_attr->Enable && !runtime->is_module_bypass;
		cacp_cfg->mode = ca_attr->CaCpMode;
		cacp_cfg->iso_ratio = runtime->ca_attr.ISORatio;

		if (ca_attr->CaCpMode == 0) {
			memcpy(cacp_cfg->ca_y_ratio_lut, runtime->ca_attr.YRatioLut, sizeof(CVI_U16) * CA_LUT_NUM);
			u8CurSat = isp_ca_adjust_saturation_iir(pstIspCtx->stAwbResult.u8AdjCASaturation);
			if (u8CurSat > 0) {
				i =  pstIspCtx->stAwbResult.u8AdjCASatLuma - (u8CurSat / 2);
				i = i > 0 ? i : 0;
				for (; i < CA_LUT_NUM; i++) {
					if (pstIspCtx->stAwbResult.u8AdjCASatLuma > i) {
						u8AdjustSat = u8CurSat -
								(pstIspCtx->stAwbResult.u8AdjCASatLuma - i);
					} else {
						u8AdjustSat = u8CurSat;
					}
					cacp_cfg->ca_y_ratio_lut[i] = 128 - u8AdjustSat;
				}
			}
		} else {
			memcpy(cacp_cfg->cp_y_lut, ca_attr->CPLutY, sizeof(CVI_U8) * CA_LUT_NUM);
			memcpy(cacp_cfg->cp_u_lut, ca_attr->CPLutU, sizeof(CVI_U8) * CA_LUT_NUM);
			memcpy(cacp_cfg->cp_v_lut, ca_attr->CPLutV, sizeof(CVI_U8) * CA_LUT_NUM);
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_ca_proc_info(VI_PIPE ViPipe)
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
#endif //
	UNUSED(ViPipe);
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_ca_ctrl_runtime  *_get_ca_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_ca_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CA, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_ca_ctrl_check_ca_attr_valid(const ISP_CA_ATTR_S *pstCAAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCAAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstCAAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstCAAttr, UpdateInterval, 0, 0xff);
	// CHECK_VALID_CONST(pstCAAttr, CaCpMode, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_ARRAY(pstCAAttr, CPLutY, CA_LUT_NUM, 0, 0xff);
	// CHECK_VALID_ARRAY(pstCAAttr, CPLutU, CA_LUT_NUM, 0, 0xff);
	// CHECK_VALID_ARRAY(pstCAAttr, CPLutV, CA_LUT_NUM, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstCAAttr, ISORatio, 0, 0x7ff);
	CHECK_VALID_AUTO_ISO_2D(pstCAAttr, YRatioLut, CA_LUT_NUM, 0, 0x7ff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_ca_ctrl_get_ca_attr(VI_PIPE ViPipe, const ISP_CA_ATTR_S **pstCAAttr)
{
	if (pstCAAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_ca_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CA, (CVI_VOID *) &shared_buffer);
	*pstCAAttr = &shared_buffer->stCAAttr;

	return ret;
}

CVI_S32 isp_ca_ctrl_set_ca_attr(VI_PIPE ViPipe, const ISP_CA_ATTR_S *pstCAAttr)
{
	if (pstCAAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_ca_ctrl_runtime *runtime = _get_ca_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_ca_ctrl_check_ca_attr_valid(pstCAAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CA_ATTR_S *p = CVI_NULL;

	isp_ca_ctrl_get_ca_attr(ViPipe, &p);
	memcpy((void *)p, pstCAAttr, sizeof(*pstCAAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

