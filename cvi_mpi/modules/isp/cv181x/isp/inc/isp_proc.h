/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_proc.h
 * Description:
 *
 */

#ifndef _ISP_PROC_H_
#define _ISP_PROC_H_

#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef ARCH_RTOS_CV181X
//#define ENABLE_ISP_PROC_DEBUG 1
#endif

#ifdef ENABLE_ISP_PROC_DEBUG

typedef void (*ISP_GetAAAVer_FUNC)(CVI_U16 *pVer, CVI_U16 *pSubVer);

typedef enum _ISP_AAA_TYPE_E {
	ISP_AAA_TYPE_FIRST,
	ISP_AAA_TYPE_AE = ISP_AAA_TYPE_FIRST,
	ISP_AAA_TYPE_AWB,
	ISP_AAA_TYPE_AF,
	ISP_AAA_TYPE_END,
	ISP_AAA_TYPE_NUM = ISP_AAA_TYPE_END
} ISP_AAA_TYPE_E;

CVI_S32 isp_proc_regAAAGetVerCb(VI_PIPE ViPipe, ISP_AAA_TYPE_E aaaType,
				ISP_GetAAAVer_FUNC pfn_getVer);
CVI_S32 isp_proc_unRegAAAGetVerCb(VI_PIPE ViPipe, ISP_AAA_TYPE_E aaaType);

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_PROC_H_
