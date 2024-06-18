/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps_local.h
 * Description:
 */

#ifndef _CVI_ISPD2_CALLBACK_FUNCS_APPS_LOCAL_H_
#define _CVI_ISPD2_CALLBACK_FUNCS_APPS_LOCAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cvi_type.h"

#include "cvi_ispd2_callback_funcs_apps.h"

// -----------------------------------------------------------------------------
#define RAW_INFO_BUFFER_SIZE			(48 * 1024)		// 48K
#define REG_DUMP_BUFFER_SIZE			(240 * 1024)	// 240K
#define SEND_DATA_SIZE					(256 * 1024)	// 256K
#define MAX_GET_FRAME_TIMEOUT			(1000)

#define RAW_FRAME_TILE_MODE_WIDTH		(2304)

#define REG_DUMP_TEMP_LOCATION			"/tmp/hwRegDump.json"
#define RAW_INFO_TEMP_LOCATION			"/tmp/frameRawInfo.txt"

// -----------------------------------------------------------------------------
#define MULTIPLE_4(value)				(((value) + 3) >> 2 << 2)

// -----------------------------------------------------------------------------
typedef struct {
	CVI_S32			s32Brightness;
	CVI_S32			s32Contrast;
	CVI_S32			s32Saturation;
	CVI_S32			s32Hue;
} TVPSSAdjGroupInfo;

typedef struct {
	CVI_U16		width;
	CVI_U16		height;
	CVI_U16		stride[3];
	CVI_U32		size;
	CVI_U8		pixelFormat;
	CVI_U8		bayerID;
	CVI_U8		compress;
	CVI_U8		fusionFrame;
} TFrameHeader;

#endif // _CVI_ISPD2_CALLBACK_FUNCS_APPS_LOCAL_H_
