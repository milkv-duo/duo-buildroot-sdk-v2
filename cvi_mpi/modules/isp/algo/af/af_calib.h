/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: af_calib.h
 * Description:
 *
 */

#ifndef _AF_CALIB_H_
#define _AF_CALIB_H_

#include <stdlib.h>

#include "cvi_comm_3a.h"
#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define AF_SENSOR_NUM (VI_MAX_PIPE_NUM)
#define AAA_ABS(a) ((a) > 0 ? (a) : -(a))
#define AAA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AAA_MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct _AF_CALIB_CTX_S {
	//common param
	CVI_U32 u32ZoomOffsetCnt;
	CVI_U32 u32FocusOffsetCnt;
	CVI_U32 u32ZoomResetCnt;
	CVI_U32 u32FocusResetCnt;
	CVI_U16 u16MaxZoomStep;
	CVI_U16 u16MaxFocusStep;
	CVI_U32 u32zoomRange;
	CVI_U32 u32focusRange;
	CVI_U32 u32ZoomPos;
	CVI_U32 u32FocusPos;
	CVI_U16 u16FocusRevStep;
	CVI_U16 u16ZoomRevStep;
	CVI_U16 zoomBacklash;
	CVI_U16 focusBacklash;
	AF_DIRECTION eZoomDir;
	AF_DIRECTION eFocusDir;
	CVI_BOOL bInitZoomPos;
	CVI_BOOL bInitFocusPos;
	CVI_BOOL bPrintSetInfo;
} AF_CALIB_CTX_S;

typedef enum _AF_CTL_TYPE {
	AF_CTL_FOCUS,
	AF_CTL_ZOOM,
} AF_CTL_TYPE;

CVI_S32 AF_ENTER_CAILB_FLOW(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AF_CALIB_H_
