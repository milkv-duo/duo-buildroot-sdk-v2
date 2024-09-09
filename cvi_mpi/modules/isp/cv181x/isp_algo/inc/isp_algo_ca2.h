/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ca2.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CA2_H_
#define _ISP_ALGO_CA2_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct ca2_param_in {
	ISP_U8_PTR Ca2In;
	ISP_U16_PTR Ca2Out;
	//CVI_U32 rsv[32];
};

struct ca2_param_out {
	CVI_U16 Ca2Slope[CA_LITE_NODE-1];
};

CVI_S32 isp_algo_ca2_main(
	struct ca2_param_in *ca2_param_in, struct ca2_param_out *ca2_param_out);
CVI_S32 isp_algo_ca2_init(void);
CVI_S32 isp_algo_ca2_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CA2_H_
