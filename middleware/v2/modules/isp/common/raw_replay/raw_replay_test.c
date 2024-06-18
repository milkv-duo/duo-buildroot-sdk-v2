/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: raw_replay_test.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "cvi_sys.h"
#include "cvi_vi.h"
#include "cvi_isp.h"

#include "raw_replay.h"
#include "cvi_comm_isp.h"

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation" /* Or  "-Wformat-overflow"  */
#endif

#define _BUFF_LEN  128
#define LOGOUT(fmt, arg...) printf("[RAW REPLAY AUTO TEST] %s,%d: " fmt, __func__, __LINE__, ##arg)

#define ERROR_IF(EXP)                  \
	do {                               \
		if (EXP) {                     \
			LOGOUT("error: "#EXP"\n"); \
		}                              \
	} while (0)

typedef struct {
	CVI_U8 *pHeader;
	CVI_U8 *pData;
	int intRawSize;
} AUTO_TEST_CTX_S;

static AUTO_TEST_CTX_S *pAutoTestCtx;

CVI_S32 rawReplayAutoTestInit(const char *pathPrefix)
{
	CVI_S32 ret = CVI_SUCCESS;

	char path[_BUFF_LEN] = {0};
	char cmd[_BUFF_LEN] = {0};

	FILE *fp = NULL;

	snprintf(path, _BUFF_LEN, "%s/sensor_cfg.ini", pathPrefix);

	snprintf(cmd, _BUFF_LEN, "cp %s /mnt/data/sensor_cfg.ini", path);

	system(cmd);

	do {
		struct stat statbuf;

		stat(path, &statbuf);

		pAutoTestCtx->pHeader = (CVI_U8 *) calloc(1, sizeof(RAW_REPLAY_INFO));
		if (pAutoTestCtx->pHeader == NULL) {
			LOGOUT("calloc failed!!!\n");
			ret = CVI_FAILURE;
			break;
		}

		snprintf(path, _BUFF_LEN, "%s/header.bin", pathPrefix);

		fp = fopen(path, "rb");
		if (fp == NULL) {
			LOGOUT("fopen %s failed!!!\n", path);
			ret = CVI_FAILURE;
			break;
		}

		fread(pAutoTestCtx->pHeader, statbuf.st_size, 1, fp);
		fclose(fp);

		snprintf(path, _BUFF_LEN, "%s/test.raw", pathPrefix);

		stat(path, &statbuf);

		pAutoTestCtx->intRawSize = statbuf.st_size;

		pAutoTestCtx->pData = (CVI_U8 *) calloc(1, pAutoTestCtx->intRawSize);
		if (pAutoTestCtx->pData == NULL) {
			LOGOUT("calloc failed!!!\n");
			ret = CVI_FAILURE;
			break;
		}

		fp = fopen(path, "rb");
		if (fp == NULL) {
			LOGOUT("fopen %s failed!!!\n", path);
			ret = CVI_FAILURE;
			break;
		}

		fread(pAutoTestCtx->pData, pAutoTestCtx->intRawSize, 1, fp);
		fclose(fp);

	} while (0);

	return ret;
}

void rawReplayAutoTestSetConfig(void)
{
	CVI_S32 ret = CVI_SUCCESS;
	ISP_DRC_ATTR_S stDRCAttr;
	ISP_TNR_ATTR_S stTNRAttr;

#ifdef ARCH_CV182X
	ISP_FSWDR_ATTR_S stFSWDRAttr;

	ret = CVI_ISP_GetFSWDRAttr(0, &stFSWDRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
	stFSWDRAttr.WDRDitherEnable = 0;
	ret = CVI_ISP_SetFSWDRAttr(0, &stFSWDRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
#endif

	ret = CVI_ISP_GetDRCAttr(0, &stDRCAttr);
	ERROR_IF(ret != CVI_SUCCESS);
	stDRCAttr.ToneCurveSmooth = 0;
	ret = CVI_ISP_SetDRCAttr(0, &stDRCAttr);
	ERROR_IF(ret != CVI_SUCCESS);

	ret = CVI_ISP_GetTNRAttr(0, &stTNRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
	stTNRAttr.Enable = 0;
	ret = CVI_ISP_SetTNRAttr(0, &stTNRAttr);
	ERROR_IF(ret != CVI_SUCCESS);
}

CVI_S32 rawReplayAutoTestSaveYuv(const char *pathPrefix)
{

	CVI_S32 ret = CVI_SUCCESS;

	char path[_BUFF_LEN] = {0};

	FILE *fp = NULL;

	snprintf(path, _BUFF_LEN, "%s/output.yuv", pathPrefix);

	fp = fopen(path, "wb");
	if (fp == NULL) {
		LOGOUT("fopen %s failed!!!\n", path);
		return CVI_FAILURE;
	}

	VIDEO_FRAME_INFO_S stVideoFrame;
	VI_CROP_INFO_S stVICrop;

	memset(&stVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
	memset(&stVICrop, 0, sizeof(VI_CROP_INFO_S));

	ret = CVI_VI_GetChnFrame(0, 0, &stVideoFrame, -1);
	ERROR_IF(ret != CVI_SUCCESS);

	ret = CVI_VI_GetChnCrop(0, 0, &stVICrop);
	ERROR_IF(ret != CVI_SUCCESS);

	CVI_U32 plane_offset, au32PlanarSize[3];
	CVI_U32 u32PlanarCount = 0;

	switch (stVideoFrame.stVFrame.enPixelFormat) {
	case PIXEL_FORMAT_YUV_PLANAR_420:
		u32PlanarCount = 3;
		if ((ret == CVI_SUCCESS) && (stVICrop.bEnable)) {

			CVI_U32 u32LumaStride, u32ChromaStride, u32CropHeight;

			u32LumaStride = ALIGN((stVICrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
			u32ChromaStride = ALIGN(((stVICrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
			u32CropHeight = ALIGN(stVICrop.stCropRect.u32Height, 2);

			au32PlanarSize[0] = u32LumaStride * u32CropHeight;
			au32PlanarSize[1] = u32ChromaStride * u32CropHeight / 2;
			au32PlanarSize[2] = u32ChromaStride * u32CropHeight / 2;
		} else {
			au32PlanarSize[0] = stVideoFrame.stVFrame.u32Stride[0] * stVideoFrame.stVFrame.u32Height;
			au32PlanarSize[1] = stVideoFrame.stVFrame.u32Stride[1] * stVideoFrame.stVFrame.u32Height / 2;
			au32PlanarSize[2] = stVideoFrame.stVFrame.u32Stride[2] * stVideoFrame.stVFrame.u32Height / 2;
		}
		break;
	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	default:
		u32PlanarCount = 2;
		if ((ret == CVI_SUCCESS) && (stVICrop.bEnable)) {
			CVI_U32 u32LumaStride, u32ChromaStride, u32CropHeight;

			u32LumaStride = ALIGN((stVICrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
			u32ChromaStride = ALIGN(((stVICrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
			u32CropHeight = ALIGN(stVICrop.stCropRect.u32Height, 2);

			au32PlanarSize[0] = u32LumaStride * u32CropHeight;
			au32PlanarSize[1] = u32ChromaStride * u32CropHeight / 2;
			au32PlanarSize[2] = 0;
		} else {
			au32PlanarSize[0] = stVideoFrame.stVFrame.u32Stride[0] * stVideoFrame.stVFrame.u32Height;
			au32PlanarSize[1] = stVideoFrame.stVFrame.u32Stride[1] * stVideoFrame.stVFrame.u32Height / 2;
			au32PlanarSize[2] = 0;
		}
		break;
	}

	if (ret == CVI_SUCCESS) {

		CVI_U32 image_size = stVideoFrame.stVFrame.u32Length[0] +
			stVideoFrame.stVFrame.u32Length[1] +
			stVideoFrame.stVFrame.u32Length[2];

		CVI_U8 *vir_addr = (CVI_U8 *) CVI_SYS_Mmap(stVideoFrame.stVFrame.u64PhyAddr[0], image_size);

		CVI_SYS_IonInvalidateCache(stVideoFrame.stVFrame.u64PhyAddr[0], vir_addr, image_size);

		plane_offset = 0;

		for (CVI_U32 u32PlanarIdx = 0; u32PlanarIdx < u32PlanarCount; u32PlanarIdx++) {

			stVideoFrame.stVFrame.pu8VirAddr[u32PlanarIdx] = (CVI_U8 *)vir_addr + plane_offset;

			plane_offset += stVideoFrame.stVFrame.u32Length[u32PlanarIdx];

			fwrite(stVideoFrame.stVFrame.pu8VirAddr[u32PlanarIdx], 1, au32PlanarSize[u32PlanarIdx], fp);
		}

		CVI_SYS_Munmap(vir_addr, image_size);
	}

	ret = CVI_VI_ReleaseChnFrame(0, 0, &stVideoFrame);
	ERROR_IF(ret != CVI_SUCCESS);

	fclose(fp);

	return ret;
}

CVI_S32 rawReplayAutoTestStart(const char *pathPrefix)
{

	CVI_S32 ret = CVI_SUCCESS;

	ret = set_raw_replay_data(pAutoTestCtx->pHeader, pAutoTestCtx->pData,
									1, 0, pAutoTestCtx->intRawSize);

	start_raw_replay(0);
	sleep(1);
	rawReplayAutoTestSetConfig();
	LOGOUT("startRawReplay......\n");

	sleep(5);

	LOGOUT("save yuv......\n");

	ret = rawReplayAutoTestSaveYuv(pathPrefix);

	system("mv /mnt/data/sensor_cfg.ini.bak /mnt/data/sensor_cfg.ini");

	stop_raw_replay();

	return ret;
}

CVI_S32 rawReplayAutoTestMd5(const char *pathPrefix)
{
	CVI_S32 ret = CVI_SUCCESS;
	char cmd[_BUFF_LEN] = {0};
	char path[_BUFF_LEN] = {0};
	FILE *fp = NULL;

	char *pSrc = NULL;
	char *pDes = NULL;

	int size = 0x00;

	do {
		snprintf(path, _BUFF_LEN, "%s/output.yuv", pathPrefix);

		snprintf(cmd, _BUFF_LEN, "md5sum %s > /tmp/temp.md5", path);
		system(cmd);

		struct stat statbuf;

		stat(path, &statbuf);
		size = statbuf.st_size;

		pDes = (char *) calloc(1, size);
		if (pDes == NULL) {
			LOGOUT("calloc failed!!!\n");
			ret = CVI_FAILURE;
			break;
		}

		fp = fopen("/tmp/temp.md5", "rb");
		if (fp == NULL) {
			LOGOUT("fopen temp.md5 failed!!!\n");
			ret = CVI_FAILURE;
			break;
		}

		fread(pDes, size, 1, fp);
		fclose(fp);

		snprintf(path, _BUFF_LEN, "%s/md5.txt", pathPrefix);

		stat(path, &statbuf);
		size = statbuf.st_size;

		pSrc = (char *) calloc(1, size);
		if (pSrc == NULL) {
			LOGOUT("calloc failed!!!\n");
			ret = CVI_FAILURE;
			break;
		}

		fp = fopen(path, "rb");
		if (fp == NULL) {
			LOGOUT("fopen %s failed!!!\n", path);
			ret = CVI_FAILURE;
			break;
		}

		fread(pSrc, size, 1, fp);
		fclose(fp);

		LOGOUT("src md5: %s\n", pSrc);
		LOGOUT("des md5: %s\n", pDes);

		if (memcmp(pSrc, pDes, size - 1) == 0) {
			LOGOUT("\n\n\tTEST SUCCESS\n\n");
			ret = CVI_SUCCESS;
		} else {
			LOGOUT("\n\n\tTEST FAILURE\n\n");
			ret = CVI_FAILURE;
		}

	} while (0);

	if (pSrc != NULL) {
		free(pSrc);
	}

	if (pDes != NULL) {
		free(pDes);
	}

	return ret;
}

CVI_S32 start_raw_replay_auto_test(VI_PIPE ViPipe, const CVI_CHAR *pathPrefix)
{
	CVI_S32 ret = CVI_SUCCESS;

	ViPipe = ViPipe;

	pAutoTestCtx = calloc(1, sizeof(AUTO_TEST_CTX_S));

	if (pAutoTestCtx == NULL) {
		LOGOUT("calloc fail!!!\n");
		return CVI_FAILURE;
	}

	system("mv /mnt/data/sensor_cfg.ini /mnt/data/sensor_cfg.ini.bak");

	do {
		ret = rawReplayAutoTestInit(pathPrefix);
		if (ret != CVI_SUCCESS) {
			system("mv /mnt/data/sensor_cfg.ini.bak /mnt/data/sensor_cfg.ini");
			break;
		}

		ret = rawReplayAutoTestStart(pathPrefix);
		if (ret != CVI_SUCCESS) {
			break;
		}

		ret = rawReplayAutoTestMd5(pathPrefix);
		if (ret != CVI_SUCCESS) {
			break;
		}

	} while (0);

	//system("mv /mnt/data/sensor_cfg.ini.bak /mnt/data/sensor_cfg.ini");

	if (pAutoTestCtx->pHeader != NULL) {
		free(pAutoTestCtx->pHeader);
	}

	if (pAutoTestCtx->pData != NULL) {
		free(pAutoTestCtx->pData);
	}

	free(pAutoTestCtx);

	return ret;
}

#if defined(__GNUC__) && defined(__riscv)
#pragma GCC diagnostic pop
#endif