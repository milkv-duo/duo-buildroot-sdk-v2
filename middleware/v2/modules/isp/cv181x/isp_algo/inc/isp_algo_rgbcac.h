/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_rgbcac.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_RGBCAC_H_
#define _ISP_ALGO_RGBCAC_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define RGBCAC_EDGE_SCALE_LUT_SIZE		(17)

struct rgbcac_param_in {
	CVI_U8 u8DePurpleStrMax0;
	CVI_U8 u8DePurpleStrMin0;
	CVI_U8 u8DePurpleStrMax1;
	CVI_U8 u8DePurpleStrMin1;
	CVI_U8 au8EdgeGainIn[3];
	CVI_U8 au8EdgeGainOut[3];
};

struct rgbcac_param_out {
	CVI_U16 u16DePurpleStrRatioMax0;
	CVI_U16 u16DePurpleStrRatioMin0;
	CVI_U16 u16DePurpleStrRatioMax1;
	CVI_U16 u16DePurpleStrRatioMin1;
	CVI_U8 au8EdgeScaleLut[RGBCAC_EDGE_SCALE_LUT_SIZE];
};

CVI_S32 isp_algo_rgbcac_main(struct rgbcac_param_in *rgbcac_param_in, struct rgbcac_param_out *rgbcac_param_out);
CVI_S32 isp_algo_rgbcac_init(void);
CVI_S32 isp_algo_rgbcac_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_RGBCAC_H_
