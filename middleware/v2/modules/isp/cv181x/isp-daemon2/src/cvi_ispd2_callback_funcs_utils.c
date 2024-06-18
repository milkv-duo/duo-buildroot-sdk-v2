/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_utils.c
 * Description:
 */

#include "cvi_ispd2_callback_funcs_local.h"

#include "cvi_sys.h"

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeMessage(TJSONRpcContentOut *ptContentOut, CVI_S32 s32StatusCode, const char *pszMessage)
{
	ptContentOut->s32StatusCode = s32StatusCode;
	if (pszMessage) {
		snprintf(ptContentOut->szErrorMessage, MAX_CONTENT_ERROR_MSG_LENGTH, "%s", pszMessage);
	} else {
		snprintf(ptContentOut->szErrorMessage, MAX_CONTENT_ERROR_MSG_LENGTH, "None");
	}
}

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeAPIErrorMessage(TJSONRpcContentOut *ptContentOut)
{
	CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptContentOut, NULL);
}

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(TJSONRpcContentOut *ptContentOut, const char *pszMessage)
{
	if (pszMessage) {
		char szMessage[MAX_CONTENT_ERROR_MSG_LENGTH];

		snprintf(szMessage, MAX_CONTENT_ERROR_MSG_LENGTH, "Internal API Error : %s", pszMessage);
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR, szMessage);
	} else {
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Internal API Error");
	}
}

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeFileErrorMessage(TJSONRpcContentOut *ptContentOut)
{
	CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_FILE_ERROR,
		"Internal File Error");
}

// -----------------------------------------------------------------------------
void CVI_ISPD2_Utils_ComposeMissParameterErrorMessage(TJSONRpcContentOut *ptContentOut)
{
	CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_MISS_PARAMETER_ERROR,
		"Can't find mandatory parameter");
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_Utils_GetCurrentVPSSInfo(const TISPDeviceInfo *ptDevInfo, CVI_S32 *pVPSSGrp, CVI_S32 *pVPSSChn)
{
	MMF_CHN_S			stSrcChn;
	MMF_BIND_DEST_S		stBindDest;
	CVI_S32				s32TmpVPSSGrp, s32TmpVPSSChn;
	VI_VPSS_MODE_S stVIVPSSMode;

	s32TmpVPSSGrp		= ptDevInfo->s32VpssGrp;
	s32TmpVPSSChn		= ptDevInfo->s32VpssChn;

	stSrcChn.enModId	= CVI_ID_VI;
	stSrcChn.s32DevId	= 0;
	stSrcChn.s32ChnId	= ptDevInfo->s32ViChn;

	CVI_SYS_GetVIVPSSMode(&stVIVPSSMode);

	if (stVIVPSSMode.aenMode[ptDevInfo->s32ViPipe] != VI_OFFLINE_VPSS_ONLINE &&
			stVIVPSSMode.aenMode[ptDevInfo->s32ViPipe] != VI_ONLINE_VPSS_ONLINE) {
		if (CVI_SYS_GetBindbySrc(&stSrcChn, &stBindDest) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_SYS_GetBindbySrc fail (online mode)");
		} else {
			if (stBindDest.astMmfChn[0].enModId == CVI_ID_VPSS) {
				s32TmpVPSSGrp = stBindDest.astMmfChn[0].s32DevId;
				s32TmpVPSSChn = stBindDest.astMmfChn[0].s32ChnId;
			}
		}
	}

	*pVPSSGrp = s32TmpVPSSGrp;
	*pVPSSChn = s32TmpVPSSChn;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
