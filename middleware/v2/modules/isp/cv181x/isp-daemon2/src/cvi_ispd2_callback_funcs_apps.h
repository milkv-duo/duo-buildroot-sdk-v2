/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_callback_funcs_apps.h
 * Description:
 */

#ifndef _CVI_ISPD2_CALLBACK_FUNCS_APPS_H_
#define _CVI_ISPD2_CALLBACK_FUNCS_APPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "cvi_type.h"
#include "cvi_ispd2_define.h"

#include "cvi_ispd2_callback_funcs_apps_define.h"

// -----------------------------------------------------------------------------
CVI_S32 CVI_ISPD2_InitialFrameData(TFrameData *ptData);
CVI_S32 CVI_ISPD2_ReleaseFrameData(TISPDeviceInfo *ptDevInfo);

CVI_S32 CVI_ISPD2_InitialBinaryData(TBinaryData *ptBinaryOut);
CVI_S32 CVI_ISPD2_ReleaseBinaryData(TBinaryData *ptBinaryOut);

CVI_S32 CVI_ISPD2_InitialRawReplayHandle(TRawReplayHandle *ptHandle);
CVI_S32 CVI_ISPD2_ReleaseRawReplayHandle(TRawReplayHandle *ptHandle);
CVI_S32 CVI_ISPD2_GetRawReplayYuv(TRawReplayHandle *ptHandle,  CVI_U8 yuvIndex,
	CVI_U8 src, CVI_S32 pipeOrGrp, CVI_S32 chn, VIDEO_FRAME_INFO_S *pstFrameInfo);

CVI_S32 CVI_ISPD2_ResetBinaryInStructure(TBinaryData *ptBinaryIn);

CVI_S32 CVI_ISPD2_CBFunc_Handle_BinaryOut(int iFd, TJSONRpcBinaryOut *ptBinaryInfo);

#endif // _CVI_ISPD2_CALLBACK_FUNCS_APPS_H_
