/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_event_server.h
 * Description:
 */

#ifndef _CVI_ISPD2_EVENT_SERVER_H_
#define _CVI_ISPD2_EVENT_SERVER_H_

#include <stdio.h>
#include <stdint.h>

#include "cvi_ispd2_local.h"

typedef enum {
	EEMPTY = 0,
	EPARTIAL_JSONRPC = 1,
	EPARTIAL_BINARY = 2
} EPREV_DATA_TYPE;

typedef struct {
	TISPDaemon2Info		*ptDaemonInfo;
	int					iFd;
	char				*pszRecvBuffer;
	CVI_U32				u32RecvBufferOffset;
	CVI_U32				u32RecvBufferSize;
	EPREV_DATA_TYPE		eRecvBufferContentType;
	struct timeval		tvLastRecvTime;
	TPACKETINFO			stPacketInfo;
} TISPDaemon2ConnectInfo;

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_ES_CreateService(TISPDaemon2Info *ptObject, uint16_t u16ServicePort);
CVI_S32 CVI_ISPD2_ES_RunService(TISPDaemon2Info *ptObject);
CVI_S32 CVI_ISPD2_ES_DestoryService(TISPDaemon2Info *ptObject);

#endif // _CVI_ISPD2_EVENT_SERVER_H_
