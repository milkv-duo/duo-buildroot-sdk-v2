/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: stream_porting.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <linux/kernel.h>
#include <common/sample_comm.h>

#include "stream_porting.h"
#ifndef VPSS_OUT_WIDTH
#define VPSS_OUT_WIDTH 1280
#endif
#ifndef VPSS_OUT_HEIGHT
#define VPSS_OUT_HEIGHT 720
#endif

#define LIMIT_RANGE(value, min, max)	((value) > (max) ? (max) : ((value) < (min) ? (min) : (value)))

#define ISP_DAEMON_TOOL_LOG(level, fmt, ...) do {		\
	if (1)												\
		syslog(level, fmt, ##__VA_ARGS__);				\
	if (1)												\
		printf("[%d]" fmt "\n", level, ##__VA_ARGS__);	\
} while (0)

#if STREAM_TYPE == STREAM_TYPE_LIVE555
//RTSP_PARAM please refer to
//https://cvitekcn-my.sharepoint.com/:x:/g/personal/tjyoung_cvitek_com/EavTSnA63CNAl2ZXmKwZnaoBK45WzoKdsrEQ7gwXNxKOpQ?e=l3hgB8

#define RTSP_MODE_ENV "CVI_RTSP_MODE"
#define RTSP_JSON_ENV "CVI_RTSP_JSON"

RTSP_SERVICE_HANDLE hdl;

CVI_BOOL stream_porting_init(void)
{
	return 0;
}

CVI_BOOL stream_porting_run(void)
{
#define PATH_SIZE 200
	CVI_S32 ret = 0;
	CVI_S32 rtspMode = 0;
	CVI_CHAR rtspJsonFile[PATH_SIZE];
	const char *input = getenv(RTSP_MODE_ENV);

	if (input) {
		rtspMode = atoi(input);
		rtspMode = LIMIT_RANGE(rtspMode, 0, 1);
	}
	printf("set %s as %d[0:multiSensors->oneRtsp 1:multiSensors->multiRtsp]\n",
			RTSP_MODE_ENV, rtspMode);

	input = getenv(RTSP_JSON_ENV);

	if (input) {
		strncpy(rtspJsonFile, input, PATH_SIZE);
	}

	ret = CVI_RTSP_SERVICE_CreateFromJsonFile(&hdl, rtspJsonFile);
	if (ret < 0) {
		printf("CVI_RTSP_SERVICE_CreateFromJsonFile fail %d\n", ret);
	}
	printf("CVI_RTSP_SERVICE_CreateFromJsonFile[%s]\n", rtspJsonFile);

	return ret;
}

CVI_BOOL stream_porting_stop(void)
{
	int ret = 0;

	if (hdl != 0) {
		ret = CVI_RTSP_SERVICE_Destroy(&hdl);
		if (ret < 0) {
			printf("CVI_RTSP_SERVICE_Destroy fail\n");
		}
		hdl = 0;
	}

	return ret;
}

CVI_BOOL stream_porting_uninit(void)
{
	return 0;
}
#else  // STREAM_TYPE == STREAM_TYPE_PANEL
SAMPLE_VI_CONFIG_S	stViConfig = {};

static CVI_S32 isp_start_vpss(SIZE_S *stVPSSSizeIn)
{
	SIZE_S stSizeOut;
	CVI_S32 s32Ret = CVI_SUCCESS;

	stSizeOut.u32Width = VPSS_OUT_WIDTH;
	stSizeOut.u32Height = VPSS_OUT_HEIGHT;

	s32Ret = SAMPLE_PLAT_VPSS_INIT(0, *stVPSSSizeIn, stSizeOut);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "VPSS init fail, 0x%x", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_VI_Bind_VPSS(0, 0, 0);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "VI bind VPSS fail, 0x%x", s32Ret);
		return s32Ret;
	}

	return s32Ret;
}

static CVI_S32 isp_start_vo(void)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	s32Ret = SAMPLE_PLAT_VO_INIT();
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "VO init fail, 0x%x", s32Ret);
		return s32Ret;
	}

	CVI_VO_SetChnRotation(0, 0, ROTATION_90);
	SAMPLE_COMM_VPSS_Bind_VO(0, 0, 0, 0);
	return s32Ret;
}

CVI_BOOL stream_porting_init(void)
{
	return 0;
}

CVI_BOOL stream_porting_run(void)
{
	SAMPLE_INI_CFG_S	stIniCfg = {};

	stIniCfg.enSource = VI_PIPE_FRAME_SOURCE_DEV;
	stIniCfg.devNum = 1;
	stIniCfg.enSnsType[0] = SONY_IMX327_MIPI_2M_30FPS_12BIT;
	stIniCfg.enWDRMode[0] = WDR_MODE_NONE;
	stIniCfg.s32BusId[0] = 3;
	stIniCfg.s32SnsI2cAddr[0] = -1;
	stIniCfg.MipiDev[0] = 0xFF;
	stIniCfg.u8UseMultiSns = 0;
	stIniCfg.enSnsType[1] = SONY_IMX327_SLAVE_MIPI_2M_30FPS_12BIT;
	stIniCfg.s32BusId[1] = 0;
	stIniCfg.s32SnsI2cAddr[1] = -1;
	stIniCfg.MipiDev[1] = 0xFF;

	// Get config from ini if found.
	if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
		ISP_DAEMON_TOOL_LOG(LOG_INFO, "%s", "INI Parse complete");
	}

	SIZE_S stSize;
	PIC_SIZE_E enPicSize;
	CVI_S32 s32Ret = CVI_SUCCESS;
	/************************************************
	 * step1:  Config VI
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "SAMPLE_COMM_VI_IniToViCfg fail, 0x%x", s32Ret);
		return -1;
	}

	/************************************************
	 * step2:  Get input size
	 ************************************************/
	s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType[0], &enPicSize);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "SAMPLE_COMM_VI_GetSizeBySensor fail, 0x%x", s32Ret);
		return -1;
	}

	s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "SAMPLE_COMM_SYS_GetPicSize fail, 0x%x", s32Ret);
		return -1;
	}

	/************************************************
	 * step3:  Init modules
	 ************************************************/
	s32Ret = SAMPLE_PLAT_SYS_INIT(stSize);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON_TOOL_LOG(LOG_ERR, "SYS init fail, 0x%x", s32Ret);
		return -1;
	}

	if (stIniCfg.enSource == VI_PIPE_FRAME_SOURCE_DEV) {
		s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
		if (s32Ret != CVI_SUCCESS) {
			ISP_DAEMON_TOOL_LOG(LOG_ERR, "VI init fail, 0x%x", s32Ret);
			return -1;
		}
	}

	isp_start_vpss(&stSize);
	isp_start_vo();
	return 0;
}

CVI_BOOL stream_porting_stop(void)
{
	//SAMPLE_COMM_SYS_Exit will destroy vo&vpss
	return 0;
}

CVI_BOOL stream_porting_uninit(void)
{
	SAMPLE_COMM_VI_DestroyIsp(&stViConfig);
	SAMPLE_COMM_VI_DestroyVi(&stViConfig);
	SAMPLE_COMM_SYS_Exit();
	return 0;
}
#endif
