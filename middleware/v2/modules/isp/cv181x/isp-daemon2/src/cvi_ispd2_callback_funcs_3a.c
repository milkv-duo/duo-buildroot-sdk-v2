/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_3a.c
 * Description:
 */

#include "cvi_ispd2_callback_funcs_local.h"

#include "isp_json_struct_local.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_af.h"
#include "3A_internal.h"

// -----------------------------------------------------------------------------
#define CVI_3A_SET_API(set_api_name, get_api_name, struct_name) \
	{ \
		struct_name stAttr; \
		char content_buffer[CONTENT_BUFFER_SIZE] = { 0 }; \
\
		JSONObject *pJsonContentForISPBin = ISPD2_json_object_new_object(); \
		TISPDeviceInfo *ptDevInfo = ptContentIn->ptDeviceInfo; \
\
		snprintf(content_buffer, CONTENT_BUFFER_SIZE, "%s-%s", GET_RESPONSE_KEY_NAME, #struct_name);\
		ISPD2_json_object_object_add(pJsonContentForISPBin, content_buffer, ptContentIn->pParams); \
		ISPD2_json_object_get(ptContentIn->pParams); \
\
		get_api_name(ptDevInfo->s32ViPipe, &stAttr); \
		struct_name##_JSON(JSONBIN_READ, pJsonContentForISPBin, content_buffer, &stAttr); \
\
		if (set_api_name(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) { \
			CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut); \
			ISPD2_json_object_put(pJsonContentForISPBin); \
			return CVI_FAILURE; \
		} \
\
		ISPD2_json_object_put(pJsonContentForISPBin); \
		UNUSED(pJsonResponse); \
		return CVI_SUCCESS; \
	}

// -----------------------------------------------------------------------------
#define CVI_3A_GET_API(get_api_name, struct_name) \
	{ \
		struct_name stAttr; \
\
		TISPDeviceInfo *ptDevInfo = ptContentIn->ptDeviceInfo; \
		if (get_api_name(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) { \
			CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut); \
			return CVI_FAILURE; \
		} \
		struct_name##_JSON(JSONBIN_WRITE, pJsonResponse, GET_RESPONSE_KEY_NAME, &stAttr); \
\
		return CVI_SUCCESS; \
	}

// -----------------------------------------------------------------------------
#define CVI_3A_SET_API_EX(module_name, struct_name) \
	{ \
		CVI_3A_SET_API(CVI_ISP_Set##module_name##Attr, CVI_ISP_Get##module_name##Attr, struct_name) \
	}

// -----------------------------------------------------------------------------
#define CVI_3A_GET_API_EX(module_name, struct_name) \
	{ \
		CVI_3A_GET_API(CVI_ISP_Get##module_name##Attr, struct_name) \
	}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(Exposure, ISP_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(Exposure, ISP_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWDRExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(WDRExposure, ISP_WDR_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWDRExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(WDRExposure, ISP_WDR_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSmartExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(SmartExposure, ISP_SMART_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSmartExposureAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(SmartExposure, ISP_SMART_EXPOSURE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(AERoute, ISP_AE_ROUTE_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(AERoute, ISP_AE_ROUTE_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API(CVI_ISP_SetAERouteAttrEx, CVI_ISP_GetAERouteAttrEx, ISP_AE_ROUTE_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_GetAERouteAttrEx, ISP_AE_ROUTE_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteSFAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(AERouteSF, ISP_AE_ROUTE_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteSFAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(AERouteSF, ISP_AE_ROUTE_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteSFAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API(CVI_ISP_SetAERouteSFAttrEx, CVI_ISP_GetAERouteSFAttrEx, ISP_AE_ROUTE_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteSFAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_GetAERouteSFAttrEx, ISP_AE_ROUTE_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_QueryExposureInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_QueryExposureInfo, ISP_EXP_INFO_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetIrisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(Iris, ISP_IRIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetIrisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(Iris, ISP_IRIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDCIrisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(Dciris, ISP_DCIRIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDCIrisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(Dciris, ISP_DCIRIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWBAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API_EX(WB, ISP_WB_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWBAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API_EX(WB, ISP_WB_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAWBAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API(CVI_ISP_SetAWBAttrEx, CVI_ISP_GetAWBAttrEx, ISP_AWB_ATTR_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAWBAttrEx(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_GetAWBAttrEx, ISP_AWB_ATTR_EX_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWBCalibration(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_SET_API(CVI_ISP_SetWBCalibration, CVI_ISP_GetWBCalibration, ISP_AWB_Calibration_Gain_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWBCalibration(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_GetWBCalibration, ISP_AWB_Calibration_Gain_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_QueryWBInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_3A_GET_API(CVI_ISP_QueryWBInfo, ISP_WB_INFO_S);
}

// -----------------------------------------------------------------------------
