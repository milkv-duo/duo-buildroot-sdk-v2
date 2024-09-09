/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_csc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CSC_H_
#define _ISP_ALGO_CSC_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct csc_param_in {
	ISP_CSC_COLORGAMUT eCscColorGamut;
	CVI_U8 u8hue;
	CVI_U8 u8luma;
	CVI_U8 u8contrast;
	CVI_U8 u8saturation;
	CVI_S16 s16userCscCoef[CSC_MATRIX_SIZE];
	CVI_S16 s16userCscOffset[CSC_OFFSET_SIZE];
};

struct csc_param_out {
	CVI_S16 s16cscCoef[CSC_MATRIX_SIZE];
	CVI_S16 s16cscOffset[CSC_OFFSET_SIZE];
};

CVI_S32 isp_algo_csc_main(struct csc_param_in *csc_param_in, struct csc_param_out *csc_param_out);
CVI_S32 isp_algo_csc_init(void);
CVI_S32 isp_algo_csc_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CSC_H_
