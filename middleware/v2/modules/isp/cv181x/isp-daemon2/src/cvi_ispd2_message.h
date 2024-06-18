/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_message.h
 * Description:
 */

#ifndef _CVI_ISPD2_MESSAGE_H_
#define _CVI_ISPD2_MESSAGE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cvi_type.h"
#include "cvi_ispd2_define.h"

#define MAX_CBFUNC_NAME_SIZE			(48)
#define MESSAGE_PARTICAL_WRITE_LENGTH	(10240)		// 10K

typedef struct {
	char				szName[MAX_CBFUNC_NAME_SIZE];
	CVI_U32				u32NameLen;
	JSONRpcCBFunction	cbFunction;
	void				*pvData;
} TISPD2Handler;

typedef struct {
	TISPD2Handler		*ptHandler;
	CVI_U32				u32Count;
	CVI_U32				u32MaxCount;
} TISPD2HandlerInfo;

typedef enum {
	EMESSAGE_FORMAT_EMPTY						= 0,
	EMESSAGE_FORMAT_JSONRPC_ARRAY_HEAD			= (1 << 0),
	EMESSAGE_FORMAT_JSONRPC_ARRAY_TAIL			= (1 << 1),
	EMESSAGE_FORMAT_JSONRPC_PARTICAL			= (1 << 5),
	EMESSAGE_FORMAT_JSONRPC_ELEMENT_HEAD		= (1 << 7),
	EMESSAGE_FORMAT_JSONRPC_ELEMENT_TAIL		= (1 << 8),
	// EMESSAGE_FORMAT_TEXT_INVALID,
	// EMESSAGE_FORMAT_JSONRPC_ARRAY_COMPLETE,
	// EMESSAGE_FORMAT_JSONRPC_ELEMENT_COMPLETE,
	EMESSAGE_FORMAT_BINARY						= (1 << 10)
} EMessageFormat;

typedef enum {
	PACKET_RECEIVE_FINISH = 0,
	PACKET_RECEIVE_PROCESSING = 1
} EPacketState;
// -----------------------------------------------------------------------------

CVI_S32 CVI_ISPD2_Message_InitialHandler(TISPD2HandlerInfo *ptHandlerInfo);
CVI_S32 CVI_ISPD2_Message_ReleaseHandler(TISPD2HandlerInfo *ptHandlerInfo);

CVI_S32 CVI_ISPD2_Message_RegisterHandler(TISPD2HandlerInfo *ptHandlerInfo,
	JSONRpcCBFunction pcbFunction, const char *pszName, void *pvData);

CVI_S32 CVI_ISPD2_Message_HandleMessage(const char *pBufIn, CVI_U32 u32BufInSize, int iFd,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo);
CVI_S32 CVI_ISPD2_Message_HandleBinary(const char *pBufIn, CVI_U32 u32BufInSize, int iFd,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo);

EPacketState CVI_ISPD2_Message_CommandChecker(TPACKETINFO *pstPacketInfo,
	const char *pszBuf, CVI_U32 u32CheckStart, CVI_U32 u32CheckTail);
CVI_BOOL CVI_ISPD2_Message_ResetRecvCommandChecker(const char *pszBuf);

#endif // _CVI_ISPD2_MESSAGE_H_
