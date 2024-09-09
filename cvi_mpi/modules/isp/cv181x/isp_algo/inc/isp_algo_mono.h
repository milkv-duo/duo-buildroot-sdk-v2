/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_mono.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_MONO_H_
#define _ISP_ALGO_MONO_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct mono_param_in {
	CVI_U8 filter_type;
	//CVI_U32 rsv[32];
};

struct mono_param_out {
	CVI_U8 weight_lut[16];
};

CVI_S32 isp_algo_mono_main(struct mono_param_in *mono_param_in, struct mono_param_out *mono_param_out);
CVI_S32 isp_algo_mono_init(void);
CVI_S32 isp_algo_mono_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_MONO_H_
