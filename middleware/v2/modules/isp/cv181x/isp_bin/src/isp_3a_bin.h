/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_3a_bin.h
 * Description:
 *
 */

#ifndef _ISP_3A_BIN_H
#define _ISP_3A_BIN_H

#include <linux/cvi_common.h>
#include "isp_param_header.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

CVI_S32 isp_3aBinAttr_get_size(VI_PIPE ViPipe, CVI_U32 *size);
CVI_S32 isp_3aBinAttr_get_param(VI_PIPE ViPipe, FILE *fp);
CVI_S32 isp_3aBinAttr_get_parambuf(VI_PIPE ViPipe, CVI_U8 *buffer);
CVI_S32 isp_3aBinAttr_set_param(VI_PIPE ViPipe, CVI_U8 **binPtr);
CVI_S32 isp_3aJsonAttr_set_param(VI_PIPE ViPipe, ISP_3A_Parameter_Structures *stPtr);
CVI_S32 isp_3aJsonAttr_get_param(VI_PIPE ViPipe, ISP_3A_Parameter_Structures *stPtr);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_3A_BIN_H
