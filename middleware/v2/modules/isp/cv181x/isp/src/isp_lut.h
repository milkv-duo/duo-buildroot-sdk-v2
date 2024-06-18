/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_lut.h
 * Description:
 *
 */

#ifndef _ISP_LUT_H_
#define _ISP_LUT_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define GAMMA_LUT_EXPAND_COUNT (4096)

extern const CVI_U16 gau16ReverseSRGBLut[GAMMA_LUT_EXPAND_COUNT];

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_LUT_H_
