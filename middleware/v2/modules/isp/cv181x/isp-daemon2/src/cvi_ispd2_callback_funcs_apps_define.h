/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps_define.h
 * Description:
 */

#ifndef _CVI_ISPD2_CALLBACK_FUNCS_APPS_DEFINE_H_
#define _CVI_ISPD2_CALLBACK_FUNCS_APPS_DEFINE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cvi_type.h"
#include "cvi_comm_vb.h"

#if defined(CHIP_ARCH_CV183X) || defined(CHIP_ARCH_CV182X)
#include "cvi_defines.h"
#include "cvi_comm_video.h"
#elif defined(__CV181X__) || defined(__CV180X__)
#include "linux/cvi_defines.h"
#include "linux/cvi_comm_video.h"
#endif // CHIP_ARCH

#define MAX_WDR_FUSION_FRAMES		(4)

// -----------------------------------------------------------------------------
typedef enum {
	IMAGE_FROM_VI,
	IMAGE_FROM_VPSS
} EImageSource;

typedef struct {
	VI_PIPE			ViPipe;
	VI_CHN			ViChn;
	VPSS_GRP		VpssGrp;
	VPSS_CHN		VpssChn;
	EImageSource	eImageSrc;
	CVI_BOOL		bGetRawReplayYuv;
	CVI_U8			u8GetRawReplayYuvId;
	CVI_U8			*pau8FrameAddr[MAX_WDR_FUSION_FRAMES];
	CVI_U32			u32FrameSize[MAX_WDR_FUSION_FRAMES];
} TGetFrameInfo;

typedef struct {
	CVI_U8		numFrame;
	CVI_U16		width;
	CVI_U16		height;
	CVI_U16		stride[3];
	CVI_U8		pixelFormat;
	CVI_U8		curFrame;
	CVI_U32		size;
} TYUVHeader;

typedef struct {
	CVI_U8		numFrame;
	CVI_U16		width;
	CVI_U16		height;
	CVI_U16		stride[3];
	CVI_U8		curFrame;
	CVI_U8		bayerID;
	CVI_U8		compress;
	CVI_U16		cropX;
	CVI_U16		cropY;
	CVI_U8		fusionFrame;
	CVI_U32		exposure[4];
	CVI_U32		ispDGain;
	CVI_U32		iso;
	CVI_U32		colorTemp;
	CVI_U16		wbRGain;
	CVI_U16		wbBGain;
	CVI_S16		ccm[9];
	CVI_U16		blcOffset[4];
	CVI_U16		blcGain[4];
	CVI_U32		exposureRatio;
	CVI_U32		exposureAGain;
	CVI_U32		exposureDGain;
	CVI_U16		cropWidth;
	CVI_U16		cropHeight;
	CVI_U32		size;
} TRAWHeader;

typedef struct {
	TYUVHeader			*pstYUVHeader;		// YUV only
	TRAWHeader			*pstRAWHeader;		// RAW only
	CVI_U8				*pu8ImageBuffer;
	CVI_U8				*pu8AELogBuffer;	// RAW only
	CVI_U8				*pu8AWBLogBuffer;	// RAW only
	CVI_U8				*pu8RawInfo;		// RAW only
	CVI_U8				*pu8RegDump;		// RAW only
	CVI_U32				u32YUVFrameSize;	// YUV only
	CVI_U32				u32RawFrameSize;	// RAW only
	CVI_U32				u32CurFrame;
	CVI_U32				u32TotalFrames;
	CVI_U32				u32MemOffset;
	VB_POOL				getRawVbPoolID;		// RAW only
	VB_BLK				getRawVbBlk;		// RAW only

	CVI_BOOL			bTightlyMode;
	VIDEO_FRAME_INFO_S	astVideoFrame[MAX_WDR_FUSION_FRAMES];
	VIDEO_FRAME_INFO_S	astVpssVideoFrame[MAX_WDR_FUSION_FRAMES];	// RAW only
	TGetFrameInfo		stFrameInfo;			// non-tightly mode

	CVI_BOOL			onRawReplay;
	CVI_BOOL			bAE10RAWMode;
} TFrameData;

typedef enum {
	EBINARYDATA_NONE					= 0,
	EBINARYDATA_AE_BIN_DATA				= 1,
	EBINARYDATA_AWB_BIN_DATA			= 2,
	EBINARYDATA_TUNING_BIN_DATA			= 3,
	EBINARYDATA_RAW_DATA				= 4,
	EBINARYDATA_TOOL_DEFINITION_DATA	= 5
} EBinaryDataType;

typedef enum {
	EBINARYSTATE_DEFAULT	= 0,
	EBINARYSTATE_INITIAL	= 1,
	EBINARYSTATE_RECV		= 2,
	EBINARYSTATE_DONE		= 3,
	EBINARYSTATE_ERROR		= 4
} EBinaryDataState;

typedef struct {
	EBinaryDataType		eDataType;
	EBinaryDataState	eDataState;
	CVI_U32				u32Size;
	CVI_U32				u32RecvSize;
	CVI_U8				*pu8Buffer;
	CVI_U32				u32BufferSize;
} TBinaryData;

typedef enum {
	ECONTENTDATA_NONE,
	ECONTENTDATA_SINGLE_YUV_FRAME,
	ECONTENTDATA_MULTIPLE_YUV_FRAMES,
	ECONTENTDATA_MULTIPLE_RAW_FRAMES,
	ECONTENTDATA_AE_BIN_DATA,
	ECONTENTDATA_AWB_BIN_DATA,
	ECONTENTDATA_TUNING_BIN_DATA,
	ECONTENTDATA_TOOL_DEFINITION_DATA
} EBinaryContentType;

typedef enum {
	ERELEASE_ALL_DATA_KEEP				= 0,
	ERELEASE_FRAME_DATA_YES				= (1 << 0),
	ERELEASE_FRAME_DATA_CHECK_FRAMES	= (1 << 1),
	ERELEASE_BINARY_OUT_DATA_YES		= (1 << 10),
	ERELEASE_BINARY_AE_BIN				= (1 << 11),
	ERELEASE_BINARY_AWB_BIN				= (1 << 12)
} EReleaseDataMode;

typedef struct {
	EBinaryContentType		eContentType;
	CVI_VOID				*pvDevInfo;
	EReleaseDataMode		eReleaseMode;
} TExportDataInfo;

typedef struct {
	void *pSoHandle;

	CVI_S32 (*set_raw_replay_data)(const CVI_VOID *header, const CVI_VOID *data,
		CVI_U32 totalFrame, CVI_U32 curFrame, CVI_U32 rawFrameSize);
	CVI_S32 (*start_raw_replay)(VI_PIPE ViPipe);
	CVI_S32 (*stop_raw_replay)();
	CVI_S32 (*get_raw_replay_yuv)(CVI_U8 yuvIndex, CVI_U8 src, CVI_S32 pipeOrGrp,
		CVI_S32 chn, VIDEO_FRAME_INFO_S *pstFrameInfo);

	CVI_VOID *header;
	CVI_VOID *data;
	CVI_U32 u32DataSize;
} TRawReplayHandle;

typedef struct _I2C_DEVICE {
	CVI_U8 devId;
	CVI_U8 devAddr;
	CVI_U8 addrBytes;
	CVI_U16 startAddr;
	CVI_U16 length;
} I2C_DEVICE;
#endif // _CVI_ISPD2_CALLBACK_FUNCS_APPS_DEFINE_H_
