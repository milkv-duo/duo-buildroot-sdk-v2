/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dis_ctrl.c
 * Description:
 *
 */

#include "stdio.h"
#include "stdlib.h"

#include "cvi_base.h"
#include "cvi_isp.h"
#include "cvi_comm_isp.h"

#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_control.h"
#include "isp_sts_ctrl.h"
#include "isp_dis_ctrl.h"
#include "isp_mgr_buf.h"
#include "vi_ioctl.h"

#ifdef DIS_PC_PLATFORM
#include "dis_platform.h"
#endif

#define NEW_DIS_CTRL_ARCH


const struct isp_module_ctrl dis_mod = {
	.init = isp_dis_ctrl_init,
	.uninit = isp_dis_ctrl_uninit,
	.suspend = isp_dis_ctrl_suspend,
	.resume = isp_dis_ctrl_resume,
	.ctrl = isp_dis_ctrl_ctrl
};

typedef struct _S_DIS_DBG_S {
	CVI_BOOL enable;
	CVI_BOOL isAligned;
	CVI_U32 dbgFrmNo;
	CVI_U32 norFrmNo;
	FILE *fp;
} S_DIS_DBG_S;

S_DIS_DBG_S sDisDbg;

static CVI_S32 isp_dis_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dis_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dis_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dis_ctrl_postprocess(VI_PIPE ViPipe);

static struct isp_dis_ctrl_runtime *_get_dis_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dis_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	runtime->isEnable = CVI_FALSE;

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	vi_sdk_disable_dis(pstIspCtx->ispDevFd, ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;
	disMainInit(ViPipe);

	return ret;
}

CVI_S32 isp_dis_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");

	CVI_S32 ret = CVI_SUCCESS;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	disMainUnInit(ViPipe);

	disAlgoDestroy(ViPipe);
	return ret;
}

CVI_S32 isp_dis_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dis_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dis_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
#ifdef NEW_DIS_CTRL_ARCH
	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dis_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDis;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDis = runtime->is_module_bypass;
		break;
	default:
		break;
	}
#else
	UNUSED(ViPipe);
	UNUSED(cmd);
	UNUSED(input);

#endif
	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static void dis_debug_init(const ISP_DIS_ATTR_S *pAttr, const ISP_DIS_CONFIG_S *pConfig, CVI_U32 dFrameCnt)
{
#if USE_CAM_VDO
	return;
#endif

	CVI_U8 isDbgMode;

	if (pConfig->mode == DIS_MODE_DEBUG)
		isDbgMode = 1;
	else
		isDbgMode = 0;

	if (!sDisDbg.enable) {
		if (isDbgMode) {
			sDisDbg.enable = 1;
		}
		if (!sDisDbg.enable) {
			return;
		}
	} else {
		if (isDbgMode == 0) {
			sDisDbg.enable = 0;
			if (sDisDbg.fp) {
				printf("----close\n");
				fclose(sDisDbg.fp);
				sDisDbg.fp = NULL;
			}
		}
		return;
	}

	printf("disDbgInit\n");

	sDisDbg.dbgFrmNo = 0;
	sDisDbg.isAligned = 0;
	sDisDbg.norFrmNo = 45;

	sDisDbg.fp = fopen("/var/log/dis_dbg.bin", "wb");
	if (sDisDbg.fp) {
		fprintf(sDisDbg.fp, "Frame:%d %d\n", dFrameCnt, sDisDbg.norFrmNo);
		fprintf(sDisDbg.fp, "ISP_DIS_ATTR_S:%zu\n", sizeof(ISP_DIS_ATTR_S));
		fprintf(sDisDbg.fp, "ISP_DIS_CONFIG_S:%zu\n", sizeof(ISP_DIS_CONFIG_S));
		fprintf(sDisDbg.fp, "REG_DIS:%zu\n", sizeof(REG_DIS));
		fprintf(sDisDbg.fp, "DIS_MAIN_INPUT_S:%zu\n", sizeof(DIS_MAIN_INPUT_S));
		fprintf(sDisDbg.fp, "ISP_DIS_STATS_INFO:%zu\n", sizeof(ISP_DIS_STATS_INFO));
		fprintf(sDisDbg.fp, "DIS_MAIN_OUTPUT_S:%zu\n", sizeof(DIS_MAIN_OUTPUT_S));

		fwrite(pAttr, 1, sizeof(ISP_DIS_ATTR_S), sDisDbg.fp);
		fwrite(pConfig, 1, sizeof(ISP_DIS_CONFIG_S), sDisDbg.fp);
	}
}

static void dis_debug_proc(DIS_MAIN_INPUT_S *pIn, DIS_MAIN_OUTPUT_S *pOut)
{
#define TEST_FRM_NUM (30)
	CVI_S16 testX, testY;

	if (!sDisDbg.enable) {
		return;
	}
	if (!sDisDbg.fp) {
		return;
	}

	if (sDisDbg.isAligned == 0) {
		testX = pIn->greg_dis.reg_MARGIN_INI_L * 8 / 10;
		testY = pIn->greg_dis.reg_MARGIN_INI_U * 8 / 10;
		if (testX > 30)
			testX = 30;
		if (testY > 30)
			testY = 30;
		if (sDisDbg.dbgFrmNo < TEST_FRM_NUM) {
			pOut->coordiate.xcor = pIn->greg_dis.reg_MARGIN_INI_L;
			pOut->coordiate.ycor = pIn->greg_dis.reg_MARGIN_INI_U;
		} else if ((sDisDbg.dbgFrmNo - TEST_FRM_NUM) % 4 == 0) {
			pOut->coordiate.xcor = pIn->greg_dis.reg_MARGIN_INI_L + testX;
			pOut->coordiate.ycor = pIn->greg_dis.reg_MARGIN_INI_U + testY;
		} else if ((sDisDbg.dbgFrmNo - TEST_FRM_NUM) % 4 == 2) {
			pOut->coordiate.xcor = pIn->greg_dis.reg_MARGIN_INI_L - testX;
			pOut->coordiate.ycor = pIn->greg_dis.reg_MARGIN_INI_U - testY;
		} else {
			pOut->coordiate.xcor = pIn->greg_dis.reg_MARGIN_INI_L;
			pOut->coordiate.ycor = pIn->greg_dis.reg_MARGIN_INI_U;
		}
		if (sDisDbg.dbgFrmNo >= (sDisDbg.norFrmNo)) {
			sDisDbg.isAligned = 1;
		}
	}
}

static void dis_debug_proc_dump(DIS_MAIN_INPUT_S *pIn, DIS_MAIN_OUTPUT_S *pOut)
{//3500us
	if (!sDisDbg.enable) {
		return;
	}
	if (!sDisDbg.fp) {
		return;
	}

	DIS_STATS_INFO *disStt = (DIS_STATS_INFO *) ISP_PTR_CAST_VOID(pIn->disStt);

	fwrite(pIn, 1, sizeof(DIS_MAIN_INPUT_S), sDisDbg.fp);
	fwrite(disStt, 1, sizeof(ISP_DIS_STATS_INFO), sDisDbg.fp);
	fwrite(pOut, 1, sizeof(DIS_MAIN_OUTPUT_S), sDisDbg.fp);
	sDisDbg.dbgFrmNo++;
}

#ifndef NEW_DIS_CTRL_ARCH

CVI_S32 isp_dis_pre_eof(VI_PIPE ViPipe)
{
	ISP_ALGO_RESULT_S *pResult = NULL;

	isp_dis_ctrl_pre_be_eof(ViPipe, (ISP_ALGO_RESULT_S *)pResult);
	return CVI_SUCCESS;
}

CVI_S32 isp_dis_get_gms_attr(VI_PIPE ViPipe, struct cvi_vip_isp_gms_config *gmsCfg)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dis_ctrl_get_gms_attr(ViPipe, gmsCfg);
	return ret;
}
#endif

static CVI_S32 isp_dis_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dis_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dis_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dis_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	//set_dis_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dis_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	const ISP_DIS_CONFIG_S *disConfig;
	const ISP_DIS_ATTR_S *disAttr;

	CVI_U32 imgW, imgH;
	CVI_U32 marginL, marginR, marginU, marginD;
	CVI_U32 jitterX, jitterY;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

#ifdef NEW_DIS_CTRL_ARCH
	CVI_U32 scene = 0;

	isp_control_get_scene_info(ViPipe, &scene);
	isp_dis_ctrl_get_dis_attr(ViPipe, &disAttr);
	isp_dis_ctrl_get_dis_config(ViPipe, &disConfig);
#else
	ISP_DIS_ATTR_S tmpAttr;
	ISP_DIS_CONFIG_S tmpConfig;

	disAttr = &tmpAttr;
	disConfig = &tmpConfig;
	CVI_ISP_GetDisAttr(ViPipe, &tmpAttr);
	CVI_ISP_GetDisConfig(ViPipe, &tmpConfig);
#endif
	CVI_ISP_GetPubAttr(ViPipe, (ISP_PUB_ATTR_S *)&runtime->pstPubAttr);

	imgW = runtime->pstPubAttr.stSnsSize.u32Width;
	imgH = runtime->pstPubAttr.stSnsSize.u32Height;

#ifdef NEW_DIS_CTRL_ARCH
	CVI_BOOL dis_supported = ((disAttr->enable && !runtime->is_module_bypass) || disAttr->stillCrop)
		&& ((scene == FE_ON_BE_OFF_POST_OFF_SC || scene == FE_OFF_BE_ON_POST_OFF_SC)) && (imgW <= 2880);
#else
	CVI_BOOL dis_supported = (imgW <= 2880) && (disAttr->enable);
#endif

	if (runtime->isEnable != dis_supported) {

		ISP_CTX_S *pstIspCtx = NULL;

		ISP_GET_CTX(ViPipe, pstIspCtx);

		if (dis_supported) {
			vi_sdk_enable_dis(pstIspCtx->ispDevFd, ViPipe);
		} else {
			vi_sdk_disable_dis(pstIspCtx->ispDevFd, ViPipe);
		}

		runtime->isEnable = dis_supported;
	}

	if (!dis_supported)
		return ret;

	disAlgoCreate(ViPipe);

	runtime->process_updated = CVI_TRUE;
	runtime->postprocess_updated = CVI_TRUE;

	//ISP_DIS_STATS_INFO  to DIS_STATS_INFO
#ifdef NEW_DIS_CTRL_ARCH
	isp_sts_ctrl_get_gms_sts(ViPipe, (ISP_DIS_STATS_INFO **)&(runtime->inPtr.disStt));
#else
	isp_sts_getDisStt(ViPipe, &(runtime->inPtr.disStt));
	runtime->preprocess_updated = CVI_TRUE;
#endif
	if (runtime->preprocess_updated == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;

	//Input param
	runtime->inPtr.greg_dis.reg_sw_DIS_en = disAttr->enable && !runtime->is_module_bypass;

	marginL = marginR = ((((imgW * (100 - disConfig->cropRatio)) / 100) >> 2) << 1);
	marginU = marginD = ((((imgH * (100 - disConfig->cropRatio)) / 100) >> 2) << 1);

	runtime->inPtr.greg_dis.reg_MARGIN_INI_U = marginU;//crop ratio
	runtime->inPtr.greg_dis.reg_MARGIN_INI_D = marginD;
	runtime->inPtr.greg_dis.reg_MARGIN_INI_L = marginL;
	runtime->inPtr.greg_dis.reg_MARGIN_INI_R = marginR;

	runtime->inPtr.greg_dis.reg_HIST_NORMX = 6;
	runtime->inPtr.greg_dis.reg_HIST_NORMY = 6;
	runtime->inPtr.greg_dis.reg_XHIST_LENGTH = XHIST_LENGTH;//fixed
	runtime->inPtr.greg_dis.reg_YHIST_LENGTH = YHIST_LENGTH;//fixed

	jitterX = ((((imgW * disAttr->horizontalLimit) / 1000) >> 3) << 3);
	jitterX = MIN3(jitterX, MAX_MVX_JITTER_RANGE, (marginL + marginR));
	jitterY = ((((imgH * disAttr->verticalLimit) / 1000) >> 3) << 3);
	jitterY = MIN3(jitterY, MAX_MVY_JITTER_RANGE, (marginU + marginD));

	jitterX = jitterX >> 1;
	jitterY = jitterY >> 1;

	runtime->inPtr.greg_dis.reg_MAX_MVX_JITTER_RANGE = jitterX;
	runtime->inPtr.greg_dis.reg_MAX_MVY_JITTER_RANGE = jitterY;

	//sw algo used +
	runtime->inPtr.greg_dis.reg_SAD_CONFRM_RANGEX = 8;
	runtime->inPtr.greg_dis.reg_SAD_CONFRM_RANGEY = 8;

	runtime->inPtr.greg_dis.reg_BLOCK_LENGTH_X = 128;
	runtime->inPtr.greg_dis.reg_BLOCK_LENGTH_Y = 128;

	runtime->inPtr.greg_dis.reg_cfrmsad_norm = 32;
	runtime->inPtr.greg_dis.reg_cfrmsad_max = 32;
	runtime->inPtr.greg_dis.reg_cfrmgain_xcoring = 12;
	runtime->inPtr.greg_dis.reg_cfrmgain_ycoring = 12;
	runtime->inPtr.greg_dis.reg_cfrmsad_xcoring = 16;
	runtime->inPtr.greg_dis.reg_cfrmsad_ycoring = 16;
	runtime->inPtr.greg_dis.reg_cfrmflag_xthd = 6;
	runtime->inPtr.greg_dis.reg_cfrmflag_ythd = 6;
	runtime->inPtr.greg_dis.reg_tmp_maxval_diff_thd = 12;
	runtime->inPtr.greg_dis.reg_tmp_maxval_gain = 16;
	runtime->inPtr.greg_dis.reg_xhist_cplx_thd = 60;
	runtime->inPtr.greg_dis.reg_yhist_cplx_thd = 60;

	runtime->inPtr.greg_dis.reg_force_offset_en = 0;
	runtime->inPtr.greg_dis.reg_force_xoffset = 0;
	runtime->inPtr.greg_dis.reg_force_yoffset = 0;
	//sw algo used -

	UNUSED(algoResult);

	DIS_STATS_INFO *disStt = (DIS_STATS_INFO *) ISP_PTR_CAST_VOID(runtime->inPtr.disStt);

	dis_debug_init(disAttr, disConfig, disStt->frameCnt);

	return ret;
}

static CVI_S32 isp_dis_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = disMain(ViPipe, &runtime->inPtr, &runtime->outPtr);

	dis_debug_proc(&runtime->inPtr, &runtime->outPtr);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_dis_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	if (runtime->postprocess_updated) {
		//Set cropping info
		struct dis_info mdisRes;
		CVI_U32 imgW, imgH;
		CVI_U32 marginL, marginR, marginU, marginD;

		imgW = runtime->pstPubAttr.stSnsSize.u32Width;
		imgH = runtime->pstPubAttr.stSnsSize.u32Height;

		marginU = runtime->inPtr.greg_dis.reg_MARGIN_INI_U;//crop ratio
		marginD = runtime->inPtr.greg_dis.reg_MARGIN_INI_D;
		marginL = runtime->inPtr.greg_dis.reg_MARGIN_INI_L;
		marginR = runtime->inPtr.greg_dis.reg_MARGIN_INI_R;

		DIS_STATS_INFO *disStt = (DIS_STATS_INFO *) ISP_PTR_CAST_VOID(runtime->inPtr.disStt);

		mdisRes.sensor_num = ViPipe;
		mdisRes.frm_num = disStt->frameCnt;
		mdisRes.dis_i.start_x = runtime->outPtr.coordiate.xcor;
		mdisRes.dis_i.start_y = runtime->outPtr.coordiate.ycor;
		mdisRes.dis_i.end_x = (imgW - (marginL + marginR)) + mdisRes.dis_i.start_x;
		mdisRes.dis_i.end_y = (imgH - (marginU + marginD)) + mdisRes.dis_i.start_y;

		ISP_LOG_DEBUG("Crop info [%d][%d](%d %d %d %d)\n", mdisRes.sensor_num, mdisRes.frm_num,
			mdisRes.dis_i.start_x, mdisRes.dis_i.start_y, mdisRes.dis_i.end_x, mdisRes.dis_i.end_y);

		CVI_VI_SET_DIS_INFO(mdisRes);

		runtime->postprocess_updated = CVI_FALSE;

		//dump debug here, after CVI_VI_SET_DIS_INFO
		dis_debug_proc_dump(&runtime->inPtr, &runtime->outPtr);
	}

	return ret;
}

static struct isp_dis_ctrl_runtime *_get_dis_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dis_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GMS, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dis_ctrl_set_dis_attr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S *pstDisAttr)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
#ifdef NEW_DIS_CTRL_ARCH
	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstDisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DIS_ATTR_S *p = CVI_NULL;

	isp_dis_ctrl_get_dis_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDisAttr, sizeof(*pstDisAttr));

	runtime->preprocess_updated = CVI_TRUE;
#else
	UNUSED(ViPipe);
	UNUSED(pstDisAttr);

#endif
	return ret;
}

CVI_S32 isp_dis_ctrl_get_dis_attr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S **pstDisAttr)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef NEW_DIS_CTRL_ARCH

	if (pstDisAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct isp_dis_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GMS, (CVI_VOID *) &shared_buffer);
	*pstDisAttr = &shared_buffer->stDisAttr;
#else
	UNUSED(ViPipe);
	UNUSED(pstDisAttr);
#endif
	return ret;
}

CVI_S32 isp_dis_ctrl_set_dis_config(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S *pstDisConfig)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
#ifdef NEW_DIS_CTRL_ARCH
	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (pstDisConfig == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DIS_CONFIG_S *p = CVI_NULL;

	isp_dis_ctrl_get_dis_config(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDisConfig, sizeof(*pstDisConfig));

	runtime->preprocess_updated = CVI_TRUE;
#else
	UNUSED(ViPipe);
	UNUSED(pstDisConfig);
#endif
	return ret;
}

CVI_S32 isp_dis_ctrl_get_dis_config(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S **pstDisConfig)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef NEW_DIS_CTRL_ARCH

	if (pstDisConfig == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct isp_dis_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GMS, (CVI_VOID *) &shared_buffer);
	*pstDisConfig = &shared_buffer->stDisConfig;
#else
	UNUSED(ViPipe);
	UNUSED(pstDisConfig);
#endif
	return ret;
}

CVI_S32 isp_dis_ctrl_get_gms_attr(VI_PIPE ViPipe, struct cvi_vip_isp_gms_config *gmsCfg)
{
#define MAX_GAP (255)
#define MIN_GAP (10)
	//gap >= 10 && gap <=255 (overflow)
	//(x_section_size +1)*3 + x_gap*2 + offset_x + 4 < imgW // 4 cycle by RTL limit
	//(y_section_size +1)*3 + y_gap*2 + offset_y  < imgH

	CVI_U32 imgW, imgH;
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 curSize, gap, limitW, limitH;

	if (ViPipe > (DIS_SENSOR_NUM - 1))
		return CVI_SUCCESS;

	struct isp_dis_ctrl_runtime *runtime = _get_dis_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}


	imgW = runtime->pstPubAttr.stSnsSize.u32Width;
	imgH = runtime->pstPubAttr.stSnsSize.u32Height;

	limitW = imgW - 4;//4 cycle by RTL limit
	limitH = imgH - 0;

	gmsCfg->offset_x = 0;
	gmsCfg->x_gap = MIN_GAP;
	gmsCfg->x_section_size = XHIST_LENGTH - 1;

	curSize = gmsCfg->offset_x + (gmsCfg->x_section_size + 1) * DIS_MAX_WINDOW_X_NUM + gmsCfg->x_gap * 2;
	//curSize = 770;
	if (limitW < curSize) {
		gmsCfg->x_section_size = (limitW - gmsCfg->x_gap * 2) / DIS_MAX_WINDOW_X_NUM - 1;
		while (((gmsCfg->x_section_size - 2) % 4) != 0) {
			gmsCfg->x_gap++;
			gmsCfg->x_section_size = (limitW - gmsCfg->x_gap * 2) / DIS_MAX_WINDOW_X_NUM - 1;
		}
	} else {
		curSize = (gmsCfg->x_section_size + 1) * DIS_MAX_WINDOW_X_NUM;
		gap = (limitW - curSize) / (DIS_MAX_WINDOW_X_NUM + 1);
		if (gap < MIN_GAP) {
			gmsCfg->offset_x = gap;
			gmsCfg->x_gap = MIN_GAP;
		} else if (gap <= MAX_GAP) {
			gmsCfg->offset_x = gap;
			gmsCfg->x_gap = gap;
		} else {//limitW =1794 ,gap =256
			gap = MAX_GAP;
			gmsCfg->x_gap = gap;
			curSize = gmsCfg->offset_x + (gmsCfg->x_section_size + 1) * DIS_MAX_WINDOW_X_NUM
				+ gmsCfg->x_gap * 2;
			//1278
			gmsCfg->offset_x = (imgW - curSize) / 2;
		}
	}

	gmsCfg->offset_y = 0;
	gmsCfg->y_gap = MIN_GAP;
	gmsCfg->y_section_size = YHIST_LENGTH - 1;

	curSize = gmsCfg->offset_y + (gmsCfg->y_section_size + 1) * DIS_MAX_WINDOW_Y_NUM + gmsCfg->y_gap * 2;
	//curSize = 770;
	if (limitH < curSize) {
		gmsCfg->y_section_size = (limitH - gmsCfg->y_gap * 2) / DIS_MAX_WINDOW_Y_NUM - 1;
		while (((gmsCfg->y_section_size - 2) % 4) != 0) {
			gmsCfg->y_gap++;
			gmsCfg->y_section_size = (limitH - gmsCfg->y_gap * 2) / DIS_MAX_WINDOW_Y_NUM - 1;
		}
	} else {
		curSize = (gmsCfg->y_section_size + 1) * DIS_MAX_WINDOW_Y_NUM;
		gap = (limitH - curSize) / (DIS_MAX_WINDOW_Y_NUM + 1);
		if (gap < MIN_GAP) {
			gmsCfg->offset_y = gap;
			gmsCfg->y_gap = MIN_GAP;
		} else if (gap <= MAX_GAP) {
			gmsCfg->offset_y = gap;
			gmsCfg->y_gap = gap;
		} else {//limitH =1794 ,gap =256
			gap = MAX_GAP;
			gmsCfg->y_gap = gap;
			curSize = gmsCfg->offset_y + (gmsCfg->y_section_size + 1) * DIS_MAX_WINDOW_Y_NUM
				+ gmsCfg->y_gap * 2;
			//1278
			gmsCfg->offset_y = (imgH - curSize) / 2;
		}
	}

	return ret;
}

