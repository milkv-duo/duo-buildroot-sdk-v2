/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_isp.c
 * Description:
 */
#include "cvi_ispd2_callback_funcs_local.h"

#include "isp_json_struct_local.h"
#include "cvi_isp.h"

// -----------------------------------------------------------------------------
#define CVI_ISP_SET_API(set_api_name, get_api_name, struct_name) \
	{ \
		struct_name stAttr; \
		char content_buffer[CONTENT_BUFFER_SIZE] = {0}; \
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
#define CVI_ISP_GET_API(get_api_name, struct_name) \
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
#define CVI_ISP_SET_API_EX(module_name, struct_name) \
	{ \
		CVI_ISP_SET_API(CVI_ISP_Set##module_name##Attr, CVI_ISP_Get##module_name##Attr, struct_name) \
	}

// -----------------------------------------------------------------------------
#define CVI_ISP_GET_API_EX(module_name, struct_name) \
	{ \
		CVI_ISP_GET_API(CVI_ISP_Get##module_name##Attr, struct_name) \
	}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetPubAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	CVI_S32			s32Ret = CVI_SUCCESS;
	double			dlFrameRate = 0;

	ISP_PUB_ATTR_S	stAttr;

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	if (CVI_ISP_GetPubAttr(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	GET_DOUBLE_FROM_JSON(ptContentIn->pParams, "/f32FrameRate", dlFrameRate, s32Ret);

	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	stAttr.f32FrameRate = (CVI_FLOAT)(dlFrameRate);

	if (CVI_ISP_SetPubAttr(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetPubAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	ISP_PUB_ATTR_S	stAttr;

	if (CVI_ISP_GetPubAttr(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("stWndRect.s32X", stAttr.stWndRect.s32X, pDataOut);
	SET_INT_TO_JSON("stWndRect.s32Y", stAttr.stWndRect.s32Y, pDataOut);
	SET_INT_TO_JSON("stWndRect.u32Width", stAttr.stWndRect.u32Width, pDataOut);
	SET_INT_TO_JSON("stWndRect.u32Height", stAttr.stWndRect.u32Height, pDataOut);
	SET_INT_TO_JSON("stSnsSize.u32Width", stAttr.stSnsSize.u32Width, pDataOut);
	SET_INT_TO_JSON("stSnsSize.u32Height", stAttr.stSnsSize.u32Height, pDataOut);
	SET_DOUBLE_TO_JSON("f32FrameRate", stAttr.f32FrameRate, pDataOut);
	SET_INT_TO_JSON("enBayer", stAttr.enBayer, pDataOut);
	SET_INT_TO_JSON("enWDRMode", stAttr.enWDRMode, pDataOut);
	SET_INT_TO_JSON("u8SnsMode", stAttr.u8SnsMode, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDehazeAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Dehaze, ISP_DEHAZE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDehazeAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Dehaze, ISP_DEHAZE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetColorToneAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(ColorTone, ISP_COLOR_TONE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetColorToneAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(ColorTone, ISP_COLOR_TONE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetGammaAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Gamma, ISP_GAMMA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetGammaAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Gamma, ISP_GAMMA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAutoGammaAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(AutoGamma, ISP_AUTO_GAMMA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAutoGammaAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(AutoGamma, ISP_AUTO_GAMMA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetGammaCurveByType(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	CVI_S32				s32Ret;

	ISP_GAMMA_ATTR_S	stAttr;
	CVI_S32				s32CurveType;

	s32Ret = CVI_SUCCESS;
	GET_INT_FROM_JSON(ptContentIn->pParams, "/curveType", s32CurveType, s32Ret);
	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetGammaCurveByType(ptDevInfo->s32ViPipe, &stAttr, s32CurveType) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISP_GAMMA_ATTR_S_JSON(JSONBIN_WRITE, pJsonResponse, GET_RESPONSE_KEY_NAME, &stAttr);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CAC, ISP_CAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CAC, ISP_CAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CNR, ISP_CNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CNR, ISP_CNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCNRMotionNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CNRMotionNR, ISP_CNR_MOTION_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCNRMotionNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CNRMotionNR, ISP_CNR_MOTION_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(NR, ISP_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(NR, ISP_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNRFilterAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(NRFilter, ISP_NR_FILTER_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNRFilterAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(NRFilter, ISP_NR_FILTER_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(YNR, ISP_YNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(YNR, ISP_YNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRMotionNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(YNRMotionNR, ISP_YNR_MOTION_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRMotionNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(YNRMotionNR, ISP_YNR_MOTION_NR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRFilterAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(YNRFilter, ISP_YNR_FILTER_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRFilterAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(YNRFilter, ISP_YNR_FILTER_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNR, ISP_TNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNR, ISP_TNR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRNoiseModelAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNRNoiseModel, ISP_TNR_NOISE_MODEL_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRNoiseModelAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNRNoiseModel, ISP_TNR_NOISE_MODEL_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRLumaMotionAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNRLumaMotion, ISP_TNR_LUMA_MOTION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRLumaMotionAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNRLumaMotion, ISP_TNR_LUMA_MOTION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRGhostAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNRGhost, ISP_TNR_GHOST_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRGhostAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNRGhost, ISP_TNR_GHOST_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRMtPrtAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNRMtPrt, ISP_TNR_MT_PRT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRMtPrtAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNRMtPrt, ISP_TNR_MT_PRT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRMotionAdaptAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(TNRMotionAdapt, ISP_TNR_MOTION_ADAPT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRMotionAdaptAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(TNRMotionAdapt, ISP_TNR_MOTION_ADAPT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDCIAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(DCI, ISP_DCI_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDCIAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(DCI, ISP_DCI_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetLDCIAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(LDCI, ISP_LDCI_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetLDCIAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(LDCI, ISP_LDCI_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDemosaicAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Demosaic, ISP_DEMOSAIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDemosaicAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Demosaic, ISP_DEMOSAIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDemosaicDemoireAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(DemosaicDemoire, ISP_DEMOSAIC_DEMOIRE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDemosaicDemoireAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(DemosaicDemoire, ISP_DEMOSAIC_DEMOIRE_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMeshShadingAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(MeshShading, ISP_MESH_SHADING_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMeshShadingAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(MeshShading, ISP_MESH_SHADING_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMeshShadingGainLutAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(MeshShadingGainLut, ISP_MESH_SHADING_GAIN_LUT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMeshShadingGainLutAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(MeshShadingGainLut, ISP_MESH_SHADING_GAIN_LUT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetBlackLevelAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(BlackLevel, ISP_BLACK_LEVEL_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetBlackLevelAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(BlackLevel, ISP_BLACK_LEVEL_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCCMAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CCM, ISP_CCM_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCCMAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CCM, ISP_CCM_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCCMSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CCMSaturation, ISP_CCM_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCCMSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CCMSaturation, ISP_CCM_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetFSWDRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(FSWDR, ISP_FSWDR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetFSWDRAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(FSWDR, ISP_FSWDR_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDRCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(DRC, ISP_DRC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDRCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(DRC, ISP_DRC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDPDynamicAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(DPDynamic, ISP_DP_DYNAMIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDPDynamicAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(DPDynamic, ISP_DP_DYNAMIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDPStaticAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(DPStatic, ISP_DP_STATIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDPStaticAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(DPStatic, ISP_DP_STATIC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCrosstalkAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Crosstalk, ISP_CROSSTALK_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCrosstalkAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Crosstalk, ISP_CROSSTALK_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetPreSharpenAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(PreSharpen, ISP_PRESHARPEN_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetPreSharpenAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(PreSharpen, ISP_PRESHARPEN_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSharpenAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Sharpen, ISP_SHARPEN_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSharpenAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Sharpen, ISP_SHARPEN_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYContrastAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(YContrast, ISP_YCONTRAST_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYContrastAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(YContrast, ISP_YCONTRAST_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Saturation, ISP_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Saturation, ISP_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetStatisticsConfig(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	ISP_STATISTICS_CFG_S	stAttr;
	JSONObject		*pstJsonObject = ISPD2_json_object_new_object();

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
	if (CVI_ISP_GetStatisticsConfig(ptDevInfo->s32ViPipe, &stAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ISPD2_json_pointer_set(&ptContentIn->pParams, "/unKey", pstJsonObject);
	SET_UINT64_TO_JSON("bit1FEAeGloStat", stAttr.unKey.bit1FEAeGloStat, pstJsonObject);
	SET_UINT64_TO_JSON("bit1FEAeLocStat", stAttr.unKey.bit1FEAeLocStat, pstJsonObject);
	SET_UINT64_TO_JSON("bit1AwbStat1", stAttr.unKey.bit1AwbStat1, pstJsonObject);
	SET_UINT64_TO_JSON("bit1AwbStat2", stAttr.unKey.bit1AwbStat2, pstJsonObject);
	SET_UINT64_TO_JSON("bit1FEAfStat", stAttr.unKey.bit1FEAfStat, pstJsonObject);
	SET_UINT64_TO_JSON("bit14Rsv", stAttr.unKey.bit14Rsv, pstJsonObject);

	CVI_ISP_SET_API(CVI_ISP_SetStatisticsConfig, CVI_ISP_GetStatisticsConfig, ISP_STATISTICS_CFG_S);
	ISPD2_json_object_put(pstJsonObject);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetStatisticsConfig(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API(CVI_ISP_GetStatisticsConfig, ISP_STATISTICS_CFG_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNoiseProfileAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(NoiseProfile, ISP_CMOS_NOISE_CALIBRATION_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNoiseProfileAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(NoiseProfile, ISP_CMOS_NOISE_CALIBRATION_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Dis, ISP_DIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDisAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Dis, ISP_DIS_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDisConfig(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API(CVI_ISP_SetDisConfig, CVI_ISP_GetDisConfig, ISP_DIS_CONFIG_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDisConfig(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API(CVI_ISP_GetDisConfig, ISP_DIS_CONFIG_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMonoAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Mono, ISP_MONO_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMonoAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Mono, ISP_MONO_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetRGBCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(RGBCAC, ISP_RGBCAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetRGBCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(RGBCAC, ISP_RGBCAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetLCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(LCAC, ISP_LCAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetLCACAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(LCAC, ISP_LCAC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetClutAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(Clut, ISP_CLUT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetClutAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(Clut, ISP_CLUT_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetClutSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(ClutSaturation, ISP_CLUT_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetClutSaturationAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(ClutSaturation, ISP_CLUT_SATURATION_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetVCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(VC, ISP_VC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetVCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(VC, ISP_VC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCSCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CSC, ISP_CSC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCSCAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CSC, ISP_CSC_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCAAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CA, ISP_CA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCAAttr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CA, ISP_CA_ATTR_S);
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCA2Attr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_SET_API_EX(CA2, ISP_CA2_ATTR_S);
}

CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCA2Attr(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_ISP_GET_API_EX(CA2, ISP_CA2_ATTR_S);
}

// -----------------------------------------------------------------------------
