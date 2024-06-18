/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs.h
 * Description:
 */

#ifndef _CVI_ISPD2_CALLBACK_FUNCS_H_
#define _CVI_ISPD2_CALLBACK_FUNCS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cvi_type.h"
#include "cvi_ispd2_define.h"


// -----------------------------------------------------------------------------
// Daemon (System)
CVI_S32 CVI_ISPD2_CBFunc_GetVersionInfo(TJSONRpcContentIn * ptIn,
	TJSONRpcContentOut * ptOut, JSONObject * pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_SetTopInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetTopInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetDRCHistogramInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetChipInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetWDRInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_Get3AStatisticsInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetAFStatisticsInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ExportBinFile(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_FixBinToFlash(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_SetLogLevelInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// Daemon Applications
CVI_S32 CVI_ISPD2_CBFunc_GetSingleYUVFrame(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetMultipleYUVFrames(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetMultipleRAWFrames(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetImageInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetImageData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_StopGetImageData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetBinaryInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_PrepareBinaryData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetBinaryData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_StartBracketing(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_FinishBracketing(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_GetBracketingData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_SetBinaryData(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

CVI_S32 CVI_ISPD2_CBFunc_ImportBinFile(TBinaryData *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

CVI_S32 CVI_ISPD2_CBFunc_RecvRawReplayDataInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_SendRawReplayData(TISPDeviceInfo *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_StartRawReplay(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_CancelRawReplay(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_I2cRead(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_I2cWrite(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// ISP
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDehazeAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDehazeAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetColorToneAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetColorToneAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetGammaAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetGammaAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAutoGammaAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAutoGammaAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetGammaCurveByType(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCNRMotionNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCNRMotionNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNRFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNRFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetRLSCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetRLSCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRMotionNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRMotionNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYNRFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYNRFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRNoiseModelAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRNoiseModelAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRLumaMotionAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRLumaMotionAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRGhostAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRGhostAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRMtPrtAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRMtPrtAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetTNRMotionAdaptAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetTNRMotionAdaptAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDCIAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDCIAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetLDCIAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetLDCIAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDemosaicAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDemosaicAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDemosaicDemoireAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDemosaicDemoireAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDemosaicFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDemosaicFilterAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMeshShadingAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMeshShadingAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMeshShadingGainLutAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMeshShadingGainLutAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetBlackLevelAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetBlackLevelAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCCMAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCCMAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetFSWDRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetFSWDRAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDRCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDRCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDPDynamicAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDPDynamicAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDPStaticAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDPStaticAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCrosstalkAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCrosstalkAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSharpenAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSharpenAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetPreSharpenAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetPreSharpenAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetYContrastAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetYContrastAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCCMSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCCMSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetNoiseProfileAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetNoiseProfileAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDisConfig(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDisConfig(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetMonoAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetMonoAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetRGBCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetRGBCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetLCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetLCACAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetClutAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetClutAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetClutSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetClutSaturationAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetVCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetVCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCSCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCSCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCAAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCAAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetCA2Attr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetCA2Attr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetStatisticsConfig(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetStatisticsConfig(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

CVI_S32 CVI_ISPD2_CBFunc_ISP_SetPubAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetPubAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// AE
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWDRExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWDRExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetSmartExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetSmartExposureAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteSFAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteSFAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAERouteSFAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAERouteSFAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_QueryExposureInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetIrisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetIrisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetDCIrisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetDCIrisAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// AWB
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWBAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWBAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetAWBAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetAWBAttrEx(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_SetWBCalibration(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_GetWBCalibration(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_ISP_QueryWBInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// VI
CVI_S32 CVI_ISPD2_CBFunc_VI_SetChnLDCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VI_GetChnLDCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// VPSS
CVI_S32 CVI_ISPD2_CBFunc_VPSS_SetChnLDCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VPSS_GetChnLDCAttr(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VPSS_SetGrpProcAmp(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VPSS_GetGrpProcAmp(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// VO
CVI_S32 CVI_ISPD2_CBFunc_VO_SetGammaInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VO_GetGammaInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

// VC
CVI_S32 CVI_ISPD2_CBFunc_VC_SetCodingParamInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_GetCodingParamInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_SetGopModeInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_GetGopModeInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_SetRCAttrInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_GetRCAttrInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_SetRCParamInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);
CVI_S32 CVI_ISPD2_CBFunc_VC_GetRCParamInfo(TJSONRpcContentIn *ptIn,
	TJSONRpcContentOut *ptOut, JSONObject *pJsonRes);

#endif // _CVI_ISPD2_CALLBACK_FUNCS_H_
