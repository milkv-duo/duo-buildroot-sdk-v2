/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_cac.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CAC_H_
#define _ISP_ALGO_CAC_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define CAC_EDGE_SCALE_LUT_SIZE		(17)

struct cac_param_in {
	CVI_U8 u8DePurpleCbStr;
	CVI_U8 u8DePurpleCrStr;
	CVI_U8 u8DePurpleStrMaxRatio;
	CVI_U8 u8DePurpleStrMinRatio;
	CVI_U8 au8EdgeGainIn[3];
	CVI_U8 au8EdgeGainOut[3];
};

struct cac_param_out {
	CVI_U8 u8CbStr;
	CVI_U8 u8CrStr;
	CVI_U8 u8DePurpleStrRatioMax;
	CVI_U8 u8DePurpleStrRatioMin;
	CVI_U8 au8EdgeScaleLut[CAC_EDGE_SCALE_LUT_SIZE];
};

CVI_S32 isp_algo_cac_init(void);
CVI_S32 isp_algo_cac_uninit(void);
CVI_S32 isp_algo_cac_main(struct cac_param_in *cac_param_in, struct cac_param_out *cac_param_out);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CAC_H_
