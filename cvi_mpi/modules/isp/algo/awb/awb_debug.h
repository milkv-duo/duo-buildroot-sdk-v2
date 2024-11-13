/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: awb_debug.h
 * Description:
 *
 */

#ifndef _AWB_DEBUG_H_
#define _AWB_DEBUG_H_

#include "cvi_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void AWB_SetDebugMode(CVI_U8 mode);
CVI_U8 AWB_GetDebugMode(void);
void AWB_Debug(void);
void AWB_ShowInfoList(void);
void CVI_AWB_AutoTest(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AWB_DEBUG_H_
