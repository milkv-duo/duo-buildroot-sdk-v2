/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2.c
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cvi_ispd2_local.h"
#include "cvi_ispd2_log.h"

#include "cvi_ispd2_event_server.h"
#include "cvi_ispd2_callback_funcs_apps.h"

// -----------------------------------------------------------------------------
#define ENV_CONFIG_DEBUG_LEVEL				"ISPD2_CONFIG_LOG_LEVEL"

uint8_t				gu8ISPD2_LogExportLevel = LOG_NOTICE;
TISPDaemon2Info		gtObject = {0};

// -----------------------------------------------------------------------------
static void CVI_ISPD2_InitialDaemonInfo(TISPDaemon2Info *ptObject)
{
	TISPDeviceInfo *ptDeviceInfo = &(ptObject->tDeviceInfo);

	ptObject->pUVLoop = NULL;
	ptObject->thDaemonThreadId = 0;
	ptObject->bServiceThreadRunning = CVI_FALSE;
	ptObject->u8ClientCount = 0;

	memset(&(ptObject->UVServerEx), 0, sizeof(cvi_uv_stream_t_ex));

	CVI_ISPD2_Message_InitialHandler(&(ptObject->tHandlerInfo));

	ptDeviceInfo->s32ViPipe = 0;
	ptDeviceInfo->s32ViChn = 0;
	ptDeviceInfo->s32VpssGrp = 0;
	ptDeviceInfo->s32VpssChn = 0;
	ptDeviceInfo->bVPSSBindCtrl = CVI_FALSE;

	CVI_ISPD2_InitialFrameData(&(ptDeviceInfo->tFrameData));
	CVI_ISPD2_InitialBinaryData(&(ptDeviceInfo->tAEBinData));
	CVI_ISPD2_InitialBinaryData(&(ptDeviceInfo->tAWBBinData));
	CVI_ISPD2_InitialBinaryData(&(ptDeviceInfo->tBinaryOutData));
	CVI_ISPD2_InitialRawReplayHandle(&(ptDeviceInfo->tRawReplayHandle));
}

// -----------------------------------------------------------------------------
static void CVI_ISPD2_ReleaseDaemonInfo(TISPDaemon2Info *ptObject)
{
	TISPDeviceInfo *ptDeviceInfo = &(ptObject->tDeviceInfo);

	CVI_ISPD2_ReleaseFrameData(ptDeviceInfo);
	CVI_ISPD2_ReleaseBinaryData(&(ptDeviceInfo->tAEBinData));
	CVI_ISPD2_ReleaseBinaryData(&(ptDeviceInfo->tAWBBinData));
	CVI_ISPD2_ReleaseBinaryData(&(ptDeviceInfo->tBinaryOutData));
	CVI_ISPD2_ReleaseRawReplayHandle(&(ptDeviceInfo->tRawReplayHandle));

	CVI_ISPD2_Message_ReleaseHandler(&(ptObject->tHandlerInfo));
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_SetDefaultLogLevel(void)
{
	char *pszEnv = getenv(ENV_CONFIG_DEBUG_LEVEL);
	char *pszEnd = NULL;

	if (pszEnv == NULL) {
		return CVI_FAILURE;
	}

	long lLevel = strtol(pszEnv, &pszEnd, 10);

	if ((lLevel < LOG_EMERG) || (lLevel > LOG_DEBUG) || (pszEnv == pszEnd)) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Invalid log level");
		return CVI_FAILURE;
	}

	gu8ISPD2_LogExportLevel = (uint8_t)lLevel;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
void isp_daemon2_init(unsigned int port)
{
	CVI_ISPD2_SetDefaultLogLevel();
	CVI_ISPD2_InitialDaemonInfo(&gtObject);
	CVI_ISPD2_ES_CreateService(&gtObject, port);

	CVI_ISPD2_ConfigMessageHandler(&(gtObject.tHandlerInfo));

	CVI_ISPD2_ES_RunService(&gtObject);
}

// -----------------------------------------------------------------------------
void isp_daemon2_uninit(void)
{
	CVI_ISPD2_ES_DestoryService(&gtObject);

	CVI_ISPD2_ReleaseDaemonInfo(&gtObject);
}

// -----------------------------------------------------------------------------
void isp_daemon2_enable_device_bind_control(int enable)
{
	TISPDeviceInfo	*ptDeviceInfo = &(gtObject.tDeviceInfo);

	ptDeviceInfo->bVPSSBindCtrl = (enable) ? CVI_TRUE : CVI_FALSE;
}

// -----------------------------------------------------------------------------
