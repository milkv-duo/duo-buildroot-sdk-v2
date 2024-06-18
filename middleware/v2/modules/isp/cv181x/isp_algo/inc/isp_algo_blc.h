/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_blc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_BLC_H_
#define _ISP_ALGO_BLC_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct blc_param_in {
	CVI_U16 ofs_r;
	CVI_U16 ofs_gr;
	CVI_U16 ofs_gb;
	CVI_U16 ofs_b;
	CVI_U16 ofs_r2;
	CVI_U16 ofs_gr2;
	CVI_U16 ofs_gb2;
	CVI_U16 ofs_b2;
	CVI_U16 gain_r;
	CVI_U16 gain_gr;
	CVI_U16 gain_gb;
	CVI_U16 gain_b;
	// CVI_U32 rsv[ALGO_RSV_PARAM_NUM];
	//CVI_U32 rsv[32];
};

struct blc_param_out {
	CVI_U16 ofs_r;
	CVI_U16 ofs_gr;
	CVI_U16 ofs_gb;
	CVI_U16 ofs_b;
	CVI_U16 ofs_r2;
	CVI_U16 ofs_gr2;
	CVI_U16 ofs_gb2;
	CVI_U16 ofs_b2;
	CVI_U16 gain_r;
	CVI_U16 gain_gr;
	CVI_U16 gain_gb;
	CVI_U16 gain_b;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_BLC_H_
