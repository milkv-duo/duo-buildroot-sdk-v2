/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_detect_flicker.h
 * Description:
 *
 */

#ifndef _AE_DETECT_FLICKER_H_
#define _AE_DETECT_FLICKER_H_

#define FLICK_FRAME_NEED (8)
#define FLICK_V_BIN (8)


typedef enum _FLICKER_STATUS {
	FCLICKER_NONE,
	FCLICKER_YES,
	FCLICKER_NO,
} FLICKER_STATUS;

typedef struct _S_AE_DET_FLICKER {
	CVI_BOOL bIsEnable;
	CVI_U32 VLuma[FLICK_FRAME_NEED][8];
	CVI_U8 inx;
	FLICKER_STATUS FlickerStatus;
	CVI_U8 fps;
	CVI_BOOL bIsDbgWrite;
	CVI_BOOL bIsDbgLog;
	CVI_U8 thres50Hz[4];
	CVI_U8 thres60Hz[4];
	FILE *fidDbg;
} S_AE_DET_FLICKER;




void AE_EnableFlickerDetect(CVI_U8 sID, CVI_U8 ena);
void AE_DoFlickDetect(CVI_U8 sID, CVI_S16 expinx);


#endif	//_AE_DETECT_FLICKER_H_
