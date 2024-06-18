/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_bin.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <linux/cvi_common.h>
#include <linux/cvi_defines.h>
#include "cvi_comm_isp.h"
#include "isp_bin.h"
#include "cvi_json_struct_comm.h"
#include "isp_json_struct.h"
#include "isp_3a_bin.h"
#include "cvi_isp.h"
#include "cvi_ae.h"

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

#define JSON_ISP_PARAM_NUM 5
#define JSON_3A_PARAM_NUM 2
#define JSON_TOTAL_NUM  (JSON_ISP_PARAM_NUM + JSON_3A_PARAM_NUM)

#define CVI_TRACE_ISP_BIN(level, fmt, ...) \
	CVI_TRACE(level, CVI_ID_ISP, "%s:%d:%s(): " fmt, __FILENAME__, __LINE__, __func__, ##__VA_ARGS__)

#define CVI_SET_PARAM_TO_JSON(func, key, param, case)	do {													\
		json_object = JSON_GetNewObject();																\
		if (json_object) {																				\
			func(W_FLAG, json_object, key, param, case);														\
			json_len[idx] = JSON_GetJsonStrLen(json_object);											\
			total_len += json_len[idx];																	\
			json_buf[idx] = (CVI_S8 *)malloc(json_len[idx]);											\
			if (json_buf[idx] == NULL) {																\
				ret = CVI_BIN_MALLOC_ERR;																\
				CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");							\
				JSON_ObjectPut(json_object);															\
				goto ERROR_HANDLER;																		\
			}																							\
			memcpy(json_buf[idx], JSON_GetJsonStrContent(json_object), json_len[idx]);					\
			JSON_ObjectPut(json_object);																\
			json_object = NULL;																			\
			idx++;																						\
		}else {																							\
			CVI_TRACE_ISP_BIN(LOG_WARNING, "(id:%d %s isp_raw_parameter )Get New Object fail.\n", id, key);\
			ret = CVI_BIN_JSONHANLE_ERROR;																\
		}																								\
	}while(0);

#define CVI_GET_PARAM_FROM_JSON(func, key, param, case)	do {								\
		json_object = JSON_TokenerParse(buffer + cur_pos);						\
		cur_pos += strlen(buffer + cur_pos) + 1;									\
		if (json_object) {																\
				func(R_FLAG, json_object, key, param, case);								\
				JSON_ObjectPut(json_object);												\
				json_object = NULL;															\
			} else {																		\
				printf( "(id:%d %s)Creat json tokener fail.\n", id, key);						\
				ret = CVI_BIN_JSONHANLE_ERROR;													\
				goto ERROR_HANDLER;																\
			}																				\
	}while(0);

#if 0
_Static_assert(
	(sizeof(ISP_Parameter_Structures) +
	sizeof(ISP_WDR_EXPOSURE_ATTR_S) +
	sizeof(ISP_EXPOSURE_ATTR_S) +
	sizeof(ISP_AE_ROUTE_S) +
	sizeof(ISP_AE_ROUTE_EX_S) +
	sizeof(ISP_AE_WIN_STATISTICS_CFG_S) +
	sizeof(ISP_WB_ATTR_S) +
	sizeof(ISP_AWB_ATTR_EX_S) +
	sizeof(ISP_AWB_Calibration_Gain_S)) == ISP_BIN_SIZE, "Pre-defined bin size incorrect !!!");
#endif

//CVI_BIN_EXTRA_S.Desc[624-1023] 400 bytes
//[pqbin md5 50] + [reserved 50]
//[tool version 40] + [isp commitId 10] + [sensorNum 5] + [sensorName1 55] + [sensorName2 55]
//[sensorName3 55]  + [sensorName4 55] + [isp branch 20] + [version 4] + [generate mode - A:auto M:Manual 1]
#define DESC_SIZE 624
#define BIN_MD5_SIZE 50
#define PQBIN_RESERVE_SIZE 50
#define TOOLVERSION_SIZE 40
#define BIN_COMMIT_SIZE 10
#define SENSORNUM_SIZE 5
#define SENSORNAME_SIZE 55
#define BIN_GERRIT_SIZE 20
#define PQBINVERSION_SIZE 4
#define PQBINVERSION "V1.1"
#define PQBINCREATE_MODE_SIZE 1
#define SUPPORT_VI_MAX_PIPE_NUM 4	//Force change to 4 Instead of using VI_MAX_PIPE_NUM (now it's 6).

typedef struct _SENSOR_INFO {
	CVI_U8 num;
	CVI_CHAR name[SUPPORT_VI_MAX_PIPE_NUM][SENSORNAME_SIZE];
} SENSOR_INFO;

static CVI_S32 isp_bin_getBinSizeImp(VI_PIPE ViPipe, CVI_U32 *binSize);
static CVI_S32 isp_bin_getBinParamImp(VI_PIPE ViPipe, CVI_U8 *addr, CVI_U32 binSize);
static CVI_S32 isp_bin_setBinParamImp(VI_PIPE ViPipe, FILE *fp);
static CVI_S32 isp_bin_setBinParamImptobuf(VI_PIPE ViPipe, unsigned char *buffer);
static CVI_S32 isp_bin_checkMd5(VI_PIPE ViPipe, CVI_U8 *addr, CVI_U32 binSize);
static CVI_S32 isp_bin_checkBinVersion(CVI_U8 *addr, CVI_U32 binSize);
static CVI_S32 isp_get_paramstruct(VI_PIPE ViPipe, ISP_Parameter_Structures *pstParaBuf);
static CVI_S32 isp_set_paramstruct(VI_PIPE ViPipe, ISP_Parameter_Structures *pstParaBuf);

CVI_S32 header_bin_getBinSize(CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	*binSize = sizeof(CVI_BIN_HEADER);

	return ret;
}

CVI_S32 header_bin_getBinParam(CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_checkBinVersion(addr, binSize);

	return ret;
}

CVI_S32 isp_bin_getSensorInfo(SENSOR_INFO *sensorInfo)
{
	#define NAME_SIZE 20
	CVI_U8 i = 0;
	CVI_U8 sensor_num = 0;

	for (i = 0; i < SUPPORT_VI_MAX_PIPE_NUM; ++i) {
		char sensorNameEnv[NAME_SIZE] = {0};

		memset(sensorNameEnv, 0, NAME_SIZE);
		snprintf(sensorNameEnv, NAME_SIZE, "SENSORNAME%d", i);
		CVI_CHAR *pszEnv = getenv(sensorNameEnv);

		if (pszEnv != NULL) {
			sensor_num++;
			strncpy((*sensorInfo).name[i], pszEnv, SENSORNAME_SIZE);
		}
	}
	(*sensorInfo).num = sensor_num;

	return CVI_SUCCESS;
}

static CVI_S32 bin_get_repo_info(CVI_CHAR *gerritid, CVI_CHAR *commitid, CVI_CHAR *md5)
{
	//show current isp branch & commit
#ifndef ISP_BIN_GERRIT
#	define ISP_BIN_GERRIT "NULL"
#endif
#ifndef ISP_BIN_COMMIT
#	define ISP_BIN_COMMIT "NULL"
#endif
#ifndef ISP_BIN_MD5
#	define ISP_BIN_MD5 "Not Define Md5"
#endif
	size_t len = 0;

	if (commitid == NULL || gerritid == NULL) {
		return CVI_FAILURE;
	}

	len = strlen(ISP_BIN_GERRIT) + 1;
	strncpy(gerritid, ISP_BIN_GERRIT, len);
	len = strlen(ISP_BIN_COMMIT) + 1;
	strncpy(commitid, ISP_BIN_COMMIT, len);
	len = strlen(ISP_BIN_MD5) + 1;
	strncpy(md5, ISP_BIN_MD5, len);

	return CVI_SUCCESS;
}

CVI_S32 header_bin_GetHeaderDescInfo(CVI_CHAR *pOutDesc, enum CVI_BIN_CREATMODE inCreatMode)
{
	//Supplement isp branch and isp commitId to pqbin
	CVI_S32 ret = CVI_SUCCESS;
	SENSOR_INFO sensorInfo;
	CVI_CHAR binCommitId[BIN_COMMIT_SIZE] = { 0 };
	CVI_CHAR binGerrit[BIN_GERRIT_SIZE] = { 0 };
	CVI_CHAR binMd5[BIN_MD5_SIZE] = { 0 };

	isp_bin_getSensorInfo(&sensorInfo);
	bin_get_repo_info(binGerrit, binCommitId, binMd5);

	pOutDesc += DESC_SIZE;
	strncpy(pOutDesc, binMd5, BIN_MD5_SIZE);
	pOutDesc += BIN_MD5_SIZE;
	pOutDesc += PQBIN_RESERVE_SIZE;
	pOutDesc += TOOLVERSION_SIZE;
	strncpy(pOutDesc, binCommitId, BIN_COMMIT_SIZE);
	pOutDesc += BIN_COMMIT_SIZE;
	if (inCreatMode != CVI_BIN_AUTO) {
		snprintf(pOutDesc, SENSORNUM_SIZE, "%d", sensorInfo.num);
	}
	pOutDesc += SENSORNUM_SIZE;
	if (inCreatMode != CVI_BIN_AUTO) {
		for (CVI_U8 i = 0; i < sensorInfo.num; ++i) {
			strncpy(pOutDesc + i * SENSORNAME_SIZE, sensorInfo.name[i], SENSORNAME_SIZE);
		}
	}
	pOutDesc += SENSORNAME_SIZE * SUPPORT_VI_MAX_PIPE_NUM;
	strncpy(pOutDesc, binGerrit, BIN_GERRIT_SIZE);
	pOutDesc += BIN_GERRIT_SIZE;
	strncpy(pOutDesc, PQBINVERSION, PQBINVERSION_SIZE);
	pOutDesc += PQBINVERSION_SIZE;
	if (inCreatMode == CVI_BIN_AUTO) {
		pOutDesc[0] = 'A';
	} else {
		pOutDesc[0] = 'M';
	}

	return ret;
}

CVI_S32 header_bin_setBinParambuf(unsigned char *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER *pstHeader = NULL;

	pstHeader = (CVI_BIN_HEADER *)buffer;
	CVI_CHAR *pDesc = (CVI_CHAR *)pstHeader->extraInfo.Desc;

	header_bin_GetHeaderDescInfo(pDesc, CVI_BIN_MANUAL);

	return ret;
}

CVI_S32 header_bin_setBinParam(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BIN_HEADER header;

	fseek(fp, -sizeof(CVI_BIN_HEADER), SEEK_CUR);
	fread(&header, sizeof(CVI_BIN_HEADER), 1, fp);
	fseek(fp, -sizeof(CVI_BIN_HEADER), SEEK_CUR);
	CVI_CHAR *pDesc = (CVI_CHAR *)header.extraInfo.Desc;

	header_bin_GetHeaderDescInfo(pDesc, CVI_BIN_MANUAL);

	fwrite(&header, sizeof(CVI_BIN_HEADER), 1, fp);

	return ret;
}

CVI_S32 isp_bin_getBinSize_p0(CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinSizeImp(0, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinSize_p1(CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinSizeImp(1, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinSize_p2(CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinSizeImp(2, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinSize_p3(CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinSizeImp(3, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinParam_p0(CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinParamImp(0, addr, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinParam_p1(CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinParamImp(1, addr, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinParam_p2(CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinParamImp(2, addr, binSize);

	return ret;
}

CVI_S32 isp_bin_getBinParam_p3(CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_getBinParamImp(3, addr, binSize);

	return ret;
}

CVI_S32 isp_bin_setBinParambuf_p0(CVI_U8 *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImptobuf(0, buffer);

	return ret;
}

CVI_S32 isp_bin_setBinParambuf_p1(CVI_U8 *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImptobuf(1, buffer);

	return ret;
}

CVI_S32 isp_bin_setBinParambuf_p2(CVI_U8 *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImptobuf(2, buffer);

	return ret;
}

CVI_S32 isp_bin_setBinParambuf_p3(CVI_U8 *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImptobuf(3, buffer);

	return ret;
}

CVI_S32 isp_bin_setBinParam_p0(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImp(0, fp);

	return ret;
}

CVI_S32 isp_bin_setBinParam_p1(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImp(1, fp);

	return ret;
}

CVI_S32 isp_bin_setBinParam_p2(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImp(2, fp);

	return ret;
}

CVI_S32 isp_bin_setBinParam_p3(FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_bin_setBinParamImp(3, fp);

	return ret;
}

static CVI_S32 isp_bin_getBinSizeImp(VI_PIPE ViPipe, CVI_U32 *binSize)
{
	CVI_S32 ret = CVI_SUCCESS;

	CVI_U32 total_size = 0, size = 0;

	total_size += sizeof(ISP_Parameter_Structures);
	isp_3aBinAttr_get_size(ViPipe, &size);
	total_size += size;
	*binSize = total_size;

	return ret;
}

static CVI_S32 isp_bin_getBinParamImp(VI_PIPE ViPipe, CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U8 *buf_ofs = addr;
	CVI_BIN_HEADER *pstHeader = (CVI_BIN_HEADER *)buf_ofs;

	if (isp_bin_checkMd5(ViPipe, addr, binSize) != CVI_SUCCESS) {
		CVI_TRACE_ISP_BIN(LOG_WARNING, "section[CVI_BIN_ID_ISP%d] Md5 not matched\n", ViPipe);
		return CVI_FAILURE;
	}
	for (CVI_S32 idx = CVI_BIN_ID_MIN; idx < CVI_BIN_ID_ISP0 + ViPipe; idx++) {
		buf_ofs += pstHeader->size[idx];
	}

	isp_set_paramstruct(ViPipe, (ISP_Parameter_Structures *)buf_ofs);
	buf_ofs += sizeof(ISP_Parameter_Structures);
	isp_3aBinAttr_set_param(ViPipe, &buf_ofs);

	CVI_TRACE_ISP_BIN(LOG_DEBUG, "apply bin setting to %d\n", ViPipe);

	return ret;
}

static CVI_S32 isp_bin_setBinParamImp(VI_PIPE ViPipe, FILE *fp)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_Parameter_Structures *pstParam = NULL;

	pstParam = (ISP_Parameter_Structures *)malloc(sizeof(ISP_Parameter_Structures));
	if(pstParam == NULL) {
		CVI_TRACE_ISP_BIN(LOG_ERR, "%s\n", "Allocate memory fail");
		return CVI_BIN_MALLOC_ERR;
	}

	isp_get_paramstruct(ViPipe, pstParam);
	fwrite(pstParam, sizeof(ISP_Parameter_Structures), 1, fp);

	isp_3aBinAttr_get_param(ViPipe, fp);
	free(pstParam);
	return ret;
}

static CVI_S32 isp_bin_setBinParamImptobuf(VI_PIPE ViPipe, unsigned char *buffer)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_Parameter_Structures *pstParam = NULL;

	pstParam = (ISP_Parameter_Structures *)malloc(sizeof(ISP_Parameter_Structures));
	if(pstParam == NULL) {
		CVI_TRACE_ISP_BIN(LOG_ERR, "%s\n", "Allocate memory fail");
		return CVI_BIN_MALLOC_ERR;
	}
	isp_get_paramstruct(ViPipe, pstParam);
	memcpy(buffer, pstParam, sizeof(ISP_Parameter_Structures));
	buffer += sizeof(ISP_Parameter_Structures);

	isp_3aBinAttr_get_parambuf(ViPipe, buffer);
	free(pstParam);
	return ret;
}

static CVI_S32 isp_bin_checkMd5(VI_PIPE ViPipe, CVI_U8 *addr, CVI_U32 binSize)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_U32 size = 0;
	CVI_CHAR binCommitId[BIN_COMMIT_SIZE];
	CVI_CHAR binGerrit[BIN_GERRIT_SIZE];
	CVI_CHAR curMd5[BIN_MD5_SIZE] = { 0 };
	CVI_CHAR binMd5[BIN_MD5_SIZE] = { 0 };
	CVI_BIN_HEADER *header = (CVI_BIN_HEADER *)addr;
	CVI_BIN_EXTRA_S *pExtraInfo = &(header->extraInfo);

	strncpy(binMd5, (CVI_CHAR *)(pExtraInfo->Desc + DESC_SIZE), BIN_MD5_SIZE);
	bin_get_repo_info(binGerrit, binCommitId, curMd5);

	if (strcmp(binMd5, curMd5)) {
		CVI_TRACE_ISP_BIN(LOG_ERR, "%s%s%s",
			"md5 mismatch means that you will read json in PQBIN, ",
			"but the disadvantage is that the boot speed is slightly reduced. ",
			"If you want to read bin in PQBIN, please make pqbin again.\n");
		CVI_TRACE_ISP_BIN(LOG_ERR, "please update to version (branch:%s commitId:%s)\n",
			binGerrit, binCommitId);
		ret = CVI_FAILURE;
	} else {
		isp_bin_getBinSizeImp(ViPipe, &size);
		if (size != binSize) {
			CVI_TRACE_ISP_BIN(LOG_ERR, "vi(%d), Bin Size not matched(%u vs %u),", ViPipe, size, binSize);
			CVI_TRACE_ISP_BIN(LOG_ERR, "%s%s",
				"Size mismatch means that someone added the pqbin structure ",
				"but it was not retrieved by the MD5 script. this must be fixed immediately!\n");
			CVI_TRACE_ISP_BIN(LOG_ERR, "please update to version (branch:%s commitId:%s)\n",
				binGerrit, binCommitId);
			ret = CVI_FAILURE;
		}
	}

	return ret;
}

static CVI_S32 isp_bin_checkBinVersion(CVI_U8 *addr, CVI_U32 binSize)
{
	printf("********************************************************************************\n");
	//get CVI_BIN_EXTRA_S
	CVI_BIN_HEADER *header = (CVI_BIN_HEADER *)addr;
	CVI_BIN_EXTRA_S *pExtraInfo = &(header->extraInfo);

	//get sensor info
	SENSOR_INFO sensorInfo;
	//show pqbin isp branch & commit & sensor
	CVI_CHAR toolVersion[TOOLVERSION_SIZE];
	CVI_CHAR sensorNum[SENSORNUM_SIZE];
	CVI_CHAR sensorName[SUPPORT_VI_MAX_PIPE_NUM][SENSORNAME_SIZE];
	CVI_CHAR binCommitId[BIN_COMMIT_SIZE] = {0};
	CVI_CHAR binGerrit[BIN_GERRIT_SIZE] = {0};
	CVI_CHAR binMd5[BIN_MD5_SIZE] = { 0 };
	CVI_CHAR pqbinVersion[PQBINVERSION_SIZE + 1] = { 0 };
	CVI_CHAR pqbinMode[PQBINCREATE_MODE_SIZE + 1] = { 0 };
	CVI_CHAR *pDesc = (CVI_CHAR *)pExtraInfo->Desc;

	memset(toolVersion, 0, TOOLVERSION_SIZE);
	memset(sensorNum, 0, SENSORNUM_SIZE);
	memset(sensorName, 0, SENSORNAME_SIZE * SUPPORT_VI_MAX_PIPE_NUM);
	memset(binCommitId, 0, BIN_COMMIT_SIZE);
	memset(binGerrit, 0, BIN_GERRIT_SIZE);

	//show curSw isp_bin branch & commit
	printf("cvi_bin_isp message\n");

	bin_get_repo_info(binGerrit, binCommitId, binMd5);
	isp_bin_getSensorInfo(&sensorInfo);

	printf("%-15s%-15s%-15s%-15s\n", "gerritId:", binGerrit, "commitId:", binCommitId);
	printf("%-15s%-15s\n", "md5:", binMd5);
	printf("%-15s%-15d\n", "sensorNum", sensorInfo.num);
	for (CVI_U8 i = 0; i < sensorInfo.num; ++i) {
		printf("%s%-5d%-15s\n", "sensorName", i, sensorInfo.name[i]);
	}
	printf("\n");

	//show pqbin isp branch & commit & sensor
	pDesc += DESC_SIZE;
	strncpy(binMd5, pDesc, BIN_MD5_SIZE);
	pDesc += BIN_MD5_SIZE;
	pDesc += PQBIN_RESERVE_SIZE;
	strncpy(toolVersion, pDesc, TOOLVERSION_SIZE);
	pDesc += TOOLVERSION_SIZE;
	strncpy(binCommitId, pDesc, BIN_COMMIT_SIZE);
	pDesc += BIN_COMMIT_SIZE;
	strncpy(sensorNum, pDesc, SENSORNUM_SIZE);
	pDesc += SENSORNUM_SIZE;
	for (CVI_U8 i = 0; i < atoi(sensorNum); ++i) {
		strncpy(sensorName[i], pDesc + i * SENSORNAME_SIZE, SENSORNAME_SIZE);
	}
	pDesc += SENSORNAME_SIZE * SUPPORT_VI_MAX_PIPE_NUM;
	strncpy(binGerrit, pDesc, BIN_GERRIT_SIZE);
	pDesc += BIN_GERRIT_SIZE;
	strncpy(pqbinVersion, pDesc, PQBINVERSION_SIZE);
	pDesc += PQBINVERSION_SIZE;
	strncpy(pqbinMode, pDesc, PQBINCREATE_MODE_SIZE);

	printf("PQBIN message\n");
	printf("%-15s%-15s%-15s%-15s\n", "gerritId:", binGerrit, "commitId:", binCommitId);
	printf("%-15s%-15s\n", "md5:", binMd5);
	printf("%-15s%-15s\n", "sensorNum", sensorNum);
	for (CVI_U8 i = 0; i < sensorInfo.num; ++i) {
		printf("%s%-5d%-15s\n", "sensorName", i, sensorName[i]);
	}
	printf("\n");

	//show pqbin pqtool branch & version & Mode
	printf("%-15s%-15s%-15s%-15s\n", "author:", pExtraInfo->Author, "desc:", pExtraInfo->Desc);
	printf("%-15s%-15s%-15s%-15s\n%-20s%-20s%-6s%-5s\n",
		"createTime:", pExtraInfo->Time, "version:", pqbinVersion,
		"tool Version:", toolVersion, "mode:", pqbinMode);
	printf("********************************************************************************\n");

	// check sensor
	for (CVI_U8 i = 0; i < sensorInfo.num; ++i) {
		if (strcmp(sensorInfo.name[i], sensorName[i]) != 0) {
			printf("sensorName(%d) mismatch, mwSns:%s != pqBinSns:%s\n",
				i, sensorInfo.name[i], sensorName[i]);
			return CVI_SUCCESS; //not check now
		}
	}

	//check md5
	if (strcmp(binMd5, ISP_BIN_MD5)) {
		printf("pqbin md5 mismatch, mwMd5:%s != pqBinMd5:%s", ISP_BIN_MD5, binMd5);
		return CVI_SUCCESS;
	}

	UNUSED(binSize);

	return CVI_SUCCESS;
}

static CVI_S32 isp_set_paramstruct(VI_PIPE ViPipe, ISP_Parameter_Structures *pstParaBuf)
{
	if (((ViPipe) < 0) || ((ViPipe) >= SUPPORT_VI_MAX_PIPE_NUM)) {
		CVI_TRACE_ISP_BIN(LOG_ERR, "ViPipe %d value error\n", ViPipe);
		return CVI_FAILURE;
	}

	if (pstParaBuf == CVI_NULL) {
		return CVI_FAILURE;
	}

	//Pub attr
	ISP_PUB_ATTR_S stPubAttr;

	CVI_ISP_GetPubAttr(ViPipe, &stPubAttr);
	stPubAttr.f32FrameRate = pstParaBuf->pub_attr.f32FrameRate;
	CVI_ISP_SetPubAttr(ViPipe, &stPubAttr);

	// PRE_RAW
	CVI_ISP_SetBlackLevelAttr(ViPipe, &pstParaBuf->blc);
	CVI_ISP_SetDPDynamicAttr(ViPipe, &pstParaBuf->dpc_dynamic);
	CVI_ISP_SetDPStaticAttr(ViPipe, &pstParaBuf->dpc_static);
	CVI_ISP_SetDPCalibrate(ViPipe, &pstParaBuf->DPCalib);
	CVI_ISP_SetCrosstalkAttr(ViPipe, &pstParaBuf->crosstalk);

	// Raw-top
	CVI_ISP_SetNRAttr(ViPipe, &pstParaBuf->bnr);
	CVI_ISP_SetNRFilterAttr(ViPipe, &pstParaBuf->bnr_filter);
	CVI_ISP_SetDemosaicAttr(ViPipe, &pstParaBuf->demosaic);
	CVI_ISP_SetDemosaicDemoireAttr(ViPipe, &pstParaBuf->demosaic_demoire);
	CVI_ISP_SetRGBCACAttr(ViPipe, &pstParaBuf->rgbcac);
	CVI_ISP_SetLCACAttr(ViPipe, &pstParaBuf->lcac);
	CVI_ISP_SetDisAttr(ViPipe, &pstParaBuf->disAttr);
	CVI_ISP_SetDisConfig(ViPipe, &pstParaBuf->disConfig);

	// RGB-top
	CVI_ISP_SetMeshShadingAttr(ViPipe, &pstParaBuf->mlsc);
	CVI_ISP_SetMeshShadingGainLutAttr(ViPipe, &pstParaBuf->mlscLUT);
	CVI_ISP_SetSaturationAttr(ViPipe, &pstParaBuf->Saturation);
	CVI_ISP_SetCCMAttr(ViPipe, &pstParaBuf->ccm);
	CVI_ISP_SetCCMSaturationAttr(ViPipe, &pstParaBuf->ccm_saturation);
	CVI_ISP_SetColorToneAttr(ViPipe, &pstParaBuf->colortone);
	CVI_ISP_SetFSWDRAttr(ViPipe, &pstParaBuf->fswdr);
	CVI_ISP_SetWDRExposureAttr(ViPipe, &pstParaBuf->WDRExposure);
	CVI_ISP_SetDRCAttr(ViPipe, &pstParaBuf->drc);
	CVI_ISP_SetGammaAttr(ViPipe, &pstParaBuf->gamma);
	CVI_ISP_SetAutoGammaAttr(ViPipe, &pstParaBuf->autoGamma);
	CVI_ISP_SetDehazeAttr(ViPipe, &pstParaBuf->dehaze);
	CVI_ISP_SetClutAttr(ViPipe, &pstParaBuf->clut);
	CVI_ISP_SetClutSaturationAttr(ViPipe, &pstParaBuf->clut_saturation);
	CVI_ISP_SetCSCAttr(ViPipe, &pstParaBuf->csc);
	CVI_ISP_SetVCAttr(ViPipe, &pstParaBuf->vc_motion);

	// YUV-top
	CVI_ISP_SetDCIAttr(ViPipe, &pstParaBuf->dci);
	CVI_ISP_SetLDCIAttr(ViPipe, &pstParaBuf->ldci);
	CVI_ISP_SetPreSharpenAttr(ViPipe, &pstParaBuf->presharpen);
	CVI_ISP_SetTNRAttr(ViPipe, &pstParaBuf->tnr);
	CVI_ISP_SetTNRNoiseModelAttr(ViPipe, &pstParaBuf->tnr_noise_model);
	CVI_ISP_SetTNRLumaMotionAttr(ViPipe, &pstParaBuf->tnr_luma_motion);
	CVI_ISP_SetTNRGhostAttr(ViPipe, &pstParaBuf->tnr_ghost);
	CVI_ISP_SetTNRMtPrtAttr(ViPipe, &pstParaBuf->tnr_mt_prt);
	CVI_ISP_SetTNRMotionAdaptAttr(ViPipe, &pstParaBuf->tnr_motion_adapt);
	CVI_ISP_SetYNRAttr(ViPipe, &pstParaBuf->ynr);
	CVI_ISP_SetYNRFilterAttr(ViPipe, &pstParaBuf->ynr_filter);
	CVI_ISP_SetYNRMotionNRAttr(ViPipe, &pstParaBuf->ynr_motion);
	CVI_ISP_SetCNRAttr(ViPipe, &pstParaBuf->cnr);
	CVI_ISP_SetCNRMotionNRAttr(ViPipe, &pstParaBuf->cnr_motion);
	CVI_ISP_SetCACAttr(ViPipe, &pstParaBuf->cac);
	CVI_ISP_SetSharpenAttr(ViPipe, &pstParaBuf->sharpen);
	CVI_ISP_SetCAAttr(ViPipe, &pstParaBuf->ca);
	CVI_ISP_SetCA2Attr(ViPipe, &pstParaBuf->ca2);
	CVI_ISP_SetYContrastAttr(ViPipe, &pstParaBuf->ycontrast);

	// Other
	CVI_ISP_SetNoiseProfileAttr(ViPipe, &pstParaBuf->np);
	CVI_ISP_SetMonoAttr(ViPipe, &pstParaBuf->mono);

	return CVI_SUCCESS;
}

static CVI_S32 isp_get_paramstruct(VI_PIPE ViPipe, ISP_Parameter_Structures *pstParaBuf)
{
	if (((ViPipe) < 0) || ((ViPipe) >= SUPPORT_VI_MAX_PIPE_NUM)) {
		CVI_TRACE_ISP_BIN(LOG_ERR, "ViPipe %d value error\n", ViPipe);
		return CVI_FAILURE;
	}

	if (pstParaBuf == CVI_NULL) {
		return CVI_FAILURE;
	}

	//Pub attr
	CVI_ISP_GetPubAttr(ViPipe, &pstParaBuf->pub_attr);

	// PRE_RAW
	CVI_ISP_GetBlackLevelAttr(ViPipe, &pstParaBuf->blc);
	CVI_ISP_GetDPDynamicAttr(ViPipe, &pstParaBuf->dpc_dynamic);
	CVI_ISP_GetDPStaticAttr(ViPipe, &pstParaBuf->dpc_static);
	CVI_ISP_GetDPCalibrate(ViPipe, &pstParaBuf->DPCalib);
	CVI_ISP_GetCrosstalkAttr(ViPipe, &pstParaBuf->crosstalk);

	// Raw-top
	CVI_ISP_GetNRAttr(ViPipe, &pstParaBuf->bnr);
	CVI_ISP_GetNRFilterAttr(ViPipe, &pstParaBuf->bnr_filter);
	CVI_ISP_GetDemosaicAttr(ViPipe, &pstParaBuf->demosaic);
	CVI_ISP_GetDemosaicDemoireAttr(ViPipe, &pstParaBuf->demosaic_demoire);
	CVI_ISP_GetRGBCACAttr(ViPipe, &pstParaBuf->rgbcac);
	CVI_ISP_GetLCACAttr(ViPipe, &pstParaBuf->lcac);
	CVI_ISP_GetDisAttr(ViPipe, &pstParaBuf->disAttr);
	CVI_ISP_GetDisConfig(ViPipe, &pstParaBuf->disConfig);

	// RGB-top
	CVI_ISP_GetMeshShadingAttr(ViPipe, &pstParaBuf->mlsc);
	CVI_ISP_GetMeshShadingGainLutAttr(ViPipe, &pstParaBuf->mlscLUT);
	CVI_ISP_GetSaturationAttr(ViPipe, &pstParaBuf->Saturation);
	CVI_ISP_GetCCMAttr(ViPipe, &pstParaBuf->ccm);
	CVI_ISP_GetCCMSaturationAttr(ViPipe, &pstParaBuf->ccm_saturation);
	CVI_ISP_GetColorToneAttr(ViPipe, &pstParaBuf->colortone);
	CVI_ISP_GetFSWDRAttr(ViPipe, &pstParaBuf->fswdr);
	CVI_ISP_GetWDRExposureAttr(ViPipe, &pstParaBuf->WDRExposure);
	CVI_ISP_GetDRCAttr(ViPipe, &pstParaBuf->drc);
	CVI_ISP_GetGammaAttr(ViPipe, &pstParaBuf->gamma);
	CVI_ISP_GetAutoGammaAttr(ViPipe, &pstParaBuf->autoGamma);
	CVI_ISP_GetDehazeAttr(ViPipe, &pstParaBuf->dehaze);
	CVI_ISP_GetClutAttr(ViPipe, &pstParaBuf->clut);
	CVI_ISP_GetClutSaturationAttr(ViPipe, &pstParaBuf->clut_saturation);
	CVI_ISP_GetCSCAttr(ViPipe, &pstParaBuf->csc);
	CVI_ISP_GetVCAttr(ViPipe, &pstParaBuf->vc_motion);

	// YUV-top
	CVI_ISP_GetDCIAttr(ViPipe, &pstParaBuf->dci);
	CVI_ISP_GetLDCIAttr(ViPipe, &pstParaBuf->ldci);
	CVI_ISP_GetPreSharpenAttr(ViPipe, &pstParaBuf->presharpen);
	CVI_ISP_GetTNRAttr(ViPipe, &pstParaBuf->tnr);
	CVI_ISP_GetTNRNoiseModelAttr(ViPipe, &pstParaBuf->tnr_noise_model);
	CVI_ISP_GetTNRLumaMotionAttr(ViPipe, &pstParaBuf->tnr_luma_motion);
	CVI_ISP_GetTNRGhostAttr(ViPipe, &pstParaBuf->tnr_ghost);
	CVI_ISP_GetTNRMtPrtAttr(ViPipe, &pstParaBuf->tnr_mt_prt);
	CVI_ISP_GetTNRMotionAdaptAttr(ViPipe, &pstParaBuf->tnr_motion_adapt);
	CVI_ISP_GetYNRAttr(ViPipe, &pstParaBuf->ynr);
	CVI_ISP_GetYNRFilterAttr(ViPipe, &pstParaBuf->ynr_filter);
	CVI_ISP_GetYNRMotionNRAttr(ViPipe, &pstParaBuf->ynr_motion);
	CVI_ISP_GetCNRAttr(ViPipe, &pstParaBuf->cnr);
	CVI_ISP_GetCNRMotionNRAttr(ViPipe, &pstParaBuf->cnr_motion);
	CVI_ISP_GetCACAttr(ViPipe, &pstParaBuf->cac);
	CVI_ISP_GetSharpenAttr(ViPipe, &pstParaBuf->sharpen);
	CVI_ISP_GetCAAttr(ViPipe, &pstParaBuf->ca);
	CVI_ISP_GetCA2Attr(ViPipe, &pstParaBuf->ca2);
	CVI_ISP_GetYContrastAttr(ViPipe, &pstParaBuf->ycontrast);

	// other
	CVI_ISP_GetNoiseProfileAttr(ViPipe, &pstParaBuf->np);
	CVI_ISP_GetMonoAttr(ViPipe, &pstParaBuf->mono);

	return CVI_SUCCESS;
}


/**************************************************************************
 *   Json related APIs.
 **************************************************************************/
CVI_S32 isp_json_getParamFromJsonbuffer(const char *buffer, enum CVI_BIN_SECTION_ID id)
{
	JSON *json_object;
	CVI_S32 ret = CVI_SUCCESS;
	ISP_Parameter_Structures *pisp_parameter = NULL;
	ISP_3A_Parameter_Structures *pisp_3a_parameter = NULL;
	VI_PIPE ViPipe = id - CVI_BIN_ID_ISP0;
	CVI_S32 cur_pos = 0, i;

	pisp_parameter = (ISP_Parameter_Structures *)malloc(sizeof(ISP_Parameter_Structures));
	if (pisp_parameter == NULL) {
		CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");
		return CVI_BIN_MALLOC_ERR;
	}
	pisp_3a_parameter = (ISP_3A_Parameter_Structures *)malloc(sizeof(ISP_3A_Parameter_Structures));
	if (pisp_3a_parameter == NULL) {
		CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");
		free(pisp_parameter);
		return CVI_BIN_MALLOC_ERR;
	}
	isp_get_paramstruct(ViPipe, pisp_parameter);
	if (pisp_parameter->disConfig.motionLevel == 0) {
		pisp_parameter->disConfig.motionLevel = DIS_MOTION_LEVEL_NORMAL;
	}

	isp_3aJsonAttr_get_param(ViPipe, pisp_3a_parameter);
	/*read new para and set para.*/

	for(i = 0;i < JSON_ISP_PARAM_NUM; i++) {
		CVI_GET_PARAM_FROM_JSON(ISP_PARAMETER_BUFFER_JSON, "isp_parameter", pisp_parameter, i+1);
	}
	for(i = 0; i < JSON_3A_PARAM_NUM; i++) {
		CVI_GET_PARAM_FROM_JSON(ISP_3A_PARAMETER_BUFFER_JSON, "isp_3a_parameter", pisp_3a_parameter,i+1);
	}

	isp_set_paramstruct(ViPipe, pisp_parameter);
	isp_3aJsonAttr_set_param(ViPipe, pisp_3a_parameter);

ERROR_HANDLER:
	free(pisp_parameter);
	free(pisp_3a_parameter);
	return ret;
}

CVI_S32 isp_json_setParamToJsonbuffer(CVI_S8 **buffer, enum CVI_BIN_SECTION_ID id, CVI_S32 *len)
{
	JSON *json_object;
	ISP_Parameter_Structures *pisp_parameter = NULL;
	ISP_3A_Parameter_Structures *pisp_3a_parameter = NULL;
	VI_PIPE ViPipe = id - CVI_BIN_ID_ISP0;
	CVI_S32 ret = CVI_SUCCESS, idx =0, total_len = 0, i;
	CVI_S32 json_len[JSON_TOTAL_NUM] = {0};
	CVI_S8 *json_buf[JSON_TOTAL_NUM] = {NULL};

	pisp_parameter = (ISP_Parameter_Structures *)malloc(sizeof(ISP_Parameter_Structures));
	if (pisp_parameter == NULL) {
		CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");
		return CVI_BIN_MALLOC_ERR;
	}
	pisp_3a_parameter = (ISP_3A_Parameter_Structures *)malloc(sizeof(ISP_3A_Parameter_Structures));
	if (pisp_3a_parameter == NULL) {
		CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");
		free(pisp_parameter);
		return CVI_BIN_MALLOC_ERR;
	}
	isp_get_paramstruct(ViPipe, pisp_parameter);
	if (pisp_parameter->disConfig.motionLevel == 0) {
		pisp_parameter->disConfig.motionLevel = DIS_MOTION_LEVEL_NORMAL;
	}

	isp_3aJsonAttr_get_param(ViPipe, pisp_3a_parameter);

	for(i = 0;i < JSON_ISP_PARAM_NUM; i++) {
		CVI_SET_PARAM_TO_JSON(ISP_PARAMETER_BUFFER_JSON, "isp_parameter", pisp_parameter, i+1);
	}
	for(i = 0; i < JSON_3A_PARAM_NUM; i++) {
		CVI_SET_PARAM_TO_JSON(ISP_3A_PARAMETER_BUFFER_JSON, "isp_3a_parameter", pisp_3a_parameter,i+1);
	}

	*buffer = (CVI_S8 *)malloc(total_len);
	if (*buffer == NULL) {
		ret = CVI_BIN_MALLOC_ERR;
		CVI_TRACE_ISP_BIN(LOG_WARNING, "%s\n", "Allocate memory fail");
		goto ERROR_HANDLER;
	}else {
		for(i = 0; i < idx; i++){
			memcpy(*buffer + *len, json_buf[i], json_len[i]);
			*len += json_len[i];
		}
	}

ERROR_HANDLER:
	for(i = 0; i < idx; i++) {
		free(json_buf[i]);
	}

	free(pisp_parameter);
	free(pisp_3a_parameter);
	return ret;
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif