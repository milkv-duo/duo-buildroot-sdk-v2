/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ipc.h
 * Description:
 *
 */

#ifndef _ISP_IPC_H_
#define _ISP_IPC_H_

#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _ISP_AE_DEBUG_FUNC_S {
	CVI_S32 (*pfn_ae_dumplog)(void);
	CVI_S32 (*pfn_ae_setdumplogpath)(const char *szPath);
} ISP_AE_DEBUG_FUNC_S;

CVI_S32 CVI_ISP_AELibRegInternalCallBack(VI_PIPE ViPipe, ISP_AE_DEBUG_FUNC_S *pstRegister);
CVI_S32 CVI_ISP_AELibUnRegInternalCallBack(VI_PIPE ViPipe);

typedef struct _ISP_AWB_DEBUG_FUNC_S {
	CVI_S32 (*pfn_awb_dumplog)(CVI_U8 sID);
	CVI_S32 (*pfn_awb_setdumplogpath)(const char *szPath);
} ISP_AWB_DEBUG_FUNC_S;

CVI_S32 CVI_ISP_AWBLibRegInternalCallBack(VI_PIPE ViPipe, ISP_AWB_DEBUG_FUNC_S *pstRegister);
CVI_S32 CVI_ISP_AWBLibUnRegInternalCallBack(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_IPC_H_
