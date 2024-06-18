/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dpc_ctrl.c
 * Description:
 *
 */

#ifndef ARCH_RTOS_CV181X
#include "cvi_sys.h"
#endif

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"
#include "cvi_vi.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "cvi_isp.h"
#include "dpcm_api.h"
#include <sys/time.h>

#include "isp_dpc_ctrl.h"
#include "isp_algo_dpc.h"
#include "isp_mgr_buf.h"

#define DPC_CALIB_MULTITHREAD

const struct isp_module_ctrl dpc_mod = {
	.init = isp_dpc_ctrl_init,
	.uninit = isp_dpc_ctrl_uninit,
	.suspend = isp_dpc_ctrl_suspend,
	.resume = isp_dpc_ctrl_resume,
	.ctrl = isp_dpc_ctrl_ctrl
};

#ifndef ARCH_RTOS_CV181X
static pthread_mutex_t dpc_lock;     //[VI_MAX_PIPE_NUM];
static pthread_t dpc_tid;            //[VI_MAX_PIPE_NUM];
#endif

static CVI_S32 isp_dpc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dpc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_dpc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dpc_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_dpc_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_dpc_ctrl_check_dpc_dynamic_attr_valid(const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr);
static CVI_S32 isp_dpc_ctrl_check_dpc_static_attr_valid(const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr);

#ifndef ARCH_RTOS_CV181X
static void *ISP_DPCalibrateThread(void *param);
static CVI_S32 isp_dpc_ctrl_check_dpc_calibrate_valid(const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr);
#endif

static struct isp_dpc_ctrl_runtime  *_get_dpc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dpc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_dpc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dpc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dpc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDpc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDpc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dpc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dpc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_dpc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_dpc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_dpc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dpc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(dpc_dynamic_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!dpc_dynamic_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (dpc_dynamic_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(dpc_dynamic_attr, ClusterSize);
		MANUAL(dpc_dynamic_attr, BrightDefectToNormalPixRatio);
		MANUAL(dpc_dynamic_attr, DarkDefectToNormalPixRatio);
		MANUAL(dpc_dynamic_attr, FlatThreR);
		MANUAL(dpc_dynamic_attr, FlatThreG);
		MANUAL(dpc_dynamic_attr, FlatThreB);
		MANUAL(dpc_dynamic_attr, FlatThreMinG);
		MANUAL(dpc_dynamic_attr, FlatThreMinRB);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(dpc_dynamic_attr, ClusterSize, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, BrightDefectToNormalPixRatio, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, DarkDefectToNormalPixRatio, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreR, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreG, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreB, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreMinG, INTPLT_POST_ISO);
		AUTO(dpc_dynamic_attr, FlatThreMinRB, INTPLT_POST_ISO);

		#undef AUTO
	}

	// ParamIn
	// runtime->dpc_param_in.BrightDefectToNormalPixRatio = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
	// runtime->dpc_param_in.DarkDefectToNormalPixRatio = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;

	// ParamOut

	runtime->process_updated = CVI_TRUE;

	UNUSED(algoResult);

	return ret;
}

#if 0
static CVI_S32 isp_dpc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_dpc_main(
		(struct dpc_param_in *)&runtime->dpc_param_in,
		(struct dpc_param_out *)&runtime->dpc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_dpc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_dpc_config *dpc_cfg[2] = {
		(struct cvi_vip_isp_dpc_config *)&(pre_be_addr->tun_cfg[tun_idx].dpc_cfg[0]),
		(struct cvi_vip_isp_dpc_config *)&(pre_be_addr->tun_cfg[tun_idx].dpc_cfg[1]),
	};

	const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;
	const ISP_DP_STATIC_ATTR_S *dpc_static_attr = NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);
	isp_dpc_ctrl_get_dpc_static_attr(ViPipe, &dpc_static_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		for (CVI_U32 idx = 0 ; idx < 2 ; idx++) {
			dpc_cfg[idx]->inst = idx;
			dpc_cfg[idx]->update = 0;
		}
	} else {
		runtime->dpc_cfg.update = 1;
		// runtime->dpc_cfg.inst;
		runtime->dpc_cfg.enable = (dpc_dynamic_attr->Enable || dpc_static_attr->Enable)
									&& !runtime->is_module_bypass;
		runtime->dpc_cfg.staticbpc_enable = dpc_static_attr->Enable;
		runtime->dpc_cfg.dynamicbpc_enable = dpc_dynamic_attr->DynamicDPCEnable;
		runtime->dpc_cfg.cluster_size = runtime->dpc_dynamic_attr.ClusterSize;

		if (dpc_static_attr->Enable) {
			#define DPC_MAX_SIZE 4096
			CVI_U32 bp_cnt = 0;
			CVI_U32 brightMax = dpc_static_attr->BrightCount;
			CVI_U32 darkMax = dpc_static_attr->DarkCount;

			if (brightMax + darkMax > DPC_MAX_SIZE) {
				CVI_U16 total = brightMax + darkMax;

				brightMax =  brightMax * DPC_MAX_SIZE / total;
				darkMax =  darkMax * DPC_MAX_SIZE / total;
			}
			// bp_tbl = { (offsetx) | (offset_y << 12), ....}
			#define CONVERT_BP_FORMAT(bp) ((((bp) & 0x0FFF0000) >> (16-12)) + ((bp) & 0x00000FFF))

			for (CVI_U32 c = 0; c < brightMax; c++, bp_cnt++)
				runtime->dpc_cfg.bp_tbl[bp_cnt] = CONVERT_BP_FORMAT(dpc_static_attr->BrightTable[c]);
			for (CVI_U32 c = 0; c < darkMax; c++, bp_cnt++)
				runtime->dpc_cfg.bp_tbl[bp_cnt] = CONVERT_BP_FORMAT(dpc_static_attr->DarkTable[c]);
			runtime->dpc_cfg.bp_cnt = bp_cnt;

			#undef CONVERT_BP_FORMAT
		}

		struct cvi_isp_dpc_tun_cfg *cfg_dpc  = (struct cvi_isp_dpc_tun_cfg *)&runtime->dpc_cfg.dpc_cfg;

		//TODO@ST Check formula
		cfg_dpc->DPC_3.bits.DPC_R_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_3.bits.DPC_G_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_4.bits.DPC_B_BRIGHT_PIXEL_RATIO = runtime->dpc_dynamic_attr.BrightDefectToNormalPixRatio;
		cfg_dpc->DPC_4.bits.DPC_R_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_5.bits.DPC_G_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_5.bits.DPC_B_DARK_PIXEL_RATIO = runtime->dpc_dynamic_attr.DarkDefectToNormalPixRatio;
		cfg_dpc->DPC_6.bits.DPC_R_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_6.bits.DPC_G_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_6.bits.DPC_B_DARK_PIXEL_MINDIFF = 11;
		cfg_dpc->DPC_7.bits.DPC_R_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_7.bits.DPC_G_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_7.bits.DPC_B_BRIGHT_PIXEL_UPBOUD_RATIO = 236;
		cfg_dpc->DPC_8.bits.DPC_FLAT_THRE_MIN_RB = runtime->dpc_dynamic_attr.FlatThreMinRB;
		cfg_dpc->DPC_8.bits.DPC_FLAT_THRE_MIN_G = runtime->dpc_dynamic_attr.FlatThreMinG;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_R = runtime->dpc_dynamic_attr.FlatThreR;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_G = runtime->dpc_dynamic_attr.FlatThreG;
		cfg_dpc->DPC_9.bits.DPC_FLAT_THRE_B = runtime->dpc_dynamic_attr.FlatThreB;

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
			runtime->dpc_cfg.inst = idx;
			runtime->dpc_cfg.enable = runtime->dpc_cfg.enable && map_pipe_to_enable[idx];
			memcpy(dpc_cfg[idx], &(runtime->dpc_cfg), sizeof(struct cvi_vip_isp_dpc_config));
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

#ifndef ARCH_RTOS_CV181X
static CVI_S32 dump_raw(VI_PIPE ViPipe, CVI_U16 *bayerBuffer,
					CVI_U16 width, CVI_U16 height, BAYER_FORMAT_E bayerFormat)
{
	char img_name[128] = {0,}, order_id[8] = {0,};
	FILE *output;
	struct timeval tv1;

	switch (bayerFormat) {
	case BAYER_FORMAT_BG:
		snprintf(order_id, sizeof(order_id), "BG");
		break;
	case BAYER_FORMAT_GB:
		snprintf(order_id, sizeof(order_id), "GB");
		break;
	case BAYER_FORMAT_GR:
		snprintf(order_id, sizeof(order_id), "GR");
		break;
	case BAYER_FORMAT_RG:
	default:
		snprintf(order_id, sizeof(order_id), "RG");
		break;
	}

	gettimeofday(&tv1, NULL);
	snprintf(img_name, sizeof(img_name), "./vi_%d_byrid_%s_w_%d_h_%d_bit_%d_tv_%ld_%ld.raw",
			ViPipe, order_id,
			width,
			height,
			16, tv1.tv_sec, tv1.tv_usec
			);

	output = fopen(img_name, "wb");
	if (output) {
		ISP_LOG_DEBUG("dump raw to %s\n", img_name);
		size_t bayerBufferSize = width
							* height
							* 2;
		fwrite(bayerBuffer,
			bayerBufferSize,
			1, output);
		fclose(output);
	} else {
		ISP_LOG_DEBUG("Open dump destnation file %s fail\n", img_name);
	}
	return 0;
}

static CVI_VOID crop_image(CVI_U32 stride, ISP_SNS_SYNC_INFO_S *snsCropInfo,
				CVI_U8 *src, CVI_U8 *dst, CVI_FLOAT pixel_per_byte)
{
	if ((src == NULL) || (dst == NULL)) {
		ISP_LOG_ERR("input error src(%p), dst(%p)\n", src, dst);
	}

	// Two pixel packed with three bytes in raw12 format

	// ofs_x = pixel
	CVI_U32 ofs_x = (snsCropInfo->ispCfg.img_size[0].stWndRect.s32X * pixel_per_byte);
	// ofs_y = line.
	CVI_U32 ofs_y = snsCropInfo->ispCfg.img_size[0].stWndRect.s32Y;
	CVI_U32 img_w = (snsCropInfo->ispCfg.img_size[0].stWndRect.u32Width * pixel_per_byte);
	CVI_U32 img_h = snsCropInfo->ispCfg.img_size[0].stWndRect.u32Height;
	CVI_U32 sns_w = stride;
	CVI_U32 sns_h = snsCropInfo->ispCfg.img_size[0].stSnsSize.u32Height;

	ISP_LOG_DEBUG("ofs_x/y(%d, %d), img_w/h(%d, %d), sns_w/h(%d, %d)\n",
		ofs_x, ofs_y, img_w, img_h, sns_w, sns_h);

	CVI_U8 *addr_src = src + ofs_y * sns_w + ofs_x;
	CVI_U8 *addr_dst = dst;

	for (CVI_U32 i = 0 ; i < img_h ; i++) {
		memcpy(addr_dst, addr_src, img_w);
		addr_src += sns_w;
		addr_dst += img_w;
	}

	UNUSED(sns_h);
}

static CVI_S32 ISP_GetRawBuffer(VI_PIPE ViPipe, CVI_U8 *bayerBuffer, BAYER_FORMAT_E *bayerFormat)
{
	VIDEO_FRAME_INFO_S stVideoFrame;
	VI_DUMP_ATTR_S attr;
	ISP_SNS_SYNC_INFO_S snsCropInfo;

	memset(&stVideoFrame, 0, sizeof(stVideoFrame));

	stVideoFrame.stVFrame.enPixelFormat = PIXEL_FORMAT_RGB_BAYER_12BPP;

	// Set Attr
	attr.bEnable = 1;
	attr.u32Depth = 0;
	attr.enDumpType = VI_DUMP_TYPE_RAW;
	CVI_VI_SetPipeDumpAttr(ViPipe, &attr);

	// Check Attr effective
	attr.bEnable = 0;
	attr.enDumpType = VI_DUMP_TYPE_IR;
	CVI_VI_GetPipeDumpAttr(ViPipe, &attr);

	if ((attr.bEnable != 1) || (attr.enDumpType != VI_DUMP_TYPE_RAW)) {
		ISP_LOG_ERR("Enable(%d), DumpType(%d)\n", attr.bEnable, attr.enDumpType);
	}

	if (CVI_VI_GetPipeFrame(ViPipe, &stVideoFrame, 1000) != CVI_SUCCESS) {
		ISP_LOG_ERR("CVI_VI_GetPipeFrame (Pipe: %d) timeout\n", ViPipe);
		return CVI_FAILURE;
	}

	size_t image_size = stVideoFrame.stVFrame.u32Length[0];
	CVI_U32 stride = (image_size/stVideoFrame.stVFrame.u32Height);

	*bayerFormat = stVideoFrame.stVFrame.enBayerFormat;

	if (attr.enDumpType == VI_DUMP_TYPE_RAW) {
		unsigned char *ptr = ISP_MALLOC(image_size);

		if (ptr == NULL) {
			ISP_LOG_ERR("malloc size:%zu fail\n", image_size);
			CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
			return CVI_FAILURE;
		}

		stVideoFrame.stVFrame.pu8VirAddr[0]
			= CVI_SYS_Mmap(stVideoFrame.stVFrame.u64PhyAddr[0], image_size);

#if SDK_LIB_BIT == 32
		ISP_LOG_DEBUG("paddr(%llu) vaddr(0x%p)\n",
			stVideoFrame.stVFrame.u64PhyAddr[0], stVideoFrame.stVFrame.pu8VirAddr[0]);
#else
		ISP_LOG_DEBUG("paddr(%lu) vaddr(0x%p)\n",
			stVideoFrame.stVFrame.u64PhyAddr[0], stVideoFrame.stVFrame.pu8VirAddr[0]);
#endif

		memcpy(ptr, (const void *)stVideoFrame.stVFrame.pu8VirAddr[0], image_size);
		isp_sensor_get_crop_info(ViPipe, &snsCropInfo);

		if (stVideoFrame.stVFrame.enCompressMode == COMPRESS_MODE_NONE) {
			CVI_U32 cropWidth = snsCropInfo.ispCfg.img_size[0].stWndRect.u32Width;
			CVI_U32 cropHeight = snsCropInfo.ispCfg.img_size[0].stWndRect.u32Height;
			CVI_U8 *pCropImg = ISP_MALLOC(cropWidth * cropHeight * 1.5);

			if (pCropImg == NULL) {
				ISP_LOG_ERR("malloc size:%f fail\n", cropWidth * cropHeight * 1.5);
				CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
				free(ptr);
				return CVI_FAILURE;
			}

			crop_image(stride, &snsCropInfo, ptr, pCropImg, 1.5);
			Bayer_12bit_2_16bit(pCropImg, (CVI_U16 *)bayerBuffer, cropWidth, cropHeight, cropWidth);
			free(pCropImg); pCropImg = NULL;
		} else if (stVideoFrame.stVFrame.enCompressMode == COMPRESS_MODE_TILE) {
			RAW_INFO rawInfo = {0};

			rawInfo.width = stVideoFrame.stVFrame.u32Width;
			rawInfo.height = stVideoFrame.stVFrame.u32Height;
			rawInfo.stride = stride;
			rawInfo.buffer = ptr;
			CVI_U8 *pDecImg = ISP_MALLOC(rawInfo.width * rawInfo.height * 2);

			if (pDecImg == NULL) {
				ISP_LOG_ERR("malloc size:%u fail\n", rawInfo.width * rawInfo.height * 2);
				CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
				free(ptr);
				return CVI_FAILURE;
			}

			if (decoderRaw(rawInfo, pDecImg) != CVI_SUCCESS) {
				ISP_LOG_ERR("decoderRaw err\n");
				CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
				free(ptr);
				return CVI_FAILURE;
			}
			crop_image(rawInfo.width * 2, &snsCropInfo, pDecImg, bayerBuffer, 2);
			free(pDecImg); pDecImg = NULL;
		} else {
			ISP_LOG_ERR("not support [%d]compress mode\n",
				stVideoFrame.stVFrame.enCompressMode);
			CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
			free(ptr);
			return CVI_FAILURE;
		}

		CVI_SYS_Munmap((void *)stVideoFrame.stVFrame.pu8VirAddr[0], image_size);

		free(ptr); ptr = NULL;
	} else {
		CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);
		return CVI_FAILURE;
	}

	CVI_VI_ReleasePipeFrame(ViPipe, &stVideoFrame);

	return CVI_SUCCESS;
}

typedef struct {
	CVI_U32 pixel;
	CVI_U16 count;
} PIXEL_ST;

#define BIT8_TO_12(v) ((v)*16)
#define BIT12_TO_8(v) ((v)/16)
#define BAD_RATIO 0.9
#define MODIFY_AE_SETTING	0
static CVI_S32 ISP_SetDPCalibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	if ((ViPipe < 0) || (ViPipe >= VI_MAX_PIPE_NUM)) {
		ISP_LOG_ERR("ViPipe %d value error\n", ViPipe);
		return -ENODEV;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ISP_DP_CALIB_ATTR_S *pattr = NULL;

	pattr = ISP_MALLOC(sizeof(ISP_DP_CALIB_ATTR_S));
	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "malloc fail");
		return CVI_FAILURE;
	}

	memcpy(pattr, pstDPCalibAttr, sizeof(ISP_DP_CALIB_ATTR_S));

	if (pattr->EnableDetect) {
		CVI_U16 width = pstIspCtx->stSnsImageMode.u16Width;
		CVI_U16 height = pstIspCtx->stSnsImageMode.u16Height;
		CVI_U16 stride = width;	//unit is pixel
		PIXEL_ST *pBadPixelST = NULL;
		CVI_U16 *bayerBuffer16 = NULL;

		pBadPixelST = ISP_CALLOC(1, sizeof(PIXEL_ST) * STATIC_DP_COUNT_MAX);
		if (pBadPixelST == NULL) {
			ISP_LOG_ERR("malloc failed, pBadPixelST(%p)\n", (void *)pBadPixelST);
			goto ERROR;
		}
		bayerBuffer16 = ISP_CALLOC(1, width * height * sizeof(CVI_U16));	//12bit data save in 16bit

		if (!bayerBuffer16) {
			ISP_LOG_ERR("Calloc failed, bayerBuffer16(%p)\n", (void *)bayerBuffer16);
			goto ERROR;
		}

		BAYER_FORMAT_E bayerFormat;

		ISP_LOG_DEBUG("DPC RAW: width(%u), height(%d)\n", width, height);

		CVI_U32 reg_image_cnt = 0;

		pattr->FinishThresh = pattr->StartThresh;
		pattr->Count = 0;

#if MODIFY_AE_SETTING
		ISP_PUB_ATTR_S stPubAttr = {}, stPubAttrOrig = {};
		ISP_EXPOSURE_ATTR_S stExpAttr = {}, stExpAttrOrig = {};

		if (pattr->StaticDPType == ISP_STATIC_DP_BRIGHT) {
			if (CVI_ISP_GetPubAttr(ViPipe, &stPubAttrOrig) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_GetPubAttr Pipe: %d fail\n", ViPipe);
			}

			stPubAttr = stPubAttrOrig;
			stPubAttr.f32FrameRate = 5;

			if (CVI_ISP_SetPubAttr(ViPipe, &stPubAttr) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_SetPubAttr Pipe: %d for DPC fail\n", ViPipe);
			}

			if (CVI_ISP_GetExposureAttr(ViPipe, &stExpAttrOrig) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_GetExposureAttr Pipe: %d fail\n", ViPipe);
			}
			stExpAttr = stExpAttrOrig;
			stExpAttr.enOpType = OP_TYPE_MANUAL;
			stExpAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
			stExpAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
			stExpAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
			stExpAttr.stManual.enISPDGainOpType = OP_TYPE_MANUAL;
			stExpAttr.stManual.u32ExpTime = 200000;		// 200ms
			stExpAttr.stManual.u32AGain = 0x400;		// 1x
			stExpAttr.stManual.u32DGain = 0x400;		// 1x
			stExpAttr.stManual.u32ISPDGain = 0x400;		// 1x

			if (CVI_ISP_SetExposureAttr(ViPipe, &stExpAttr) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_SetExposureAttr Pipe: %d for DPC fail\n", ViPipe);
			}
			usleep(200000);
		}
#endif	//MODIFY_AE_SETTING

		time_t currTime = time(0);
		time_t endTime = currTime + pattr->TimeLimit;
		DPC_Input input = {
			.image = bayerBuffer16,
			.width = width,
			.height = height,
			.stride = stride,
			.abs_thresh = (pattr->StartThresh << 2),
			.dpType = pattr->StaticDPType,
			.table_max_size = STATIC_DP_COUNT_MAX,
			.verbose = 0,
			.debug = 0,
		};

#ifdef DPC_DEBUG
		DPC_PrintInput(&input);
#endif // DPC_DEBUG

		// Detect
		do {
			currTime = time(0);
			CVI_U32 Table[STATIC_DP_COUNT_MAX];

			// get frame
			if (ISP_GetRawBuffer(ViPipe, (CVI_U8 *)bayerBuffer16, &bayerFormat) != CVI_SUCCESS) {
				ISP_LOG_DEBUG("ISP_GetRawBuffer timeout\n");
				continue;
			}

			if (pattr->saveFileEn)
				dump_raw(ViPipe, bayerBuffer16, width, height, bayerFormat);

			DPC_Output output = {
				.table = Table,
				.table_size = 0,
			};

			DPC_Calib(&input, &output);

			for (CVI_U32 i = 0; i < output.table_size; ++i) {
				for (CVI_U32 j = 0; j < STATIC_DP_COUNT_MAX; ++j) {
					if ((pBadPixelST + j)->pixel == 0) {
						(pBadPixelST + j)->pixel = Table[i];
						(pBadPixelST + j)->count += 1;
						break;
					} else if ((pBadPixelST + j)->pixel == Table[i]) {
						(pBadPixelST + j)->count += 1;
						break;
					}
				}
			}

			// next loop
			reg_image_cnt++;
		} while (currTime < endTime);

		for (CVI_U32 j = 0; j < STATIC_DP_COUNT_MAX; ++j) {
			if ((pBadPixelST + j)->pixel == 0) {
				break;
			} else if ((pBadPixelST + j)->count > reg_image_cnt * BAD_RATIO) {
				pattr->Table[pattr->Count++] = (pBadPixelST + j)->pixel;
			}
		}

#ifdef DPC_DEBUG
		DPC_Output output = {
			.table = pattr->Table,
			.table_size = pattr->Count,
		};

		DPC_PrintOutput(&input, &output);
#endif // DPC_DEBUG
#if MODIFY_AE_SETTING
		if (pattr->StaticDPType == ISP_STATIC_DP_BRIGHT) {
			if (CVI_ISP_SetPubAttr(ViPipe, &stPubAttrOrig) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_SetPubAttr Pipe: %d fail\n", ViPipe);
			}

			if (CVI_ISP_SetExposureAttr(ViPipe, &stExpAttrOrig) != CVI_SUCCESS) {
				ISP_LOG_ERR("CVI_ISP_SetExposureAttr Pipe: %d fail\n", ViPipe);
			}
		}

		if (reg_image_cnt == 0) {
			ISP_LOG_ERR("Bad pixel detect timeout\n");
			pattr->Status = ISP_STATUS_TIMEOUT;
			goto ERROR;
		}
#endif

		pattr->Status = ISP_STATUS_SUCCESS;

ERROR:
		pattr->EnableDetect = 0;

		const ISP_DP_CALIB_ATTR_S *p = CVI_NULL;

		isp_dpc_ctrl_get_dpc_calibrate(ViPipe, &p);
		memcpy((CVI_VOID *) p, pattr, sizeof(ISP_DP_CALIB_ATTR_S));

		if (bayerBuffer16 != NULL)
			free(bayerBuffer16);
		if (pattr != NULL)
			free(pattr);
		if (pBadPixelST != NULL)
			free(pBadPixelST);
	} else {
		if (pattr != NULL)
			free(pattr);
	}
	return CVI_SUCCESS;
}

static void *ISP_DPCalibrateThread(void *param)
{
	VI_PIPE ViPipe = *(VI_PIPE *)param;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (!runtime)
		return 0;

#ifdef DPC_CALIB_MULTITHREAD
	pthread_detach(pthread_self());
	pthread_mutex_lock(&dpc_lock);
#endif // DPC_CALIB_MULTITHREAD

	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);
	ISP_SetDPCalibrate(ViPipe, &shared_buffer->stDPCalibAttr);

#ifdef DPC_CALIB_MULTITHREAD
	pthread_mutex_unlock(&dpc_lock);
	pthread_exit(NULL);
#endif // DPC_CALIB_MULTITHREAD

	return 0;
}
#endif

static CVI_S32 set_dpc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

		const ISP_DP_DYNAMIC_ATTR_S *dpc_dynamic_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		if (runtime == CVI_NULL) {
			return CVI_FAILURE;
		}

		isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &dpc_dynamic_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DPCEnable = dpc_dynamic_attr->Enable;
		pProcST->DPCisManualMode = dpc_dynamic_attr->enOpType;
		//manual or auto
		pProcST->DPCClusterSize = runtime->dpc_dynamic_attr.ClusterSize;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static struct isp_dpc_ctrl_runtime  *_get_dpc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dpc_ctrl_check_dpc_dynamic_attr_valid(const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDPCDynamicAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDPCDynamicAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDPCDynamicAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstDPCDynamicAttr, ClusterSize, 0x0, 0x3);

	return ret;
}

static CVI_S32 isp_dpc_ctrl_check_dpc_static_attr_valid(const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDPStaticAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDPStaticAttr, BrightCount, 0x0, 0xFFF);
	CHECK_VALID_CONST(pstDPStaticAttr, DarkCount, 0x0, 0xFFF);
	CHECK_VALID_ARRAY_1D(pstDPStaticAttr, BrightTable, STATIC_DP_COUNT_MAX, 0x0, 0x1fff1fff);
	CHECK_VALID_ARRAY_1D(pstDPStaticAttr, DarkTable, STATIC_DP_COUNT_MAX, 0x0, 0x1fff1fff);

	return ret;
}

#ifndef ARCH_RTOS_CV181X
static CVI_S32 isp_dpc_ctrl_check_dpc_calibrate_valid(const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDPCalibAttr, EnableDetect, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDPCalibAttr, StaticDPType, ISP_STATIC_DP_BRIGHT, ISP_STATIC_DP_DARK);
	// CHECK_VALID_CONST(pstDPCalibAttr, StartThresh, 0x0, 0xff);
	CHECK_VALID_CONST(pstDPCalibAttr, CountMax, 0x0, 0xfff);
	CHECK_VALID_CONST(pstDPCalibAttr, CountMin, 0x0, pstDPCalibAttr->CountMax);
	CHECK_VALID_CONST(pstDPCalibAttr, TimeLimit, 0x0, 0x640);
	// CHECK_VALID_CONST(pstDPCalibAttr, saveFileEn, CVI_FALSE, CVI_TRUE);

	// read only
	// CHECK_VALID_ARRAY_1D(pstDPCalibAttr, Table, STATIC_DP_COUNT_MAX, 0x0, 0x1fff1fff);
	// CHECK_VALID_CONST(pstDPCalibAttr, FinishThresh, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDPCalibAttr, Count, 0x0, 0xfff);
	// CHECK_VALID_CONST(pstDPCalibAttr, Status, 0x0, 0x2);

	return ret;
}
#endif

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dpc_ctrl_get_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S **pstDPCDynamicAttr)
{
	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	*pstDPCDynamicAttr = &shared_buffer->stDPCDynamicAttr;

	return ret;
}

CVI_S32 isp_dpc_ctrl_set_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr)
{
	if (pstDPCDynamicAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dpc_ctrl_check_dpc_dynamic_attr_valid(pstDPCDynamicAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DP_DYNAMIC_ATTR_S *p = CVI_NULL;

	isp_dpc_ctrl_get_dpc_dynamic_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDPCDynamicAttr, sizeof(*pstDPCDynamicAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_dpc_ctrl_get_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S **pstDPStaticAttr)
{
	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	*pstDPStaticAttr = &shared_buffer->stDPStaticAttr;

	return ret;
}

CVI_S32 isp_dpc_ctrl_set_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr)
{
	if (pstDPStaticAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dpc_ctrl_check_dpc_static_attr_valid(pstDPStaticAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DP_STATIC_ATTR_S *p = CVI_NULL;

	isp_dpc_ctrl_get_dpc_static_attr(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDPStaticAttr, sizeof(*pstDPStaticAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

#ifndef ARCH_RTOS_CV181X
CVI_S32 isp_dpc_ctrl_get_dpc_calibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S **pstDPCalibAttr)
{
	if (pstDPCalibAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

#ifdef DPC_CALIB_MULTITHREAD
	pthread_mutex_lock(&dpc_lock);
#endif // DPC_CALIB_MULTITHREAD

	struct isp_dpc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DPC, (CVI_VOID *) &shared_buffer);

	*pstDPCalibAttr = &shared_buffer->stDPCalibAttr;

#ifdef DPC_CALIB_MULTITHREAD
	pthread_mutex_unlock(&dpc_lock);
#endif // DPC_CALIB_MULTITHREAD

	return ret;
}

CVI_S32 isp_dpc_ctrl_set_dpc_calibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr)
{
	if (pstDPCalibAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dpc_ctrl_runtime *runtime = _get_dpc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dpc_ctrl_check_dpc_calibrate_valid(pstDPCalibAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	if (pstDPCalibAttr->EnableDetect == CVI_FALSE)
		return ret;

	const ISP_DP_CALIB_ATTR_S *p = CVI_NULL;

	isp_dpc_ctrl_get_dpc_calibrate(ViPipe, &p);

	memcpy((CVI_VOID *) p, pstDPCalibAttr, sizeof(*pstDPCalibAttr));

#ifdef DPC_CALIB_MULTITHREAD
	struct sched_param param;
	pthread_attr_t attr;

	param.sched_priority = 80;
	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	pthread_attr_setschedparam(&attr, &param);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_create(&dpc_tid, &attr, ISP_DPCalibrateThread, (void *)&ViPipe);
#else
	ISP_DPCalibrateThread((void *)&ViPipe);
#endif // DPC_CALIB_MULTITHREAD

	return CVI_SUCCESS;
}
#endif

