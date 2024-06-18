/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_sts_ref.h
 * Description:
 *
 */

#ifndef _ISP_STS_REF_H_
#define _ISP_STS_REF_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _ISP_DIS_STATS_INFO {
	CVI_U32 frameCnt;
	CVI_U16  histX[DIS_MAX_WINDOW_X_NUM][DIS_MAX_WINDOW_Y_NUM][XHIST_LENGTH];
	CVI_U16  histY[DIS_MAX_WINDOW_X_NUM][DIS_MAX_WINDOW_Y_NUM][YHIST_LENGTH];
} ISP_DIS_STATS_INFO;

typedef struct _ISP_DCI_STATISTICS_S {
	CVI_U16 hist[DCI_BINS_NUM];
} ISP_DCI_STATISTICS_S;

typedef struct _ISP_MOTION_STATS_INFO {
	CVI_U32 frameCnt;
	CVI_U8 gridWidth;
	CVI_U8 gridHeight;
	ISP_U8_PTR motionStsData;
	CVI_U32 motionStsBufSize;
} ISP_MOTION_STATS_INFO;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_STS_REF_H_
