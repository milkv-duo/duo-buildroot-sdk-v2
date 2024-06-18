/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: 3A_internal.h
 * Description:
 *
 */

#ifndef _3A_INTERNAL_H_
#define _3A_INTERNAL_H_

#include "cvi_comm_3a.h"
#include "cvi_awb_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/* AE */
void CVI_AE_GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 mode,
	CVI_U32 w, CVI_U32 h, CVI_U32 nRawBlc);
void CVI_AE_SetAeSimMode(CVI_BOOL bMode);
CVI_BOOL CVI_AE_IsAeSimMode(void);
CVI_S32 CVI_ISP_GetAELogBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize);
CVI_S32 CVI_ISP_GetAELogBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize);
CVI_S32 CVI_ISP_GetAEBinBuf(VI_PIPE ViPipe, CVI_U8 *pBuf, CVI_U32 bufSize);
CVI_S32 CVI_ISP_GetAEBinBufSize(VI_PIPE ViPipe, CVI_U32 *bufSize);

CVI_S32 CVI_ISP_AEBracketingStart(VI_PIPE ViPipe);
CVI_S32 CVI_ISP_AEBracketingSetExpsoure(VI_PIPE ViPipe, CVI_S16 leEvX10, CVI_S16 seEvX10);
CVI_S32 CVI_ISP_AEBracketingSetSimple(CVI_BOOL bEnable);
CVI_S32 CVI_ISP_AEBracketingFinish(VI_PIPE ViPipe);

CVI_S32 CVI_ISP_SyncSensorCfg(VI_PIPE ViPipe);

CVI_S32 CVI_ISP_AESetRawDumpFrameID(VI_PIPE ViPipe, CVI_U32 fid, CVI_U16 frmNum);
CVI_S32 CVI_ISP_AEGetRawReplayExpBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 *bufSize);
void CVI_ISP_AESetRawReplayMode(VI_PIPE ViPipe, CVI_BOOL bMode);
CVI_S32 CVI_ISP_AESetRawReplayExposure(VI_PIPE ViPipe, const ISP_EXP_INFO_S *pstExpInfo);
CVI_S32 CVI_ISP_GetAERawReplayFrmNum(VI_PIPE ViPipe, CVI_U8 *bootfrmNum, CVI_U8 *ispDgainPeriodNum);


/* AWB */
void CVI_ISP_SetAwbSimMode(CVI_BOOL bMode);
CVI_BOOL CVI_ISP_GetAwbSimMode(void);
CVI_S32 CVI_ISP_GetAWBSnapLogBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize);
CVI_S32 CVI_ISP_GetAWBDbgBinBuf(VI_PIPE ViPipe, CVI_U8 *buf, CVI_U32 bufSize);
CVI_S32 CVI_ISP_GetAWBDbgBinSize(void);
CVI_S32 CVI_ISP_SetWBCalibration(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S *pstWBCalib);
CVI_S32 CVI_ISP_GetWBCalibration(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S *pstWBCalib);
CVI_S32 CVI_ISP_SetWBCalibrationEx(VI_PIPE ViPipe, const ISP_AWB_Calibration_Gain_S_EX *pstWBCalib);
CVI_S32 CVI_ISP_GetWBCalibrationEx(VI_PIPE ViPipe, ISP_AWB_Calibration_Gain_S_EX *pstWBCalib);
CVI_S32 CVI_ISP_GetAWBCurve(VI_PIPE ViPipe, ISP_WB_CURVE_S *pshWBCurve);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _3A_INTERNAL_H_
