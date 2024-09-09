/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_daemon.c
 * Description:
 */
#include "cvi_ispd2_callback_funcs_local.h"

#include "cvi_sys.h"
#include "cvi_isp.h"

#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_af.h"
#include "3A_internal.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetVersionInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	char szVersion[64];

	snprintf(szVersion, 64, "%d.%d.%d.%d",
		ISP_DAEMON2_VERSION_MAJOR, ISP_DAEMON2_VERSION_MINOR,
		ISP_DAEMON2_VERSION_REVISION, ISP_DAEMON2_VERSION_BUILD);

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_STRING_TO_JSON("Version", szVersion, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_CBFunc_VIVPSSBindCtrl(const TISPDeviceInfo *ptDevInfo, CVI_S32 ViPipe, CVI_S32 ViChn)
{
	if (ptDevInfo->s32ViChn == ViChn) {
		return CVI_SUCCESS;
	}

	MMF_CHN_S			stSrcChn;
	MMF_BIND_DEST_S		stBindDest;

	stSrcChn.enModId	= CVI_ID_VI;
	stSrcChn.s32DevId	= 0;
	stSrcChn.s32ChnId	= ptDevInfo->s32ViChn;

	if (CVI_SYS_GetBindbySrc(&stSrcChn, &stBindDest) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_SYS_GetBindbySrc fail");
		return CVI_FAILURE;
	}
	if (CVI_SYS_UnBind(&stSrcChn, &stBindDest.astMmfChn[0]) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "CVI_SYS_UnBind VI(%d, %d)-VPSS(%d, %d) fail",
			stSrcChn.s32DevId, stSrcChn.s32ChnId,
			stBindDest.astMmfChn[0].s32DevId, stBindDest.astMmfChn[0].s32ChnId);
		return CVI_FAILURE;
	}

	stSrcChn.enModId	= CVI_ID_VI;
	stSrcChn.s32DevId	= 0;
	stSrcChn.s32ChnId	= ViChn;

	if (CVI_SYS_Bind(&stSrcChn, &stBindDest.astMmfChn[0]) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "CVI_SYS_Bind VI(%d, %d)-VPSS(%d, %d) fail",
			stSrcChn.s32DevId, stSrcChn.s32ChnId,
			stBindDest.astMmfChn[0].s32DevId, stBindDest.astMmfChn[0].s32ChnId);
		return CVI_FAILURE;
	}

	UNUSED(ViPipe);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_SetTopInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	CVI_S32			ViPipe, ViChn, VpssGrp, VpssChn;
	CVI_S32			s32Ret;

	ViPipe = ptDevInfo->s32ViPipe;
	ViChn = ptDevInfo->s32ViChn;
	VpssGrp = ptDevInfo->s32VpssGrp;
	VpssChn = ptDevInfo->s32VpssChn;

	s32Ret = CVI_SUCCESS;
	GET_INT_FROM_JSON(ptContentIn->pParams, "/ViPipe", ViPipe, s32Ret);
	GET_INT_FROM_JSON(ptContentIn->pParams, "/ViChn", ViChn, s32Ret);
	GET_INT_FROM_JSON(ptContentIn->pParams, "/VpssGrp", VpssGrp, s32Ret);
	GET_INT_FROM_JSON(ptContentIn->pParams, "/VpssChn", VpssChn, s32Ret);

	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (ptDevInfo->bVPSSBindCtrl) {
		CVI_ISPD2_CBFunc_VIVPSSBindCtrl(ptDevInfo, ViPipe, ViChn);
	}

	ptDevInfo->s32ViPipe = ViPipe;
	ptDevInfo->s32ViChn = ViChn;
	ptDevInfo->s32VpssGrp = VpssGrp;
	ptDevInfo->s32VpssChn = VpssChn;

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetTopInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	CVI_S32			VpssGrp, VpssChn;
	MMF_CHN_S		stSrcChn;
	MMF_BIND_DEST_S	stBindDest;
	VI_VPSS_MODE_S stVIVPSSMode;

	stSrcChn.enModId	= CVI_ID_VI;
	stSrcChn.s32DevId	= 0;
	stSrcChn.s32ChnId	= ptDevInfo->s32ViChn;

	VpssGrp = ptDevInfo->s32VpssGrp;
	VpssChn = ptDevInfo->s32VpssChn;

	CVI_SYS_GetVIVPSSMode(&stVIVPSSMode);

	if (stVIVPSSMode.aenMode[ptDevInfo->s32ViPipe] != VI_OFFLINE_VPSS_ONLINE &&
			stVIVPSSMode.aenMode[ptDevInfo->s32ViPipe] != VI_ONLINE_VPSS_ONLINE) {
		if (CVI_SYS_GetBindbySrc(&stSrcChn, &stBindDest) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_SYS_GetBindbySrc fail (VI online mode)");
		} else {
			if (stBindDest.astMmfChn[0].enModId == CVI_ID_VPSS) {
				VpssGrp = stBindDest.astMmfChn[0].s32DevId;
				VpssChn = stBindDest.astMmfChn[0].s32ChnId;
			}
		}
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("ViPipe", ptDevInfo->s32ViPipe, pDataOut);
	SET_INT_TO_JSON("ViChn", ptDevInfo->s32ViChn, pDataOut);
	SET_INT_TO_JSON("VpssGrp", VpssGrp, pDataOut);
	SET_INT_TO_JSON("VpssChn", VpssChn, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetDRCHistogramInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo			*ptDevInfo = ptContentIn->ptDeviceInfo;

	ISP_INNER_STATE_INFO_S	stInnerStateInfo = {};
	ISP_EXP_INFO_S expData = {};

	if (CVI_ISP_QueryInnerStateInfo(ptDevInfo->s32ViPipe, &stInnerStateInfo) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_ISP_QueryExposureInfo(ptDevInfo->s32ViPipe, &expData) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	// ISP_INNER_STATE_INFO_S
	SET_INT_TO_JSON("GlobalToneBinNum", stInnerStateInfo.drcGlobalToneBinNum, pDataOut);
	SET_INT_TO_JSON("GlobalToneBinSEStep", stInnerStateInfo.drcGlobalToneBinSEStep, pDataOut);
	SET_INT_TO_JSON("Exposure Ratio", expData.u32WDRExpRatio, pDataOut);
	SET_INT_ARRAY_TO_JSON("GlobalToneCurve", LTM_GLOBAL_CURVE_NODE_NUM, stInnerStateInfo.drcGlobalTone, pDataOut);
	SET_INT_ARRAY_TO_JSON("DarkToneCurve", LTM_DARK_CURVE_NODE_NUM, stInnerStateInfo.drcDarkTone, pDataOut);
	SET_INT_ARRAY_TO_JSON("BrightToneCurve", LTM_BRIGHT_CURVE_NODE_NUM, stInnerStateInfo.drcBrightTone, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetChipInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_U32 u32ChipID;

	if (CVI_SYS_GetChipId(&u32ChipID) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("Chip ID", u32ChipID, pDataOut);
	SET_STRING_TO_JSON("ISP Branch", ISP_BRANCH, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetWDRInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	ISP_PUB_ATTR_S	stPubAttr;

	if (CVI_ISP_GetPubAttr(ptDevInfo->s32ViPipe, &stPubAttr) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("wdrmode", stPubAttr.enWDRMode, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_SetLogLevelInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_S32		s32Ret = CVI_SUCCESS;
	CVI_S32		s32LogLevel = 0;

	GET_INT_FROM_JSON(ptContentIn->pParams, "/Level", s32LogLevel, s32Ret);

	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if ((s32LogLevel < LOG_EMERG) || (s32LogLevel > LOG_DEBUG)) {
		char szOutputMessage[128];

		snprintf(szOutputMessage, 128, "Invalid log level : %d, (system : %d, range : %d ~ %d)",
			s32LogLevel, gu8ISPD2_LogExportLevel, LOG_EMERG, LOG_DEBUG);

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "%s", szOutputMessage);
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS, szOutputMessage);
		return CVI_FAILURE;
	}

	gu8ISPD2_LogExportLevel = (uint8_t)s32LogLevel;

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("Level", gu8ISPD2_LogExportLevel, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_Get3AStatisticsInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo			*ptDevInfo = ptContentIn->ptDeviceInfo;

	ISP_STATISTICS_CFG_S	stStatistics;
	ISP_AE_STATISTICS_S		stAEStatistic;
	ISP_AF_STATISTICS_S		stAFStatistic;
	ISP_WB_STATISTICS_S		stWBStatistic;
	CVI_U8					*pu8AWBDbgBin;
	s_AWB_DBG_S				*ptAWBDbg;
	CVI_S32					s32AWBDbgBinSize;

	if (CVI_ISP_GetAEStatistics(ptDevInfo->s32ViPipe, &stAEStatistic) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetFocusStatistics(ptDevInfo->s32ViPipe, &stAFStatistic) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetWBStatistics(ptDevInfo->s32ViPipe, &stWBStatistic) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetStatisticsConfig(ptDevInfo->s32ViPipe, &stStatistics) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	pu8AWBDbgBin = NULL;

	s32AWBDbgBinSize = CVI_ISP_GetAWBDbgBinSize();
	if (s32AWBDbgBinSize <= 0) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAWBDbgBinSize fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	pu8AWBDbgBin = (CVI_U8 *)malloc(sizeof(CVI_U8) * s32AWBDbgBinSize);
	if (pu8AWBDbgBin == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate memory for AWB Debug bin buffer fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}
	if (CVI_ISP_GetAWBDbgBinBuf(ptDevInfo->s32ViPipe, pu8AWBDbgBin, s32AWBDbgBinSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAWBDbgBinBuf fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		SAFE_FREE(pu8AWBDbgBin);
		return CVI_FAILURE;
	}
	ptAWBDbg = (s_AWB_DBG_S *)pu8AWBDbgBin;

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	// ISP_AE_STATISTICS_S
	SET_INT_ARRAY_TO_JSON("FE0 Histogram", MAX_HIST_BINS, stAEStatistic.au32FEHist1024Value[0][0], pDataOut);
	SET_INT_ARRAY_TO_JSON("FE1 Histogram", MAX_HIST_BINS, stAEStatistic.au32FEHist1024Value[1][0], pDataOut);
	SET_INT_TO_JSON("FE0 Global R", stAEStatistic.au16FEGlobalAvg[0][0][0], pDataOut);
	SET_INT_TO_JSON("FE0 Global Gr", stAEStatistic.au16FEGlobalAvg[0][0][1], pDataOut);
	SET_INT_TO_JSON("FE0 Global Gb", stAEStatistic.au16FEGlobalAvg[0][0][2], pDataOut);
	SET_INT_TO_JSON("FE0 Global B", stAEStatistic.au16FEGlobalAvg[0][0][3], pDataOut);
	SET_INT_TO_JSON("FE1 Global R", stAEStatistic.au16FEGlobalAvg[1][0][0], pDataOut);
	SET_INT_TO_JSON("FE1 Global Gr", stAEStatistic.au16FEGlobalAvg[1][0][1], pDataOut);
	SET_INT_TO_JSON("FE1 Global Gb", stAEStatistic.au16FEGlobalAvg[1][0][2], pDataOut);
	SET_INT_TO_JSON("FE1 Global B", stAEStatistic.au16FEGlobalAvg[1][0][3], pDataOut);

	// 2D array
	JSONObject *pFE0_ZoneR_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE0_ZoneGr_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE0_ZoneGb_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE0_ZoneB_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE1_ZoneR_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE1_ZoneGr_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE1_ZoneGb_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pFE1_ZoneB_Value2DArray = ISPD2_json_object_new_array();

	//merge 34x30 to 17x15
	for (CVI_U32 u32RowIdx = 0; u32RowIdx < (CVI_U32)AE_ZONE_ROW / 2; ++u32RowIdx) {
		JSONObject *pFE0_ZoneR_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE0_ZoneGr_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE0_ZoneGb_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE0_ZoneB_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE1_ZoneR_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE1_ZoneGr_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE1_ZoneGb_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pFE1_ZoneB_ValueSubArray = ISPD2_json_object_new_array();

		for (CVI_U32 u32ColIdx = 0; u32ColIdx < (CVI_U32)AE_ZONE_COLUMN / 2; ++u32ColIdx) {
			ISPD2_json_object_array_add(pFE0_ZoneR_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2][3] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2 + 1][3] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2][3] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][3]) / 4));
			ISPD2_json_object_array_add(pFE0_ZoneGr_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2][2] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2 + 1][2] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2][2] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][2]) / 4));
			ISPD2_json_object_array_add(pFE0_ZoneGb_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2][1] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2 + 1][1] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2][1] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][1]) / 4));
			ISPD2_json_object_array_add(pFE0_ZoneB_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2][0] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2][u32ColIdx * 2 + 1][0] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2][0] +
				stAEStatistic.au16FEZoneAvg[0][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][0]) / 4));

			ISPD2_json_object_array_add(pFE1_ZoneR_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2][3] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2 + 1][3] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2][3] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][3]) / 4));
			ISPD2_json_object_array_add(pFE1_ZoneGr_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2][2] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2 + 1][2] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2][2] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][2]) / 4));
			ISPD2_json_object_array_add(pFE1_ZoneGb_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2][1] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2 + 1][1] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2][1] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][1]) / 4));
			ISPD2_json_object_array_add(pFE1_ZoneB_ValueSubArray,
				ISPD2_json_object_new_int((
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2][0] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2][u32ColIdx * 2 + 1][0] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2][0] +
				stAEStatistic.au16FEZoneAvg[1][0][u32RowIdx * 2 + 1][u32ColIdx * 2 + 1][0]) / 4));
		}

		ISPD2_json_object_array_add(pFE0_ZoneR_Value2DArray, pFE0_ZoneR_ValueSubArray);
		ISPD2_json_object_array_add(pFE0_ZoneGr_Value2DArray, pFE0_ZoneGr_ValueSubArray);
		ISPD2_json_object_array_add(pFE0_ZoneGb_Value2DArray, pFE0_ZoneGb_ValueSubArray);
		ISPD2_json_object_array_add(pFE0_ZoneB_Value2DArray, pFE0_ZoneB_ValueSubArray);
		ISPD2_json_object_array_add(pFE1_ZoneR_Value2DArray, pFE1_ZoneR_ValueSubArray);
		ISPD2_json_object_array_add(pFE1_ZoneGr_Value2DArray, pFE1_ZoneGr_ValueSubArray);
		ISPD2_json_object_array_add(pFE1_ZoneGb_Value2DArray, pFE1_ZoneGb_ValueSubArray);
		ISPD2_json_object_array_add(pFE1_ZoneB_Value2DArray, pFE1_ZoneB_ValueSubArray);
	}

	ISPD2_json_object_object_add(pDataOut, "FE0 Zone R", pFE0_ZoneR_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE0 Zone Gr", pFE0_ZoneGr_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE0 Zone Gb", pFE0_ZoneGb_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE0 Zone B", pFE0_ZoneB_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE1 Zone R", pFE1_ZoneR_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE1 Zone Gr", pFE1_ZoneGr_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE1 Zone Gb", pFE1_ZoneGb_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "FE1 Zone B", pFE1_ZoneB_Value2DArray);

	// ISP_AF_STATISTICS_S
	// 2D array
	JSONObject *pAF_ZoneV1_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pAF_ZoneH1_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pAF_ZoneH2_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pAF_ZoneHlCnt_Value2DArray = ISPD2_json_object_new_array();

	ISP_FOCUS_ZONE_S *ptZoneMetrics = NULL;

	for (CVI_U32 u32RowIdx = 0; u32RowIdx < (CVI_U32)AF_ZONE_ROW; ++u32RowIdx) {
		JSONObject *pAF_ZoneV1_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pAF_ZoneH1_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pAF_ZoneH2_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pAF_ZoneHlCnt_ValueSubArray = ISPD2_json_object_new_array();

		ptZoneMetrics = stAFStatistic.stFEAFStat.stZoneMetrics[u32RowIdx];
		for (CVI_U32 u32ColIdx = 0; u32ColIdx < (CVI_U32)AF_ZONE_COLUMN; ++u32ColIdx) {
			ISPD2_json_object_array_add(pAF_ZoneV1_ValueSubArray,
				ISPD2_json_object_new_int(ptZoneMetrics[u32ColIdx].u32v0));
			ISPD2_json_object_array_add(pAF_ZoneH1_ValueSubArray,
				ISPD2_json_object_new_uint64(ptZoneMetrics[u32ColIdx].u64h0));
			ISPD2_json_object_array_add(pAF_ZoneH2_ValueSubArray,
				ISPD2_json_object_new_uint64(ptZoneMetrics[u32ColIdx].u64h1));
			ISPD2_json_object_array_add(pAF_ZoneHlCnt_ValueSubArray,
				ISPD2_json_object_new_int(ptZoneMetrics[u32ColIdx].u16HlCnt));
		}

		ISPD2_json_object_array_add(pAF_ZoneV1_Value2DArray, pAF_ZoneV1_ValueSubArray);
		ISPD2_json_object_array_add(pAF_ZoneH1_Value2DArray, pAF_ZoneH1_ValueSubArray);
		ISPD2_json_object_array_add(pAF_ZoneH2_Value2DArray, pAF_ZoneH2_ValueSubArray);
		ISPD2_json_object_array_add(pAF_ZoneHlCnt_Value2DArray, pAF_ZoneHlCnt_ValueSubArray);
	}

	ISPD2_json_object_object_add(pDataOut, "AF Zone V1", pAF_ZoneV1_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "AF Zone H1", pAF_ZoneH1_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "AF Zone H2", pAF_ZoneH2_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "AF Zone HlCnt", pAF_ZoneHlCnt_Value2DArray);

	// ISP_WB_STATISTICS_S
	SET_INT_TO_JSON("WB Global R", stWBStatistic.u16GlobalR, pDataOut);
	SET_INT_TO_JSON("WB Global G", stWBStatistic.u16GlobalG, pDataOut);
	SET_INT_TO_JSON("WB Global B", stWBStatistic.u16GlobalB, pDataOut);
	SET_INT_TO_JSON("WB Global CountAll", stWBStatistic.u16CountAll, pDataOut);
	SET_INT_ARRAY_TO_JSON("WB Zone R", AWB_ZONE_NUM, stWBStatistic.au16ZoneAvgR, pDataOut);
	SET_INT_ARRAY_TO_JSON("WB Zone G", AWB_ZONE_NUM, stWBStatistic.au16ZoneAvgG, pDataOut);
	SET_INT_ARRAY_TO_JSON("WB Zone B", AWB_ZONE_NUM, stWBStatistic.au16ZoneAvgB, pDataOut);
	SET_INT_ARRAY_TO_JSON("WB Zone CountAll", AWB_ZONE_NUM, stWBStatistic.au16ZoneCountAll, pDataOut);
	SET_INT_ARRAY_TO_JSON("AWB Curve R", 256, ptAWBDbg->u16CurveR, pDataOut);
	SET_INT_ARRAY_TO_JSON("AWB Curve B", 256, ptAWBDbg->u16CurveB, pDataOut);
	SET_INT_ARRAY_TO_JSON("AWB Curve B Top", 256, ptAWBDbg->u16CurveB_Top, pDataOut);
	SET_INT_ARRAY_TO_JSON("AWB Curve B Bot", 256, ptAWBDbg->u16CurveB_Bot, pDataOut);
	SET_INT_TO_JSON("AWB Balance R", ptAWBDbg->u16BalanceR, pDataOut);
	SET_INT_TO_JSON("AWB Balance B", ptAWBDbg->u16BalanceB, pDataOut);
	SET_INT_TO_JSON("AWB Curve Left Limit", ptAWBDbg->dbgInfoAttrEx[0].u16CurveLLimit, pDataOut);
	SET_INT_TO_JSON("AWB Curve Right Limit", ptAWBDbg->dbgInfoAttrEx[0].u16CurveRLimit, pDataOut);
	SET_INT_TO_JSON("ExtraLightEn", ptAWBDbg->dbgInfoAttrEx[0].bExtraLightEn, pDataOut);

	SET_INT_TO_JSON("LightInfo[0].WhiteRgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[0].u16WhiteRgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[0].WhiteBgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[0].u16WhiteBgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[0].LightStatus", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[0].u8LightStatus, pDataOut);
	SET_INT_TO_JSON("LightInfo[0].Radius", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[0].u8Radius, pDataOut);
	SET_INT_TO_JSON("LightInfo[1].WhiteRgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[1].u16WhiteRgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[1].WhiteBgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[1].u16WhiteBgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[1].LightStatus", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[1].u8LightStatus, pDataOut);
	SET_INT_TO_JSON("LightInfo[1].Radius", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[1].u8Radius, pDataOut);
	SET_INT_TO_JSON("LightInfo[2].WhiteRgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[2].u16WhiteRgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[2].WhiteBgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[2].u16WhiteBgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[2].LightStatus", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[2].u8LightStatus, pDataOut);
	SET_INT_TO_JSON("LightInfo[2].Radius", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[2].u8Radius, pDataOut);
	SET_INT_TO_JSON("LightInfo[3].WhiteRgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[3].u16WhiteRgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[3].WhiteBgain", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[3].u16WhiteBgain, pDataOut);
	SET_INT_TO_JSON("LightInfo[3].LightStatus", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[3].u8LightStatus, pDataOut);
	SET_INT_TO_JSON("LightInfo[3].Radius", ptAWBDbg->dbgInfoAttrEx[0].stLightInfo[3].u8Radius, pDataOut);
	SET_INT_TO_JSON("AWB Calibration Status", ptAWBDbg->calib_sts, pDataOut);

	JSONObject *pAWB_CropInfo_ValueArray = ISPD2_json_object_new_array();

	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinWnum));
	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinHnum));
	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinOffX));
	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinOffY));
	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinWsize));
	ISPD2_json_object_array_add(pAWB_CropInfo_ValueArray, ISPD2_json_object_new_int(ptAWBDbg->u16WinHsize));
	ISPD2_json_object_object_add(pDataOut, "AWB WinCrop", pAWB_CropInfo_ValueArray);
	SET_INT_TO_JSON("AWB WinCrop Status", stStatistics.stWBCfg.stCrop.bEnable, pDataOut);

	SET_INT_ARRAY_TO_JSON("AWB CurveRegion", 3, ptAWBDbg->u16Region_R, pDataOut);

	JSONObject *pAWB_CalibPT_ValueArray = ISPD2_json_object_new_array();

	for (CVI_U32 u32PtsIdx = 0; u32PtsIdx < AWB_CALIB_PTS_NUM; ++u32PtsIdx) {
		ISPD2_json_object_array_add(pAWB_CalibPT_ValueArray,
			ISPD2_json_object_new_int(ptAWBDbg->CalibRgain[u32PtsIdx]));
		ISPD2_json_object_array_add(pAWB_CalibPT_ValueArray,
			ISPD2_json_object_new_int(ptAWBDbg->CalibBgain[u32PtsIdx]));
		ISPD2_json_object_array_add(pAWB_CalibPT_ValueArray,
			ISPD2_json_object_new_int(ptAWBDbg->CalibTemp[u32PtsIdx]));
	}
	ISPD2_json_object_object_add(pDataOut, "AWB Calib PT", pAWB_CalibPT_ValueArray);

	SAFE_FREE(pu8AWBDbgBin);
	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetAFStatisticsInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo				*ptDevInfo = ptContentIn->ptDeviceInfo;

	ISP_FOCUS_STATISTICS_CFG_S	*ptFocusStatistics;
	ISP_STATISTICS_CFG_S		stStatistics;
	ISP_AF_STATISTICS_S			stAFStatistic;

	if (CVI_ISP_GetStatisticsConfig(ptDevInfo->s32ViPipe, &stStatistics) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	ptFocusStatistics = &(stStatistics.stFocusCfg);

	if (CVI_ISP_GetFocusStatistics(ptDevInfo->s32ViPipe, &stAFStatistic) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_TO_JSON("Hwnd", ptFocusStatistics->stConfig.u16Hwnd, pDataOut);
	SET_INT_TO_JSON("Vwnd", ptFocusStatistics->stConfig.u16Vwnd, pDataOut);
	SET_INT_TO_JSON("Crop.Enable", ptFocusStatistics->stConfig.stCrop.bEnable, pDataOut);
	SET_INT_TO_JSON("Crop.X", ptFocusStatistics->stConfig.stCrop.u16X, pDataOut);
	SET_INT_TO_JSON("Crop.Y", ptFocusStatistics->stConfig.stCrop.u16Y, pDataOut);
	SET_INT_TO_JSON("Crop.W", ptFocusStatistics->stConfig.stCrop.u16W, pDataOut);
	SET_INT_TO_JSON("Crop.H", ptFocusStatistics->stConfig.stCrop.u16H, pDataOut);

	JSONObject *pAF_ZoneV1_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pAF_ZoneH1_Value2DArray = ISPD2_json_object_new_array();
	JSONObject *pAF_ZoneH2_Value2DArray = ISPD2_json_object_new_array();

	ISP_FOCUS_ZONE_S *ptZoneMetrics = NULL;

	for (CVI_U32 u32RowIdx = 0; u32RowIdx < (CVI_U32)AF_ZONE_ROW; ++u32RowIdx) {
		JSONObject *pAF_ZoneV1_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pAF_ZoneH1_ValueSubArray = ISPD2_json_object_new_array();
		JSONObject *pAF_ZoneH2_ValueSubArray = ISPD2_json_object_new_array();

		ptZoneMetrics = stAFStatistic.stFEAFStat.stZoneMetrics[u32RowIdx];
		for (CVI_U32 u32ColIdx = 0; u32ColIdx < (CVI_U32)AF_ZONE_COLUMN; ++u32ColIdx) {
			ISPD2_json_object_array_add(pAF_ZoneV1_ValueSubArray,
				ISPD2_json_object_new_int(ptZoneMetrics[u32ColIdx].u32v0));
			ISPD2_json_object_array_add(pAF_ZoneH1_ValueSubArray,
				ISPD2_json_object_new_uint64(ptZoneMetrics[u32ColIdx].u64h0));
			ISPD2_json_object_array_add(pAF_ZoneH2_ValueSubArray,
				ISPD2_json_object_new_uint64(ptZoneMetrics[u32ColIdx].u64h1));
		}

		ISPD2_json_object_array_add(pAF_ZoneV1_Value2DArray, pAF_ZoneV1_ValueSubArray);
		ISPD2_json_object_array_add(pAF_ZoneH1_Value2DArray, pAF_ZoneH1_ValueSubArray);
		ISPD2_json_object_array_add(pAF_ZoneH2_Value2DArray, pAF_ZoneH2_ValueSubArray);
	}

	ISPD2_json_object_object_add(pDataOut, "AF Zone V1", pAF_ZoneV1_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "AF Zone H1", pAF_ZoneH1_Value2DArray);
	ISPD2_json_object_object_add(pDataOut, "AF Zone H2", pAF_ZoneH2_Value2DArray);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
