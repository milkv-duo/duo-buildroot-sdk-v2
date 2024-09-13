/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_demosaic.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DEMOSAIC_H_
#define _ISP_ALGO_DEMOSAIC_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct demosaic_param_in {
	CVI_U16 SatGainIn[2];
	CVI_U16 SatGainOut[2];
	CVI_U16 ProtectColorGainIn[2];
	CVI_U16 ProtectColorGainOut[2];
	CVI_U16 EdgeGainIn[2];
	CVI_U16 EdgeGainOut[2];
	CVI_U16 DetailGainIn[2];
	CVI_U16 DetailGaintOut[2];

	//CVI_U32 rsv[32];
};

struct demosaic_param_out {
	CVI_S32 SatGainSlope;
	CVI_S32 ProtectColorGainSlope;
	CVI_S32 EdgeGainSlope;
	CVI_S32 DetailGainSlope;
};

CVI_S32 isp_algo_demosaic_main(
	struct demosaic_param_in *demosaic_param_in,
	struct demosaic_param_out *demosaic_param_out);
CVI_S32 isp_algo_demosaic_init(void);
CVI_S32 isp_algo_demosaic_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DEMOSAIC_H_
