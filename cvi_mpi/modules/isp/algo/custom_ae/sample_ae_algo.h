/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_ae_algo.h
 * Description:
 *
 */

#ifndef _SAMPLE_AE_ALGO_H_
#define _SAMPLE_AE_ALGO_H_

#include "cvi_comm_3a.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef unsigned char           CVI_U8;
typedef unsigned short          CVI_U16;
typedef unsigned int            CVI_U32;

typedef enum _AE_WDR_FRAME {
	AE_LE = 0,
	AE_SE,
	AE_MAX_WDR_FRAME_NUM,
} AE_WDR_FRAME;

#define AE_GAIN_BASE (1024)
#define AE_WDR_RATIO_BASE (64)

#define AE_MAX_LUMA (1023)
#define WDR_LE_SE_RATIO	(4.0)


#define HIST_BIN_SIZE (256)

typedef struct _AEINFO_S {
	CVI_U16 au16ZoneAvg[AE_ZONE_ROW][AE_ZONE_COLUMN][4];
} AEINFO_S;

typedef struct _SAMPLE_AE_ALGO_S {
	CVI_U8  frmNum;
	CVI_U8  winXNum;
	CVI_U8  winYNum;
	ISP_FE_AE_STAT_3_S *pstFEAeStat3;
} SAMPLE_AE_ALGO_S;

typedef struct _SAMPLE_AE_CTRL_S {
	CVI_U32 seExp;
	CVI_U32 seGain;
	CVI_U32 leExp;
	CVI_U32 leGain;
	CVI_U16 rWb_Gain;
	CVI_U16 bWb_Gain;
	CVI_U8 bIsStable;
} SAMPLE_AE_CTRL_S;


void SAMPLE_AE_Algo(SAMPLE_AE_ALGO_S *pInfo, SAMPLE_AE_CTRL_S *pCtr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _SAMPLE_AE_ALGO_H_ */
