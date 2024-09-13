/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_vi.c
 * Description:
 */

#include <linux/cvi_comm_vpss.h>
#include <linux/cvi_comm_gdc.h>
#include "cvi_ispd2_callback_funcs_local.h"

#include "cvi_vi.h"
#include "cvi_gdc.h"
#include "cvi_vpss.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VI_VPSS_GetLDCChnSize(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	VI_CHN_ATTR_S	stViAttr = {};
	VPSS_CHN_ATTR_S stVpssAttr = {};

	if (CVI_VI_GetChnAttr(ptDevInfo->s32ViPipe, ptDevInfo->s32ViChn, &stViAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_VPSS_GetChnAttr(ptDevInfo->s32VpssGrp, ptDevInfo->s32VpssChn, &stVpssAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();
	JSONObject *pViAttrObject = ISPD2_json_object_new_object();
	JSONObject *pVpssAttrObject = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("u32Width", stViAttr.stSize.u32Width, pViAttrObject);
	SET_INT_TO_JSON("u32Height", stViAttr.stSize.u32Height, pViAttrObject);
	SET_INT_TO_JSON("u32Width", stVpssAttr.u32Width, pVpssAttrObject);
	SET_INT_TO_JSON("u32Height", stVpssAttr.u32Height, pVpssAttrObject);

	ISPD2_json_object_object_add(pDataOut, "stViAttr", pViAttrObject);
	ISPD2_json_object_object_add(pDataOut, "stVpssAttr", pVpssAttrObject);
	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}
// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_VI_LDCBinData(CVI_S32 ViPipe, CVI_S32 ViChn, TBinaryData *ptBinaryData,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	if ((ptBinaryData->eDataType != EBINARYDATA_VI_LDC_BIN_DATA)
		|| (ptBinaryData->pu8Buffer == NULL)
		|| (ptBinaryData->u32Size == 0)
	) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Invalid binary info.");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	MESH_DUMP_ATTR_S meshDumpAttr = {0};
	VI_LDC_ATTR_S stViLDCAttr = {0};
	LDC_ATTR_S stLDCAttr;

	ret = CVI_VI_GetChnLDCAttr(ViPipe, ViChn, &stViLDCAttr);
	if (ret == CVI_SUCCESS) {
		stLDCAttr = stViLDCAttr.stAttr;
		meshDumpAttr.enModId = CVI_ID_VI;
		meshDumpAttr.viMeshAttr.chn = ViChn;
		ret = CVI_GDC_LoadMeshWithBuf(&meshDumpAttr, &stLDCAttr, ptBinaryData->pu8Buffer, ptBinaryData->u32Size);
		if (ret != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG_EX(LOG_ALERT, "(VI) CVI_GDC_LoadMeshWithBuf fail");
			CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptContentOut, "(VI) CVI_GDC_LoadMeshWithBuf fail");
		}
	} else {
		ISP_DAEMON2_DEBUG_EX(LOG_ALERT, "(VI) Load LDC bin to buf fail!");
		CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptContentOut, "(VI) Load LDC bin to buf fail!");
		return CVI_FAILURE;
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

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
