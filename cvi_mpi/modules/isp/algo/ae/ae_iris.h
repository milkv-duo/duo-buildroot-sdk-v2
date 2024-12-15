/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: ae_iris.h
 * Description:
 *
 */

#ifndef _AE_IRIS_H_
#define _AE_IRIS_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

void AE_IrisAttr_Init(CVI_U8 sID);
CVI_S32 AE_DCIrisPwm_Init(VI_PIPE ViPipe);
CVI_S32 AE_SetDCIrisPwmCtrlPath(const char *szPath);
void AE_RunDCIris(VI_PIPE ViPipe, ISP_OP_TYPE_E aeMode);
void AE_UpdateIrisAttr(CVI_U8 sID);
void AE_UpdateDcIrisAttr(VI_PIPE ViPipe);
CVI_BOOL AE_IsDCIris(CVI_U8 sID);
CVI_BOOL AE_IsUseDcIris(CVI_U8 sID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AE_IRIS_H_
