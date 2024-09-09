/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_define.h
 * Description:
 */

#ifndef _CVI_ISPD2_DEFINE_H_
#define _CVI_ISPD2_DEFINE_H_

#include <stdio.h>
#include <stdlib.h>

#include "cvi_ispd2_version.h"
#include "cvi_ispd2_json_wrapper.h"
#include "cvi_ispd2_callback_funcs_apps_define.h"

#define JSONRPC_VERSION						"2.0"

#define JSONRPC_CODE_OK						(0)
#define JSONRPC_CODE_PARSE_ERROR			(-32700)
	// receive invalid json content
#define JSONRPC_CODE_INVALID_REQUEST		(-32600)
#define JSONRPC_CODE_METHOD_NOT_FOUND		(-32601)
#define JSONRPC_CODE_INVALID_PARAMS			(-32602)
#define JSONRPC_CODE_INTERNAL_ERROR			(-32603)

#define JSONRPC_CODE_SERVER_ERROR			(-32000)
	// Server side error (-32000 to -32099)
#define JSONRPC_CODE_INTERNAL_API_ERROR		(-32050)
	// SDK internal error
#define JSONRPC_CODE_INTERNAL_FILE_ERROR	(-32051)
	// MW internal error (target file issue)
#define JSONRPC_CODE_MISS_PARAMETER_ERROR	(-32060)
	// Can't find mandatory parameter

// #define JSONRPC_CODE_BINARY_INPUT_MODE		(-32200)
	// Equal to OK response, just notify next packet is binary data
#define JSONRPC_CODE_BINARY_OUTPUT_MODE		(-32300)
	// Skip jsonrpc response, write binray content

#define JSONRPC_RESET_RECV_STATE_METHOD		"CVI_ResetRecvState"

#define MAX_CONTENT_ERROR_MSG_LENGTH		(256)
#define MAX_TCP_DATA_SIZE_PER_TRANSMIT		(10240)		// 10K


#ifndef UNUSED
#	define UNUSED(x) { (void)(x); }
#endif // UNUSED

#ifndef MIN
#	define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif // MIN

#ifndef SAFE_FREE
#	define SAFE_FREE(x) { \
	if (x) { \
		free((x)); ((x)) = NULL; \
	} \
}
#endif // SAFE_FREE

typedef struct {
	CVI_S32				s32ViPipe;
	CVI_S32				s32ViChn;
	CVI_S32				s32VpssGrp;
	CVI_S32				s32VpssChn;
	CVI_BOOL			bVPSSBindCtrl;
	TFrameData			tFrameData;
	TExportDataInfo		tExportDataInfo;
	TBinaryData			tAEBinData;
	TBinaryData			tAWBBinData;
	TBinaryData			tBinaryOutData;
	CVI_BOOL			bKeepBinaryInDataInfoOnce;
	TBinaryData			tBinaryInData;
	TRawReplayHandle	tRawReplayHandle;
} TISPDeviceInfo;

typedef struct {
	CVI_VOID			*pvData;
	CVI_BOOL			bDataValid;
	CVI_U32				eBinaryDataType;
	// CVI_U32				u32DataSize;
} TJSONRpcBinaryOut;

typedef struct {
	CVI_VOID			*pvData;
	TISPDeviceInfo		*ptDeviceInfo;
	JSONObject			*pParams;
	// CVI_U32				u32DataSize;
} TJSONRpcContentIn;

typedef struct {
	CVI_S32				s32StatusCode;
	char				szErrorMessage[MAX_CONTENT_ERROR_MSG_LENGTH];
	TJSONRpcBinaryOut	*ptBinaryOut;
} TJSONRpcContentOut;

typedef struct {
	CVI_U32	u32SquareBracketNum;
	CVI_U32	u32BracketNum;
	CVI_U32	u32CurPacketSize;
	CVI_U32	u32CurPacketReadOffset;
} TPACKETINFO;

typedef CVI_S32 (*JSONRpcCBFunction)(TJSONRpcContentIn *ptContentIn,
									TJSONRpcContentOut *ptContentOut,
									JSONObject *pJsonResponse);

#endif // _CVI_ISPD2_DEFINE_H_
