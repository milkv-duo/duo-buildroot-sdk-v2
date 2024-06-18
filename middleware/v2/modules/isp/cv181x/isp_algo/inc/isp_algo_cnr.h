/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_cnr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CNR_H_
#define _ISP_ALGO_CNR_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct cnr_param_in {
	CVI_U8 filter_type;
	//CVI_U32 rsv[32];
};

struct cnr_param_out {
	CVI_U8 weight_lut[CNR_MOTION_LUT_NUM];
};

CVI_S32 isp_algo_cnr_main(struct cnr_param_in *cnr_param_in, struct cnr_param_out *cnr_param_out);
CVI_S32 isp_algo_cnr_init(void);
CVI_S32 isp_algo_cnr_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CNR_H_
