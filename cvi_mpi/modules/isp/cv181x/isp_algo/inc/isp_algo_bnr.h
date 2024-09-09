/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_bnr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_BNR_H_
#define _ISP_ALGO_BNR_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
	unsigned int reg_bnr_weight_intra_0;
	unsigned int reg_bnr_weight_intra_1;
	unsigned int reg_bnr_weight_intra_2;
	unsigned int reg_bnr_weight_norm_intra1;
	unsigned int reg_bnr_weight_norm_intra2;
} BNR_KERNEL_S;

struct bnr_param_in {
	ISP_CMOS_NOISE_CALIBRATION_S np;
	CVI_U32 iso;
	CVI_U32 ispdgain;
	CVI_U32 wb_rgain;
	CVI_U32 wb_bgain;

	CVI_U8 window_type;
	CVI_U8 filter_type;

	CVI_BOOL CoringParamEnable;
	CVI_S32 NpSlope[ISP_BAYER_CHN_NUM];
	CVI_S32 NpLumaThr[ISP_BAYER_CHN_NUM];
	CVI_S32 NpLowOffset[ISP_BAYER_CHN_NUM];
	CVI_S32 NpHighOffset[ISP_BAYER_CHN_NUM];

	//CVI_U32 rsv[32];
};

struct bnr_param_out {
	CVI_S32 NpSlope[ISP_BAYER_CHN_NUM];
	CVI_S32 NpLumaThr[ISP_BAYER_CHN_NUM];
	CVI_S32 NpLowOffset[ISP_BAYER_CHN_NUM];
	CVI_S32 NpHighOffset[ISP_BAYER_CHN_NUM];

	BNR_KERNEL_S kernel;
	CVI_U8 weight_lut[256];
};

CVI_S32 isp_algo_bnr_main(struct bnr_param_in *bnr_param_in, struct bnr_param_out *bnr_param_out);
CVI_S32 isp_algo_bnr_init(void);
CVI_S32 isp_algo_bnr_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_BNR_H_
