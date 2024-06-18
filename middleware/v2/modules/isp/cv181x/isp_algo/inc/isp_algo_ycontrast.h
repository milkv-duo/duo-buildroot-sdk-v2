/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ycontrast.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_YCONTRAST_H_
#define _ISP_ALGO_YCONTRAST_H_

#include "cvi_type.h"
#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define YCURVE_LUT_ENTRY_NUM		(64)

struct ycontrast_param_in {
	CVI_U8		ContrastLow;
	CVI_U8		ContrastHigh;
	CVI_U8		CenterLuma;
};

struct ycontrast_param_out {
	CVI_U8		au8YCurve_Lut[YCURVE_LUT_ENTRY_NUM];
	CVI_U16		u16YCurve_LastPoint;
};

CVI_S32 isp_algo_ycontrast_main(
	struct ycontrast_param_in *param_in, struct ycontrast_param_out *param_out);
CVI_S32 isp_algo_ycontrast_init(void);
CVI_S32 isp_algo_ycontrast_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_YCONTRAST_H_
