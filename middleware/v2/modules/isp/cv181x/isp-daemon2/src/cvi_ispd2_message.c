/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_message.c
 * Description:
 */

#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "cvi_ispd2_log.h"
#include "cvi_ispd2_message.h"

#include "cvi_ispd2_json_wrapper.h"
#include "cvi_ispd2_callback_funcs.h"
#include "cvi_ispd2_callback_funcs_apps.h"

#define DEFAULT_RPC_HANDLER_COUNT		(170)
#define DEFAULT_RPC_HANDLER_ADD_STEP	(10)


#define JSONRPC_FLAG_ARRAY_HEAD			'['
#define JSONRPC_FLAG_ARRAY_TAIL			']'
#define JSONRPC_FLAG_ELEMENT_HEAD		'{'
#define JSONRPC_FLAG_ELEMENT_TAIL		'}'

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Message_InitialHandler(TISPD2HandlerInfo *ptHandlerInfo)
{
	ptHandlerInfo->ptHandler = NULL;
	ptHandlerInfo->u32Count = 0;
	ptHandlerInfo->u32MaxCount = 0;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Message_ReleaseHandler(TISPD2HandlerInfo *ptHandlerInfo)
{
	SAFE_FREE(ptHandlerInfo->ptHandler);

	ptHandlerInfo->u32Count = 0;
	ptHandlerInfo->u32MaxCount = 0;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Message_RegisterHandler(TISPD2HandlerInfo *ptHandlerInfo,
	JSONRpcCBFunction pcbFunction, const char *pszName, void *pvData)
{
	if (ptHandlerInfo->ptHandler == NULL) {
		ptHandlerInfo->ptHandler = (TISPD2Handler *)calloc(DEFAULT_RPC_HANDLER_COUNT,
			sizeof(TISPD2Handler));
		if (ptHandlerInfo->ptHandler == NULL) {
			return CVI_FAILURE;
		}

		ptHandlerInfo->u32Count = 0;
		ptHandlerInfo->u32MaxCount = DEFAULT_RPC_HANDLER_COUNT;
	}

	if (ptHandlerInfo->u32Count == ptHandlerInfo->u32MaxCount) {
		TISPD2Handler *pTemp = NULL;
		CVI_U32 u32NewCount;

		u32NewCount = ptHandlerInfo->u32MaxCount + DEFAULT_RPC_HANDLER_ADD_STEP;
		pTemp = (TISPD2Handler *)realloc(ptHandlerInfo->ptHandler, u32NewCount * sizeof(TISPD2Handler));
		if (pTemp == NULL) {
			return CVI_FAILURE;
		}
		ptHandlerInfo->ptHandler = pTemp;
		ptHandlerInfo->u32MaxCount = u32NewCount;
	}

	TISPD2Handler *ptHandlerAddr = ptHandlerInfo->ptHandler + ptHandlerInfo->u32Count;

	snprintf(ptHandlerAddr->szName, MAX_CBFUNC_NAME_SIZE, "%s", pszName);
	ptHandlerAddr->u32NameLen = (CVI_U32)strlen(ptHandlerAddr->szName);
	ptHandlerAddr->cbFunction = pcbFunction;
	ptHandlerAddr->pvData = pvData;

	ptHandlerInfo->u32Count++;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_Message_ComposeResponse(JSONObject *pJsonResponse,
	JSONObject *pJsonContent, CVI_S32 s32StatusCode, CVI_S32 s32Id)
{
	ISPD2_json_object_object_add(pJsonResponse, "jsonrpc", ISPD2_json_object_new_string(JSONRPC_VERSION));
	JSONObject *pJsonResult = ISPD2_json_object_new_object();
	ISPD2_json_object_object_add(pJsonResult, "status", ISPD2_json_object_new_int(s32StatusCode));

	int iIsJsonObject = ISPD2_json_object_is_type(pJsonContent, ISPD2_json_type_object);
	int iIsJsonArray = ISPD2_json_object_is_type(pJsonContent, ISPD2_json_type_array);

	JSONObject *pJsonContentPtr = NULL;
	JSONObject *pJsontmp = NULL;

	if (iIsJsonObject) {
		if (ISPD2_json_object_object_length(pJsonContent) > 0) {
			ISPD2_json_pointer_get(pJsonContent, "/Content", &pJsonContentPtr);
		}
	} else if (iIsJsonArray) {
		if (ISPD2_json_object_array_length(pJsonContent) > 0) {
			pJsonContentPtr = ISPD2_json_object_array_get_idx(pJsonContent, 0);
		}
	}
	if (pJsonContentPtr) {
		struct lh_table *obj_table = json_object_get_object(pJsonContent);
		for(int i = 0; i < obj_table->size; i++) {
			if(obj_table->table[i].v == pJsonContentPtr) {
				ISPD2_json_object_object_add(pJsonResult, "params", pJsonContentPtr);
				pJsontmp = ISPD2_json_object_new_object();
				obj_table->table[i].v = pJsontmp;
				break;
			}
		}
	}

	ISPD2_json_object_object_add(pJsonResponse, "result", pJsonResult);

	if (s32Id > 0) {
		ISPD2_json_object_object_add(pJsonResponse, "id", ISPD2_json_object_new_int(s32Id));
	} else {
		ISPD2_json_object_object_add(pJsonResponse, "id", ISPD2_json_object_new_null());
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_Message_ComposeErrorResponse(JSONObject *pJsonResponse,
	CVI_S32 s32ErrorCode, const char *pszMessage, CVI_S32 s32Id)
{
	ISPD2_json_object_object_add(pJsonResponse, "jsonrpc", ISPD2_json_object_new_string(JSONRPC_VERSION));

	JSONObject *pJsonError = ISPD2_json_object_new_object();

	ISPD2_json_object_object_add(pJsonError, "code", ISPD2_json_object_new_int(s32ErrorCode));
	if (pszMessage) {
		ISPD2_json_object_object_add(pJsonError, "message", ISPD2_json_object_new_string(pszMessage));
	}
	ISPD2_json_object_object_add(pJsonResponse, "error", pJsonError);

	if (s32Id > 0) {
		ISPD2_json_object_object_add(pJsonResponse, "id", ISPD2_json_object_new_int(s32Id));
	} else {
		ISPD2_json_object_object_add(pJsonResponse, "id", ISPD2_json_object_new_null());
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_Message_CheckValidAndGetInfo(JSONObject *pJsonRoot,
	char *pszErrorMessage, CVI_U32 u32MaxMessageLen, CVI_BOOL *pbNotifyMode, CVI_U32 *pu32Id)
{
	JSONObject	*pJsonPtr = NULL;

	int			iRes;

	*pbNotifyMode = CVI_FALSE;
	*pu32Id = 0;

	// Check version
	iRes = ISPD2_json_pointer_get(pJsonRoot, "/jsonrpc", &pJsonPtr);
	if (iRes != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find json-rpc version");
		snprintf(pszErrorMessage, u32MaxMessageLen, "JSON-RPC Version Error");
		return JSONRPC_CODE_PARSE_ERROR;
	}

	// printf("%d, %s\n",
	//	strlen(ISPD2_json_object_to_json_string_ext(json_pointer, JSON_C_TO_STRING_PRETTY)),
	//	ISPD2_json_object_to_json_string_ext(json_pointer, JSON_C_TO_STRING_PRETTY));

	iRes = strcmp(ISPD2_json_object_get_string(pJsonPtr), JSONRPC_VERSION);
	if (iRes != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Json-rpc version mismatch");
		snprintf(pszErrorMessage, u32MaxMessageLen, "JSON-RPC Version Mismatch");
		return JSONRPC_CODE_INVALID_REQUEST;
	}

	// Check method
	iRes = ISPD2_json_pointer_get(pJsonRoot, "/method", &pJsonPtr);
	if (iRes != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find json-rpc method");
		snprintf(pszErrorMessage, u32MaxMessageLen, "Parse Error (Method)");
		return JSONRPC_CODE_PARSE_ERROR;
	}

	// Check id
	iRes = ISPD2_json_pointer_get(pJsonRoot, "/id", &pJsonPtr);
	if (iRes != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find json-rpc id -> notify mode");
		*pbNotifyMode = CVI_TRUE;
		return JSONRPC_CODE_OK;
	}
	iRes = ISPD2_json_object_is_type(pJsonPtr, ISPD2_json_type_null);
	if (iRes == 1) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Json-rpc id null type -> notify mode");
		*pbNotifyMode = CVI_TRUE;
		return JSONRPC_CODE_OK;
	}

	*pu32Id = strtol(ISPD2_json_object_get_string(pJsonPtr), NULL, 10);

	return JSONRPC_CODE_OK;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_Message_HandleSingleRequest(JSONObject *pJsonRoot, JSONObject *pJsonResponse,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo,
	CVI_BOOL *pbSkipJSONRPCResponse, TJSONRpcBinaryOut *ptBinaryData)
{
	TISPD2Handler		*ptHandlerAddr = NULL;
	JSONObject			*pJsonMethodPtr = NULL;
	JSONObject			*pJsonDataResponse = NULL;
	char				szErrorMessage[64] = {'\0'};

	TJSONRpcContentIn	tContentIn;
	TJSONRpcContentOut	tContentOut;
	CVI_U32				u32Id;
	CVI_S32				s32CheckCode;
	CVI_BOOL			bNotifyMode;
	CVI_BOOL			bMethodMatch;
	int					iRes;

	*pbSkipJSONRPCResponse = CVI_FALSE;

	s32CheckCode = CVI_ISPD2_Message_CheckValidAndGetInfo(pJsonRoot, szErrorMessage, 64, &bNotifyMode, &u32Id);
	if (s32CheckCode != JSONRPC_CODE_OK) {
		CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse,
			s32CheckCode, szErrorMessage, u32Id);
		return CVI_SUCCESS;
	}

	iRes = ISPD2_json_pointer_get(pJsonRoot, "/method", &pJsonMethodPtr);
	if (iRes != 0) {
		CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse,
			JSONRPC_CODE_PARSE_ERROR, "Parse Error (Method)", u32Id);
		return CVI_SUCCESS;
	}

	const char *pszMethod = ISPD2_json_object_get_string(pJsonMethodPtr);
	CVI_U32 u32MethodLen = (CVI_U32)strlen(pszMethod);

	// Content In
	tContentIn.pvData = NULL;
	tContentIn.ptDeviceInfo = ptDeviceInfo;
	tContentIn.pParams = NULL;

	ISPD2_json_pointer_get(pJsonRoot, "/params", &(tContentIn.pParams));

	// Content Out
	tContentOut.s32StatusCode = JSONRPC_CODE_OK;
	tContentOut.ptBinaryOut = ptBinaryData;
	tContentOut.ptBinaryOut->pvData = NULL;
	tContentOut.ptBinaryOut->bDataValid = CVI_FALSE;
	tContentOut.ptBinaryOut->eBinaryDataType = 0;
	// tContentOut.ptBinaryOut->u32DataSize = 0;

	pJsonDataResponse = ISPD2_json_object_new_object();
	// pJsonDataResponse = ISPD2_json_object_new_array();

	bMethodMatch = CVI_FALSE;
	ptHandlerAddr = ptHandlerInfo->ptHandler;
	for (CVI_U32 u32Idx = 0; u32Idx < ptHandlerInfo->u32Count; ++u32Idx, ptHandlerAddr++) {
		if (u32MethodLen != ptHandlerAddr->u32NameLen)
			continue;

		if (strcmp(pszMethod, ptHandlerAddr->szName) == 0) {
			ptHandlerAddr->cbFunction(&tContentIn, &tContentOut, pJsonDataResponse);
			bMethodMatch = CVI_TRUE;
			break;
		}
	}

	if (!bMethodMatch) {
		CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse,
			JSONRPC_CODE_METHOD_NOT_FOUND, "Method Not Found", u32Id);
		ISPD2_json_object_put(pJsonDataResponse);
		return CVI_SUCCESS;
	}

	if ((bNotifyMode) || (tContentOut.s32StatusCode == JSONRPC_CODE_BINARY_OUTPUT_MODE)) {
		ISPD2_json_object_put(pJsonDataResponse);
		*pbSkipJSONRPCResponse = CVI_TRUE;
		return CVI_SUCCESS;
	}

	if (tContentOut.s32StatusCode == JSONRPC_CODE_OK) {
		CVI_ISPD2_Message_ComposeResponse(pJsonResponse, pJsonDataResponse,
			tContentOut.s32StatusCode, u32Id);
	} else {
		CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse, tContentOut.s32StatusCode,
			tContentOut.szErrorMessage, u32Id);
	}

	ISPD2_json_object_put(pJsonDataResponse);
	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static void CVI_ISPD2_Message_SendJsonMessage(int iFd, JSONObject *pJsonObj)
{
#ifdef USE_JSONC_API_TO_WRITE_CONTENT_TO_CLIENT
	if (ISPD2_json_object_to_fd(iFd, pJsonObj, JSON_C_TO_STRING_SPACED) != 0) {
		ISP_DAEMON2_DEBUG_EX(LOG_WARNING, "Return message fail : %s",
			ISPD2_json_util_get_last_err();
	}
#else
	const char	*pszJsonContent = NULL;
	CVI_U32		u32ContentLength = 0;
	CVI_U32		u32WriteLen = 0;
	ssize_t		lWriteLen = 0;

	pszJsonContent = ISPD2_json_object_to_json_string_length(pJsonObj,
		JSON_C_TO_STRING_SPACED, (size_t *) &u32ContentLength);

	while (u32WriteLen < u32ContentLength) {
		CVI_U32 u32WriteDataLen = MIN(u32ContentLength - u32WriteLen, MESSAGE_PARTICAL_WRITE_LENGTH);

		// printf("Write JSON Content To Dest : %d + %d / %d\n",
		//	u32WriteLen, u32WriteDataLen, u32ContentLength);
		lWriteLen = write(iFd, pszJsonContent + u32WriteLen, u32WriteDataLen);
		if (lWriteLen >= 0) {
			u32WriteLen += lWriteLen;
		} else {
			if ((lWriteLen == -1) && (errno == EAGAIN)) {
				usleep(1000);
				// printf("EAGAIN\n");
				continue;
			} else {
				ISP_DAEMON2_DEBUG_EX(LOG_WARNING, "Write partial content to socket fail (%d)\"%s\"",
					errno, strerror(errno));
				return;
			}
		}
	}
#endif // USE_JSONC_API_TO_WRITE_CONTENT_TO_CLIENT
}

// -----------------------------------------------------------------------------
static void CVI_ISPD2_Message_HandleJsonObject(JSONObject *pJsonRoot, int iFd,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo)
{
	TJSONRpcBinaryOut	tBinaryOut;

	int iIsJsonArray = ISPD2_json_object_is_type(pJsonRoot, ISPD2_json_type_array);

	if (iIsJsonArray) {
		JSONObject	*pJsonResponseArray = ISPD2_json_object_new_array();
		CVI_BOOL	bExportToClient = CVI_FALSE;
		CVI_BOOL	bSkipJSONRPCResponseTemp;

		memset(&tBinaryOut, 0, sizeof(TJSONRpcBinaryOut));

		for (CVI_U32 u32Idx = 0; u32Idx < ISPD2_json_object_array_length(pJsonRoot); ++u32Idx) {
			JSONObject	*pObj = ISPD2_json_object_array_get_idx(pJsonRoot, u32Idx);
			JSONObject	*pJsonSubResponse = ISPD2_json_object_new_object();

			ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "\t[%u]=%s",
				u32Idx, ISPD2_json_object_to_json_string_ext(pObj, JSON_C_TO_STRING_SPACED));

			CVI_ISPD2_Message_HandleSingleRequest(pObj, pJsonSubResponse,
				ptHandlerInfo, ptDeviceInfo, &bSkipJSONRPCResponseTemp, &tBinaryOut);

			if (!bSkipJSONRPCResponseTemp) {
				bExportToClient = CVI_TRUE;
				ISPD2_json_object_array_add(pJsonResponseArray, pJsonSubResponse);
			} else {
				ISPD2_json_object_put(pJsonSubResponse);
			}
		}

		if (bExportToClient) {
			CVI_ISPD2_Message_SendJsonMessage(iFd, pJsonResponseArray);
		}

		if (tBinaryOut.bDataValid) {
			CVI_ISPD2_CBFunc_Handle_BinaryOut(iFd, &tBinaryOut);
		}

		ISPD2_json_object_put(pJsonResponseArray);
	} else {
		JSONObject	*pJsonResponse = ISPD2_json_object_new_object();
		CVI_BOOL	bSkipJSONRPCResponse;

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "\t[O]=%s",
			ISPD2_json_object_to_json_string_ext(pJsonRoot, JSON_C_TO_STRING_SPACED));

		memset(&tBinaryOut, 0, sizeof(TJSONRpcBinaryOut));

		CVI_ISPD2_Message_HandleSingleRequest(pJsonRoot, pJsonResponse,
			ptHandlerInfo, ptDeviceInfo, &bSkipJSONRPCResponse, &tBinaryOut);

		if (!bSkipJSONRPCResponse) {
			CVI_ISPD2_Message_SendJsonMessage(iFd, pJsonResponse);
		}

		if (tBinaryOut.bDataValid) {
			CVI_ISPD2_CBFunc_Handle_BinaryOut(iFd, &tBinaryOut);
		}

		ISPD2_json_object_put(pJsonResponse);
	}
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Message_HandleMessage(const char *pBufIn, CVI_U32 u32BufInSize, int iFd,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo)
{
	JSONTokener		*pJsonTok = ISPD2_json_tokener_new();
	JSONObject		*pJsonObj = NULL;
	JSONObject		*pJsonResponse = NULL;
	CVI_U32			u32ReadOffset, u32ParseOffset;

	ISPD2_json_tokener_set_flags(pJsonTok, ISPD2_json_tokener_flags);
	u32ReadOffset = 0;
	do {
		pJsonObj = ISPD2_json_tokener_parse_ex(pJsonTok, pBufIn + u32ReadOffset, -1);
		if (pJsonTok->err != ISPD2_json_tokener_success) {
			ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "JSON parsing error: %s (offset : %d)",
				ISPD2_json_tokener_error_desc(pJsonTok->err), pJsonTok->char_offset);

			pJsonResponse = ISPD2_json_object_new_object();
			CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse,
				JSONRPC_CODE_PARSE_ERROR, "Parse Error (Content)", -1);
			CVI_ISPD2_Message_SendJsonMessage(iFd, pJsonResponse);
			ISPD2_json_object_put(pJsonResponse);
			pJsonResponse = NULL;
			break;
		}

		CVI_ISPD2_Message_HandleJsonObject(pJsonObj, iFd, ptHandlerInfo, ptDeviceInfo);

		u32ParseOffset = (CVI_U32)ISPD2_json_tokener_get_parse_end(pJsonTok);
		u32ReadOffset += u32ParseOffset;
	} while (u32ReadOffset < u32BufInSize);

	ISPD2_json_object_put(pJsonObj);
	json_tokener_free(pJsonTok);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Message_HandleBinary(const char *pBufIn, CVI_U32 u32BufInSize, int iFd,
	const TISPD2HandlerInfo *ptHandlerInfo, TISPDeviceInfo *ptDeviceInfo)
{
	TBinaryData			*ptBinaryData = &(ptDeviceInfo->tBinaryInData);
	JSONObject			*pJsonDataResponse = NULL;
	TJSONRpcContentOut	tContentOut;

	pJsonDataResponse = ISPD2_json_object_new_object();

	tContentOut.s32StatusCode = JSONRPC_CODE_OK;
	tContentOut.ptBinaryOut = NULL;

	switch (ptBinaryData->eDataType) {
	case EBINARYDATA_TUNING_BIN_DATA:
		CVI_ISPD2_CBFunc_ImportBinFile(ptBinaryData, &tContentOut, pJsonDataResponse);
		break;
	case EBINARYDATA_RAW_DATA:
		CVI_ISPD2_CBFunc_SendRawReplayData(ptDeviceInfo, &tContentOut, pJsonDataResponse);
		break;
	default:
		tContentOut.s32StatusCode = JSONRPC_CODE_METHOD_NOT_FOUND;
		snprintf(tContentOut.szErrorMessage, MAX_CONTENT_ERROR_MSG_LENGTH,
			"Request binary data target error");
		break;
	}

	JSONObject	*pJsonResponse = ISPD2_json_object_new_object();

	if (tContentOut.s32StatusCode == JSONRPC_CODE_OK) {
		CVI_ISPD2_Message_ComposeResponse(pJsonResponse, pJsonDataResponse,
			tContentOut.s32StatusCode, 0);
	} else {
		CVI_ISPD2_Message_ComposeErrorResponse(pJsonResponse, tContentOut.s32StatusCode,
			tContentOut.szErrorMessage, 0);

	}

	CVI_ISPD2_Message_SendJsonMessage(iFd, pJsonResponse);
	ISPD2_json_object_put(pJsonResponse);
	ISPD2_json_object_put(pJsonDataResponse);

	UNUSED(pBufIn);
	UNUSED(u32BufInSize);
	UNUSED(ptHandlerInfo);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
EPacketState CVI_ISPD2_Message_CommandChecker(TPACKETINFO *pstPacketInfo,
	const char *pszBuf, CVI_U32 u32CheckStart, CVI_U32 u32CheckTail)
{
	unsigned char value = 0;
	EPacketState ePacketState = PACKET_RECEIVE_PROCESSING;

	for (; u32CheckStart <= u32CheckTail; u32CheckStart++) {
		value = *(pszBuf + u32CheckStart);
		if (value == JSONRPC_FLAG_ARRAY_HEAD) {
			pstPacketInfo->u32SquareBracketNum++;
		} else if (value == JSONRPC_FLAG_ARRAY_TAIL) {
			pstPacketInfo->u32SquareBracketNum--;
		} else if (value == JSONRPC_FLAG_ELEMENT_HEAD) {
			pstPacketInfo->u32BracketNum++;
		} else if (value == JSONRPC_FLAG_ELEMENT_TAIL) {
			pstPacketInfo->u32BracketNum--;
		}

		if ((pstPacketInfo->u32SquareBracketNum == 0) && (pstPacketInfo->u32BracketNum == 0)) {
			ePacketState = PACKET_RECEIVE_FINISH;
			pstPacketInfo->u32CurPacketReadOffset = 0;
			pstPacketInfo->u32CurPacketSize = u32CheckStart + 1;
			break;
		}
	}

	if (u32CheckStart < u32CheckTail) {
		/*exist the whole packet*/
		pstPacketInfo->u32CurPacketReadOffset = u32CheckStart + 1;
	} else if (u32CheckStart == (u32CheckTail + 1)) {
		/*not exist the whole packet*/
		pstPacketInfo->u32CurPacketReadOffset = u32CheckStart;
	}

	return ePacketState;
}

// -----------------------------------------------------------------------------
CVI_BOOL CVI_ISPD2_Message_ResetRecvCommandChecker(const char *pszBuf)
{
	JSONObject	*pJsonRoot = ISPD2_json_tokener_parse(pszBuf);
	JSONObject	*pJsonPtr = NULL;

	if (pJsonRoot == NULL) {
		return CVI_FALSE;
	}

	if (ISPD2_json_pointer_get(pJsonRoot, "/jsonrpc", &pJsonPtr) != 0) {
		// ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find json-rpc version");
		ISPD2_json_object_put(pJsonRoot);
		return CVI_FALSE;
	}
	if (strcmp(ISPD2_json_object_get_string(pJsonPtr), JSONRPC_VERSION) != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Json-rpc version mismatch");
		ISPD2_json_object_put(pJsonRoot);
		return CVI_FALSE;
	}

	if (ISPD2_json_pointer_get(pJsonRoot, "/method", &pJsonPtr) != 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find json-rpc method");
		ISPD2_json_object_put(pJsonRoot);
		return CVI_FALSE;
	}
	if (strcmp(ISPD2_json_object_get_string(pJsonPtr), JSONRPC_RESET_RECV_STATE_METHOD) != 0) {
		// ISP_DAEMON2_DEBUG(LOG_DEBUG, "Json-rpc method is not reset recv state");
		ISPD2_json_object_put(pJsonRoot);
		return CVI_FALSE;
	}

	ISP_DAEMON2_DEBUG(LOG_DEBUG, "Recv. Json-rpc method => reset recv state\n");
	ISPD2_json_object_put(pJsonRoot);
	return CVI_TRUE;
}

// -----------------------------------------------------------------------------
