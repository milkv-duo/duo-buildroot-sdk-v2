/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ynr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_YNR_H_
#define _ISP_ALGO_YNR_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define YNR_CORING_NUM 6

typedef struct {
	unsigned int reg_ynr_weight_intra_0;
	unsigned int reg_ynr_weight_intra_1;
	unsigned int reg_ynr_weight_intra_2;
	unsigned int reg_ynr_weight_norm_intra1;
	unsigned int reg_ynr_weight_norm_intra2;
} YNR_KERNEL_S;

struct ynr_param_in {
	CVI_BOOL CoringParamEnable;
	CVI_BOOL isWdrMode;
	CVI_U32 u32ExpRatio;
	CVI_U8 NoiseCoringBaseLuma[YNR_CORING_NUM];
	CVI_U8 NoiseCoringAdvLuma[YNR_CORING_NUM];
	CVI_U8 NoiseCoringBaseOffset[YNR_CORING_NUM];
	CVI_U8 NoiseCoringAdvOffset[YNR_CORING_NUM];
	ISP_CMOS_NOISE_CALIBRATION_S np;
	CVI_U32 iso;
	CVI_U32 ispdgain;
	CVI_U32 wb_rgain;
	CVI_U32 wb_bgain;
	float CCM[9];
	ISP_U16_PTR GammaLUT;

	CVI_U8 window_type;
	CVI_U8 filter_type;
	//CVI_U32 rsv[32];
};

struct ynr_param_out {
	CVI_U8 NoiseCoringBaseLuma[YNR_CORING_NUM];
	CVI_U8 NoiseCoringAdvLuma[YNR_CORING_NUM];
	CVI_U8 NoiseCoringBaseOffset[YNR_CORING_NUM];
	CVI_U8 NoiseCoringAdvOffset[YNR_CORING_NUM];
	CVI_S16 NpSlopeBase[YNR_CORING_NUM-1];
	CVI_S16 NpSlopeAdvance[YNR_CORING_NUM-1];

	YNR_KERNEL_S kernel;
	CVI_U8 weight_lut[64];
};

CVI_S32 isp_algo_ynr_main(struct ynr_param_in *ynr_param_in, struct ynr_param_out *ynr_param_out);
CVI_S32 isp_algo_ynr_init(void);
CVI_S32 isp_algo_ynr_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_YNR_H_
