/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps_rawreplay.c
 * Description:
 */

#include <unistd.h>
#include <errno.h>

#include <dlfcn.h>

#include "cvi_ispd2_callback_funcs_local.h"
#include "cvi_ispd2_callback_funcs_apps_local.h"

#include "raw_replay.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_InitialRawReplayHandle(TRawReplayHandle *ptHandle)
{
	memset(ptHandle, 0, sizeof(TRawReplayHandle));
	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_ReleaseRawReplayHandle(TRawReplayHandle *ptHandle)
{
	if (ptHandle->pSoHandle == NULL) {
		return CVI_SUCCESS;
	}

	dlclose(ptHandle->pSoHandle);
	ptHandle->pSoHandle = NULL;

	SAFE_FREE(ptHandle->header);
	SAFE_FREE(ptHandle->data);

	memset(ptHandle, 0, sizeof(TRawReplayHandle));

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_InitRawRepalyHandle(TRawReplayHandle *pHandle)
{
	char *error = NULL;

	dlerror();
	pHandle->pSoHandle = dlopen("libraw_replay.so", RTLD_LAZY);
	error = dlerror();

	if (error != NULL) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "RawReplay dlopen error : %s", error);
	}

	if (pHandle->pSoHandle == NULL) {
		return CVI_FAILURE;
	}

	do {
		dlerror();

		pHandle->set_raw_replay_data = (CVI_S32 (*)(const CVI_VOID *,
				const CVI_VOID *, CVI_U32, CVI_U32, CVI_U32))
				dlsym(pHandle->pSoHandle, "set_raw_replay_data");
		error = dlerror();
		if (error != NULL) {
			break;
		}

		pHandle->start_raw_replay = (CVI_S32 (*)(VI_PIPE)) dlsym(pHandle->pSoHandle, "start_raw_replay");
		error = dlerror();
		if (error != NULL) {
			break;
		}

		pHandle->stop_raw_replay = (CVI_S32 (*)(CVI_VOID)) dlsym(pHandle->pSoHandle, "stop_raw_replay");
		error = dlerror();
		if (error != NULL) {
			break;
		}

		pHandle->get_raw_replay_yuv = (CVI_S32 (*)(CVI_U8, CVI_U8, CVI_S32, CVI_S32, VIDEO_FRAME_INFO_S *))
			dlsym(pHandle->pSoHandle, "get_raw_replay_yuv");
		error = dlerror();
		if (error != NULL) {
			break;
		}

	} while (0);

	if (error != NULL) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "RawReplay dlsym error : %s", error);
		dlclose(pHandle->pSoHandle);
		pHandle->pSoHandle = NULL;
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_RecvRawReplayDataInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	TISPDeviceInfo	*ptDevInfo = ptIn->ptDeviceInfo;
	TRawReplayHandle *pRawReplayHandle = &(ptDevInfo->tRawReplayHandle);

	if (pRawReplayHandle->pSoHandle == NULL) {

		s32Ret = CVI_ISPD2_InitRawRepalyHandle(pRawReplayHandle);

		if (s32Ret != CVI_SUCCESS) {
			CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut,
				"load libraw_replay.so failed, please put it to board.");
			return CVI_FAILURE;
		}
	}

	if (pRawReplayHandle->header == NULL) {
		pRawReplayHandle->header = calloc(1, sizeof(RAW_REPLAY_INFO));
		if (pRawReplayHandle == NULL) {
			CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut,
				"calloc raw header buffer failed.");
			return CVI_FAILURE;
		}
	}

	RAW_REPLAY_INFO *pHeader = (RAW_REPLAY_INFO *) pRawReplayHandle->header;

	if (ptIn->pParams == NULL) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptOut);
		return CVI_FAILURE;
	}

	pHeader->isValid = 1;

	int lightValueX100 = 0;

	GET_INT_FROM_JSON(ptIn->pParams, "/raw_numFrame", pHeader->numFrame, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_curFrame", pHeader->curFrame, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_width", pHeader->width, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_height", pHeader->height, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_bayerID", pHeader->bayerID, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_enWDR", pHeader->enWDR, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_ISO", pHeader->ISO, s32Ret);

	GET_INT_FROM_JSON(ptIn->pParams, "/raw_lightValueX100", lightValueX100, s32Ret);
	pHeader->lightValue = ((CVI_FLOAT) lightValueX100 / (CVI_FLOAT) 100);

	GET_INT_FROM_JSON(ptIn->pParams, "/raw_colorTemp", pHeader->colorTemp, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_ispDGain", pHeader->ispDGain, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_exposureRatio", pHeader->exposureRatio, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_exposureAGain", pHeader->exposureAGain, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_exposureDGain", pHeader->exposureDGain, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_longExposure", pHeader->longExposure, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_shortExposure", pHeader->shortExposure, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_AGainSF", pHeader->AGainSF, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_DGainSF", pHeader->DGainSF, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_ispDGainSF", pHeader->ispDGainSF, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_WB_RGain", pHeader->WB_RGain, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_WB_BGain", pHeader->WB_BGain, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_size", pHeader->size, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_roiFrameNum", pHeader->roiFrameNum, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_stRoiRect.s32X", pHeader->stRoiRect.s32X, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_stRoiRect.s32Y", pHeader->stRoiRect.s32Y, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_stRoiRect.u32Width", pHeader->stRoiRect.u32Width, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_stRoiRect.u32Height", pHeader->stRoiRect.u32Height, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_roiFrameSize", pHeader->roiFrameSize, s32Ret);
	GET_INT_FROM_JSON(ptIn->pParams, "/raw_op_mode", pHeader->op_mode, s32Ret);
	GET_INT_ARRAY_FROM_JSON(ptIn->pParams, "/raw_BLCOffset", 4, CVI_S32, pHeader->BLC_Offset, s32Ret);
	GET_INT_ARRAY_FROM_JSON(ptIn->pParams, "/raw_BLCGain", 4, CVI_S32, pHeader->BLC_Gain, s32Ret);

	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptOut);
		return CVI_FAILURE;
	}

	//ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "raw replay params: %s",
	//	ISPD2_json_object_to_json_string_ext(ptIn->pParams, JSON_C_TO_STRING_SPACED));

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "raw replay, totalFrame: %d, curFrame: %d, enWDR: %d, op_mode: %d",
		pHeader->numFrame,
		pHeader->curFrame,
		pHeader->enWDR,
		pHeader->op_mode);

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "raw repaly, frame info: %dX%d, %d, roi frame number: %d, %d",
		pHeader->width,
		pHeader->height,
		pHeader->size,
		pHeader->roiFrameNum,
		pHeader->roiFrameSize);

	if (pRawReplayHandle->data == NULL ||
		pHeader->curFrame == 0) {

		SAFE_FREE(pRawReplayHandle->data);

		pRawReplayHandle->data = calloc(1, pHeader->size);
		pRawReplayHandle->u32DataSize = pHeader->size;
		if (pRawReplayHandle->data == NULL) {
			CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut,
				"calloc raw data buffer failed.");
			return CVI_FAILURE;
		}
	}

	UNUSED(pJsonRes);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_SendRawReplayData(TISPDeviceInfo *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes)
{
	TBinaryData	*ptBinaryData = &(ptIn->tBinaryInData);
	TRawReplayHandle *pRawReplayHandle = &(ptIn->tRawReplayHandle);

	RAW_REPLAY_INFO *pHeader = (RAW_REPLAY_INFO *) pRawReplayHandle->header;

	if ((ptBinaryData->eDataType != EBINARYDATA_RAW_DATA)
		|| (ptBinaryData->pu8Buffer == NULL)
		|| (ptBinaryData->u32Size == 0)
	) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Invalid binary info.");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptOut);
		return CVI_FAILURE;
	}

	if (pRawReplayHandle->set_raw_replay_data(pRawReplayHandle->header,
			pRawReplayHandle->data,
			pHeader->numFrame,
			pHeader->curFrame,
			pHeader->size) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "set raw repaly data fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut, "set raw replay data fail");
		return CVI_FAILURE;
	}

	ptOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonRes);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_StartRawReplay(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	TRawReplayHandle *pRawReplayHandle = &(ptIn->ptDeviceInfo->tRawReplayHandle);

	if (pRawReplayHandle->pSoHandle == NULL) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut,
			"init failed.");
		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG(LOG_DEBUG, "start raw repaly");

	pRawReplayHandle->start_raw_replay(ptIn->ptDeviceInfo->s32ViPipe);

	UNUSED(pJsonRes);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_CancelRawReplay(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	TRawReplayHandle *pRawReplayHandle = &(ptIn->ptDeviceInfo->tRawReplayHandle);

	if (pRawReplayHandle->pSoHandle == NULL) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptOut,
			"init failed.");
		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG(LOG_DEBUG, "stop raw repaly");

	pRawReplayHandle->stop_raw_replay();

	SAFE_FREE(pRawReplayHandle->header);
	SAFE_FREE(pRawReplayHandle->data);

	UNUSED(pJsonRes);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_GetRawReplayYuv(TRawReplayHandle *ptHandle, CVI_U8 yuvIndex,
	CVI_U8 src, CVI_S32 pipeOrGrp, CVI_S32 chn, VIDEO_FRAME_INFO_S *pstFrameInfo)
{
	if (ptHandle == NULL) {
		return CVI_FAILURE;
	}

	if (ptHandle->get_raw_replay_yuv == NULL) {
		return CVI_FAILURE;
	}

	return ptHandle->get_raw_replay_yuv(yuvIndex, src, pipeOrGrp, chn, pstFrameInfo);
}

// -----------------------------------------------------------------------------
