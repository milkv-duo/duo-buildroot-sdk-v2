/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_local.h
 * Description:
 */

#ifndef _CVI_ISPD2_CALLBACK_FUNCS_LOCAL_H_
#define _CVI_ISPD2_CALLBACK_FUNCS_LOCAL_H_

#include "cvi_ispd2_callback_funcs.h"
#include "cvi_ispd2_log.h"

#define JSONBIN_WRITE		(0)
#define JSONBIN_READ		(1)

#define GET_RESPONSE_KEY_NAME			"Content"

#define VC_PARAM_JSON_LOCATION			"/mnt/data/vc_param.json"
#define CONTENT_BUFFER_SIZE 50

// -----------------------------------------------------------------------------
#define JSON_DEBUG_PRINT(title, json_info) { \
	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "%s:\n%s", \
		title, ISPD2_json_object_to_json_string_ext(json_info, JSON_C_TO_STRING_SPACED)); \
}

// -----------------------------------------------------------------------------
#define GET_INT_FROM_JSON(json_in, key_path, value_out, return_code) \
{ \
	JSONObject *pTempPtr = NULL; \
	int iRes = ISPD2_json_pointer_get(json_in, key_path, &pTempPtr); \
	if (iRes != 0) { \
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find "key_path""); \
		return_code = CVI_FAILURE; \
	} else { \
		value_out = ISPD2_json_object_get_int(pTempPtr); \
	} \
}

// -----------------------------------------------------------------------------
#define GET_DOUBLE_FROM_JSON(json_in, key_path, value_out, return_code) \
{ \
	JSONObject *pTempPtr = NULL; \
	int iRes = ISPD2_json_pointer_get(json_in, key_path, &pTempPtr); \
	if (iRes != 0) { \
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find "key_path""); \
		return_code = CVI_FAILURE; \
	} else { \
		value_out = ISPD2_json_object_get_double(pTempPtr); \
	} \
}

// -----------------------------------------------------------------------------
#define GET_STRING_FROM_JSON(json_in, key_path, value_out, return_code) \
{ \
	JSONObject *pTempPtr = NULL; \
	int iRes = ISPD2_json_pointer_get(json_in, key_path, &pTempPtr); \
	if (iRes != 0) { \
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find "key_path""); \
		return_code = CVI_FAILURE; \
	} else { \
		value_out = ISPD2_json_object_get_string(pTempPtr); \
	} \
}

// -----------------------------------------------------------------------------
#define GET_INT_ARRAY_FROM_JSON(json_in, key_path, array_len, value_type, value_out, return_code) \
{ \
	JSONObject *pTempPtr = NULL; \
	if (ISPD2_json_pointer_get(json_in, key_path, &pTempPtr) == 0) { \
		if (ISPD2_json_object_is_type(pTempPtr, ISPD2_json_type_array)) { \
			if ((CVI_U32)ISPD2_json_object_array_length(pTempPtr) == (CVI_U32)array_len) { \
				JSONObject *pTempArrayPtr = NULL; \
\
				for (CVI_U32 u32Idx = 0; u32Idx < (CVI_U32)array_len; ++u32Idx) { \
					pTempArrayPtr = ISPD2_json_object_array_get_idx(pTempPtr, u32Idx); \
					if (pTempArrayPtr) { \
						value_out[u32Idx] = \
							(value_type)ISPD2_json_object_get_int(pTempArrayPtr); \
					} \
				} \
			} else { \
				ISP_DAEMON2_DEBUG(LOG_DEBUG, "Data format invalid"); \
				return_code = CVI_FAILURE; \
			} \
		} else { \
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Data format invalid"); \
			return_code = CVI_FAILURE; \
		} \
	} else { \
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Can't find "key_path""); \
		return_code = CVI_FAILURE; \
	} \
}

// -----------------------------------------------------------------------------
#define SET_INT_TO_JSON(key_path, value_in, json_out) \
{ \
	ISPD2_json_object_object_add(json_out, key_path, ISPD2_json_object_new_int(value_in)); \
}

// -----------------------------------------------------------------------------
#define SET_UINT64_TO_JSON(key_path, value_in, json_out) \
{ \
	ISPD2_json_object_object_add(json_out, key_path, ISPD2_json_object_new_uint64(value_in)); \
}

// -----------------------------------------------------------------------------
#define SET_DOUBLE_TO_JSON(key_path, value_in, json_out) \
{ \
	ISPD2_json_object_object_add(json_out, key_path, ISPD2_json_object_new_double(value_in)); \
}

// -----------------------------------------------------------------------------
#define SET_STRING_TO_JSON(key_path, string_in, json_out) \
{ \
	ISPD2_json_object_object_add(json_out, key_path, ISPD2_json_object_new_string(string_in)); \
}

// -----------------------------------------------------------------------------
#define SET_INT_ARRAY_TO_JSON(key_path, array_len, value_in, json_out) \
{ \
	JSONObject *pValueArray = ISPD2_json_object_new_array(); \
\
	for (CVI_U32 u32Idx = 0; u32Idx < (CVI_U32)array_len; ++u32Idx) { \
		ISPD2_json_object_array_add(pValueArray, ISPD2_json_object_new_int(value_in[u32Idx])); \
	} \
	ISPD2_json_object_object_add(json_out, key_path, pValueArray); \
}

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeMessage(TJSONRpcContentOut *ptContentOut, CVI_S32 s32StatusCode, const char *pszMessage);
void CVI_ISPD2_Utils_ComposeAPIErrorMessage(TJSONRpcContentOut *ptContentOut);
void CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(TJSONRpcContentOut *ptContentOut, const char *pszMessage);
void CVI_ISPD2_Utils_ComposeFileErrorMessage(TJSONRpcContentOut *ptContentOut);
void CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(TJSONRpcContentOut *ptContentOut);

CVI_S32 CVI_ISPD2_Utils_GetCurrentVPSSInfo(const TISPDeviceInfo *ptDevInfo, CVI_S32 *pVPSSGrp, CVI_S32 *pVPSSChn);

#endif // _CVI_ISPD2_CALLBACK_FUNCS_LOCAL_H_
