/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_vo.c
 * Description:
 */

#include "cvi_ispd2_callback_funcs_local.h"

#include "cvi_vo.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VO_SetGammaInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	VO_GAMMA_INFO_S	stAttr;
	CVI_S32			s32Ret;

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	if (CVI_VO_GetGammaInfo(&stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	s32Ret = CVI_SUCCESS;
	GET_INT_FROM_JSON(ptContentIn->pParams, "/enable", stAttr.enable, s32Ret);
	GET_INT_FROM_JSON(ptContentIn->pParams, "/osd_apply", stAttr.osd_apply, s32Ret);
	GET_INT_ARRAY_FROM_JSON(ptContentIn->pParams, "/value", VO_GAMMA_NODENUM, CVI_U8, stAttr.value, s32Ret);

	if (CVI_VO_SetGammaInfo(&stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VO_GetGammaInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	VO_GAMMA_INFO_S	stAttr;

	if (CVI_VO_GetGammaInfo(&stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("enable", stAttr.enable, pDataOut);
	SET_INT_TO_JSON("osd_apply", stAttr.osd_apply, pDataOut);
	SET_INT_ARRAY_TO_JSON("value", VO_GAMMA_NODENUM, stAttr.value, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
