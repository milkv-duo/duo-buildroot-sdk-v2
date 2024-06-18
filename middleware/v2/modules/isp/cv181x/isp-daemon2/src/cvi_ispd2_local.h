/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_local.h
 * Description:
 */

#ifndef _CVI_ISPD2_LOCAL_H_
#define _CVI_ISPD2_LOCAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "cvi_type.h"
#include "cvi_ispd2.h"

#include "uv.h"

#include "cvi_ispd2_define.h"
#include "cvi_ispd2_message.h"

// -----------------------------------------------------------------------------
typedef struct {
	cvi_uv_tcp_t		UVTcp;
	void			*ptHandle;
} cvi_uv_stream_t_ex;

typedef struct {
	cvi_uv_loop_t			*pUVLoop;
	cvi_uv_stream_t_ex		UVServerEx;
	pthread_t			thDaemonThreadId;
	CVI_BOOL			bServiceThreadRunning;
	CVI_U8				u8ClientCount;
	TISPD2HandlerInfo	tHandlerInfo;
	TISPDeviceInfo		tDeviceInfo;
} TISPDaemon2Info;

// -----------------------------------------------------------------------------
void CVI_ISPD2_ConfigMessageHandler(TISPD2HandlerInfo *ptHandlerInfo);

#endif // _CVI_ISPD2_LOCAL_H_
