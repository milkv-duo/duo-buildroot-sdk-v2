/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_wb_ctrl.c
 * Description:
 *
 */
#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_interpolate.h"
#include "isp_defines.h"
#include "isp_wb_ctrl.h"
// #include "isp_algo_wb.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
#define WB_UNIT_GAIN 1024
#define WB_INV_Q   (12)
#define WB_INV_R   (12)
#define WB_INV_T   (WB_INV_Q + WB_INV_R)
#define WBGINV_MAX ((1 << WB_INV_T) - 1)
#define GAIN_EXTEND  ((1 << WB_INV_R) * WB_UNIT_GAIN)

const struct isp_module_ctrl wb_mod = {
	.init = isp_wb_ctrl_init,
	.uninit = isp_wb_ctrl_uninit,
	.suspend = isp_wb_ctrl_suspend,
	.resume = isp_wb_ctrl_resume,
	.ctrl = isp_wb_ctrl_ctrl
};

enum WBG_ID {
	WBG_ID_PRE0_FE_RGBMAP_LE = 0,
	WBG_ID_PRE0_FE_RGBMAP_SE,
	WBG_ID_PRE1_FE_RGBMAP_LE,
	WBG_ID_PRE1_FE_RGBMAP_SE,
	WBG_ID_PRE2_FE_RGBMAP_LE,
	WBG_ID_RAW_TOP_LE,
	WBG_ID_RAW_TOP_SE,
	WBG_ID_MAX,
};

static CVI_S32 isp_wb_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_wb_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_wb_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_wb_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_VOID isp_wb_ctrl_set_colortone_attr_default(VI_PIPE ViPipe);
static CVI_S32 isp_wb_ctrl_check_wb_colortone_attr_valid(const ISP_COLOR_TONE_ATTR_S *pstColorToneAttr);
// static CVI_S32 set_wb_proc_info(VI_PIPE ViPipe);
static struct isp_wb_ctrl_runtime  *_get_wb_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_wb_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_wb_ctrl_set_colortone_attr_default(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

CVI_S32 isp_wb_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_wb_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_wb_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_wb_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_wb_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_wb_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_wb_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_wb_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_wb_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	// set_wb_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_wb_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_COLOR_TONE_ATTR_S *wb_colortone_attr = NULL;

	isp_wb_ctrl_get_wb_colortone_attr(ViPipe, &wb_colortone_attr);

	CVI_BOOL is_preprocess_update = CVI_TRUE;

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	CVI_U16 wb_rgain, wb_ggain, wb_bgain;
	CVI_U16 ct_rgain, ct_ggain, ct_bgain;

	wb_rgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_R];
	wb_ggain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_GR];
	wb_bgain = algoResult->au32WhiteBalanceGain[ISP_BAYER_CHN_B];
	if (wb_rgain == 0) {
		ISP_LOG_ERR("wb_rgain is 0\n");
	}
	if (wb_ggain == 0) {
		ISP_LOG_ERR("wb_ggain is 0\n");
	}
	if (wb_bgain == 0) {
		ISP_LOG_ERR("wb_bgain is 0\n");
	}

	ct_rgain = wb_colortone_attr->u16RedCastGain;
	ct_ggain = wb_colortone_attr->u16GreenCastGain;
	ct_bgain = wb_colortone_attr->u16BlueCastGain;
	if (ct_rgain == 0) {
		ISP_LOG_ERR("cg_rgain is 0\n");
	}
	if (ct_ggain == 0) {
		ISP_LOG_ERR("cg_ggain is 0\n");
	}
	if (ct_bgain == 0) {
		ISP_LOG_ERR("cg_bgain is 0\n");
	}

	runtime->wb_attr.u16Rgain = wb_rgain * ct_rgain / WB_UNIT_GAIN;
	runtime->wb_attr.u16Grgain = wb_ggain * ct_ggain / WB_UNIT_GAIN;
	runtime->wb_attr.u16Gbgain = runtime->wb_attr.u16Grgain;
	runtime->wb_attr.u16Bgain = wb_bgain * ct_bgain / WB_UNIT_GAIN;

	runtime->process_updated = CVI_TRUE;

	return ret;
}

#if 0
static CVI_S32 isp_wb_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_wb_main(
		(struct wb_param_in *)&runtime->wb_param_in,
		(struct wb_param_out *)&runtime->wb_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_wb_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
#if ENABLE_FE_WBG_UPDATE
	struct cvi_vip_isp_fe_cfg *pre_fe_addr = get_pre_fe_tuning_buf_addr(ViPipe);
#endif
	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);
#if ENABLE_FE_WBG_UPDATE
	struct cvi_vip_isp_wbg_config *pre_fe_wbg_cfg[2] = {
		(struct cvi_vip_isp_wbg_config *)&(pre_fe_addr->tun_cfg[tun_idx].wbg_cfg[0]),
		(struct cvi_vip_isp_wbg_config *)&(pre_fe_addr->tun_cfg[tun_idx].wbg_cfg[1]),
	};
#endif
	struct cvi_vip_isp_wbg_config *post_wbg_cfg[2] = {
		(struct cvi_vip_isp_wbg_config *)&(post_addr->tun_cfg[tun_idx].wbg_cfg[0]),
		(struct cvi_vip_isp_wbg_config *)&(post_addr->tun_cfg[tun_idx].wbg_cfg[1]),
	};

	CVI_U8 inst_base = 0;
	CVI_U8 post_num = 0;

#if ENABLE_FE_WBG_UPDATE
	CVI_U8 pre_fe_num = 0;
#if defined(__CV180X__)
	switch (ViPipe) {
	case 0:
		inst_base = WBG_ID_PRE0_FE_RGBMAP_LE;
		pre_fe_num = 1;
		post_num = 1;
	break;
	default:
	break;
	}
#else
	switch (ViPipe) {
	case 0:
		inst_base = WBG_ID_PRE0_FE_RGBMAP_LE;
		pre_fe_num = 2;
		post_num = 2;
	break;
	case 1:
		inst_base = WBG_ID_PRE1_FE_RGBMAP_LE;
		pre_fe_num = 2;
		post_num = 2;
	break;
	case 2:
		inst_base = WBG_ID_PRE2_FE_RGBMAP_LE;
		pre_fe_num = 1;
		post_num = 2;
	break;
	}
#endif
#else
	inst_base = WBG_ID_RAW_TOP_LE;
#if defined(__CV180X__)
	post_num = 1;
#else
	post_num = 2;
#endif
#endif

	const ISP_COLOR_TONE_ATTR_S *wb_colortone_attr = NULL;

	isp_wb_ctrl_get_wb_colortone_attr(ViPipe, &wb_colortone_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
#if ENABLE_FE_WBG_UPDATE
		for (CVI_U8 i = 0; i < pre_fe_num ; i++) {
			pre_fe_wbg_cfg[i]->inst = i + inst_base;
			pre_fe_wbg_cfg[i]->update = 0;
		}
#endif
		inst_base = WBG_ID_RAW_TOP_LE;
		for (CVI_U8 i = 0; i < post_num ; i++) {
			post_wbg_cfg[i]->inst = i + inst_base;
			post_wbg_cfg[i]->update = 0;
		}
	} else {
		runtime->wbg_cfg.update = 1;
		runtime->wbg_cfg.enable = 1;
		runtime->wbg_cfg.bypass = 0;

		runtime->wbg_cfg.rgain = runtime->wb_attr.u16Rgain;
		runtime->wbg_cfg.ggain = runtime->wb_attr.u16Grgain;
		runtime->wbg_cfg.bgain = runtime->wb_attr.u16Bgain;
		runtime->wbg_cfg.rgain_fraction = MIN((GAIN_EXTEND / MAX(runtime->wb_attr.u16Rgain, 1)), WBGINV_MAX);
		runtime->wbg_cfg.ggain_fraction = MIN((GAIN_EXTEND / MAX(runtime->wb_attr.u16Grgain, 1)), WBGINV_MAX);
		runtime->wbg_cfg.bgain_fraction = MIN((GAIN_EXTEND / MAX(runtime->wb_attr.u16Bgain, 1)), WBGINV_MAX);

		ISP_CTX_S *pstIspCtx = NULL;
		CVI_BOOL *map_pipe_to_enable;
		CVI_BOOL map_pipe_to_enable_sdr[2] = {1, 0};
		CVI_BOOL map_pipe_to_enable_wdr[2] = {1, 1};

		ISP_GET_CTX(ViPipe, pstIspCtx);
		if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
			map_pipe_to_enable = map_pipe_to_enable_wdr;
		else
			map_pipe_to_enable = map_pipe_to_enable_sdr;
#if ENABLE_FE_WBG_UPDATE
		for (CVI_U8 i = 0; i < pre_fe_num ; i++) {
			runtime->wbg_cfg.inst = i + inst_base;
			runtime->wbg_cfg.enable = map_pipe_to_enable[i];
			memcpy(pre_fe_wbg_cfg[i], &(runtime->wbg_cfg), sizeof(struct cvi_vip_isp_wbg_config));
		}
#endif
		inst_base = WBG_ID_RAW_TOP_LE;
		for (CVI_U8 i = 0; i < post_num ; i++) {
			runtime->wbg_cfg.inst = i + inst_base;
			runtime->wbg_cfg.enable = map_pipe_to_enable[i];
			memcpy(post_wbg_cfg[i], &(runtime->wbg_cfg), sizeof(struct cvi_vip_isp_wbg_config));
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_VOID isp_wb_ctrl_set_colortone_attr_default(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_COLOR_TONE_ATTR_S attr = {0};

	INIT_V(attr, u16RedCastGain, 1024);
	INIT_V(attr, u16GreenCastGain, 1024);
	INIT_V(attr, u16BlueCastGain, 1024);

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	isp_wb_ctrl_set_wb_colortone_attr(ViPipe, &attr);
}

#if 0
static CVI_S32 set_wb_proc_info(VI_PIPE ViPipe)
{
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	return CVI_SUCCESS;
}
#endif
#endif // ENABLE_ISP_C906L

static struct isp_wb_ctrl_runtime  *_get_wb_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_wb_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_WBGAIN, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_wb_ctrl_check_wb_colortone_attr_valid(const ISP_COLOR_TONE_ATTR_S *pstColorToneAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstColorToneAttr, u16RedCastGain, 0x0, 0x1000);
	CHECK_VALID_CONST(pstColorToneAttr, u16GreenCastGain, 0x0, 0x1000);
	CHECK_VALID_CONST(pstColorToneAttr, u16BlueCastGain, 0x0, 0x1000);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_wb_ctrl_get_wb_colortone_attr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S **pstColorToneAttr)
{
	if (pstColorToneAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_WBGAIN, (CVI_VOID *) &shared_buffer);
	*pstColorToneAttr = &shared_buffer->stColorToneAttr;

	return ret;
}

CVI_S32 isp_wb_ctrl_set_wb_colortone_attr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S *pstColorToneAttr)
{
	if (pstColorToneAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_wb_ctrl_runtime *runtime = _get_wb_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_wb_ctrl_check_wb_colortone_attr_valid(pstColorToneAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_COLOR_TONE_ATTR_S *p = CVI_NULL;

	isp_wb_ctrl_get_wb_colortone_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstColorToneAttr, sizeof(*pstColorToneAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

