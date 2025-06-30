/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: isp_algo_teaisp_bnr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_TEAISP_BNR_H_
#define _ISP_ALGO_TEAISP_BNR_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct teaisp_bnr_param_in {
	ISP_VOID_PTR np;
	CVI_U32 iso;
	CVI_U16 NoiseLevel;
	CVI_U16 NoiseHiLevel;
};

struct teaisp_bnr_param_out {
	CVI_FLOAT slope;
	CVI_FLOAT intercept;
};

CVI_S32 isp_algo_teaisp_bnr_main(struct teaisp_bnr_param_in *in, struct teaisp_bnr_param_out *out);
CVI_S32 isp_algo_teaisp_bnr_init(void);
CVI_S32 isp_algo_teaisp_bnr_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_TEAISP_BNR_H_
