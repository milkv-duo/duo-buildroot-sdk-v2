/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_gamma.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_GAMMA_H_
#define _ISP_ALGO_GAMMA_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct gamma_param_in {
	//CVI_U32 rsv[32];
};

struct gamma_param_out {

};

CVI_S32 isp_algo_gamma_main(struct gamma_param_in *gamma_param_in, struct gamma_param_out *gamma_param_out);
CVI_S32 isp_algo_gamma_init(void);
CVI_S32 isp_algo_gamma_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_GAMMA_H_
