/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_debug.h
 * Description:
 *
 */

#ifndef _AE_DEBUG_H_
#define _AE_DEBUG_H_

#include "cvi_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum _AE_TEST_ITEM {
	AE_TEST_SHUTTER,
	AE_TEST_ISO_NUM,
	AE_TEST_GAIN,
	AE_TEST_TOTAL
} AE_TEST_ITEM;

typedef struct _sEV_S {
	CVI_S16 tv;
	CVI_S16 sv;
} sEV_S;

typedef struct _AEB_RAW_HEADER {
	CVI_U16 width;
	CVI_U16 height;
	CVI_U16 cropX;
	CVI_U16 cropY;
	CVI_U32 perLineByte;
} AEB_RAW_HEADER;

#define AE_TEST_RAW_NUM	(10)
#define AE_TEST_RAW_AWB	(99)

#define SENSOR_PORTING_START_ITEM	(66)
#define SENSOR_PORTING_END_ITEM		(81)

#define AE_DEBUG_ENABLE_LOG_TO_FILE  200
#define AE_DEBUG_DISABLE_LOG_TO_FILE 201

extern sEV_S aeb_ev[AE_TEST_RAW_NUM];


void AE_Debug(void);
void AE_ShowDebugInfo(CVI_U8 sID);
void AE_HLprintf(const char *szFmt, ...);
void CVI_AE_AutoTest(VI_PIPE ViPipe);
void AE_CviTestAEB_Raw(void);
void AE_SetParaList(void);
void AE_ShowInfoList(void);
void AE_ShowDebugInfo(CVI_U8 sID);
void AE_GetLSC(CVI_U8 sID, CVI_BOOL *enableLSC);
void AE_SetLSC(CVI_U8 sID, CVI_BOOL enableLSC);
void AE_SetManual(CVI_U8 sID, CVI_BOOL mode, CVI_U8 debugMode);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AE_DEBUG_H_
