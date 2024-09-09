/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_vc.c
 * Description:
 */

#include "cvi_ispd2_callback_funcs_local.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_SetCodingParamInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(ptContentIn->pParams, "/Coding Param", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}
	ISPD2_json_object_get(pVCGroupParam);

	ISPD2_json_object_object_add(pVCJsonData, "Coding Param", pVCGroupParam);
	ISPD2_json_object_put(pVCGroupParam);

	if (ISPD2_json_object_to_file_ext(VC_PARAM_JSON_LOCATION, pVCJsonData, JSON_C_TO_STRING_PRETTY) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	};

	ISPD2_json_object_get(pVCJsonData);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_GetCodingParamInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(pVCJsonData, "/Coding Param", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISPD2_json_object_get(pVCGroupParam);
	ISPD2_json_object_put(pVCJsonData);

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	if (ISPD2_json_object_object_add(pDataOut, "Coding Param", pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCGroupParam);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut) != 0) {
		ISPD2_json_object_put(pDataOut);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_SetGopModeInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(ptContentIn->pParams, "/Gop Mode", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}
	ISPD2_json_object_get(pVCGroupParam);

	ISPD2_json_object_object_add(pVCJsonData, "Gop Mode", pVCGroupParam);
	ISPD2_json_object_put(pVCGroupParam);

	if (ISPD2_json_object_to_file_ext(VC_PARAM_JSON_LOCATION, pVCJsonData, JSON_C_TO_STRING_PRETTY) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	};

	ISPD2_json_object_get(pVCJsonData);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_GetGopModeInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(pVCJsonData, "/Gop Mode", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISPD2_json_object_get(pVCGroupParam);
	ISPD2_json_object_put(pVCJsonData);

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	if (ISPD2_json_object_object_add(pDataOut, "Gop Mode", pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCGroupParam);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut) != 0) {
		ISPD2_json_object_put(pDataOut);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_SetRCAttrInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(ptContentIn->pParams, "/RC Attr", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		printf("Get pointer error\n");
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}
	ISPD2_json_object_get(pVCGroupParam);

	ISPD2_json_object_object_add(pVCJsonData, "RC Attr", pVCGroupParam);
	ISPD2_json_object_put(pVCGroupParam);

	if (ISPD2_json_object_to_file_ext(VC_PARAM_JSON_LOCATION, pVCJsonData, JSON_C_TO_STRING_PRETTY) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		printf("Save file error\n");
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	};

	ISPD2_json_object_get(pVCJsonData);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_GetRCAttrInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(pVCJsonData, "/RC Attr", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISPD2_json_object_get(pVCGroupParam);
	ISPD2_json_object_put(pVCJsonData);

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	if (ISPD2_json_object_object_add(pDataOut, "RC Attr", pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCGroupParam);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut) != 0) {
		ISPD2_json_object_put(pDataOut);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_SetRCParamInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(ptContentIn->pParams, "/RC Param", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}
	ISPD2_json_object_get(pVCGroupParam);

	ISPD2_json_object_object_add(pVCJsonData, "RC Param", pVCGroupParam);
	ISPD2_json_object_put(pVCGroupParam);

	if (ISPD2_json_object_to_file_ext(VC_PARAM_JSON_LOCATION, pVCJsonData, JSON_C_TO_STRING_PRETTY) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	};

	ISPD2_json_object_get(pVCJsonData);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VC_GetRCParamInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	JSONObject	*pVCJsonData = NULL;
	JSONObject	*pVCGroupParam = NULL;

	pVCJsonData = ISPD2_json_object_from_file(VC_PARAM_JSON_LOCATION);
	if (pVCJsonData == NULL) {
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_pointer_get(pVCJsonData, "/RC Param", &pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCJsonData);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISPD2_json_object_get(pVCGroupParam);
	ISPD2_json_object_put(pVCJsonData);

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	if (ISPD2_json_object_object_add(pDataOut, "RC Param", pVCGroupParam) != 0) {
		ISPD2_json_object_put(pVCGroupParam);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut) != 0) {
		ISPD2_json_object_put(pDataOut);
		CVI_ISPD2_Utils_ComposeFileErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
