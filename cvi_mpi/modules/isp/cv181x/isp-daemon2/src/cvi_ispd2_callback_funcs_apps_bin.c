/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps_bin.c
 * Description:
 */

#include <sys/stat.h>

#include "cvi_ispd2_callback_funcs_local.h"
#include "cvi_ispd2_callback_funcs_apps_local.h"
#include "cvi_bin.h"
#include "cvi_vpss.h"

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_WriteISPRegToBinBuffer(CVI_U8 *pu8Buffer, CVI_U32 u32MaxBufSize,
	CVI_BIN_EXTRA_S *ptBinExtra, CVI_U32 *pu32DataLen)
{
	if (pu8Buffer == NULL) {
		return CVI_FAILURE;
	}

	*pu32DataLen = 0;

	CVI_BIN_HEADER	stBinHeader;
	CVI_U32			u32BinLen;
	CVI_S32			s32Ret;

	u32BinLen = CVI_BIN_GetBinTotalLen();
	if (u32BinLen > u32MaxBufSize) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Buffer for tuning bin is less than request");
		return CVI_FAILURE;
	}

	memset(&stBinHeader, 0, sizeof(CVI_BIN_HEADER));
	if (ptBinExtra) {
		memcpy(&stBinHeader.extraInfo, ptBinExtra, sizeof(CVI_BIN_EXTRA_S));
	}
	memcpy(pu8Buffer, &stBinHeader, sizeof(CVI_BIN_HEADER));

	s32Ret = CVI_BIN_ExportBinData(pu8Buffer, u32BinLen);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_BIN_ExportBinData fail");
	} else {
		*pu32DataLen = u32BinLen;
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
static enum CVI_BIN_SECTION_ID CVI_ISPD2_GetModuleIdFromString(const char *pszTemp)
{
	enum CVI_BIN_SECTION_ID id = CVI_BIN_ID_MIN;

	if (strcmp(pszTemp, "All") == 0) {
		id = CVI_BIN_ID_MAX;
	} else if (strcmp(pszTemp, "Sensor_0") == 0) {
		id = CVI_BIN_ID_ISP0;
	} else if (strcmp(pszTemp, "Sensor_1") == 0) {
		id = CVI_BIN_ID_ISP1;
	} else if (strcmp(pszTemp, "Sensor_2") == 0) {
		id = CVI_BIN_ID_ISP2;
	} else if (strcmp(pszTemp, "Sensor_3") == 0) {
		id = CVI_BIN_ID_ISP3;
	} else if (strcmp(pszTemp, "Vpss") == 0) {
		id = CVI_BIN_ID_VPSS;
	} else if (strcmp(pszTemp, "Vdec") == 0) {
		id = CVI_BIN_ID_VDEC;
	} else if (strcmp(pszTemp, "Venc") == 0) {
		id = CVI_BIN_ID_VENC;
	} else if (strcmp(pszTemp, "Vo") == 0) {
		id = CVI_BIN_ID_VO;
	}

	return id;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_WriteSingleISPRegToBinBuffer(enum CVI_BIN_SECTION_ID id, CVI_U8 *pu8Buffer,
	CVI_U32 u32MaxBufSize, CVI_BIN_EXTRA_S *ptBinExtra, CVI_U32 *pu32DataLen)
{
	if (pu8Buffer == NULL) {
		return CVI_FAILURE;
	}

	*pu32DataLen = 0;

	CVI_BIN_HEADER	stBinHeader;
	CVI_U32			u32BinLen;
	CVI_S32			s32Ret;

	u32BinLen = CVI_BIN_GetSingleISPBinLen(id);
	if (u32BinLen > u32MaxBufSize) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Buffer for module bin is less than request");
		return CVI_FAILURE;
	}

	memset(&stBinHeader, 0, sizeof(CVI_BIN_HEADER));
	if (ptBinExtra) {
		memcpy(&stBinHeader.extraInfo, ptBinExtra, sizeof(CVI_BIN_EXTRA_S));
	}
	memcpy(pu8Buffer, &stBinHeader, sizeof(CVI_BIN_HEADER));

	s32Ret = CVI_BIN_ExportSingleISPBinData(id, pu8Buffer, u32BinLen);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_BIN_ExportSingleISPBinData fail");
	} else {
		*pu32DataLen = u32BinLen;
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ExportBinFile(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TBinaryData		*ptBinaryOutData = &(ptDevInfo->tBinaryOutData);

	CVI_BIN_EXTRA_S	stBinExtra;
	CVI_U32			u32BinLen, u32BinBufSize;
	CVI_S32         s32RetTmp = CVI_SUCCESS;
	enum CVI_BIN_SECTION_ID id = CVI_BIN_ID_MIN;

	memset(&stBinExtra, 0, sizeof(CVI_BIN_EXTRA_S));

	if (ptContentIn->pParams) {
		const char *pszTemp;
		CVI_S32 s32Ret;

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/author", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Author, 32, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/desp", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Desc, 1024, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/time", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Time, 32, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/version", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Desc + 724, 1024 - 724, "%s", pszTemp);
		} else {
			stBinExtra.Desc[724] = '\0';
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/bin_type", pszTemp, s32RetTmp);
		if (s32RetTmp == CVI_SUCCESS) {
			id = CVI_ISPD2_GetModuleIdFromString(pszTemp);
		}

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Author:\n%s\nDescription:\n%s\nTime:\n%s",
			stBinExtra.Author, stBinExtra.Desc, stBinExtra.Time);
	}

	CVI_ISPD2_ReleaseBinaryData(ptBinaryOutData);

	if (s32RetTmp == CVI_SUCCESS) {
		#ifdef ENABLE_ISP_IPC
		u32BinLen = CVI_BIN_GetBinTotalLenWrap();
		#else
		u32BinLen = CVI_BIN_GetSingleISPBinLen(id);
		#endif
		u32BinBufSize = MULTIPLE_4(u32BinLen);
		ptBinaryOutData->pu8Buffer = (CVI_U8 *)malloc(sizeof(CVI_U8) * u32BinBufSize);
		if (ptBinaryOutData->pu8Buffer == NULL) {
			if (u32BinBufSize == 0) {
				ISP_DAEMON2_DEBUG(LOG_DEBUG, "Allocate size of internal buffer is 0!");
				CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
					"Allocate size of internal buffer is 0!");
			} else {
				ISP_DAEMON2_DEBUG(LOG_DEBUG, "Allocate internal buffer for single bin fail");
				CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
					"Allocate internal buffer for single bin fail");
			}
			return CVI_FAILURE;
		}

		if (CVI_ISPD2_WriteSingleISPRegToBinBuffer(id, ptBinaryOutData->pu8Buffer,
			u32BinBufSize, &stBinExtra, &u32BinLen) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Dump module bin fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Dump module bin fail");

			CVI_ISPD2_ReleaseBinaryData(ptBinaryOutData);
			return CVI_FAILURE;
		}
	} else {
		#ifdef ENABLE_ISP_IPC
		u32BinLen = CVI_BIN_GetBinTotalLenWrap();
		#else
		u32BinLen = CVI_BIN_GetBinTotalLen();
		#endif
		u32BinBufSize = MULTIPLE_4(u32BinLen);

		ptBinaryOutData->pu8Buffer = (CVI_U8 *)malloc(sizeof(CVI_U8) * u32BinBufSize);
		if (ptBinaryOutData->pu8Buffer == NULL) {
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Allocate internal buffer for tuning bin fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Allocate internal buffer for tuning bin fail");

			return CVI_FAILURE;
		}

		if (CVI_ISPD2_WriteISPRegToBinBuffer(ptBinaryOutData->pu8Buffer, u32BinBufSize,
			&stBinExtra, &u32BinLen) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Dump tuning bin fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Dump tuning bin fail");

			CVI_ISPD2_ReleaseBinaryData(ptBinaryOutData);
			return CVI_FAILURE;
		}
	}

	ptBinaryOutData->eDataType = EBINARYDATA_TUNING_BIN_DATA;
	ptBinaryOutData->u32Size = u32BinLen;

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_STRING_TO_JSON("Content type", "Tuning bin", pDataOut);
	SET_INT_TO_JSON("Data size", ptBinaryOutData->u32Size, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);

	UNUSED(ptContentIn);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_CreateFolder(const char *pszFileName)
{
#define MAX_FOLDER_HIERARCHY		(16)
	char			subFolder[MAX_FOLDER_HIERARCHY][BIN_FILE_LENGTH] = {0};
	CVI_U32			subFolderCnt = 0;

	char			tmp[BIN_FILE_LENGTH];
	struct stat		status;
	char			*token;

	if (pszFileName == NULL) {
		return CVI_FAILURE;
	}

	// Get folder hierarchy cnt
	strcpy(tmp, pszFileName);
	// get the first token
	token = strtok(tmp, "/");
	while (token != NULL) {
		subFolderCnt++;
		token = strtok(NULL, "/");
	}

	// Create folder recursively
	strcpy(tmp, pszFileName);

	token = strtok(tmp, "/");
	for (CVI_U32 i = 0; i < (subFolderCnt - 1); ++i) {
		for (CVI_U32 j = i; j < (subFolderCnt - 1); ++j) {
			sprintf(&subFolder[j][strlen(subFolder[j])], "/%s", token);
		}
		token = strtok(NULL, "/");
	}

	for (CVI_U32 i = 0; i < (subFolderCnt - 1); ++i) {
		if (stat(subFolder[i], &status) == -1) {
			ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Folder path (%s) not exist", pszFileName);
			mkdir(subFolder[i], 0666);
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_WriteISPRegToBinFp(FILE *fp, CVI_BIN_EXTRA_S *ptBinExtra)
{
	if (fp == NULL) {
		return CVI_FAILURE;
	}

	CVI_U8		*pu8Buffer = NULL;

	CVI_U32		u32BinLen, u32BinBufSize;

	u32BinLen = CVI_BIN_GetBinTotalLen();
	u32BinBufSize = MULTIPLE_4(u32BinLen);

	pu8Buffer = (CVI_U8 *)malloc(sizeof(CVI_U8) * u32BinBufSize);
	if (pu8Buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Allocate internal temp buffer for tuning bin fail");
		return CVI_FAILURE;
	}

	if (CVI_ISPD2_WriteISPRegToBinBuffer(pu8Buffer, u32BinBufSize, ptBinExtra, &u32BinLen) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Dump tuning bin fail");
		SAFE_FREE(pu8Buffer);
		return CVI_FAILURE;
	}

	CVI_U32 u32TempLen = fwrite(pu8Buffer, 1, u32BinLen, fp);

	if (u32TempLen != u32BinLen) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Write to flash size mis-match");
		SAFE_FREE(pu8Buffer);
		return CVI_FAILURE;
	}

	SAFE_FREE(pu8Buffer);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_FixBinToFlash(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	FILE		*pfFile = NULL;
	char		szBinFileName[128] = {'\0'};
	CVI_S32		s32Ret = CVI_SUCCESS;

	CVI_BIN_EXTRA_S	stBinExtra;

	if (CVI_BIN_GetBinName(szBinFileName) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_BIN_GetBinName fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	CVI_ISPD2_CreateFolder(szBinFileName);

	pfFile = fopen(szBinFileName, "wb+");
	if (pfFile == NULL) {
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Open file (%s) fail", szBinFileName);
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	memset(&stBinExtra, 0, sizeof(CVI_BIN_EXTRA_S));

	if (ptContentIn->pParams) {
		const char *pszTemp;

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/author", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Author, 32, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/desp", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Desc, 1024, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/time", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Time, 32, "%s", pszTemp);
		}

		s32Ret = CVI_SUCCESS;
		GET_STRING_FROM_JSON(ptContentIn->pParams, "/version", pszTemp, s32Ret);
		if (s32Ret == CVI_SUCCESS) {
			snprintf((char *)stBinExtra.Desc + 724, 1024 - 724, "%s", pszTemp);
		} else {
			stBinExtra.Desc[724] = '\0';
		}

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Author:\n%s\nDescription:\n%s\nTime:\n%s",
			stBinExtra.Author, stBinExtra.Desc, stBinExtra.Time);
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	s32Ret = CVI_ISPD2_WriteISPRegToBinFp(pfFile, &stBinExtra);
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Write ISP Bin to flash fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
	}

	fclose(pfFile); pfFile = NULL;
	system("sync");
	UNUSED(pJsonResponse);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_ImportBinFile(TBinaryData *ptBinaryData,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	if ((ptBinaryData->eDataType != EBINARYDATA_TUNING_BIN_DATA)
		|| (ptBinaryData->pu8Buffer == NULL)
		|| (ptBinaryData->u32Size == 0)
	) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Invalid binary info.");
		CVI_ISPD2_Utils_ComposeAPIErrorMessage(ptContentOut);
		return CVI_FAILURE;
	}

	if (CVI_BIN_ImportBinData(ptBinaryData->pu8Buffer, ptBinaryData->u32Size) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG_EX(LOG_ALERT, "CVI_BIN_ImportBinData fail");
		CVI_ISPD2_Utils_ComposeAPIErrorMessageEX(ptContentOut, "CVI_BIN_ImportBinData fail");
		return CVI_FAILURE;
	}

	for (uint32_t VpssGrp = 0; VpssGrp < VPSS_MAX_GRP_NUM; VpssGrp++) {
		CVI_U8 u8SceneIndex = 0;

		CVI_VPSS_GetBinScene(VpssGrp, &u8SceneIndex);
		if (u8SceneIndex != 0xFF) {
			CVI_VPSS_SetGrpParamfromBin(VpssGrp, u8SceneIndex);
		}
	}

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
