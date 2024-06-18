/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_crosstalk.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CROSSTALK_H_
#define _ISP_ALGO_CROSSTALK_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct crosstalk_param_in {
	//CVI_U32 rsv[32];
};

struct crosstalk_param_out {

};

CVI_S32 isp_algo_crosstalk_main(
	struct crosstalk_param_in *crosstalk_param_in, struct crosstalk_param_out *crosstalk_param_out);
CVI_S32 isp_algo_crosstalk_init(void);
CVI_S32 isp_algo_crosstalk_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CROSSTALK_H_
