/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_3a.h
 * Description:
 *
 */

#ifndef _ISP_3A_H_
#define _ISP_3A_H_

#include "cvi_comm_isp.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

// Converting betwer ISP_AE_INFO_S & ISP_AE_STATISTICS_S
typedef struct _ISP_AE_STATISTICS_COMPAT_S {
	ISP_FE_AE_STAT_1_S aeStat1[AE_MAX_NUM];
	ISP_FE_AE_STAT_2_S aeStat2[AE_MAX_NUM];
	ISP_FE_AE_STAT_3_S aeStat3[AE_MAX_NUM];
} ISP_AE_STATISTICS_COMPAT_S;

void isp_reg_lib_dump(void);

CVI_S32 isp_3aLib_smart_info_set(VI_PIPE ViPipe, const ISP_SMART_INFO_S *pstSmartInfo, CVI_U8 TimeOut);
CVI_S32 isp_3aLib_smart_info_get(VI_PIPE ViPipe, ISP_SMART_INFO_S *pstSmartInfo);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_3A_H_
