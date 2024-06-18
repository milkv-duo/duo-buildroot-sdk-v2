/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps.c
 * Description:
 */

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "cvi_ispd2_callback_funcs_local.h"
#include "cvi_ispd2_callback_funcs_apps_local.h"
#include "cvi_pqtool_json.h"

#if defined(CHIP_ARCH_CV183X) || defined(CHIP_ARCH_CV182X)
#include "cvi_math.h"
#elif defined(__CV181X__) || defined(__CV180X__)
#include "linux/cvi_math.h"
#endif // CHIP_ARCH

#include "cvi_sys.h"
#include "cvi_buffer.h"
#include "cvi_vb.h"
#include "cvi_vi.h"
#include "cvi_vpss.h"
#include "peri.h"

#include "cvi_isp.h"
#include "3A_internal.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
// #include "cvi_af.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_InitialFrameData(TFrameData *ptFrameData)
{
	ptFrameData->pstYUVHeader = NULL;
	ptFrameData->pstRAWHeader = NULL;
	ptFrameData->pu8ImageBuffer = NULL;
	ptFrameData->pu8AELogBuffer = NULL;
	ptFrameData->pu8AWBLogBuffer = NULL;
	ptFrameData->pu8RawInfo = NULL;
	ptFrameData->pu8RegDump = NULL;

	ptFrameData->u32YUVFrameSize = 0;
	ptFrameData->u32RawFrameSize = 0;

	ptFrameData->u32TotalFrames = 0;
	ptFrameData->u32CurFrame = 0;
	ptFrameData->u32MemOffset = 0;
	ptFrameData->bTightlyMode = CVI_FALSE;
	ptFrameData->getRawVbBlk = VB_INVALID_HANDLE;
	ptFrameData->getRawVbPoolID = VB_INVALID_POOLID;

	ptFrameData->bAE10RAWMode = CVI_FALSE;

	memset(ptFrameData->astVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
	memset(ptFrameData->astVpssVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
	memset(&(ptFrameData->stFrameInfo), 0, sizeof(TGetFrameInfo));

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_ReleaseFrameData(TISPDeviceInfo *ptDevInfo)
{
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo	*ptFrameInfo = &(ptFrameData->stFrameInfo);

	ptFrameData->u32YUVFrameSize = 0;
	ptFrameData->u32RawFrameSize = 0;

	ptFrameData->u32TotalFrames = 0;
	ptFrameData->u32CurFrame = 0;
	ptFrameData->u32MemOffset = 0;

	if (ptFrameData->pstYUVHeader != NULL) {
		SAFE_FREE(ptFrameData->pstYUVHeader);

		if ((ptFrameInfo->pau8FrameAddr[0] != NULL) && (ptFrameInfo->pau8FrameAddr[1] == NULL)) {
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Release YUV buffer");

			CVI_SYS_Munmap(ptFrameInfo->pau8FrameAddr[0], ptFrameInfo->u32FrameSize[0]);
			ptFrameInfo->pau8FrameAddr[0] = NULL;

			if (ptFrameInfo->eImageSrc == IMAGE_FROM_VI) {
				CVI_VI_ReleaseChnFrame(0, ptFrameInfo->ViChn, &(ptFrameData->astVideoFrame[0]));
			} else if (ptFrameInfo->eImageSrc == IMAGE_FROM_VPSS) {
				CVI_VPSS_ReleaseChnFrame(ptFrameInfo->VpssGrp, ptFrameInfo->VpssChn,
					&(ptFrameData->astVideoFrame[0]));
			}
		}
	}

	if (ptFrameData->pstRAWHeader != NULL) {
		SAFE_FREE(ptFrameData->pstRAWHeader);

		if (ptFrameInfo->pau8FrameAddr[0]) {
			for (CVI_U32 u32Idx = 0; u32Idx < MAX_WDR_FUSION_FRAMES; ++u32Idx) {
				if (
					(ptFrameInfo->pau8FrameAddr[u32Idx] != NULL)
					&& (ptFrameInfo->u32FrameSize[u32Idx] > 0)
				) {
					CVI_SYS_Munmap(ptFrameInfo->pau8FrameAddr[u32Idx],
								ptFrameInfo->u32FrameSize[u32Idx]);
					ptFrameInfo->pau8FrameAddr[u32Idx] = NULL;
					ptFrameInfo->u32FrameSize[u32Idx] = 0;
				}
			}

			CVI_VI_ReleasePipeFrame(ptFrameInfo->ViPipe, ptFrameData->astVideoFrame);
		}

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Release RAW VB pool");
		if (ptFrameData->getRawVbBlk != VB_INVALID_HANDLE) {
			CVI_VB_ReleaseBlock(ptFrameData->getRawVbBlk);
			ptFrameData->getRawVbBlk = VB_INVALID_HANDLE;
		}

		if (ptFrameData->getRawVbPoolID != VB_INVALID_POOLID) {
			CVI_VB_DestroyPool(ptFrameData->getRawVbPoolID);
			ptFrameData->getRawVbPoolID = VB_INVALID_POOLID;
		}

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Release VPSS Chn frame buffer");

		CVI_S32 VpssGrp, VpssChn;

		CVI_ISPD2_Utils_GetCurrentVPSSInfo(ptDevInfo, &VpssGrp, &VpssChn);

		CVI_S32 s32Ret = CVI_SUCCESS;

		if (ptFrameData->astVpssVideoFrame[0].stVFrame.u64PhyAddr[0] != 0) {
			s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &(ptFrameData->astVpssVideoFrame[0]));
		}
#ifdef USE_TWO_YUV_FRAME
		if (ptFrameData->astVpssVideoFrame[1].stVFrame.u64PhyAddr[0] != 0) {
			s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &(ptFrameData->astVpssVideoFrame[1]));
		}
#endif // USE_TWO_YUV_FRAME
		if (s32Ret != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_ReleaseChnFrame fail");
		}

		memset(ptFrameData->astVpssVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
	}

	memset(ptFrameData->astVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);

	SAFE_FREE(ptFrameData->pu8ImageBuffer);
	SAFE_FREE(ptFrameData->pu8RawInfo);
	SAFE_FREE(ptFrameData->pu8RegDump);
	SAFE_FREE(ptFrameData->pu8AELogBuffer);
	SAFE_FREE(ptFrameData->pu8AWBLogBuffer);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_InitialBinaryData(TBinaryData *ptBinary)
{
	ptBinary->eDataType		= EBINARYDATA_NONE;
	ptBinary->eDataState	= EBINARYSTATE_DEFAULT;
	ptBinary->u32Size		= 0;
	ptBinary->u32RecvSize	= 0;
	ptBinary->pu8Buffer		= CVI_NULL;
	ptBinary->u32BufferSize	= 0;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_ReleaseBinaryData(TBinaryData *ptBinary)
{
	ptBinary->eDataType		= EBINARYDATA_NONE;
	ptBinary->eDataState	= EBINARYSTATE_DEFAULT;
	ptBinary->u32Size		= 0;
	ptBinary->u32RecvSize	= 0;
	ptBinary->u32BufferSize	= 0;
	SAFE_FREE(ptBinary->pu8Buffer);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_ResetBinaryInStructure(TBinaryData *ptBinaryIn)
{
	ptBinaryIn->eDataType		= EBINARYDATA_NONE;
	ptBinaryIn->eDataState		= EBINARYSTATE_DEFAULT;
	ptBinaryIn->u32Size			= 0;
	ptBinaryIn->u32RecvSize		= 0;
	ptBinaryIn->pu8Buffer		= CVI_NULL;
	ptBinaryIn->u32BufferSize	= 0;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetYUVFrameInfo(const TGetFrameInfo *pstFrameInfo, TFrameHeader *pstFrameHeader)
{
	VIDEO_FRAME_INFO_S	stVideoFrame = {};
	VI_CROP_INFO_S		stVICrop = {};
	VPSS_CROP_INFO_S	stVpssCrop = {};
	EImageSource		eImageSrc = pstFrameInfo->eImageSrc;

	CVI_U32		u32Height, au32PlanarSize[3];
	CVI_S32		s32Ret, s32CropInfoRet;

	memset(pstFrameHeader, 0, sizeof(TFrameHeader));

	s32Ret = CVI_FAILURE;
	if (eImageSrc == IMAGE_FROM_VI) {
		s32Ret = CVI_VI_GetChnFrame(0, pstFrameInfo->ViChn,
			&stVideoFrame, MAX_GET_FRAME_TIMEOUT);
	} else if (eImageSrc == IMAGE_FROM_VPSS) {
		s32Ret = CVI_VPSS_GetChnFrame(pstFrameInfo->VpssGrp, pstFrameInfo->VpssChn,
			&stVideoFrame, MAX_GET_FRAME_TIMEOUT);
	}
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "GetChnFrame fail (timeout)");
		return CVI_FAILURE;
	}

	s32CropInfoRet = CVI_FAILURE;
	if (eImageSrc == IMAGE_FROM_VI) {
		s32CropInfoRet = CVI_VI_GetChnCrop(0, pstFrameInfo->ViChn, &stVICrop);
	} else if (eImageSrc == IMAGE_FROM_VPSS) {
		s32CropInfoRet = CVI_VPSS_GetChnCrop(pstFrameInfo->VpssGrp, pstFrameInfo->VpssChn, &stVpssCrop);
	}

	if ((eImageSrc == IMAGE_FROM_VI) && (s32CropInfoRet == CVI_SUCCESS) && (stVICrop.bEnable)) {
		pstFrameHeader->width		= stVICrop.stCropRect.u32Width;
		pstFrameHeader->height		= stVICrop.stCropRect.u32Height;
		pstFrameHeader->stride[0] =
			ALIGN((stVICrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
		pstFrameHeader->stride[1] =
			ALIGN(((stVICrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
		pstFrameHeader->stride[2] =
			pstFrameHeader->stride[1];
		u32Height					= ALIGN(stVICrop.stCropRect.u32Height, 2);
	} else if ((eImageSrc == IMAGE_FROM_VPSS) && (s32CropInfoRet == CVI_SUCCESS) && (stVpssCrop.bEnable)) {
		pstFrameHeader->width		= stVpssCrop.stCropRect.u32Width;
		pstFrameHeader->height		= stVpssCrop.stCropRect.u32Height;
		pstFrameHeader->stride[0] =
			ALIGN((stVpssCrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
		pstFrameHeader->stride[1] =
			ALIGN(((stVpssCrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
		pstFrameHeader->stride[2] =
			pstFrameHeader->stride[1];
		u32Height					= ALIGN(stVpssCrop.stCropRect.u32Height, 2);
	} else {
		pstFrameHeader->width		= stVideoFrame.stVFrame.u32Width;
		pstFrameHeader->height		= stVideoFrame.stVFrame.u32Height;
		pstFrameHeader->stride[0]	= stVideoFrame.stVFrame.u32Stride[0];
		pstFrameHeader->stride[1]	= stVideoFrame.stVFrame.u32Stride[1];
		pstFrameHeader->stride[2]	= stVideoFrame.stVFrame.u32Stride[2];
		u32Height					= pstFrameHeader->height;
	}

	switch (stVideoFrame.stVFrame.enPixelFormat) {
	case PIXEL_FORMAT_YUV_PLANAR_420:
		au32PlanarSize[0] = pstFrameHeader->stride[0] * u32Height;
		au32PlanarSize[1] = pstFrameHeader->stride[1] * u32Height / 2;
		au32PlanarSize[2] = pstFrameHeader->stride[2] * u32Height / 2;
	break;

	case PIXEL_FORMAT_NV12:
	case PIXEL_FORMAT_NV21:
	default:
		au32PlanarSize[0] = pstFrameHeader->stride[0] * u32Height;
		au32PlanarSize[1] = pstFrameHeader->stride[1] * u32Height / 2;
		au32PlanarSize[2] = 0;
	break;
	}

	pstFrameHeader->size = au32PlanarSize[0] + au32PlanarSize[1] + au32PlanarSize[2];
	pstFrameHeader->pixelFormat = stVideoFrame.stVFrame.enPixelFormat;

	s32Ret = CVI_FAILURE;
	if (eImageSrc == IMAGE_FROM_VI) {
		s32Ret = CVI_VI_ReleaseChnFrame(0, pstFrameInfo->ViChn, &stVideoFrame);
	} else if (eImageSrc == IMAGE_FROM_VPSS) {
		s32Ret = CVI_VPSS_ReleaseChnFrame(pstFrameInfo->VpssGrp, pstFrameInfo->VpssChn, &stVideoFrame);
	}
	if (s32Ret != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "ReleaseChnFrame fail");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_InitYUVFrameBuffer(TFrameData *ptFrameData, const TFrameHeader *ptFrameHeader,
	CVI_U32 u32TotalFrames)
{
	ptFrameData->pstYUVHeader = (TYUVHeader *)calloc(u32TotalFrames, sizeof(TYUVHeader));
	if (ptFrameData->pstYUVHeader == NULL) {
		return CVI_FAILURE;
	}

	if (ptFrameData->bTightlyMode) {
		ptFrameData->pu8ImageBuffer = (CVI_U8 *)calloc(u32TotalFrames, ptFrameData->u32YUVFrameSize);
		if (ptFrameData->pu8ImageBuffer == NULL) {
			return CVI_FAILURE;
		}
	}

	for (CVI_U32 u32Idx = 0; u32Idx < MAX_WDR_FUSION_FRAMES; ++u32Idx) {
		ptFrameData->stFrameInfo.pau8FrameAddr[u32Idx] = NULL;
		ptFrameData->stFrameInfo.u32FrameSize[u32Idx] = 0;
	}

	TYUVHeader *pstYUVHeader = ptFrameData->pstYUVHeader;

	for (CVI_U32 u32FrameIdx = 0; u32FrameIdx < u32TotalFrames; ++u32FrameIdx, pstYUVHeader++) {
		pstYUVHeader->curFrame		= u32FrameIdx;
		pstYUVHeader->numFrame		= u32TotalFrames;
		pstYUVHeader->width			= ptFrameHeader->width;
		pstYUVHeader->height		= ptFrameHeader->height;
		pstYUVHeader->stride[0]		= ptFrameHeader->stride[0];
		pstYUVHeader->stride[1]		= ptFrameHeader->stride[1];
		pstYUVHeader->stride[2]		= ptFrameHeader->stride[2];
		pstYUVHeader->pixelFormat	= ptFrameHeader->pixelFormat;
		pstYUVHeader->size			= ptFrameHeader->size;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_U8 CVI_ISPD2_GetWDRFusionFrame(WDR_MODE_E eWDRMode)
{
	CVI_U8 u8WDRFusionFrame = 1;

	switch (eWDRMode) {
	case WDR_MODE_4To1_LINE:
	case WDR_MODE_4To1_FRAME:
	case WDR_MODE_4To1_FRAME_FULL_RATE:
		u8WDRFusionFrame = 4;
		break;
	case WDR_MODE_3To1_LINE:
	case WDR_MODE_3To1_FRAME:
	case WDR_MODE_3To1_FRAME_FULL_RATE:
		u8WDRFusionFrame = 3;
		break;
	case WDR_MODE_2To1_LINE:
	case WDR_MODE_2To1_FRAME:
	case WDR_MODE_2To1_FRAME_FULL_RATE:
		u8WDRFusionFrame = 2;
		break;
	default:
		u8WDRFusionFrame = 1;
		break;
	}

	return u8WDRFusionFrame;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetRAWFrameInfo(const TGetFrameInfo *pstFrameInfo, TFrameHeader *pstFrameHeader)
{
	VI_DEV_ATTR_S		stDevAttr;
	VI_CHN_ATTR_S		stChnAttr;
	CVI_U32				u32FrameWidth, u32FrameHeight;
	CVI_U32				u32BlkSize;
	CVI_U8				u8WDRFusionFrame;

	memset(pstFrameHeader, 0, sizeof(TFrameHeader));

	if (CVI_VI_GetDevAttr(pstFrameInfo->ViPipe, &stDevAttr) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_GetDevAttr fail");
		return CVI_FAILURE;
	}

	if (CVI_VI_GetChnAttr(pstFrameInfo->ViPipe, pstFrameInfo->ViChn, &stChnAttr) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_GetChnAttr fail");
		return CVI_FAILURE;
	}

	u32FrameWidth	= stChnAttr.stSize.u32Width;
	u32FrameHeight	= stChnAttr.stSize.u32Height;
	u32BlkSize		= VI_GetRawBufferSize(u32FrameWidth, u32FrameHeight,
		PIXEL_FORMAT_RGB_BAYER_12BPP,
		stChnAttr.enCompressMode,
		DEFAULT_ALIGN,
		(u32FrameWidth > RAW_FRAME_TILE_MODE_WIDTH) ? CVI_TRUE : CVI_FALSE);
	u8WDRFusionFrame = CVI_ISPD2_GetWDRFusionFrame(stDevAttr.stWDRAttr.enWDRMode);
	u8WDRFusionFrame = (u8WDRFusionFrame < MAX_WDR_FUSION_FRAMES) ?
		u8WDRFusionFrame : MAX_WDR_FUSION_FRAMES;

	pstFrameHeader->width		= u32FrameWidth;
	pstFrameHeader->height		= u32FrameHeight;
	pstFrameHeader->stride[0]	= 0;
	pstFrameHeader->stride[1]	= 0;
	pstFrameHeader->stride[2]	= 0;
	pstFrameHeader->size		= u32BlkSize;
	pstFrameHeader->pixelFormat	= PIXEL_FORMAT_RGB_BAYER_12BPP;
	pstFrameHeader->bayerID		= stDevAttr.enBayerFormat;
	pstFrameHeader->compress	= stChnAttr.enCompressMode;
	pstFrameHeader->fusionFrame	= u8WDRFusionFrame;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_InitRAWFrameBuffer(TFrameData *ptFrameData, const TFrameHeader *ptFrameHeader,
	CVI_U32 u32TotalFrames, CVI_U32 u32CropWidth, CVI_U32 u32CropHeight)
{
	CVI_U32 u32AELogBufSize, u32AWBLogBufSize;
	CVI_U32 u32Temp;

	ptFrameData->pstRAWHeader = (TRAWHeader *)calloc(u32TotalFrames, sizeof(TRAWHeader));
	if (ptFrameData->pstRAWHeader == NULL) {
		return CVI_FAILURE;
	}

	if (ptFrameData->bTightlyMode) {
		ptFrameData->pu8ImageBuffer = (CVI_U8 *)calloc(u32TotalFrames, ptFrameHeader->size);
		if (ptFrameData->pu8ImageBuffer == NULL) {
			return CVI_FAILURE;
		}
	}

	for (CVI_U32 u32Idx = 0; u32Idx < MAX_WDR_FUSION_FRAMES; ++u32Idx) {
		ptFrameData->stFrameInfo.pau8FrameAddr[u32Idx] = NULL;
		ptFrameData->stFrameInfo.u32FrameSize[u32Idx] = 0;
	}

	ptFrameData->pu8RawInfo = (CVI_U8 *)calloc(RAW_INFO_BUFFER_SIZE, sizeof(CVI_U8));
	if (ptFrameData->pu8RawInfo == NULL) {
		return CVI_FAILURE;
	}

	ptFrameData->pu8RegDump = (CVI_U8 *)calloc(REG_DUMP_BUFFER_SIZE, sizeof(CVI_U8));
	if (ptFrameData->pu8RegDump == NULL) {
		return CVI_FAILURE;
	}

	CVI_ISP_GetAELogBufSize(0, &u32Temp);
	u32AELogBufSize = MULTIPLE_4(u32Temp);
	ptFrameData->pu8AELogBuffer = (CVI_U8 *)calloc(u32AELogBufSize, sizeof(CVI_U8));
	if (ptFrameData->pu8AELogBuffer == NULL) {
		return CVI_FAILURE;
	}

	u32AWBLogBufSize = MULTIPLE_4(AWB_SNAP_LOG_BUFF_SIZE);
	ptFrameData->pu8AWBLogBuffer = (CVI_U8 *)calloc(u32AWBLogBufSize, sizeof(CVI_U8));
	if (ptFrameData->pu8AWBLogBuffer == NULL) {
		return CVI_FAILURE;
	}

	TRAWHeader *pstRAWHeader = ptFrameData->pstRAWHeader;

	for (CVI_U32 u32FrameIdx = 0; u32FrameIdx < u32TotalFrames; ++u32FrameIdx, pstRAWHeader++) {
		pstRAWHeader->numFrame		= u32TotalFrames;
		pstRAWHeader->cropWidth		= u32CropWidth;
		pstRAWHeader->cropHeight	= u32CropHeight;
		pstRAWHeader->stride[0]		= ptFrameHeader->stride[0];
		pstRAWHeader->stride[1]		= ptFrameHeader->stride[1];
		pstRAWHeader->stride[2]		= ptFrameHeader->stride[2];
		pstRAWHeader->curFrame		= u32FrameIdx;
		pstRAWHeader->bayerID		= ptFrameHeader->bayerID;
		pstRAWHeader->compress		= ptFrameHeader->compress;
		pstRAWHeader->fusionFrame	= ptFrameHeader->fusionFrame;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateRAWHeader(TFrameData *ptFrameData)
{
	CVI_S32					ViPipe = ptFrameData->stFrameInfo.ViPipe;

	ISP_EXP_INFO_S			stExpInfo;
	ISP_WB_INFO_S			stWBInfo;
	ISP_INNER_STATE_INFO_S	stInnerStateInfo;

	if (CVI_ISP_QueryExposureInfo(ViPipe, &stExpInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_QueryExposureInfo fail");
		return CVI_FAILURE;
	}

	if (CVI_ISP_QueryWBInfo(ViPipe, &stWBInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_QueryWBInfo fail");
		return CVI_FAILURE;
	}

	if (CVI_ISP_QueryInnerStateInfo(ViPipe, &stInnerStateInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_QueryInnerStateInfo fail");
		return CVI_FAILURE;
	}

	TRAWHeader *pstRAWHeader = ptFrameData->pstRAWHeader;

	for (CVI_U32 u32FrameIdx = 0; u32FrameIdx < ptFrameData->u32TotalFrames; ++u32FrameIdx, pstRAWHeader++) {
		pstRAWHeader->exposure[0]		= stExpInfo.u32LongExpTime;
		pstRAWHeader->exposure[1]		= stExpInfo.u32ShortExpTime;
		pstRAWHeader->exposure[2]		= 0;
		pstRAWHeader->exposure[3]		= 0;
		pstRAWHeader->exposureRatio		= stExpInfo.u32WDRExpRatio;
		pstRAWHeader->ispDGain			= stExpInfo.u32ISPDGain;
		pstRAWHeader->iso				= stExpInfo.u32ISO;
		pstRAWHeader->colorTemp			= stWBInfo.u16ColorTemp;
		pstRAWHeader->wbRGain			= stWBInfo.u16Rgain;
		pstRAWHeader->wbBGain			= stWBInfo.u16Bgain;

		for (CVI_U32 u32CCMIdx = 0; u32CCMIdx < 9; ++u32CCMIdx) {
			pstRAWHeader->ccm[u32CCMIdx] = stInnerStateInfo.ccm[u32CCMIdx];
		}

		pstRAWHeader->blcOffset[0]		= stInnerStateInfo.blcOffsetR;
		pstRAWHeader->blcOffset[1]		= stInnerStateInfo.blcOffsetGr;
		pstRAWHeader->blcOffset[2]		= stInnerStateInfo.blcOffsetGb;
		pstRAWHeader->blcOffset[3]		= stInnerStateInfo.blcOffsetB;
		pstRAWHeader->blcGain[0]		= stInnerStateInfo.blcGainR;
		pstRAWHeader->blcGain[1]		= stInnerStateInfo.blcGainGr;
		pstRAWHeader->blcGain[2]		= stInnerStateInfo.blcGainGb;
		pstRAWHeader->blcGain[3]		= stInnerStateInfo.blcGainB;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetMultiplyYUV(TISPDeviceInfo *ptDevInfo, CVI_U32 u32TotalFrames)
{
	TFrameData          *ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo		*ptFrameInfo = &(ptFrameData->stFrameInfo);
	VIDEO_FRAME_INFO_S	*ptVideoFrame = ptFrameData->astVideoFrame;
	TYUVHeader			*ptYUVHeader = ptFrameData->pstYUVHeader;
	CVI_VOID			*pvVirAddr = NULL;

	VI_CROP_INFO_S		stVICrop;
	VPSS_CROP_INFO_S	stVpssCrop;
	EImageSource		eImageSrc;
	CVI_U32				u32BufOffset;
	CVI_S32				ViPipe, ViChn, VpssGrp, VpssChn;
	CVI_U32				au32PlanarSize[3];
	CVI_U32				u32PlanarCount;
	CVI_S32				s32Ret, s32CropInfoRet;

	eImageSrc		= ptFrameInfo->eImageSrc;
	ViPipe			= 0;		//ptFrameInfo->ViPipe;
	ViChn			= ptFrameInfo->ViChn;
	VpssGrp			= ptFrameInfo->VpssGrp;
	VpssChn			= ptFrameInfo->VpssChn;
	u32PlanarCount	= 0;
	u32BufOffset	= 0;

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "GetMultiplyYUV Source (%d, 0:VI, 1:VPSS), FrameCounts : %u\n"
		"VIPipe: %d, VIChn: %d, VPSSGrp: %d, VPSSChn: %d",
		eImageSrc, u32TotalFrames, ViPipe, ViChn, VpssGrp, VpssChn);

	for (CVI_U32 u32FrameIdx = 0; u32FrameIdx < u32TotalFrames; ++u32FrameIdx, ptYUVHeader++) {
		s32Ret = CVI_FAILURE;

		if (ptFrameInfo->bGetRawReplayYuv) {
			if (eImageSrc == IMAGE_FROM_VI) {
				s32Ret = CVI_ISPD2_GetRawReplayYuv(&(ptDevInfo->tRawReplayHandle),
					ptFrameInfo->u8GetRawReplayYuvId,
					0, ViPipe, ViChn, ptVideoFrame);
			} else if (eImageSrc == IMAGE_FROM_VPSS) {
				s32Ret = CVI_ISPD2_GetRawReplayYuv(&(ptDevInfo->tRawReplayHandle),
					ptFrameInfo->u8GetRawReplayYuvId,
					1, VpssGrp, VpssChn, ptVideoFrame);
			}
		} else {
			if (eImageSrc == IMAGE_FROM_VI) {
				s32Ret = CVI_VI_GetChnFrame(ViPipe, ViChn, ptVideoFrame, MAX_GET_FRAME_TIMEOUT);
			} else if (eImageSrc == IMAGE_FROM_VPSS) {
				s32Ret = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, ptVideoFrame, MAX_GET_FRAME_TIMEOUT);
			}
		}

		if (s32Ret != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "GetChnFrame fail (timeout)");
			return CVI_FAILURE;
		}

		s32CropInfoRet = CVI_FAILURE;
		if (eImageSrc == IMAGE_FROM_VI) {
			s32CropInfoRet = CVI_VI_GetChnCrop(ViPipe, ViChn, &stVICrop);
		} else if (eImageSrc == IMAGE_FROM_VPSS) {
			s32CropInfoRet = CVI_VPSS_GetChnCrop(VpssGrp, VpssChn, &stVpssCrop);
		}

		switch (ptVideoFrame->stVFrame.enPixelFormat) {
		case PIXEL_FORMAT_YUV_PLANAR_420:
			u32PlanarCount = 3;
			if (
				(eImageSrc == IMAGE_FROM_VI)
				&& (s32CropInfoRet == CVI_SUCCESS)
				&& (stVICrop.bEnable)
			) {
				CVI_U32		u32LumaStride, u32ChromaStride, u32CropHeight;

				u32LumaStride
					= ALIGN((stVICrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32ChromaStride
					= ALIGN(((stVICrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32CropHeight
					= ALIGN(stVICrop.stCropRect.u32Height, 2);

				au32PlanarSize[0]	= u32LumaStride * u32CropHeight;
				au32PlanarSize[1]	= u32ChromaStride * u32CropHeight / 2;
				au32PlanarSize[2]	= au32PlanarSize[1];
			} else if (
				(eImageSrc == IMAGE_FROM_VPSS)
				&& (s32CropInfoRet == CVI_SUCCESS)
				&& (stVpssCrop.bEnable)
			) {
				CVI_U32		u32LumaStride, u32ChromaStride, u32CropHeight;

				u32LumaStride
					= ALIGN((stVpssCrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32ChromaStride
					= ALIGN(((stVpssCrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32CropHeight
					= ALIGN(stVpssCrop.stCropRect.u32Height, 2);

				au32PlanarSize[0]	= u32LumaStride * u32CropHeight;
				au32PlanarSize[1]	= u32ChromaStride * u32CropHeight / 2;
				au32PlanarSize[2]	= au32PlanarSize[1];
			} else {
				au32PlanarSize[0]
					= ptVideoFrame->stVFrame.u32Stride[0] * ptVideoFrame->stVFrame.u32Height;
				au32PlanarSize[1]
					= ptVideoFrame->stVFrame.u32Stride[1] * ptVideoFrame->stVFrame.u32Height / 2;
				au32PlanarSize[2]
					= ptVideoFrame->stVFrame.u32Stride[2] * ptVideoFrame->stVFrame.u32Height / 2;
			}
		break;

		case PIXEL_FORMAT_NV12:
		case PIXEL_FORMAT_NV21:
		default:
			u32PlanarCount = 2;
			if (
				(eImageSrc == IMAGE_FROM_VI)
				&& (s32CropInfoRet == CVI_SUCCESS)
				&& (stVICrop.bEnable)
			) {
				CVI_U32		u32LumaStride, u32ChromaStride, u32CropHeight;

				u32LumaStride
					= ALIGN((stVICrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32ChromaStride
					= ALIGN(((stVICrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32CropHeight
					= ALIGN(stVICrop.stCropRect.u32Height, 2);

				au32PlanarSize[0]	= u32LumaStride * u32CropHeight;
				au32PlanarSize[1]	= u32ChromaStride * u32CropHeight / 2;
				au32PlanarSize[2]	= 0;
			} else if (
				(eImageSrc == IMAGE_FROM_VPSS)
				&& (s32CropInfoRet == CVI_SUCCESS)
				&& (stVpssCrop.bEnable)
			) {
				CVI_U32		u32LumaStride, u32ChromaStride, u32CropHeight;

				u32LumaStride
					= ALIGN((stVpssCrop.stCropRect.u32Width * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32ChromaStride
					= ALIGN(((stVpssCrop.stCropRect.u32Width >> 1) * 8 + 7) >> 3, DEFAULT_ALIGN);
				u32CropHeight
					= ALIGN(stVpssCrop.stCropRect.u32Height, 2);

				au32PlanarSize[0]	= u32LumaStride * u32CropHeight;
				au32PlanarSize[1]	= u32ChromaStride * u32CropHeight / 2;
				au32PlanarSize[2]	= 0;
			} else {
				au32PlanarSize[0]
					= ptVideoFrame->stVFrame.u32Stride[0] * ptVideoFrame->stVFrame.u32Height;
				au32PlanarSize[1]
					= ptVideoFrame->stVFrame.u32Stride[1] * ptVideoFrame->stVFrame.u32Height / 2;
				au32PlanarSize[2]	= 0;
			}
		break;
		}

		CVI_U32 u32MemoryBufSize;

		u32MemoryBufSize = ptVideoFrame->stVFrame.u32Length[0]
			+ ptVideoFrame->stVFrame.u32Length[1]
			+ ptVideoFrame->stVFrame.u32Length[2];

		pvVirAddr = CVI_SYS_Mmap(ptVideoFrame->stVFrame.u64PhyAddr[0], u32MemoryBufSize);
		CVI_SYS_IonInvalidateCache(ptVideoFrame->stVFrame.u64PhyAddr[0], pvVirAddr, u32MemoryBufSize);

		if (ptFrameData->bTightlyMode) {
			CVI_U32 u32PlaneOffset = 0;
			CVI_U32 u32FrameSize = 0;

			for (CVI_U32 u32PlanarIdx = 0; u32PlanarIdx < u32PlanarCount; ++u32PlanarIdx) {
				ptVideoFrame->stVFrame.pu8VirAddr[u32PlanarIdx] = (CVI_U8 *)pvVirAddr + u32PlaneOffset;
				u32PlaneOffset += ptVideoFrame->stVFrame.u32Length[u32PlanarIdx];

				if (au32PlanarSize[u32PlanarIdx] > 0) {
					memcpy(ptFrameData->pu8ImageBuffer + u32BufOffset,
						(void *)ptVideoFrame->stVFrame.pu8VirAddr[u32PlanarIdx],
						au32PlanarSize[u32PlanarIdx]);
					u32BufOffset += au32PlanarSize[u32PlanarIdx];
				}

				u32FrameSize += au32PlanarSize[u32PlanarIdx];
			}

			CVI_SYS_Munmap(pvVirAddr, u32MemoryBufSize);

			if (eImageSrc == IMAGE_FROM_VI) {
				s32Ret = CVI_VI_ReleaseChnFrame(ViPipe, ViChn, ptVideoFrame);
			} else if (eImageSrc == IMAGE_FROM_VPSS) {
				s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, ptVideoFrame);
			}
			if (s32Ret != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "ReleaseChnFrame fail");
				return CVI_FAILURE;
			}
			ptYUVHeader->size = u32FrameSize;
		} else {
			ptYUVHeader->size = u32MemoryBufSize;
			ptFrameInfo->pau8FrameAddr[0] = (CVI_U8 *)pvVirAddr;
			ptFrameInfo->u32FrameSize[0] = u32MemoryBufSize;

			for (CVI_U32 u32Idx = 1; u32Idx < MAX_WDR_FUSION_FRAMES; ++u32Idx) {
				ptFrameInfo->pau8FrameAddr[u32Idx] = CVI_NULL;
				ptFrameInfo->u32FrameSize[u32Idx] = 0;
			}
		}

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame#: %u / %u", u32FrameIdx, u32TotalFrames);
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetMultiplyRAW_Kernel(TFrameData *ptFrameData, CVI_U32 u32TotalFrames)
{
	VIDEO_FRAME_INFO_S	*ptVideoFrame = ptFrameData->astVideoFrame;
	TGetFrameInfo		*ptFrameInfo = &(ptFrameData->stFrameInfo);
	TRAWHeader			*ptRAWHeader = ptFrameData->pstRAWHeader;
	VI_PIPE				ViPipe = ptFrameData->stFrameInfo.ViPipe;

	VI_DUMP_ATTR_S		stDumpAttr;
	CVI_U32				au32ExposureSize[MAX_WDR_FUSION_FRAMES];
	CVI_U32				u32FrameSize;
	CVI_U32				u32BufOffset;

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "GetMultiplyRAW Source: VIPipe: %d, TotalFrames: %u",
		ViPipe, u32TotalFrames);

	for (CVI_U32 u32Idx = 0; u32Idx < MAX_WDR_FUSION_FRAMES; ++u32Idx) {
		ptVideoFrame[u32Idx].stVFrame.enPixelFormat = PIXEL_FORMAT_RGB_BAYER_12BPP;
		au32ExposureSize[u32Idx] = 0;
	}

	stDumpAttr.bEnable		= 1;
	stDumpAttr.u32Depth		= 0;
	stDumpAttr.enDumpType	= VI_DUMP_TYPE_RAW;
	if (CVI_VI_SetPipeDumpAttr(ViPipe, &stDumpAttr) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_SetPipeDumpAttr fail");
		return CVI_FAILURE;
	}

	u32BufOffset = 0;

	for (CVI_U32 u32FrameIdx = 0; u32FrameIdx < u32TotalFrames; ++u32FrameIdx, ptRAWHeader++) {
		if (CVI_VI_GetPipeFrame(ViPipe, ptVideoFrame, MAX_GET_FRAME_TIMEOUT) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_GetPipeFrame fail (timeout)");
			return CVI_FAILURE;
		}

		for (CVI_U32 u32Idx = 0; u32Idx < ptRAWHeader->fusionFrame; ++u32Idx) {
			if (u32Idx > 0) {
				u32FrameIdx++;
				ptRAWHeader++;
			}

			ptRAWHeader->width		= ptVideoFrame[u32Idx].stVFrame.u32Width;
			ptRAWHeader->height		= ptVideoFrame[u32Idx].stVFrame.u32Height;
			ptRAWHeader->size		= ptVideoFrame[u32Idx].stVFrame.u32Length[0];
			ptRAWHeader->cropX		= ptVideoFrame[u32Idx].stVFrame.s16OffsetLeft;
			ptRAWHeader->cropY		= ptVideoFrame[u32Idx].stVFrame.s16OffsetTop;
			u32FrameSize			= ptVideoFrame[u32Idx].stVFrame.u32Length[0];

			ptVideoFrame[u32Idx].stVFrame.pu8VirAddr[0] =
				(CVI_U8 *)CVI_SYS_Mmap(ptVideoFrame[u32Idx].stVFrame.u64PhyAddr[0],
										u32FrameSize);

			if (ptFrameData->bTightlyMode) {
				memcpy((void *)(ptFrameData->pu8ImageBuffer + u32BufOffset),
						(const void *)ptVideoFrame[u32Idx].stVFrame.pu8VirAddr[0],
						u32FrameSize);
				CVI_SYS_Munmap((void *)ptVideoFrame[u32Idx].stVFrame.pu8VirAddr[0],
								u32FrameSize);
				u32BufOffset += u32FrameSize;
			} else {
				ptFrameInfo->pau8FrameAddr[u32Idx] = ptVideoFrame[u32Idx].stVFrame.pu8VirAddr[0];
				ptFrameInfo->u32FrameSize[u32Idx] = u32FrameSize;
			}

			au32ExposureSize[u32Idx] = u32FrameSize;
		}

		if (ptFrameData->bTightlyMode) {
			if (CVI_VI_ReleasePipeFrame(ViPipe, ptVideoFrame) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_ReleasePipeFrame fail");
				return CVI_FAILURE;
			}
		}

		usleep(33 * 1000);
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame#: %u / %u, fusion frame = %u",
			u32FrameIdx, u32TotalFrames, ptRAWHeader->fusionFrame);
		for (CVI_U32 u32Idx = 0; u32Idx < ptRAWHeader->fusionFrame; ++u32Idx) {
			ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame size (%u) : %u",
				u32Idx, au32ExposureSize[u32Idx]);
		}
	}

	UNUSED(au32ExposureSize);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetMultiplyRAW_ByCustomVBPool(TFrameData *ptFrameData, CVI_U32 u32TotalFrames)
{
	TRAWHeader	*pstRAWHeader = ptFrameData->pstRAWHeader;
	CVI_U32		u32FusionFrame = pstRAWHeader[0].fusionFrame;
	CVI_S32		s32Ret = CVI_SUCCESS;

	if ((!ptFrameData->bTightlyMode) && (u32TotalFrames > u32FusionFrame)) {
		ISP_DAEMON2_DEBUG_EX(LOG_WARNING,
			"GetMultiplyRAW doesn't support total frame (%u) > (%u) at non-tightly mode",
			u32TotalFrames, u32FusionFrame);
		return CVI_FAILURE;
	}

	ptFrameData->getRawVbPoolID = VB_INVALID_POOLID;
	ptFrameData->getRawVbBlk = VB_INVALID_HANDLE;

	VB_POOL_CONFIG_S	stVBPoolCfg;

	stVBPoolCfg.u32BlkCnt = 1;
	stVBPoolCfg.u32BlkSize = VI_GetRawBufferSize(
		pstRAWHeader[0].cropWidth, pstRAWHeader[0].cropHeight,
		PIXEL_FORMAT_RGB_BAYER_12BPP,
		(pstRAWHeader[0].compress != 0) ? COMPRESS_MODE_TILE : COMPRESS_MODE_NONE,
		DEFAULT_ALIGN,
		(pstRAWHeader[0].cropWidth > RAW_FRAME_TILE_MODE_WIDTH) ? CVI_TRUE : CVI_FALSE);
	stVBPoolCfg.u32BlkSize = stVBPoolCfg.u32BlkSize * u32FusionFrame;

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Create VB pool, BlkSize: %u, WDR fusion frame: %u",
		stVBPoolCfg.u32BlkSize, pstRAWHeader[0].fusionFrame);

	ptFrameData->getRawVbPoolID = CVI_VB_CreatePool(&stVBPoolCfg);
	if (ptFrameData->getRawVbPoolID == VB_INVALID_POOLID) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VB_CreatePool fail");
		return CVI_FAILURE;
	}

	ptFrameData->getRawVbBlk = CVI_VB_GetBlock(ptFrameData->getRawVbPoolID, stVBPoolCfg.u32BlkSize);
	if (ptFrameData->getRawVbBlk == VB_INVALID_HANDLE) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VB_GetBlock fail");
		CVI_VB_DestroyPool(ptFrameData->getRawVbPoolID);
		ptFrameData->getRawVbPoolID = VB_INVALID_POOLID;
		return CVI_FAILURE;
	}

	VIDEO_FRAME_INFO_S *ptVideoFrame = ptFrameData->astVideoFrame;

	ptVideoFrame[0].stVFrame.u64PhyAddr[0] = CVI_VB_Handle2PhysAddr(ptFrameData->getRawVbBlk);
	ptVideoFrame[0].stVFrame.u32Length[0] = stVBPoolCfg.u32BlkSize / u32FusionFrame;

	if (pstRAWHeader[0].fusionFrame > 1) {
		CVI_U32 u32PerFrameSize = ptVideoFrame[0].stVFrame.u32Length[0];

		for (CVI_U32 u32Idx = 1; u32Idx < pstRAWHeader[0].fusionFrame; ++u32Idx) {
			ptVideoFrame[u32Idx].stVFrame.u64PhyAddr[0]
				= ptVideoFrame[0].stVFrame.u64PhyAddr[0] + (u32PerFrameSize * u32Idx);
			ptVideoFrame[u32Idx].stVFrame.u32Length[0] = u32PerFrameSize;
		}
	}

	s32Ret = CVI_ISPD2_GetMultiplyRAW_Kernel(ptFrameData, u32TotalFrames);

	if (ptFrameData->bTightlyMode) {
		if (ptFrameData->getRawVbBlk != VB_INVALID_HANDLE) {
			CVI_VB_ReleaseBlock(ptFrameData->getRawVbBlk);
			ptFrameData->getRawVbBlk = VB_INVALID_HANDLE;
		}

		if (ptFrameData->getRawVbPoolID != VB_INVALID_POOLID) {
			CVI_VB_DestroyPool(ptFrameData->getRawVbPoolID);
			ptFrameData->getRawVbPoolID = VB_INVALID_POOLID;
		}
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetMultiplyRAW_ByUsingVPSSBuffer(TFrameData *ptFrameData, CVI_U32 u32TotalFrames)
{
	TRAWHeader	*pstRAWHeader = ptFrameData->pstRAWHeader;
	CVI_U32		u32FusionFrame = pstRAWHeader[0].fusionFrame;
	CVI_S32		s32Ret = CVI_SUCCESS;

	if ((!ptFrameData->bTightlyMode) && (u32TotalFrames > u32FusionFrame)) {
		ISP_DAEMON2_DEBUG_EX(LOG_WARNING,
			"GetMultiplyRAW doesn't support total frame (%u) > (%u) at non-tightly mode",
			u32TotalFrames, u32FusionFrame);
		return CVI_FAILURE;
	}

	VIDEO_FRAME_INFO_S	*ptVideoFrame = ptFrameData->astVideoFrame;
	VIDEO_FRAME_INFO_S	*ptVPSSVideoFrame = ptFrameData->astVpssVideoFrame;
	TGetFrameInfo		*ptFrameInfo = &(ptFrameData->stFrameInfo);

	CVI_S32		VpssGrp, VpssChn;
	CVI_U32		u32GetFrameCount;

	VpssGrp = ptFrameInfo->VpssGrp;
	VpssChn = ptFrameInfo->VpssChn;

	u32GetFrameCount = 0;
	do {
		s32Ret = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, ptVideoFrame, MAX_GET_FRAME_TIMEOUT);
		if (s32Ret != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_GetChnFrame for LE fail");
		}

		u32GetFrameCount++;
	} while ((s32Ret != CVI_SUCCESS) && (u32GetFrameCount < 3));

	if (s32Ret != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	memcpy(&(ptVPSSVideoFrame[0]), &(ptVideoFrame[0]), sizeof(VIDEO_FRAME_INFO_S));

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "VPSS Chn Frame : Length : %u, %u, %u",
		ptVideoFrame[0].stVFrame.u32Length[0],
		ptVideoFrame[0].stVFrame.u32Length[1],
		ptVideoFrame[0].stVFrame.u32Length[2]);

	ptVideoFrame[0].stVFrame.u32Length[0] = ptVideoFrame[0].stVFrame.u32Length[0]
		+ ptVideoFrame[0].stVFrame.u32Length[1] + ptVideoFrame[0].stVFrame.u32Length[2];

#ifdef USE_TWO_YUV_FRAME
	if (pstRAWHeader[0].fusionFrame > 1) {
		u32GetFrameCount = 0;
		do {
			s32Ret = CVI_VPSS_GetChnFrame(VpssGrp, VpssChn, &(ptVideoFrame[1]), MAX_GET_FRAME_TIMEOUT);
			if (s32Ret != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_GetChnFrame for SE fail");
			}

			u32GetFrameCount++;
		} while ((s32Ret != CVI_SUCCESS) && (u32GetFrameCount < 3));

		if (s32Ret != CVI_SUCCESS) {
			if (ptVPSSVideoFrame[0].stVFrame.u64PhyAddr[0] != 0) {
				if (CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &(ptVPSSVideoFrame[0])) != CVI_SUCCESS) {
					ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_ReleaseChnFrame for LE fail");
				}
			}

			memset(ptVideoFrame, 0, 2 * sizeof(VIDEO_FRAME_INFO_S));
			memset(ptVPSSVideoFrame, 0, 2 * sizeof(VIDEO_FRAME_INFO_S));

			return CVI_FAILURE;
		}

		memcpy(&(ptVPSSVideoFrame[1]), &(ptVideoFrame[1]), sizeof(VIDEO_FRAME_INFO_S));
	}
#else
	if (u32FusionFrame > 1) {
		if (ptVideoFrame[0].stVFrame.u32Length[0] >= (pstRAWHeader[0].size * u32FusionFrame)) {
			CVI_U32 u32PerFrameSize = ptVideoFrame[0].stVFrame.u32Length[0] / u32FusionFrame;

			ptVideoFrame[0].stVFrame.u32Length[0] = u32PerFrameSize;

			for (CVI_U32 u32Idx = 1; u32Idx < u32PerFrameSize; ++u32Idx) {
				ptVideoFrame[u32Idx].stVFrame.u32Length[0] = u32PerFrameSize;
				ptVideoFrame[u32Idx].stVFrame.u64PhyAddr[0] =
					ptVideoFrame[0].stVFrame.u64PhyAddr[0] + (u32PerFrameSize * u32Idx);

				memcpy(&ptVPSSVideoFrame[u32Idx], &ptVideoFrame[u32Idx], sizeof(VIDEO_FRAME_INFO_S));
			}
		} else {
			ISP_DAEMON2_DEBUG_EX(LOG_WARNING,
				"GetMultiplyRAW (VPSS buffer), YUV buffer < sum(%d fusion frames)",
				pstRAWHeader[0].fusionFrame);

			if (ptVPSSVideoFrame[0].stVFrame.u64PhyAddr[0] != 0) {
				if (CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, ptVPSSVideoFrame) != CVI_SUCCESS) {
					ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_ReleaseChnFrame for LE fail");
				}
			}

			memset(ptVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
			memset(ptVPSSVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);

			return CVI_FAILURE;
		}
	}
#endif // USE_TWO_YUV_FRAME

	sleep(1);

	s32Ret = CVI_ISPD2_GetMultiplyRAW_Kernel(ptFrameData, u32TotalFrames);

	if (ptFrameData->bTightlyMode) {
		if (ptVPSSVideoFrame[0].stVFrame.u64PhyAddr[0] != 0) {
			s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &(ptVPSSVideoFrame[0]));
		}
#ifdef USE_TWO_YUV_FRAME
		if (ptVPSSVideoFrame[1].stVFrame.u64PhyAddr[0] != 0) {
			s32Ret = CVI_VPSS_ReleaseChnFrame(VpssGrp, VpssChn, &(ptVPSSVideoFrame[1]));
		}
#endif // USE_TWO_YUV_FRAME
		if (s32Ret != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VPSS_ReleaseChnFrame fail");
		}

		memset(ptVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
		memset(ptVPSSVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_GetMultiplyRAW(TFrameData *ptFrameData, CVI_U32 u32TotalFrames)
{
	memset(ptFrameData->astVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);
	memset(ptFrameData->astVpssVideoFrame, 0, sizeof(VIDEO_FRAME_INFO_S) * MAX_WDR_FUSION_FRAMES);

	if (CVI_ISPD2_GetMultiplyRAW_Kernel(ptFrameData, u32TotalFrames) == CVI_SUCCESS) {
		return CVI_SUCCESS;
	}

	ISP_DAEMON2_DEBUG(LOG_INFO, "GetMultiplyRAW (system buffer) fail. Try use create VB pool mode");

	if (CVI_ISPD2_GetMultiplyRAW_ByCustomVBPool(ptFrameData, u32TotalFrames) == CVI_SUCCESS) {
		return CVI_SUCCESS;
	}

	ISP_DAEMON2_DEBUG(LOG_INFO, "GetMultiplyRAW (custom VB pool) fail. Try use VPSS frame buffer mode");

	if (CVI_ISPD2_GetMultiplyRAW_ByUsingVPSSBuffer(ptFrameData, u32TotalFrames) == CVI_SUCCESS) {
		return CVI_SUCCESS;
	}

	return CVI_FAILURE;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_WriteDataToDestination(int iFd,
	CVI_VOID *pvData, CVI_U32 u32DataLen, CVI_U32 u32PartialLen)
{
	CVI_VOID	*pvDataAddr = pvData;
	CVI_U32		u32WriteLen = 0;
	ssize_t		lWriteLen = 0;

	while (u32WriteLen < u32DataLen) {
		CVI_U32 u32WriteDataLen = MIN(u32DataLen - u32WriteLen, u32PartialLen);

		// printf("Write Data To Dest : %d + %d/%d\n", u32WriteLen, u32PartialLen, u32DataLen);

		lWriteLen = write(iFd, pvDataAddr + u32WriteLen, u32WriteDataLen);
		if (lWriteLen >= 0) {
			u32WriteLen += lWriteLen;
		} else {
			if ((lWriteLen == -1) && (errno == EAGAIN)) {
				usleep(1000);
				// printf("EAGAIN\n");
				continue;
			} else {
				ISP_DAEMON2_DEBUG_EX(LOG_WARNING, "Write partial data to socket fail (%d)\"%s\"",
					errno, strerror(errno));
				return CVI_FAILURE;
			}
		}
	}

	// UNUSED(iFd);
	// UNUSED(pvDataAddr);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_WriteNONEDataToDestination(int iFd)
{
	CVI_S32		s32Ret = CVI_SUCCESS;

	char		szNoneStrings[8];

	snprintf(szNoneStrings, 8, "NONE");

	if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
		(CVI_VOID *)szNoneStrings, strlen(szNoneStrings),
		MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
		s32Ret = CVI_FAILURE;
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportSingleYUVFrame(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TYUVHeader		*pstYUVHeader = NULL;

	CVI_U32			u32CurFrame, u32TotalFrames, u32HeadLen;

	u32CurFrame		= ptFrameData->u32CurFrame;
	u32TotalFrames	= ptFrameData->u32TotalFrames;
	u32HeadLen		= sizeof(TYUVHeader);

	if ((u32CurFrame >= u32TotalFrames) || (ptFrameData->pstYUVHeader == NULL)) {
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame index error (%u >= %u) or Null YUVHeader",
			u32CurFrame, u32TotalFrames);
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
		return CVI_SUCCESS;
	}

	pstYUVHeader	= ptFrameData->pstYUVHeader;

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "YUV Image (tightly:%d)\n"
		"header: %u\nimage: %u\ninfo: %ux%u, (%u, %u, %u)",
		ptFrameData->bTightlyMode,
		u32HeadLen, pstYUVHeader->size,
		pstYUVHeader->width, pstYUVHeader->height,
		pstYUVHeader->stride[0], pstYUVHeader->stride[1], pstYUVHeader->stride[2]);

	if (ptFrameData->bTightlyMode) {
		if (ptFrameData->pu8ImageBuffer != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstYUVHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)ptFrameData->pu8ImageBuffer, pstYUVHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	} else {
		if (ptFrameData->stFrameInfo.pau8FrameAddr[0] != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstYUVHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)ptFrameData->stFrameInfo.pau8FrameAddr[0], pstYUVHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportMultipleYUVFrame(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TYUVHeader		*pstYUVHeader = NULL;

	CVI_U32			u32CurFrame, u32TotalFrames, u32MemOffset, u32HeadLen;

	u32CurFrame		= ptFrameData->u32CurFrame;
	u32TotalFrames	= ptFrameData->u32TotalFrames;
	u32MemOffset	= ptFrameData->u32MemOffset;
	u32HeadLen		= sizeof(TYUVHeader);

	if ((u32CurFrame >= u32TotalFrames) || (ptFrameData->pstYUVHeader == NULL)) {
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame index error (%u >= %u) or Null YUVHeader",
			u32CurFrame, u32TotalFrames);
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
		return CVI_SUCCESS;
	}

	pstYUVHeader	= ptFrameData->pstYUVHeader + u32CurFrame;

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "YUV Multiple Image (tightly:%d) (%u/%u, %u)\n"
		"header: %u\nimage: %u\ninfo: %ux%u, (%u, %u, %u)",
		ptFrameData->bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
		u32HeadLen, pstYUVHeader->size,
		pstYUVHeader->width, pstYUVHeader->height,
		pstYUVHeader->stride[0], pstYUVHeader->stride[1], pstYUVHeader->stride[2]);

	if (ptFrameData->bTightlyMode) {
		if (ptFrameData->pu8ImageBuffer != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstYUVHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)(ptFrameData->pu8ImageBuffer + u32MemOffset), pstYUVHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

			ptFrameData->u32MemOffset += pstYUVHeader->size;
			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	} else {
		if (ptFrameData->stFrameInfo.pau8FrameAddr[0] != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstYUVHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)ptFrameData->stFrameInfo.pau8FrameAddr[0], pstYUVHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	}

	return CVI_SUCCESS;
}

#ifdef DEBUG_DUMP_BINARY_FRAME_BUFFER
// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_WriteBufferToFile(const char *pszFilename, const CVI_VOID *pvBuffer, CVI_U32 u32BufferSize)
{
	FILE *pfFile = fopen(pszFilename, "w");
	CVI_S32 u32Ret = CVI_SUCCESS;

	if (pfFile == NULL) {
		printf("Open file for data write fail\n");
		return CVI_FAILURE;
	}

	size_t lWriteLen = 0;

	lWriteLen = fwrite(pvBuffer, sizeof(CVI_U8), u32BufferSize, pfFile);
	if ((CVI_U32)lWriteLen != u32BufferSize) {
		printf("Write data to file fail \"%s\" (%u != %u)\n",
			pszFilename, lWriteLen, u32BufferSize);

		u32Ret = CVI_FAILURE;
	}

	fclose(pfFile); pfFile = NULL;

	printf("Write data to file success \"%s\"\n", pszFilename);

	return u32Ret;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_WriteBufferToFile_EX(CVI_U32 u32FrameIdx, CVI_U32 u32TotalFrame,
	const CVI_VOID *pvBuffer, CVI_U32 u32BufferSize)
{
	char szFilename[128];

	snprintf(szFilename, 128, "/mnt/data/RAW_%u_%u.raw", u32FrameIdx, u32TotalFrame);

	return CVI_ISPD2_WriteBufferToFile(szFilename, pvBuffer, u32BufferSize);
}
#endif // DEBUG_DUMP_BINARY_FRAME_BUFFER

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportMultipleRAWFrame(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TRAWHeader		*pstRAWHeader = NULL;

	CVI_U32			u32CurFrame, u32TotalFrames, u32MemOffset, u32HeadLen;

	u32CurFrame		= ptFrameData->u32CurFrame;
	u32TotalFrames	= ptFrameData->u32TotalFrames;
	u32MemOffset	= ptFrameData->u32MemOffset;
	u32HeadLen		= sizeof(TRAWHeader);

	if ((u32CurFrame >= u32TotalFrames) || (ptFrameData->pstRAWHeader == NULL)) {
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Frame index error (%u >= %u) or Null RAWHeader",
			u32CurFrame, u32TotalFrames);
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
		return CVI_SUCCESS;
	}

	pstRAWHeader	= ptFrameData->pstRAWHeader + u32CurFrame;

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "RAW Multiple Image (tightly:%d) (%u/%u, %u)\n"
		"header:%u\nimage:%u\ninfo: %ux%u, Crop: (%u, %u), %ux%u",
		ptFrameData->bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
		u32HeadLen, pstRAWHeader->size,
		pstRAWHeader->width, pstRAWHeader->height,
		pstRAWHeader->cropX, pstRAWHeader->cropY,
		pstRAWHeader->cropWidth, pstRAWHeader->cropHeight);

	if (ptFrameData->bTightlyMode) {
		if (ptFrameData->pu8ImageBuffer != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstRAWHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)(ptFrameData->pu8ImageBuffer + u32MemOffset), pstRAWHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

#ifdef DEBUG_DUMP_BINARY_FRAME_BUFFER
			CVI_ISPD2_WriteBufferToFile_EX(u32CurFrame, u32TotalFrames,
				(const CVI_VOID *)(ptFrameData->pu8ImageBuffer + u32MemOffset),
				pstRAWHeader->size);
#endif // DEBUG_DUMP_BINARY_FRAME_BUFFER

			ptFrameData->u32MemOffset += pstRAWHeader->size;
			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	} else {
		CVI_U32 u32NonTightlyIndex = (u32CurFrame < pstRAWHeader->fusionFrame) ?
			u32CurFrame : 0;

		if (ptFrameData->stFrameInfo.pau8FrameAddr[u32NonTightlyIndex] != NULL) {
			// header
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)pstRAWHeader, u32HeadLen,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}
			// data
			if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
				(CVI_VOID *)ptFrameData->stFrameInfo.pau8FrameAddr[u32NonTightlyIndex],
				pstRAWHeader->size,
				MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
				ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
				return CVI_FAILURE;
			}

#ifdef DEBUG_DUMP_BINARY_FRAME_BUFFER
			CVI_ISPD2_WriteBufferToFile_EX(u32CurFrame, u32TotalFrames,
				(const CVI_VOID *)(ptFrameData->stFrameInfo.pau8FrameAddr[u32NonTightlyIndex]),
				pstRAWHeader->size);
#endif // DEBUG_DUMP_BINARY_FRAME_BUFFER

			ptFrameData->u32CurFrame++;
		} else {
			if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
				return CVI_FAILURE;
			}
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportAEBinData(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData	*ptBinData = &(ptDevInfo->tAEBinData);

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "AE bin data Info: %u", ptBinData->u32Size);

	if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
		if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
			(CVI_VOID *)ptBinData->pu8Buffer,
			ptBinData->u32Size,
			MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
			return CVI_FAILURE;
		}
	} else {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportAWBBinData(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData	*ptBinData = &(ptDevInfo->tAWBBinData);

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "AWB bin data Info: %u", ptBinData->u32Size);

	if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
		if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
			(CVI_VOID *)ptBinData->pu8Buffer,
			ptBinData->u32Size,
			MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
			return CVI_FAILURE;
		}
	} else {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportTuningBinData(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData *ptBinaryData = &(ptDevInfo->tBinaryOutData);

	if (ptBinaryData->eDataType != EBINARYDATA_TUNING_BIN_DATA) {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Tuning bin data Info: %u", ptBinaryData->u32Size);

	if ((ptBinaryData->pu8Buffer) && (ptBinaryData->u32Size > 0)) {
		if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
			(CVI_VOID *)ptBinaryData->pu8Buffer,
			ptBinaryData->u32Size,
			MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
			return CVI_FAILURE;
		}
	} else {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_BinaryData_ExportToolDefinitionData(int iFd, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData *ptBinaryData = &(ptDevInfo->tBinaryOutData);

	if (ptBinaryData->eDataType != EBINARYDATA_TOOL_DEFINITION_DATA) {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Tool definition data Info: %u", ptBinaryData->u32Size);

	if ((ptBinaryData->pu8Buffer) && (ptBinaryData->u32Size > 0)) {
		if (CVI_ISPD2_BinaryData_WriteDataToDestination(iFd,
			(CVI_VOID *)ptBinaryData->pu8Buffer,
			ptBinaryData->u32Size,
			MAX_TCP_DATA_SIZE_PER_TRANSMIT) != CVI_SUCCESS) {
			ISP_DAEMON2_DEBUG(LOG_ERR, "Write data to socket fail");
			return CVI_FAILURE;
		}
	} else {
		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_Handle_BinaryOut(int iFd, TJSONRpcBinaryOut *ptBinaryInfo)
{
	if ((!ptBinaryInfo->bDataValid) || (ptBinaryInfo->pvData == NULL)) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "No binary data info. for transmit");
		return CVI_FAILURE;
	}

	TExportDataInfo		*ptExportDataInfo = (TExportDataInfo *)(ptBinaryInfo->pvData);
	TISPDeviceInfo		*ptDevInfo = (TISPDeviceInfo *)ptExportDataInfo->pvDevInfo;
	EBinaryContentType	eContentType = ptExportDataInfo->eContentType;
	CVI_S32				s32Ret = CVI_SUCCESS;

	if (eContentType == ECONTENTDATA_SINGLE_YUV_FRAME) {
		s32Ret = CVI_ISPD2_BinaryData_ExportSingleYUVFrame(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_MULTIPLE_YUV_FRAMES) {
		s32Ret = CVI_ISPD2_BinaryData_ExportMultipleYUVFrame(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_MULTIPLE_RAW_FRAMES) {
		s32Ret = CVI_ISPD2_BinaryData_ExportMultipleRAWFrame(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_AE_BIN_DATA) {
		s32Ret = CVI_ISPD2_BinaryData_ExportAEBinData(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_AWB_BIN_DATA) {
		s32Ret = CVI_ISPD2_BinaryData_ExportAWBBinData(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_TUNING_BIN_DATA) {
		s32Ret = CVI_ISPD2_BinaryData_ExportTuningBinData(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_TOOL_DEFINITION_DATA) {
		s32Ret = CVI_ISPD2_BinaryData_ExportToolDefinitionData(iFd, ptDevInfo);
	} else if (eContentType == ECONTENTDATA_NONE) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "No frame data in system");

		if (CVI_ISPD2_BinaryData_WriteNONEDataToDestination(iFd) != CVI_SUCCESS) {
			s32Ret = CVI_FAILURE;
		}
	}

	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);

	// release frame data
	if (ptExportDataInfo->eReleaseMode & ERELEASE_FRAME_DATA_YES) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);
	} else if (ptExportDataInfo->eReleaseMode & ERELEASE_FRAME_DATA_CHECK_FRAMES) {
		if (ptFrameData->u32CurFrame >= ptFrameData->u32TotalFrames) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);
		}
	}

	// release ae bin data (tAEBinData)
	if (ptExportDataInfo->eReleaseMode & ERELEASE_BINARY_AE_BIN) {
		CVI_ISPD2_ReleaseBinaryData(&(ptDevInfo->tAEBinData));
	}
	// release awb bin data (tAWBBinData)
	if (ptExportDataInfo->eReleaseMode & ERELEASE_BINARY_AWB_BIN) {
		CVI_ISPD2_ReleaseBinaryData(&(ptDevInfo->tAWBBinData));
	}
	// release tuning data/tool definition data (share tBinaryOutData)
	if (ptExportDataInfo->eReleaseMode & ERELEASE_BINARY_OUT_DATA_YES) {
		CVI_ISPD2_ReleaseBinaryData(&(ptDevInfo->tBinaryOutData));
	}

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetSingleYUVFrame(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData			*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo		*pstFrameInfo = &(ptFrameData->stFrameInfo);

	VI_VPSS_MODE_S		stVIVPSSMode;
	TVPSSAdjGroupInfo	astVPSSAdjGrp_System[VPSS_MAX_GRP_NUM];
	TFrameHeader		stFrameHeader;
	CVI_BOOL			bSetVPSSDefault;
	CVI_S32				s32Ret, s32GetYUVRet;

	ptFrameData->bTightlyMode = CVI_FALSE;

	CVI_ISPD2_ReleaseFrameData(ptDevInfo);

	pstFrameInfo->ViPipe = ptDevInfo->s32ViPipe;
	pstFrameInfo->ViChn = ptDevInfo->s32ViChn;
	pstFrameInfo->VpssGrp = ptDevInfo->s32VpssGrp;
	pstFrameInfo->VpssChn = ptDevInfo->s32VpssChn;
	pstFrameInfo->eImageSrc = IMAGE_FROM_VI;

	CVI_SYS_GetVIVPSSMode(&stVIVPSSMode);
	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "VPSS mode (offline/online) : %d",
		stVIVPSSMode.aenMode[pstFrameInfo->ViPipe]);

	bSetVPSSDefault = CVI_FALSE;
	if ((stVIVPSSMode.aenMode[pstFrameInfo->ViPipe] == VI_OFFLINE_VPSS_ONLINE)
		|| (stVIVPSSMode.aenMode[pstFrameInfo->ViPipe] == VI_ONLINE_VPSS_ONLINE)) {
		pstFrameInfo->eImageSrc = IMAGE_FROM_VPSS;
		bSetVPSSDefault = CVI_TRUE;

		CVI_S32 s32DefaultBrightness, s32DefaultContrast, s32DefaultSaturation, s32DefaultHue;

		s32DefaultBrightness = 50;
		s32DefaultContrast = 50;
		s32DefaultSaturation = 50;
		s32DefaultHue = 50;

		// backup and apply default VPSS Adjustment
		TVPSSAdjGroupInfo *ptVPSSAdjGrp = astVPSSAdjGrp_System;

		for (CVI_U32 u32Idx = 0; u32Idx < VPSS_MAX_GRP_NUM; ++u32Idx, ptVPSSAdjGrp++) {
			CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_BRIGHTNESS, &(ptVPSSAdjGrp->s32Brightness));
			CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_CONTRAST, &(ptVPSSAdjGrp->s32Contrast));
			CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_SATURATION, &(ptVPSSAdjGrp->s32Saturation));
			CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_HUE, &(ptVPSSAdjGrp->s32Hue));
		}

		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_BRIGHTNESS, s32DefaultBrightness);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_CONTRAST, s32DefaultContrast);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_SATURATION, s32DefaultSaturation);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_HUE, s32DefaultHue);
	}

	if (CVI_ISPD2_GetYUVFrameInfo(pstFrameInfo, &stFrameHeader) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Get YUV frame info fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get YUV frame info fail");

		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Single YUV Frame, W/H : %u x %u, Stride : %u, Size : %u",
		stFrameHeader.width, stFrameHeader.height, stFrameHeader.stride[0], stFrameHeader.size);

	// initialize yuv header and buffer
	ptFrameData->u32YUVFrameSize = stFrameHeader.size;
	if (CVI_ISPD2_InitYUVFrameBuffer(ptFrameData, &stFrameHeader, 1) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Init YUV frame buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Init YUV frame buffer fail (Failed to allocate memory.)");

		return CVI_FAILURE;
	}

	s32GetYUVRet = CVI_ISPD2_GetMultiplyYUV(ptDevInfo, 1);

	// restore VPSS adjustment
	if (bSetVPSSDefault) {
		TVPSSAdjGroupInfo *ptVPSSAdjGrp = astVPSSAdjGrp_System;

		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_BRIGHTNESS, ptVPSSAdjGrp->s32Brightness);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_CONTRAST, ptVPSSAdjGrp->s32Contrast);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_SATURATION, ptVPSSAdjGrp->s32Saturation);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_HUE, ptVPSSAdjGrp->s32Hue);
	}

	if (s32GetYUVRet != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get YUV frame fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get YUV frame fail (Fail to get yuv frame.)");

		return CVI_FAILURE;
	}

	ptFrameData->u32MemOffset = 0;
	ptFrameData->u32CurFrame = 0;
	ptFrameData->u32TotalFrames = 1;

	// compose JSON data
	JSONObject	*pDataOut = ISPD2_json_object_new_object();
	TYUVHeader	*ptYUVHeader = ptFrameData->pstYUVHeader;
	CVI_U32		u32DataSize = sizeof(TYUVHeader) + ptYUVHeader->size;

	SET_STRING_TO_JSON("Content type", "YUV frame", pDataOut);
	SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetMultipleYUVFrames(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData			*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo		*pstFrameInfo = &(ptFrameData->stFrameInfo);
	CVI_S32				s32Ret = CVI_SUCCESS;

	VI_VPSS_MODE_S		stVIVPSSMode;
	TVPSSAdjGroupInfo	astVPSSAdjGrp_System[VPSS_MAX_GRP_NUM];
	TFrameHeader		stFrameHeader;
	CVI_BOOL			bSetVPSSDefault;
	CVI_S32				ViPipe, ViChn, VpssGrp, VpssChn;

	CVI_U32				u32TotalFrames = 0;
	CVI_BOOL			bTightlyMode = CVI_FALSE;
	EImageSource		eImageSrc = IMAGE_FROM_VI;

	if (ptContentIn->pParams) {
		int Temp;

		GET_INT_FROM_JSON(ptContentIn->pParams, "/frames", u32TotalFrames, s32Ret);

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/tightly", Temp, s32Ret);
		bTightlyMode = (Temp > 0) ? CVI_TRUE : CVI_FALSE;

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/from", Temp, s32Ret);
		eImageSrc = (Temp > 0) ? IMAGE_FROM_VPSS : IMAGE_FROM_VI;

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/getRawReplayYuv", Temp, s32Ret);
		pstFrameInfo->bGetRawReplayYuv = (Temp > 0) ? CVI_TRUE : CVI_FALSE;

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/getRawReplayYuvId", Temp, s32Ret);
		pstFrameInfo->u8GetRawReplayYuvId = Temp;
	}

	ViPipe = ptDevInfo->s32ViPipe;
	ViChn = ptDevInfo->s32ViChn;
	VpssGrp = ptDevInfo->s32VpssGrp;
	VpssChn = ptDevInfo->s32VpssChn;
	pstFrameInfo->eImageSrc = eImageSrc;

	CVI_ISPD2_ReleaseFrameData(ptDevInfo);

	if ((!bTightlyMode) && (u32TotalFrames > 1)) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "YUV multiple frame function only support 1 frame in non-tightly mode");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
			"YUV multiple frame function only support 1 frame in non-tightly mode!)");

		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Capture images from (0: VI, 1: VPSS) : %d", eImageSrc);

	bSetVPSSDefault = CVI_FALSE;
	if (eImageSrc == IMAGE_FROM_VI) {
		CVI_SYS_GetVIVPSSMode(&stVIVPSSMode);
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "VPSS mode (offline/online) : %d",
			stVIVPSSMode.aenMode[ViPipe]);

		if ((stVIVPSSMode.aenMode[ViPipe] == VI_OFFLINE_VPSS_ONLINE)
			|| (stVIVPSSMode.aenMode[ViPipe] == VI_ONLINE_VPSS_ONLINE)) {
			pstFrameInfo->eImageSrc = IMAGE_FROM_VPSS;
			bSetVPSSDefault = CVI_TRUE;
			CVI_ISPD2_Utils_GetCurrentVPSSInfo(ptDevInfo, &VpssGrp, &VpssChn);

			CVI_S32 s32DefaultBrightness, s32DefaultContrast, s32DefaultSaturation, s32DefaultHue;

			s32DefaultBrightness = 50;
			s32DefaultContrast = 50;
			s32DefaultSaturation = 50;
			s32DefaultHue = 50;

			// backup and apply default VPSS Adjustment
			TVPSSAdjGroupInfo *ptVPSSAdjGrp = astVPSSAdjGrp_System;

			for (CVI_U32 u32Idx = 0; u32Idx < VPSS_MAX_GRP_NUM; ++u32Idx, ptVPSSAdjGrp++) {
				CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_BRIGHTNESS, &(ptVPSSAdjGrp->s32Brightness));
				CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_CONTRAST, &(ptVPSSAdjGrp->s32Contrast));
				CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_SATURATION, &(ptVPSSAdjGrp->s32Saturation));
				CVI_VPSS_GetGrpProcAmp(u32Idx, PROC_AMP_HUE, &(ptVPSSAdjGrp->s32Hue));
			}

			CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_BRIGHTNESS, s32DefaultBrightness);
			CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_CONTRAST, s32DefaultContrast);
			CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_SATURATION, s32DefaultSaturation);
			CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_HUE, s32DefaultHue);
		}
	} else if (eImageSrc == IMAGE_FROM_VPSS) {
		CVI_ISPD2_Utils_GetCurrentVPSSInfo(ptDevInfo, &VpssGrp, &VpssChn);
	} else {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Image source invalid");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Image source invalid (Image source select error!)");

		return CVI_FAILURE;
	}

	ptFrameData->bTightlyMode = bTightlyMode;
	pstFrameInfo->ViPipe = ViPipe;
	pstFrameInfo->ViChn = ViChn;
	pstFrameInfo->VpssGrp = VpssGrp;
	pstFrameInfo->VpssChn = VpssChn;

	if (CVI_ISPD2_GetYUVFrameInfo(pstFrameInfo, &stFrameHeader) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get YUV frame info fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get YUV frame info fail");

		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Multiple YUV Frames (%u), W/H : %u x %u, Stride : %u, Size : %u",
		u32TotalFrames,
		stFrameHeader.width, stFrameHeader.height, stFrameHeader.stride[0], stFrameHeader.size);

	// initialize yuv header and buffer
	ptFrameData->u32YUVFrameSize = stFrameHeader.size;
	if (CVI_ISPD2_InitYUVFrameBuffer(ptFrameData, &stFrameHeader, u32TotalFrames) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Init YUV frame buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Init YUV frame buffer fail (Failed to allocate memory. Please decrease the number of frame or uncheck the \"Capture raw tightly\" checkbox.)");

		return CVI_FAILURE;
	}

	s32Ret = CVI_ISPD2_GetMultiplyYUV(ptDevInfo, u32TotalFrames);

	// restore VPSS adjustment
	if (bSetVPSSDefault) {
		TVPSSAdjGroupInfo *ptVPSSAdjGrp = astVPSSAdjGrp_System;

		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_BRIGHTNESS, ptVPSSAdjGrp->s32Brightness);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_CONTRAST, ptVPSSAdjGrp->s32Contrast);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_SATURATION, ptVPSSAdjGrp->s32Saturation);
		CVI_VPSS_SetGrpProcAmp(0, PROC_AMP_HUE, ptVPSSAdjGrp->s32Hue);
	}

	if (s32Ret != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get YUV frame fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get YUV frame fail (Fail to get yuv frame.)");

		return CVI_FAILURE;
	}

	ptFrameData->u32MemOffset = 0;
	ptFrameData->u32CurFrame = 0;
	ptFrameData->u32TotalFrames = u32TotalFrames;

	// compose JSON data
	JSONObject	*pDataOut = ISPD2_json_object_new_object();
	TYUVHeader	*ptYUVHeader = ptFrameData->pstYUVHeader;
	CVI_U32		u32DataSize = sizeof(TYUVHeader) + ptYUVHeader->size;

	SET_STRING_TO_JSON("Content type", "YUV multiple frames", pDataOut);
	SET_INT_TO_JSON("Frame count", u32TotalFrames, pDataOut);
	SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateRegDump(TFrameData *ptFrameData)
{
	FILE			*pfFile = NULL;
	const VI_PIPE	ViPipe = ptFrameData->stFrameInfo.ViPipe;
	long			lFileSize = 0;

	VI_DUMP_REGISTER_TABLE_S	stDumpRegTable;
	ISP_INNER_STATE_INFO_S		stInnerStateInfo;

	if (CVI_ISP_QueryInnerStateInfo(ViPipe, &stInnerStateInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_QueryInnerStateInfo fail");
		return CVI_FAILURE;
	}

	pfFile = fopen(REG_DUMP_TEMP_LOCATION, "w+");
	if (pfFile == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Open file fail");
		return CVI_FAILURE;
	}

	stDumpRegTable.MlscGainLut.RGain = stInnerStateInfo.mlscGainTable.RGain;
	stDumpRegTable.MlscGainLut.GGain = stInnerStateInfo.mlscGainTable.GGain;
	stDumpRegTable.MlscGainLut.BGain = stInnerStateInfo.mlscGainTable.BGain;

	if (CVI_VI_DumpHwRegisterToFile(ViPipe, pfFile, &stDumpRegTable) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_VI_DumpHwRegisterToFile fail");
		fclose(pfFile); pfFile = NULL;

		return CVI_FAILURE;
	}

	fseek(pfFile, 0, SEEK_END);
	lFileSize = ftell(pfFile);
	fseek(pfFile, 0, SEEK_SET);
	fread(ptFrameData->pu8RegDump, sizeof(char), lFileSize, pfFile);
	fclose(pfFile); pfFile = NULL;

	remove(REG_DUMP_TEMP_LOCATION);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateRAWInfo(TFrameData *ptFrameData)
{
	FILE			*pfFile = NULL;
	const VI_PIPE	ViPipe = ptFrameData->stFrameInfo.ViPipe;
	long			lFileSize = 0;

	pfFile = fopen(RAW_INFO_TEMP_LOCATION, "w+");
	if (pfFile == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Open file fail");
		return CVI_FAILURE;
	}

	if (CVI_ISP_DumpFrameRawInfoToFile(ViPipe, pfFile) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_DumpFrameRawInfoToFile fail");
		fclose(pfFile); pfFile = NULL;

		return CVI_FAILURE;
	}

	fseek(pfFile, 0, SEEK_END);
	lFileSize = ftell(pfFile);
	fseek(pfFile, 0, SEEK_SET);
	fread(ptFrameData->pu8RawInfo, sizeof(char), lFileSize, pfFile);
	fclose(pfFile); pfFile = NULL;

	remove(RAW_INFO_TEMP_LOCATION);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAELogData(TFrameData *ptFrameData)
{
	const VI_PIPE	ViPipe = ptFrameData->stFrameInfo.ViPipe;

	CVI_U32			u32LogSize;

	if (CVI_ISP_GetAELogBufSize(ViPipe, &u32LogSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "AE log size invalid");
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetAELogBuf(ViPipe, ptFrameData->pu8AELogBuffer, u32LogSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAELogBuf fail");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAEBinData(TFrameData *ptFrameData, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData		*ptAEBinData = &(ptDevInfo->tAEBinData);
	VI_PIPE			ViPipe = ptDevInfo->s32ViPipe;

	CVI_U32			u32BufferSize, u32BinSize;

	if (ptFrameData != NULL) {
		ViPipe = ptFrameData->stFrameInfo.ViPipe;
	}

	CVI_ISPD2_ReleaseBinaryData(ptAEBinData);

	if (CVI_ISP_GetAEBinBufSize(ViPipe, &u32BinSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "AE bin size invalid");
		return CVI_FAILURE;
	}
	u32BufferSize = MULTIPLE_4(u32BinSize);

	ptAEBinData->pu8Buffer = (CVI_U8 *)calloc(u32BufferSize, sizeof(CVI_U8));
	if (ptAEBinData->pu8Buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate AE bin buffer fail");
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetAEBinBuf(ViPipe, ptAEBinData->pu8Buffer, u32BinSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAEBinBuf fail");
		CVI_ISPD2_ReleaseBinaryData(ptAEBinData);
		return CVI_FAILURE;
	}

	ptAEBinData->eDataType = EBINARYDATA_AE_BIN_DATA;
	ptAEBinData->u32Size = u32BinSize;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAWBLogData(TFrameData *ptFrameData)
{
	const VI_PIPE	ViPipe = ptFrameData->stFrameInfo.ViPipe;

	if (CVI_ISP_GetAWBSnapLogBuf(ViPipe, ptFrameData->pu8AWBLogBuffer, AWB_SNAP_LOG_BUFF_SIZE) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAWBSnapLogBuf fail");
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAWBBinData(TFrameData *ptFrameData, TISPDeviceInfo *ptDevInfo)
{
	TBinaryData		*ptAWBBinData = &(ptDevInfo->tAWBBinData);
	VI_PIPE			ViPipe = ptDevInfo->s32ViPipe;

	CVI_U32			u32BufferSize;
	CVI_S32			s32BinSize;

	if (ptFrameData != NULL) {
		ViPipe = ptFrameData->stFrameInfo.ViPipe;
	}

	CVI_ISPD2_ReleaseBinaryData(ptAWBBinData);

	s32BinSize = CVI_ISP_GetAWBDbgBinSize();
	if (s32BinSize <= 0) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "AWB bin size invalid");
		return CVI_FAILURE;
	}

	u32BufferSize = MULTIPLE_4(s32BinSize);

	ptAWBBinData->pu8Buffer = (CVI_U8 *)calloc(u32BufferSize, sizeof(CVI_U8));
	if (ptAWBBinData->pu8Buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate AWB bin buffer fail");
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetAWBDbgBinBuf(ViPipe, ptAWBBinData->pu8Buffer, s32BinSize) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "CVI_ISP_GetAWBDbgBinBuf fail");
		CVI_ISPD2_ReleaseBinaryData(ptAWBBinData);
		return CVI_FAILURE;
	}

	ptAWBBinData->eDataType = EBINARYDATA_AWB_BIN_DATA;
	ptAWBBinData->u32Size = (CVI_U32)s32BinSize;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAEData(TFrameData *ptFrameData, TISPDeviceInfo *ptDevInfo)
{
	if (CVI_ISPD2_UpdateAELogData(ptFrameData) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AE log data fail");
		return CVI_FAILURE;
	}
	if (CVI_ISPD2_UpdateAEBinData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AE bin data fail");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_UpdateAWBData(TFrameData *ptFrameData, TISPDeviceInfo *ptDevInfo)
{
	if (CVI_ISPD2_UpdateAWBLogData(ptFrameData) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AWB log data fail");
		return CVI_FAILURE;
	}
	if (CVI_ISPD2_UpdateAWBBinData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AWB bin data fail");
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_Update3AData(TFrameData *ptFrameData, TISPDeviceInfo *ptDevInfo)
{
	if (CVI_ISPD2_UpdateAEData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}
	if (CVI_ISPD2_UpdateAWBData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_IsRawReplayMode(TJSONRpcContentIn *ptContentIn)
{
	//In cv181x chips, we can't dump raw when in raw replay mode
	//raw frame in pre be, raw dump in pre fe
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	VI_PIPE_FRAME_SOURCE_E frameSource = VI_PIPE_FRAME_SOURCE_DEV;
	CVI_S32				ViPipe = 0;

	ViPipe = ptDevInfo->s32ViPipe;
	if (CVI_VI_GetPipeFrameSource(ViPipe, &frameSource) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_WARNING, "Get Vi PipeFrameSource fail\n");
		return CVI_FALSE;
	}

	if (frameSource != VI_PIPE_FRAME_SOURCE_DEV) {
		//raw replay mode
		return CVI_TRUE;

	} else {
		//normal mode
		return CVI_FALSE;
	}
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetMultipleRAWFrames(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData			*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo		*pstFrameInfo = &(ptFrameData->stFrameInfo);
	CVI_S32				s32Ret = CVI_SUCCESS;

	TFrameHeader		stFrameHeader;
	ISP_PUB_ATTR_S		stPubAttr;
	CVI_S32				ViPipe, ViChn;
	CVI_U32				u32CropWidth, u32CropHeight;

	CVI_U32				u32TotalFrames = 1;
	CVI_BOOL			bTightlyMode = CVI_FALSE;
	CVI_BOOL			bDumpReg = CVI_FALSE;
	CVI_U32				u32DataSize = 0;
	CVI_BOOL			bDumpRaw = CVI_TRUE;

	ViPipe = ptDevInfo->s32ViPipe;
	ViChn = ptDevInfo->s32ViChn;

	if (ptContentIn->pParams) {
		int Temp;

		GET_INT_FROM_JSON(ptContentIn->pParams, "/frames", u32TotalFrames, s32Ret);

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/tightly", Temp, s32Ret);
		bTightlyMode = (Temp > 0) ? CVI_TRUE : CVI_FALSE;

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/dump_reg", Temp, s32Ret);
		bDumpReg = (Temp > 0) ? CVI_TRUE : CVI_FALSE;

		Temp = 0;
		GET_INT_FROM_JSON(ptContentIn->pParams, "/dump_raw", Temp, s32Ret);
		bDumpRaw = (Temp > 0) ? CVI_TRUE : CVI_FALSE;
	}

	CVI_ISPD2_ReleaseFrameData(ptDevInfo);

	if ((!bTightlyMode) && (u32TotalFrames > 1)) {
		ISP_DAEMON2_DEBUG(LOG_WARNING, "RAW multiple frame function only support 1 frame in non-tightly mode");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
			"YUV multiple frame function only support 1 frame in non-tightly mode!)");

		return CVI_FAILURE;
	}

	if (CVI_ISP_GetPubAttr(ViPipe, &stPubAttr) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Get Pub Attr fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"ISP Get pub attr fail");

		return CVI_FAILURE;
	}

	ptFrameData->bTightlyMode = bTightlyMode;
	pstFrameInfo->ViPipe = ViPipe;
	pstFrameInfo->ViChn = ViChn;
	u32CropWidth = stPubAttr.stWndRect.u32Width;
	u32CropHeight = stPubAttr.stWndRect.u32Height;

	if (CVI_ISPD2_GetRAWFrameInfo(pstFrameInfo, &stFrameHeader) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW frame info fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get RAW frame info fail");

		return CVI_FAILURE;
	}

	// WDR (fusion frame > 1) raw size is variable, use 1.15 times buffer size
	if (stFrameHeader.fusionFrame > 1) {
		stFrameHeader.size *= 1.15;
		stFrameHeader.size = MULTIPLE_4(stFrameHeader.size);
		u32TotalFrames *= stFrameHeader.fusionFrame;
	}
	ptFrameData->u32TotalFrames = u32TotalFrames;

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Multiple RawFrames (%u), W/H : %u x %u, Crop W/H : %u x %u, Size : %u\n"
		"Fusion frame = %u, BayerID : %u, Compress : %u",
		u32TotalFrames,
		stFrameHeader.width, stFrameHeader.height, u32CropWidth, u32CropHeight, stFrameHeader.size,
		stFrameHeader.fusionFrame, stFrameHeader.bayerID, stFrameHeader.compress);

	// initialize raw header and buffer
	ptFrameData->u32RawFrameSize = stFrameHeader.size;
	if (CVI_ISPD2_InitRAWFrameBuffer(ptFrameData, &stFrameHeader, u32TotalFrames,
			u32CropWidth, u32CropHeight) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Init RAW frame buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Init RAW frame buffer fail (Failed to allocate memory. Please decrease the number of frame or uncheck the \"Capture raw tightly\" checkbox.)");

		return CVI_FAILURE;
	}

	// Get RAW header
	if (CVI_ISPD2_UpdateRAWHeader(ptFrameData) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "CVI_ISPD2_UpdateRAWHeader fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Update RAW Header fail");

		return CVI_FAILURE;
	}

	// Get Reg dump
	if (bDumpReg) {
		if (CVI_ISPD2_UpdateRegDump(ptFrameData) != CVI_SUCCESS) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);

			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get Reg dump fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Get Reg dump fail");

			return CVI_FAILURE;
		}
	}

	if (!CVI_ISPD2_IsRawReplayMode(ptContentIn) && bDumpRaw) {
		// Get RAW info.
		if (CVI_ISPD2_UpdateRAWInfo(ptFrameData) != CVI_SUCCESS) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);

			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW info fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Get RAW info fail");

			return CVI_FAILURE;
		}

		if (CVI_ISPD2_GetMultiplyRAW(ptFrameData, u32TotalFrames) != CVI_SUCCESS) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);

			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW frames fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Get RAW frames fail");

			return CVI_FAILURE;
		}

		u32DataSize = sizeof(TRAWHeader) + ptFrameData->pstRAWHeader->size;
	}

	// Get 3A data
	if (CVI_ISPD2_Update3AData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get 3A data fail");

		return CVI_FAILURE;
	}

	ptFrameData->u32MemOffset = 0;
	ptFrameData->u32CurFrame = 0;
	ptFrameData->u32TotalFrames = u32TotalFrames;

	// compose JSON data
	JSONObject	*pDataOut = ISPD2_json_object_new_object();

	SET_STRING_TO_JSON("Content type", "RAW multiple frames", pDataOut);
	SET_INT_TO_JSON("Frame count", u32TotalFrames, pDataOut);
	SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);
	SET_STRING_TO_JSON("Reg dump log", (char *)ptFrameData->pu8RegDump, pDataOut);
	SET_STRING_TO_JSON("Raw info log", (char *)ptFrameData->pu8RawInfo, pDataOut);
	SET_STRING_TO_JSON("AE log", (char *)ptFrameData->pu8AELogBuffer, pDataOut);
	SET_STRING_TO_JSON("AWB log", (char *)ptFrameData->pu8AWBLogBuffer, pDataOut);
	if (u32DataSize == 0) {
		//In Raw replay mode, pqtool do not call CVI_ISPD2_CBFunc_GetImageData
		//we send raw header now
		SET_INT_TO_JSON("Bayer ID", ptFrameData->pstRAWHeader->bayerID, pDataOut);
		SET_INT_TO_JSON("Iso", ptFrameData->pstRAWHeader->iso, pDataOut);
		SET_INT_TO_JSON("Crop Width", ptFrameData->pstRAWHeader->cropWidth, pDataOut);
		SET_INT_TO_JSON("Crop Height", ptFrameData->pstRAWHeader->cropHeight, pDataOut);
	}

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetImageInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo	*pstFrameInfo = &(ptFrameData->stFrameInfo);

	CVI_U32			u32CurFrame, u32TotalFrames, u32MemOffset;
	CVI_BOOL		bTightlyMode;

	u32CurFrame		= ptFrameData->u32CurFrame;
	u32TotalFrames	= ptFrameData->u32TotalFrames;
	u32MemOffset	= ptFrameData->u32MemOffset;
	bTightlyMode	= ptFrameData->bTightlyMode;

	if (ptFrameData->pstYUVHeader != NULL) {
		// YUV frame
		TYUVHeader	*ptYUVHeader = ptFrameData->pstYUVHeader;
		CVI_BOOL	bDataReady = CVI_FALSE;
		CVI_U32		u32DataSize = 0;

		if (u32CurFrame < u32TotalFrames) {
			ptYUVHeader += u32CurFrame;

			ISP_DAEMON2_DEBUG_EX(LOG_INFO,
				"Get YUV frame info: (Tightly:%u) (%u/%u, %u), %ux%u",
				bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
				ptYUVHeader->width, ptYUVHeader->height);

			if (bTightlyMode) {
				if (ptFrameData->pu8ImageBuffer != NULL) {
					bDataReady = CVI_TRUE;
					u32DataSize = sizeof(TYUVHeader) + ptYUVHeader->size;
				}
			} else {
				// we only design 1 HW buffer for non-tightly mode
				if (pstFrameInfo->pau8FrameAddr[0] != NULL) {
					bDataReady = CVI_TRUE;
					u32DataSize = sizeof(TYUVHeader) + ptYUVHeader->size;
				}
			}
		}

		JSONObject *pDataOut = ISPD2_json_object_new_object();

		if (bDataReady) {
			SET_STRING_TO_JSON("Content type", "YUV frame", pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty frame", pDataOut);
		}
		SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
	} else if (ptFrameData->pstRAWHeader != NULL) {
		// RAW frame
		TRAWHeader	*ptRAWHeader = ptFrameData->pstRAWHeader;
		CVI_BOOL	bDataReady = CVI_FALSE;
		CVI_U32		u32DataSize = 0;

		if (u32CurFrame < u32TotalFrames) {
			ptRAWHeader += u32CurFrame;

			ISP_DAEMON2_DEBUG_EX(LOG_INFO,
				"Get RAW frame info: (Tightly:%u) (%u/%u, %u), %ux%u, Crop: (%u, %u), %ux%u",
				bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
				ptRAWHeader->width, ptRAWHeader->height,
				ptRAWHeader->cropX, ptRAWHeader->cropY,
				ptRAWHeader->cropWidth, ptRAWHeader->cropHeight);

			if (bTightlyMode) {
				if (ptFrameData->pu8ImageBuffer != NULL) {
					bDataReady = CVI_TRUE;
					u32DataSize = sizeof(TRAWHeader) + ptRAWHeader->size;
				}
			} else {
				// we only design MAX_WDR_FUSION_FRAMES HW buffer for non-tightly mode
				// but it depend on sensor fusion mode
				CVI_U32 u32NonTightlyIndex = (u32CurFrame < ptRAWHeader->fusionFrame) ?
					u32CurFrame : 0;

				if (pstFrameInfo->pau8FrameAddr[u32NonTightlyIndex] != NULL) {
					bDataReady = CVI_TRUE;
					u32DataSize = sizeof(TRAWHeader) + ptRAWHeader->size;
				}
			}
		}

		JSONObject *pDataOut = ISPD2_json_object_new_object();

		if (bDataReady) {
			SET_STRING_TO_JSON("Content type", "RAW frame", pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty frame", pDataOut);
		}
		SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
	} else {
		// empty content
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "No image data");

		// compose JSON data
		JSONObject *pDataOut = ISPD2_json_object_new_object();

		SET_STRING_TO_JSON("Content type", "Empty frame", pDataOut);
		SET_INT_TO_JSON("Data size", 0, pDataOut);

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
	}

	UNUSED(pJsonResponse);
	UNUSED(u32MemOffset);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetImageData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo	*pstFrameInfo = &(ptFrameData->stFrameInfo);
	TExportDataInfo	*ptExportDataInfo = &(ptDevInfo->tExportDataInfo);

	CVI_U32			u32CurFrame, u32TotalFrames, u32MemOffset;
	CVI_BOOL		bTightlyMode;

	u32CurFrame		= ptFrameData->u32CurFrame;
	u32TotalFrames	= ptFrameData->u32TotalFrames;
	u32MemOffset	= ptFrameData->u32MemOffset;
	bTightlyMode	= ptFrameData->bTightlyMode;

	if (ptFrameData->pstYUVHeader != NULL) {
		// YUV frame
		TYUVHeader	*ptYUVHeader = ptFrameData->pstYUVHeader;
		CVI_BOOL	bDataReady = CVI_FALSE;

		if (u32CurFrame < u32TotalFrames) {
			ptYUVHeader += u32CurFrame;

			ISP_DAEMON2_DEBUG_EX(LOG_INFO,
				"Get YUV frame data: (Tightly:%u) (%u/%u, %u), %ux%u",
				bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
				ptYUVHeader->width, ptYUVHeader->height);

			if (bTightlyMode) {
				if (ptFrameData->pu8ImageBuffer != NULL) {
					bDataReady = CVI_TRUE;
				}
			} else {
				// we only design 1 HW buffer for non-tightly mode
				if (pstFrameInfo->pau8FrameAddr[0] != NULL) {
					bDataReady = CVI_TRUE;
				}
			}
		}

		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;

		// prepare binary data
		if (bDataReady) {
			ptExportDataInfo->eContentType = ECONTENTDATA_MULTIPLE_YUV_FRAMES;
			ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
			ptExportDataInfo->eReleaseMode = ERELEASE_FRAME_DATA_CHECK_FRAMES;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
			ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
			ptExportDataInfo->eReleaseMode = ERELEASE_FRAME_DATA_YES;
		}
		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
	} else if (ptFrameData->pstRAWHeader != NULL) {
		// RAW frame
		TRAWHeader	*ptRAWHeader = ptFrameData->pstRAWHeader;
		CVI_BOOL	bDataReady = CVI_FALSE;

		if (u32CurFrame < u32TotalFrames) {
			ptRAWHeader += u32CurFrame;

			ISP_DAEMON2_DEBUG_EX(LOG_INFO,
				"Get RAW frame info: (Tightly:%u) (%u/%u, %u), %ux%u, Crop: (%u, %u), %ux%u",
				bTightlyMode, u32CurFrame, u32TotalFrames, u32MemOffset,
				ptRAWHeader->width, ptRAWHeader->height,
				ptRAWHeader->cropX, ptRAWHeader->cropY,
				ptRAWHeader->cropWidth, ptRAWHeader->cropHeight);

			if (bTightlyMode) {
				if (ptFrameData->pu8ImageBuffer != NULL) {
					bDataReady = CVI_TRUE;
				}
			} else {
				// we only design MAX_WDR_FUSION_FRAMES HW buffer for non-tightly mode
				// but it depend on sensor fusion mode
				CVI_U32 u32NonTightlyIndex = (u32CurFrame < ptRAWHeader->fusionFrame) ?
					u32CurFrame : 0;

				if (pstFrameInfo->pau8FrameAddr[u32NonTightlyIndex] != NULL) {
					bDataReady = CVI_TRUE;
				}
			}
		}

		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;

		// prepare binary data
		if (bDataReady) {
			ptExportDataInfo->eContentType = ECONTENTDATA_MULTIPLE_RAW_FRAMES;
			ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
			ptExportDataInfo->eReleaseMode = ERELEASE_FRAME_DATA_CHECK_FRAMES;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
			ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
			ptExportDataInfo->eReleaseMode = ERELEASE_FRAME_DATA_YES;
		}
		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
	} else {
		// empty content
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "No image data");

		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;

		ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_FRAME_DATA_YES;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
	}

	UNUSED(pJsonResponse);
	UNUSED(u32MemOffset);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_StopGetImageData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	CVI_ISPD2_ReleaseFrameData(ptDevInfo);

	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
static CVI_S32 CVI_ISPD2_PrepareToolDefinitionData(TISPDeviceInfo *ptDevInfo)
{
	TBinaryData		*ptToolDefData = &(ptDevInfo->tBinaryOutData);
	CVI_U32			u32BufferSize;

	CVI_ISPD2_ReleaseBinaryData(ptToolDefData);

	u32BufferSize = MULTIPLE_4(pqtool_definition_json_len);
	ptToolDefData->pu8Buffer = (CVI_U8 *)calloc(u32BufferSize, sizeof(CVI_U8));
	if (ptToolDefData->pu8Buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate tooljson buffer fail!");
		return CVI_FAILURE;
	}

	ptToolDefData->u32Size = pqtool_definition_json_len;
	memcpy(ptToolDefData->pu8Buffer, pqtool_definition_json, ptToolDefData->u32Size);
	ptToolDefData->eDataType = EBINARYDATA_TOOL_DEFINITION_DATA;

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_PrepareBinaryData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TBinaryData		*ptBinData = NULL;
	JSONObject		*pDataOut = NULL;
	CVI_S32			s32Ret = CVI_SUCCESS;

	CVI_S32			s32ContentID = -1;

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/content", s32ContentID, s32Ret);
	}

	switch (s32ContentID) {
	case EBINARYDATA_AE_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Prepare AE bin");

		if (CVI_ISPD2_UpdateAEBinData(NULL, ptDevInfo) != CVI_SUCCESS) {
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Prepare AE bin data fail");
			return CVI_FAILURE;
		}

		ptBinData = &(ptDevInfo->tAEBinData);
		pDataOut = ISPD2_json_object_new_object();

		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			SET_STRING_TO_JSON("Content type", "AE bin", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_AWB_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Prepare AWB bin");

		if (CVI_ISPD2_UpdateAWBBinData(NULL, ptDevInfo) != CVI_SUCCESS) {
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Prepare AWB bin data fail");
			return CVI_FAILURE;
		}

		ptBinData = &(ptDevInfo->tAWBBinData);
		pDataOut = ISPD2_json_object_new_object();

		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			SET_STRING_TO_JSON("Content type", "AWB bin", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_TOOL_DEFINITION_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Prepare tool definition data");

		if (CVI_ISPD2_PrepareToolDefinitionData(ptDevInfo) != CVI_SUCCESS) {
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Prepare Tool definition data fail");
			return CVI_FAILURE;
		}

		ptBinData = &(ptDevInfo->tBinaryOutData);
		pDataOut = ISPD2_json_object_new_object();

		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			SET_STRING_TO_JSON("Content type", "Tool definition data", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_TUNING_BIN_DATA:
	default:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Un-support content id");
		pDataOut = ISPD2_json_object_new_object();

		SET_STRING_TO_JSON("Content type", "Unknown", pDataOut);
		SET_INT_TO_JSON("Data size", 0, pDataOut);

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetBinaryInfo(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TBinaryData		*ptBinData = NULL;
	JSONObject		*pDataOut = NULL;
	CVI_S32			s32Ret = CVI_SUCCESS;

	CVI_S32			s32ContentID = -1;

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/content", s32ContentID, s32Ret);
	}

	switch (s32ContentID) {
	case EBINARYDATA_AE_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AE bin");
		ptBinData = &(ptDevInfo->tAEBinData);
		pDataOut = ISPD2_json_object_new_object();

		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			SET_STRING_TO_JSON("Content type", "AE bin", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_AWB_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AWB bin");
		ptBinData = &(ptDevInfo->tAWBBinData);
		pDataOut = ISPD2_json_object_new_object();

		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			SET_STRING_TO_JSON("Content type", "AWB bin", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_TUNING_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get tuning bin");
		ptBinData = &(ptDevInfo->tBinaryOutData);
		pDataOut = ISPD2_json_object_new_object();

		if (
			(ptBinData->eDataType == EBINARYDATA_TUNING_BIN_DATA)
			&& (ptBinData->pu8Buffer)
			&& (ptBinData->u32Size > 0)
		) {
			SET_STRING_TO_JSON("Content type", "Tuning bin", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	case EBINARYDATA_TOOL_DEFINITION_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get tool definition data");
		ptBinData = &(ptDevInfo->tBinaryOutData);
		pDataOut = ISPD2_json_object_new_object();

		if (
			(ptBinData->eDataType == EBINARYDATA_TOOL_DEFINITION_DATA)
			&& (ptBinData->pu8Buffer)
			&& (ptBinData->u32Size > 0)
		) {
			SET_STRING_TO_JSON("Content type", "Tool definition data", pDataOut);
			SET_INT_TO_JSON("Data size", ptBinData->u32Size, pDataOut);
		} else {
			SET_STRING_TO_JSON("Content type", "Empty data", pDataOut);
			SET_INT_TO_JSON("Data size", 0, pDataOut);
		}

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;

	default:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Un-support content id");
		pDataOut = ISPD2_json_object_new_object();

		SET_STRING_TO_JSON("Content type", "Unknown", pDataOut);
		SET_INT_TO_JSON("Data size", 0, pDataOut);

		ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
		ptContentOut->s32StatusCode = JSONRPC_CODE_OK;
		break;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetBinaryData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TExportDataInfo	*ptExportDataInfo = &(ptDevInfo->tExportDataInfo);
	TBinaryData		*ptBinData = NULL;
	CVI_S32			s32Ret = CVI_SUCCESS;

	CVI_S32			s32ContentID = -1;

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/content", s32ContentID, s32Ret);
	}

	switch (s32ContentID) {
	case EBINARYDATA_AE_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AE bin");
		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;
		ptBinData = &(ptDevInfo->tAEBinData);

		// prepare binary data
		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			ptExportDataInfo->eContentType = ECONTENTDATA_AE_BIN_DATA;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		}
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_BINARY_AE_BIN;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
		break;

	case EBINARYDATA_AWB_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get AWB bin");
		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;
		ptBinData = &(ptDevInfo->tAWBBinData);

		// prepare binary data
		if ((ptBinData->pu8Buffer) && (ptBinData->u32Size > 0)) {
			ptExportDataInfo->eContentType = ECONTENTDATA_AWB_BIN_DATA;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		}
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_BINARY_AWB_BIN;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
		break;

	case EBINARYDATA_TUNING_BIN_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get tuning bin");
		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;
		ptBinData = &(ptDevInfo->tBinaryOutData);

		// prepare binary data
		if (
			(ptBinData->eDataType == EBINARYDATA_TUNING_BIN_DATA)
			&& (ptBinData->pu8Buffer)
			&& (ptBinData->u32Size > 0)
		) {
			ptExportDataInfo->eContentType = ECONTENTDATA_TUNING_BIN_DATA;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		}
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_BINARY_OUT_DATA_YES;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
		break;

	case EBINARYDATA_TOOL_DEFINITION_DATA:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get tool definition data");
		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;
		ptBinData = &(ptDevInfo->tBinaryOutData);

		// prepare binary data
		if (
			(ptBinData->eDataType == EBINARYDATA_TOOL_DEFINITION_DATA)
			&& (ptBinData->pu8Buffer)
			&& (ptBinData->u32Size > 0)
		) {
			ptExportDataInfo->eContentType = ECONTENTDATA_TOOL_DEFINITION_DATA;
		} else {
			ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		}
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_BINARY_OUT_DATA_YES;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
		break;

	default:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Un-support content id");
		ptContentOut->s32StatusCode = JSONRPC_CODE_BINARY_OUTPUT_MODE;

		ptExportDataInfo->eContentType = ECONTENTDATA_NONE;
		ptExportDataInfo->pvDevInfo = (CVI_VOID *)(ptContentIn->ptDeviceInfo);
		ptExportDataInfo->eReleaseMode = ERELEASE_ALL_DATA_KEEP;

		ptContentOut->ptBinaryOut->pvData = (CVI_VOID *)ptExportDataInfo;
		ptContentOut->ptBinaryOut->bDataValid = CVI_TRUE;
		ptContentOut->ptBinaryOut->eBinaryDataType = 0;
		break;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_SetBinaryData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TBinaryData		*ptBinaryInData = &(ptDevInfo->tBinaryInData);
	CVI_S32			s32Ret = CVI_SUCCESS;

	CVI_S32			s32ContentID = -1;
	CVI_U32			u32DataSize = 0;

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/content", s32ContentID, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/size", u32DataSize, s32Ret);
	}

	if (u32DataSize == 0) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Binary size => 0");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
			"Un-support content id)");
		return CVI_FAILURE;
	}

	s32Ret = CVI_SUCCESS;

	switch (s32ContentID) {
	case EBINARYDATA_TUNING_BIN_DATA:
		// if (ptBinaryInData->u32Size > 0) {
		//	ISP_DAEMON2_DEBUG(LOG_DEBUG, "Previous recv. binary command not finish, reset");
		// }
		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Set to recv. tuning bin (%u)", u32DataSize);

		ptDevInfo->bKeepBinaryInDataInfoOnce = CVI_TRUE;
		ptBinaryInData->eDataType = EBINARYDATA_TUNING_BIN_DATA;
		ptBinaryInData->eDataState = EBINARYSTATE_INITIAL;
		ptBinaryInData->u32Size = u32DataSize;
		ptBinaryInData->u32RecvSize = 0;
		ptBinaryInData->pu8Buffer = CVI_NULL;
		ptBinaryInData->u32BufferSize = u32DataSize;
		break;
	case EBINARYDATA_RAW_DATA:
		s32Ret = CVI_ISPD2_CBFunc_RecvRawReplayDataInfo(ptContentIn, ptContentOut, pJsonResponse);

		ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "set to recv raw data: %u", u32DataSize);

		TRawReplayHandle *pRawReplayHandle = &(ptDevInfo->tRawReplayHandle);

		if (pRawReplayHandle->u32DataSize < u32DataSize) {
			ISP_DAEMON2_DEBUG(LOG_DEBUG, "out of data buffer size");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
				"out of data buffer size.");

			s32Ret = CVI_FAILURE;
		} else {
			ptDevInfo->bKeepBinaryInDataInfoOnce = CVI_TRUE;
			ptBinaryInData->eDataType = EBINARYDATA_RAW_DATA;
			ptBinaryInData->eDataState = EBINARYSTATE_INITIAL;
			ptBinaryInData->u32Size = u32DataSize;
			ptBinaryInData->u32RecvSize = 0;
			ptBinaryInData->pu8Buffer = pRawReplayHandle->data;
			ptBinaryInData->u32BufferSize = pRawReplayHandle->u32DataSize;
		}
		break;
	default:
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Un-support content id");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
			"Un-support content id)");

		s32Ret = CVI_FAILURE;
		break;
	}

	UNUSED(pJsonResponse);

	return s32Ret;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_StartBracketing(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData		*ptFrameData = &(ptDevInfo->tFrameData);
	CVI_S32			s32Ret = CVI_SUCCESS;
	CVI_BOOL		bAE10Raw = CVI_FALSE;

	if (ptContentIn->pParams) {
		int Temp = 0;

		GET_INT_FROM_JSON(ptContentIn->pParams, "/ae10raw", Temp, s32Ret);
		bAE10Raw = (Temp > 0) ? CVI_TRUE : CVI_FALSE;
	}

	if (bAE10Raw) {
		CVI_ISP_AEBracketingSetSimple(CVI_TRUE);
		ptFrameData->bAE10RAWMode = CVI_TRUE;
	} else {
		ptFrameData->bAE10RAWMode = CVI_FALSE;
	}

	if (CVI_ISP_AEBracketingStart(ptDevInfo->s32ViPipe) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"AE Bracketing start fail");
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_FinishBracketing(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo	*ptDevInfo = ptContentIn->ptDeviceInfo;

	CVI_ISP_AEBracketingSetSimple(CVI_FALSE);

	if (CVI_ISP_AEBracketingFinish(ptDevInfo->s32ViPipe) != CVI_SUCCESS) {
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"AE Bracketing finish fail");
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_CBFunc_GetBracketingData(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	TISPDeviceInfo		*ptDevInfo = ptContentIn->ptDeviceInfo;
	TFrameData			*ptFrameData = &(ptDevInfo->tFrameData);
	TGetFrameInfo		*pstFrameInfo = &(ptFrameData->stFrameInfo);
	CVI_S32				s32Ret = CVI_SUCCESS;

	TFrameHeader		stFrameHeader;
	ISP_PUB_ATTR_S		stPubAttr;
	CVI_S32				ViPipe, ViChn;
	CVI_U32				u32CropWidth, u32CropHeight;

	CVI_S16				s16LeEV = 0;
	CVI_S16				s16SeEV = 0;
	CVI_U32				u32TotalFrames = 1;
	CVI_BOOL			bTightlyMode = CVI_FALSE;

	ViPipe = ptDevInfo->s32ViPipe;
	ViChn = ptDevInfo->s32ViChn;

	if (ptContentIn->pParams) {
		// Due to fix tightly mode to FALSE, force frames to 1
		// GET_INT_FROM_JSON(ptContentIn->pParams, "/frames", u32TotalFrames, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/leEV", s16LeEV, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/seEV", s16SeEV, s32Ret);
	}

	CVI_ISPD2_ReleaseFrameData(ptDevInfo);

	if ((!bTightlyMode) && (u32TotalFrames > 1)) {
		ISP_DAEMON2_DEBUG(LOG_WARNING, "RAW multiple frame function only support 1 frame in non-tightly mode");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INVALID_PARAMS,
			"YUV multiple frame function only support 1 frame in non-tightly mode!)");

		return CVI_FAILURE;
	}

	if (CVI_ISP_GetPubAttr(ViPipe, &stPubAttr) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Get Pub Attr fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"ISP Get pub attr fail");

		return CVI_FAILURE;
	}

	ptFrameData->bTightlyMode = bTightlyMode;
	pstFrameInfo->ViPipe = ViPipe;
	pstFrameInfo->ViChn = ViChn;
	u32CropWidth = stPubAttr.stWndRect.u32Width;
	u32CropHeight = stPubAttr.stWndRect.u32Height;

	if (CVI_ISPD2_GetRAWFrameInfo(pstFrameInfo, &stFrameHeader) != CVI_SUCCESS) {
		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW frame info fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get RAW frame info fail");

		return CVI_FAILURE;
	}

	// WDR (fusion frame > 1) raw size is variable, use 1.15 times buffer size
	if (stFrameHeader.fusionFrame > 1) {
		stFrameHeader.size *= 1.15;
		stFrameHeader.size = MULTIPLE_4(stFrameHeader.size);
		u32TotalFrames *= stFrameHeader.fusionFrame;
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Multiple RAW Frames (%u), W/H : %u x %u, Crop W/H : %u x %u, Size : %u\n"
		"Fusion frame = %u, BayerID : %u, Compress : %d",
		u32TotalFrames,
		stFrameHeader.width, stFrameHeader.height, u32CropWidth, u32CropHeight, stFrameHeader.size,
		stFrameHeader.fusionFrame, stFrameHeader.bayerID, stFrameHeader.compress);

	// initialize raw header and buffer
	ptFrameData->u32RawFrameSize = stFrameHeader.size;
	ptFrameData->u32TotalFrames = u32TotalFrames;
	if (CVI_ISPD2_InitRAWFrameBuffer(ptFrameData, &stFrameHeader, u32TotalFrames,
			u32CropWidth, u32CropHeight) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Init RAW frame buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Init RAW frame buffer fail (Failed to allocate memory. Please decrease the number of frame or uncheck the \"Capture raw tightly\" checkbox.)");

		return CVI_FAILURE;
	}

	ISP_DAEMON2_DEBUG_EX(LOG_DEBUG, "Bracketing exposure leEV = %d, seEV = %d",
		s16LeEV, s16SeEV);

	CVI_ISP_AEBracketingSetExpsoure(ViPipe, s16LeEV, s16SeEV);
	usleep(200*1000);

	// Get Reg dump
	if (!ptFrameData->bAE10RAWMode) {
		if (CVI_ISPD2_UpdateRegDump(ptFrameData) != CVI_SUCCESS) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);

			ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get Reg dump fail");
			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Get Reg dump fail");

			return CVI_FAILURE;
		}
	}

	if (CVI_ISPD2_GetMultiplyRAW(ptFrameData, u32TotalFrames) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW frames fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get RAW frames fail");

		return CVI_FAILURE;
	}

	// Get RAW header
	if (CVI_ISPD2_UpdateRAWHeader(ptFrameData) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "CVI_ISPD2_UpdateRAWHeader fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Update RAW Header fail");

		return CVI_FAILURE;
	}

	// Get RAW info.
	if (CVI_ISPD2_UpdateRAWInfo(ptFrameData) != CVI_SUCCESS) {
		CVI_ISPD2_ReleaseFrameData(ptDevInfo);

		ISP_DAEMON2_DEBUG(LOG_DEBUG, "Get RAW info fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Get RAW info fail");

		return CVI_FAILURE;
	}

	// Get 3A data
	if (!ptFrameData->bAE10RAWMode) {
		if (CVI_ISPD2_Update3AData(ptFrameData, ptDevInfo) != CVI_SUCCESS) {
			CVI_ISPD2_ReleaseFrameData(ptDevInfo);

			CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
				"Get 3A data fail");

			return CVI_FAILURE;
		}
	}

	ptFrameData->u32MemOffset = 0;
	ptFrameData->u32CurFrame = 0;

	// compose JSON data
	JSONObject	*pDataOut = ISPD2_json_object_new_object();
	TRAWHeader	*ptRAWHeader = ptFrameData->pstRAWHeader;
	CVI_U32		u32DataSize = sizeof(TRAWHeader) + ptRAWHeader->size;

	SET_STRING_TO_JSON("Content type", "Bracketing RAW multiple frames", pDataOut);
	SET_INT_TO_JSON("Frame count", u32TotalFrames, pDataOut);
	SET_INT_TO_JSON("Data size", u32DataSize, pDataOut);

	SET_STRING_TO_JSON("Raw info log", (char *)ptFrameData->pu8RawInfo, pDataOut);

	if (!ptFrameData->bAE10RAWMode) {
		SET_STRING_TO_JSON("Reg dump log", (char *)ptFrameData->pu8RegDump, pDataOut);
		SET_STRING_TO_JSON("AE log", (char *)ptFrameData->pu8AELogBuffer, pDataOut);
		SET_STRING_TO_JSON("AWB log", (char *)ptFrameData->pu8AWBLogBuffer, pDataOut);
	}

	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

static CVI_S32 CVI_ISPD2_WriteI2C(I2C_DEVICE device, uint8_t *buffer)
{
	CVI_S32 s32Ret = 0;

	if (buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "buffer is NULL\n");
		return -1;
	}

	if ((device.addrBytes == 0) || (device.addrBytes >= 3)) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR,
			"addrBytes[%d] should be greater than 0 and less than 3\n",
			device.addrBytes);
		return -1;
	}

	s32Ret = i2cInit(device.devId, device.devAddr);
	if (s32Ret != 0) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "open i2c bus[%d] fail\n", device.devId);
		return s32Ret;
	}

	s32Ret = i2cWrite(device.startAddr, device.addrBytes, buffer, device.length);
	if (s32Ret != 0) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "i2cWrite fail\n");
	}

	i2cExit();
	return s32Ret;
}

static CVI_S32 CVI_ISPD2_ReadI2C(I2C_DEVICE device, uint8_t *buffer)
{
	CVI_S32 s32Ret = 0;

	if (buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "buffer is NULL\n");
		return -1;
	}

	if ((device.addrBytes < 1) || (device.addrBytes > 2)) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "addrBytes[%d] should be [1, 2]\n", device.addrBytes);
		return -1;
	}

	s32Ret = i2cInit(device.devId, device.devAddr);
	if (s32Ret != 0) {
		ISP_DAEMON2_DEBUG_EX(LOG_ERR, "open i2c bus[%d] fail\n", device.devId);
		return s32Ret;
	}

	s32Ret = i2cRead(device.startAddr, device.addrBytes, buffer, device.length);
	if (s32Ret != 0) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "i2cRead fail\n");
	}

	i2cExit();
	return s32Ret;
}

CVI_S32 CVI_ISPD2_CBFunc_I2cRead(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	I2C_DEVICE device = {0};

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Device ID", device.devId, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Device Address", device.devAddr, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Address Bytes", device.addrBytes, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Start", device.startAddr, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Length", device.length, s32Ret);
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Device info, devid:%d, devAddr:%d\n"
		"addrBytes:%d, startAddr:%d, length:%d",
		device.devId, device.devAddr, device.addrBytes, device.startAddr, device.length);

	CVI_U8 *buffer = (CVI_U8 *)calloc(1, device.length);

	if (buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate i2c buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Allocate i2c buffer fail");
		return CVI_FAILURE;
	}

	if (CVI_ISPD2_ReadI2C(device, buffer) != CVI_SUCCESS) {
		SAFE_FREE(buffer);
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"i2c read fail");
		return CVI_FAILURE;
	}


	JSONObject *pDataOut = ISPD2_json_object_new_object();

	SET_INT_ARRAY_TO_JSON("I2C Data", device.length, buffer, pDataOut);
	ISPD2_json_object_object_add(pJsonResponse, GET_RESPONSE_KEY_NAME, pDataOut);
	ptContentOut->s32StatusCode = JSONRPC_CODE_OK;

	SAFE_FREE(buffer);
	UNUSED(s32Ret);

	return CVI_SUCCESS;
}

CVI_S32 CVI_ISPD2_CBFunc_I2cWrite(TJSONRpcContentIn *ptContentIn,
	TJSONRpcContentOut *ptContentOut, JSONObject *pJsonResponse)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	I2C_DEVICE device = {0};

	if (ptContentIn->pParams) {
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Device ID", device.devId, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Device Address", device.devAddr, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Address Bytes", device.addrBytes, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Start", device.startAddr, s32Ret);
		GET_INT_FROM_JSON(ptContentIn->pParams, "/Length", device.length, s32Ret);
	}

	ISP_DAEMON2_DEBUG_EX(LOG_INFO, "Device info, devid:%d, devAddr:%d\n"
		"addrBytes:%d, startAddr:%d, length:%d",
		device.devId, device.devAddr, device.addrBytes, device.startAddr, device.length);

	CVI_U8 *buffer = (CVI_U8 *)calloc(1, device.length);

	if (buffer == NULL) {
		ISP_DAEMON2_DEBUG(LOG_ERR, "Allocate i2c buffer fail");
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"Allocate i2c buffer fail");
		return CVI_FAILURE;
	}

	GET_INT_ARRAY_FROM_JSON(ptContentIn->pParams, "/I2C Data", device.length, CVI_U8, buffer, s32Ret)

	if (CVI_ISPD2_WriteI2C(device, buffer) != CVI_SUCCESS) {
		SAFE_FREE(buffer);
		CVI_ISPD2_Utils_ComposeMessage(ptContentOut, JSONRPC_CODE_INTERNAL_API_ERROR,
			"i2c write fail");
		return CVI_FAILURE;
	}

	UNUSED(pJsonResponse);
	UNUSED(s32Ret);
	SAFE_FREE(buffer);

	return CVI_SUCCESS;
}

// -----------------------------------------------------------------------------
