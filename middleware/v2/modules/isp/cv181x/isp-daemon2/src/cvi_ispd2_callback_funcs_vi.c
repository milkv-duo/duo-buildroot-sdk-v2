/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_vi.c
 * Description:
 */

#include "cvi_ispd2_callback_funcs_local.h"

#include "cvi_vi.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VI_SetChnLDCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	VI_LDC_ATTR_S	stAttr;
	CVI_S32			s32Ret;
	JSONObject		*pstJsonObject = NULL;

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	if (CVI_VI_GetChnLDCAttr(ptDevInfo->s32ViPipe, ptDevInfo->s32ViChn, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	s32Ret = CVI_SUCCESS;
	GET_INT_FROM_JSON(ptContentIn->pParams, "/bEnable", stAttr.bEnable, s32Ret);

	ISPD2_json_pointer_get(ptContentIn->pParams, "/stAttr", &pstJsonObject);
	GET_INT_FROM_JSON(pstJsonObject, "/bAspect", stAttr.stAttr.bAspect, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32XRatio", stAttr.stAttr.s32XRatio, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32YRatio", stAttr.stAttr.s32YRatio, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32XYRatio", stAttr.stAttr.s32XYRatio, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32CenterXOffset", stAttr.stAttr.s32CenterXOffset, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32CenterYOffset", stAttr.stAttr.s32CenterYOffset, s32Ret);
	GET_INT_FROM_JSON(pstJsonObject, "/s32DistortionRatio", stAttr.stAttr.s32DistortionRatio, s32Ret);

	if (CVI_VI_SetChnLDCAttr(ptDevInfo->s32ViPipe, ptDevInfo->s32ViChn, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VI_GetChnLDCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	VI_LDC_ATTR_S	stAttr;

	if (CVI_VI_GetChnLDCAttr(ptDevInfo->s32ViPipe, ptDevInfo->s32ViChn, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();
	JSONObject *pLdcAttrObject = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("bEnable", stAttr.bEnable, pDataOut);
	SET_INT_TO_JSON("bAspect", stAttr.stAttr.bAspect, pLdcAttrObject);
	SET_INT_TO_JSON("s32XRatio", stAttr.stAttr.s32XRatio, pLdcAttrObject);
	SET_INT_TO_JSON("s32YRatio", stAttr.stAttr.s32YRatio, pLdcAttrObject);
	SET_INT_TO_JSON("s32XYRatio", stAttr.stAttr.s32XYRatio, pLdcAttrObject);
	SET_INT_TO_JSON("s32CenterXOffset", stAttr.stAttr.s32CenterXOffset, pLdcAttrObject);
	SET_INT_TO_JSON("s32CenterYOffset", stAttr.stAttr.s32CenterYOffset, pLdcAttrObject);
	SET_INT_TO_JSON("s32DistortionRatio", stAttr.stAttr.s32DistortionRatio, pLdcAttrObject);

	ISPD2_json_object_object_add(pDataOut, "stAttr", pLdcAttrObject);
	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
