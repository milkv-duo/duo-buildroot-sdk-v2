/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ca.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CA_H_
#define _ISP_ALGO_CA_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct ca_param_in {
	//CVI_U32 rsv[32];
};

struct ca_param_out {

};

CVI_S32 isp_algo_ca_main(
	struct ca_param_in *ca_param_in, struct ca_param_out *ca_param_out);
CVI_S32 isp_algo_ca_init(void);
CVI_S32 isp_algo_ca_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CA_H_
