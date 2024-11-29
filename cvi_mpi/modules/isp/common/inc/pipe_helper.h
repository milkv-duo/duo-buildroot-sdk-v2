/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: pipe_helper.h
 * Description:
 *
 */

#ifndef _PIPE_HELPER_H_
#define _PIPE_HELPER_H_

#include "../sample/common/sample_comm.h"

static SAMPLE_VI_CONFIG_S stViConfig;
static SIZE_S stInputSize;
static PIC_SIZE_E enSensorPicSize;

#define _H264_VENC_CHN  0
#define _JPEG_VENC_CHN  1

static inline CVI_S32 __SAMPLE_PLAT_SYS_INIT(SIZE_S stSize)
{
	VB_CONFIG_S	   stVbConf;
	CVI_U32        u32BlkSize, u32BlkRotSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	COMPRESS_MODE_E    enCompressMode   = COMPRESS_MODE_NONE;

	memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
	stVbConf.u32MaxPoolCnt		= 1;

	u32BlkSize = COMMON_GetPicBufferSize(stSize.u32Width, stSize.u32Height, SAMPLE_PIXEL_FORMAT,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkRotSize = COMMON_GetPicBufferSize(stSize.u32Height, stSize.u32Width, SAMPLE_PIXEL_FORMAT,
		DATA_BITWIDTH_8, enCompressMode, DEFAULT_ALIGN);
	u32BlkSize = MAX(u32BlkSize, u32BlkRotSize);

	stVbConf.astCommPool[0].u32BlkSize	= u32BlkSize;
	stVbConf.astCommPool[0].u32BlkCnt	= 3;
	stVbConf.astCommPool[0].enRemapMode	= VB_REMAP_MODE_CACHED;

	printf("common pool[0] BlkSize %d\n", u32BlkSize);

	s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "system init failed with %#x\n", s32Ret);
	}

	return s32Ret;
}

static inline CVI_S32 sys_vi_init(void)
{
	MMF_VERSION_S stVersion;
	SAMPLE_INI_CFG_S	   stIniCfg = {
		.enSource  = VI_PIPE_FRAME_SOURCE_DEV,
		.devNum    = 1,
		.enSnsType = SONY_IMX327_MIPI_2M_30FPS_12BIT,
		.enWDRMode = WDR_MODE_NONE,
		.s32BusId  = 3,
		.s32SnsI2cAddr = -1,
		.MipiDev   = 0xFF,
		.u8UseDualSns = 0,
		.enSns2Type = SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT,
		.s32Sns2BusId = 0,
		.s32Sns2I2cAddr = -1,
		.Sns2MipiDev = 0xFF,
	};

	PIC_SIZE_E enPicSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	LOG_LEVEL_CONF_S log_conf;

	CVI_SYS_GetVersion(&stVersion);
	SAMPLE_PRT("MMF Version:%s\n", stVersion.version);

	log_conf.enModId = CVI_ID_LOG;
	log_conf.s32Level = CVI_DBG_INFO;
	CVI_LOG_SetLevelConf(&log_conf);

	// Get config from ini if found.
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		SAMPLE_PRT("Parse complete\n");
	}

	//Set sensor number
	CVI_VI_SetDevNum(stIniCfg.devNum);

	/************************************************
	 * step1:  Config VI
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS)
		return s32Ret;

	/************************************************
	 * step2:  Get input size
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType, &enPicSize);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n", s32Ret);
		return s32Ret;
	}

	enSensorPicSize = enPicSize;

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stInputSize);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
		return s32Ret;
	}

	/************************************************
	 * step3:  Init modules
	 ************************************************/
	s32Ret = __SAMPLE_PLAT_SYS_INIT(stInputSize);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "sys init failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	{ // set online mode
		VI_VPSS_MODE_S stVIVPSSMode;
		VPSS_MODE_S stVPSSMode;

		stVIVPSSMode.aenMode[0] = VI_OFFLINE_VPSS_ONLINE;

		s32Ret = CVI_SYS_SetVIVPSSMode(&stVIVPSSMode);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_SYS_SetVIVPSSMode failed with %#x\n", s32Ret);
			return s32Ret;
		}

		//stVPSSMode.enMode = VPSS_MODE_DUAL;
		stVPSSMode.enMode = VPSS_MODE_SINGLE;
		stVPSSMode.aenInput[0] = VPSS_INPUT_ISP;
		stVPSSMode.ViPipe[0] = 0;
		stVPSSMode.aenInput[1] = VPSS_INPUT_MEM;
		stVPSSMode.ViPipe[1] = 0;

		s32Ret = CVI_SYS_SetVPSSModeEx(&stVPSSMode);
		if (s32Ret != CVI_SUCCESS) {
			CVI_TRACE_LOG(CVI_DBG_ERR, "CVI_SYS_SetVPSSModeEx failed with %#x\n", s32Ret);
			return s32Ret;
		}
	}

	s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "vi init failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	return s32Ret;
}

static inline CVI_S32 vpss_init(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	VPSS_GRP VpssGrp = 0;
	VPSS_CHN VpssChn = VPSS_CHN0;
	VPSS_GRP_ATTR_S stVpssGrpAttr;

	CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = { 0 };
	VPSS_CHN_ATTR_S astVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

	printf("sensor input size: %dx%d\n", stInputSize.u32Width, stInputSize.u32Height);

	stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
	stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
	stVpssGrpAttr.enPixelFormat = VI_PIXEL_FORMAT;
	stVpssGrpAttr.u32MaxW = stInputSize.u32Width;
	stVpssGrpAttr.u32MaxH = stInputSize.u32Height;
	stVpssGrpAttr.u8VpssDev = 0;

	abChnEnable[0] = CVI_TRUE;
	astVpssChnAttr[VpssChn].u32Width                    = stInputSize.u32Width;
	astVpssChnAttr[VpssChn].u32Height                   = stInputSize.u32Height;

	astVpssChnAttr[VpssChn].enVideoFormat               = VIDEO_FORMAT_LINEAR;
	astVpssChnAttr[VpssChn].enPixelFormat               = PIXEL_FORMAT_YUV_PLANAR_420;
	astVpssChnAttr[VpssChn].stFrameRate.s32SrcFrameRate = 25;
	astVpssChnAttr[VpssChn].stFrameRate.s32DstFrameRate = 25;
	astVpssChnAttr[VpssChn].u32Depth                    = 1;
	astVpssChnAttr[VpssChn].bMirror                     = CVI_FALSE;
	astVpssChnAttr[VpssChn].bFlip                       = CVI_FALSE;
	astVpssChnAttr[VpssChn].stAspectRatio.enMode        = ASPECT_RATIO_NONE;
	astVpssChnAttr[VpssChn].stAspectRatio.bEnableBgColor = CVI_TRUE;
	astVpssChnAttr[VpssChn].stNormalize.bEnable         = CVI_FALSE;

	s32Ret = SAMPLE_COMM_VPSS_Init(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "init vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VPSS_Start(VpssGrp, abChnEnable, &stVpssGrpAttr, astVpssChnAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "start vpss group failed. s32Ret: 0x%x !\n", s32Ret);
		return s32Ret;
	}

	return s32Ret;
}

#define DEFAULT_BS_MODE BS_MODE_QUERY_STAT
//#define DEFAULT_RC_MODE SAMPLE_RC_FIXQP
#define DEFAULT_RC_MODE SAMPLE_RC_CBR
#define DEFAULT_FPS 25
#define DEFAULT_GOP (DEFAULT_FPS * 2)
#define DEFAULT_IQP DEF_IQP
#define DEFAULT_PQP DEF_PQP
//#define DEFAULT_KBITRATE 256
#define DEFAULT_KBITRATE 8000
#define DEFAULT_FIRSTQP 34
#define DEFAULT_STAT_TIME 2
#define DEFAULT_AVBR_FRM_LOST_OPEN 1
#define DEFAULT_H264_PROFILE_DEFAULT CVI_H264_PROFILE_DEFAULT

#define CVI_H26x_IP_QP_DELTA_DEFAULT 2
#define CVI_H26x_IP_QP_DELTA_MIN -10
#define CVI_H26x_IP_QP_DELTA_MAX 30

/* define reference from middleware cvi_venc.h, need to be removed in future*/
#define CVI_H26x_INITIAL_DELAY_DEFAULT 1000 //CVI_INITIAL_DELAY_DEFAULT

#define CVI_H26x_MAX_I_PROP_DEFAULT CVI_H26X_MAX_I_PROP_DEFAULT
#define CVI_H26x_MAX_I_PROP_MAX 100
#define CVI_H26x_MAX_I_PROP_MIN 0

#define CVI_H264_CHROMA_QP_OFFSET_DEFAULT 0
#define CVI_H265_CB_QP_OFFSET_DEFAULT 0
#define CVI_H265_CR_QP_OFFSET_DEFAULT 0

#define CVI_H26x_MIN_I_PROP_DEFAULT CVI_H26X_MIN_I_PROP_DEFAULT

#define CVI_H26x_THRD_LV_DEFAULT CVI_H26X_THRDLV_DEFAULT
#define CVI_H26x_THRD_LV_MIN CVI_H26X_THRDLV_MIN
#define CVI_H26x_THRD_LV_MAX CVI_H26X_THRDLV_MAX

#define CVI_H264_ENTROPY_MODE_DEFAULT CVI_H264_ENTROPY_DEFAULT
#define CVI_H264_ENTROPY_MODE_MIN 0
#define CVI_H264_ENTROPY_MODE_MAX 1

#define CVI_H26x_CHANGE_POS_DEFAULT 90
#define CVI_H26x_CHANGE_POS_MIN 50
#define CVI_H26x_CHANGE_POS_MAX 100

#define CVI_H26x_MAX_BITRATE_DEFAULT 5000
#define CVI_H26x_MAX_BITRATE_MIN 0
#define CVI_H26x_MAX_BITRATE_MAX 1000000

#define CVI_H26x_MIN_STILL_PERCENT_DEFAULT 10
#define CVI_H26x_MIN_STILL_PERCENT_MIN 5
#define CVI_H26x_MIN_STILL_PERCENT_MAX 100

#define CVI_H26x_MAX_STILL_QP_DEFAULT 48
#define CVI_H26x_MAX_STILL_QP_MIN 0
#define CVI_H26x_MAX_STILL_QP_MAX 51

#define CVI_H26x_MOTION_SENSITIVITY_DEFAULT 80
#define CVI_H26x_MOTION_SENSITIVITY_MIN 0
#define CVI_H26x_MOTION_SENSITIVITY_MAX 256

#define CVI_H26x_AVB_FRM_GAP_DEFAULT 3
#define CVI_H26x_AVB_FRM_GAP_MIN 0
#define CVI_H26x_AVB_FRM_GAP_MAX 100

#define CVI_H26x_AVB_PURE_STILL_THR_DEFAULT 4
#define CVI_H26x_AVB_PURE_STILL_THR_MIN 0
#define CVI_H26x_AVB_PURE_STILL_THR_MAX 100

#define CVI_ROI_RELATIVE_QP false
#define CVI_ROI_ABSOLUTE_QP true
#define CVI_ROI_MAX_QP 51
#define CVI_ROI_MIN_QP -51

static inline CVI_S32 venc_init(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	PIC_SIZE_E enSize = enSensorPicSize;

	VENC_CHN VencChn = _H264_VENC_CHN;
	VENC_GOP_MODE_E enGopMode = VENC_GOPMODE_NORMALP;
	VENC_GOP_ATTR_S stGopAttr;
	SAMPLE_RC_E enRcMode;
	PAYLOAD_TYPE_E enPayLoad = PT_H264;

	chnInputCfg pIc;

	SAMPLE_COMM_VENC_InitChnInputCfg(&pIc);
	strcpy(pIc.codec, "264");

	pIc.u32Profile = DEFAULT_H264_PROFILE_DEFAULT;
	pIc.bsMode = DEFAULT_BS_MODE;  // --getBsMode=0
	pIc.rcMode = DEFAULT_RC_MODE;
	pIc.statTime = DEFAULT_STAT_TIME;
	pIc.gop = DEFAULT_GOP;
	pIc.iqp = DEFAULT_IQP;
	pIc.pqp = DEFAULT_PQP;
	pIc.bitrate = DEFAULT_KBITRATE;
	pIc.maxbitrate = CVI_H26x_MAX_BITRATE_DEFAULT;
	pIc.firstFrmstartQp = DEFAULT_FIRSTQP;
	pIc.initialDelay = CVI_H26x_INITIAL_DELAY_DEFAULT;
	pIc.u32ThrdLv = CVI_H26x_THRD_LV_DEFAULT;
	pIc.h264EntropyMode = CVI_H264_ENTROPY_MODE_DEFAULT;
	pIc.maxIprop = CVI_H26x_MAX_I_PROP_DEFAULT;
	pIc.minIprop = CVI_H26x_MIN_I_PROP_DEFAULT;
	pIc.h264ChromaQpOffset = CVI_H264_CHROMA_QP_OFFSET_DEFAULT;
	pIc.u32IntraCost = CVI_H26X_INTRACOST_DEFAULT;
	pIc.u32RowQpDelta = CVI_H26X_ROW_QP_DELTA_DEFAULT;
	pIc.num_frames = -1;
	pIc.srcFramerate = DEFAULT_FPS;
	pIc.framerate = DEFAULT_FPS;
	pIc.maxQp = DEF_264_MAXIQP;
	pIc.minQp = DEF_264_MINIQP;
	pIc.maxIqp = DEF_264_MAXQP;
	pIc.minIqp = DEF_264_MINQP;
	pIc.s32IPQpDelta = CVI_H26x_IP_QP_DELTA_DEFAULT;
	pIc.s32ChangePos = CVI_H26x_CHANGE_POS_DEFAULT;
	pIc.s32MinStillPercent = CVI_H26x_MIN_STILL_PERCENT_DEFAULT;
	pIc.u32MaxStillQP = CVI_H26x_MAX_STILL_QP_DEFAULT;
	pIc.u32MotionSensitivity = CVI_H26x_MOTION_SENSITIVITY_DEFAULT;
	pIc.s32AvbrFrmLostOpen = DEFAULT_AVBR_FRM_LOST_OPEN;
	pIc.s32AvbrFrmGap = CVI_H26x_AVB_FRM_GAP_DEFAULT;
	pIc.s32AvbrPureStillThr = CVI_H26x_AVB_PURE_STILL_THR_DEFAULT;

	enRcMode = (SAMPLE_RC_E) pIc.rcMode;

	s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "[Err]Venc Get GopAttr for %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_VENC_Start(
			&pIc,
			VencChn,
			enPayLoad,
			enSize,
			enRcMode,
			pIc.u32Profile,
			CVI_FALSE,
			&stGopAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "[Err]Venc Start failed for %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

#ifdef _ENABLE_JPEG
	VencChn = _JPEG_VENC_CHN;
	enGopMode = VENC_GOPMODE_NORMALP;
	enPayLoad = PT_JPEG;

	memset(&pIc, 0, sizeof(chnInputCfg));

	SAMPLE_COMM_VENC_InitChnInputCfg(&pIc);
	strcpy(pIc.codec, "jpg");

	pIc.rcMode = DEFAULT_RC_MODE;
	pIc.iqp = DEFAULT_IQP;
	pIc.pqp = DEFAULT_PQP;
	pIc.maxIprop = CVI_H26X_MAX_I_PROP_DEFAULT;
	pIc.minIprop = CVI_H26X_MIN_I_PROP_DEFAULT;
	pIc.gop = DEFAULT_GOP;
	pIc.bitrate = DEFAULT_KBITRATE;
	pIc.firstFrmstartQp = DEFAULT_FIRSTQP;
	pIc.num_frames = -1;
	pIc.framerate = DEFAULT_FPS;
	pIc.maxQp = DEF_264_MAXIQP;
	pIc.minQp = DEF_264_MINIQP;
	pIc.maxIqp = DEF_264_MAXQP;
	pIc.minIqp = DEF_264_MINQP;
	pIc.quality = 60; //for MJPEG

	enRcMode = (SAMPLE_RC_E) pIc.rcMode;

	s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode, &stGopAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "[Err]Venc Get GopAttr for %#x!\n", s32Ret);
		return CVI_FAILURE;
	}

	s32Ret = SAMPLE_COMM_VENC_Start(
			&pIc,
			VencChn,
			enPayLoad,
			enSize,
			enRcMode,
			pIc.u32Profile,
			CVI_FALSE,
			&stGopAttr);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_LOG(CVI_DBG_ERR, "[Err]Venc Start failed for %#x!\n", s32Ret);
		return CVI_FAILURE;
	}
#endif
	return s32Ret;
}

static inline CVI_S32 venc_deinit(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

#ifdef _ENABLE_JPEG
	s32Ret = SAMPLE_COMM_VENC_Stop(_JPEG_VENC_CHN);
#endif
	s32Ret = SAMPLE_COMM_VENC_Stop(_H264_VENC_CHN);

	return s32Ret;
}

static inline CVI_S32 vpss_deinit(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	CVI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {0};

	abChnEnable[0] = CVI_TRUE;

	s32Ret = SAMPLE_COMM_VPSS_Stop(0, abChnEnable);

	return s32Ret;
}

static inline CVI_S32 sys_vi_deinit(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	SAMPLE_COMM_VI_DestroyIsp(&stViConfig);

	SAMPLE_COMM_VI_DestroyVi(&stViConfig);

	SAMPLE_COMM_SYS_Exit();

	return s32Ret;
}

#endif // _PIPE_HELPER_H_
