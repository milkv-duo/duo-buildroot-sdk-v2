/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_fswdr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_FSWDR_H_
#define _ISP_ALGO_FSWDR_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct fswdr_param_in {
	CVI_FLOAT exposure_ratio;
	CVI_U32 iso;
	CVI_U32 digital_gain;
	CVI_U32 wb_r_gain;
	CVI_U32 wb_b_gain;
	ISP_VOID_PTR pNoiseCalibration;
	CVI_FLOAT WDRMinWeightSNRIIR;
	CVI_U16 WDRCombineShortThr;
	CVI_U16 WDRCombineLongThr;
	CVI_U16 WDRCombineMinWeight;
	CVI_U16 WDRCombineMaxWeight;
	CVI_U32 WDRFsCalPxlNum;
	CVI_S32 WDRPxlDiffSumR;
	CVI_S32 WDRPxlDiffSumG;
	CVI_S32 WDRPxlDiffSumB;
	CVI_U8 WDRMtIn[4];
	CVI_U16 WDRMtOut[4];
	CVI_BOOL WDRCombineSNRAwareEn;
	CVI_U16 WDRCombineSNRAwareSmoothLevel;
	CVI_FLOAT WDRCombineSNRAwareLowThr;
	CVI_FLOAT WDRCombineSNRAwareHighThr;
	CVI_U8 WDRCombineSNRAwareToleranceLevel;
	CVI_U16 DarkToneRefinedThrL;
	CVI_U16 DarkToneRefinedThrH;
	CVI_U16 DarkToneRefinedMaxWeight;
	CVI_U16 DarkToneRefinedMinWeight;
	CVI_U16 BrightToneRefinedThrL;
	CVI_U16 BrightToneRefinedThrH;
	CVI_U16 BrightToneRefinedMaxWeight;
	CVI_U16 BrightToneRefinedMinWeight;
	CVI_U8 WDRMotionFusionMode;
	CVI_U16 WDRMotionCombineLongThr;
	CVI_U16 WDRMotionCombineShortThr;
	CVI_U16 WDRMotionCombineMinWeight;
	CVI_U16 WDRMotionCombineMaxWeight;

	//CVI_U32 rsv[32];
};

struct fswdr_param_out {
	int ShortMaxVal;
	CVI_S32 WDRCombineSlope;
	CVI_S32 DarkToneRefinedSlope;
	CVI_S32 BrightToneRefinedSlope;
	CVI_BOOL MergeMtLsMode;
	CVI_BOOL MergeMode;
	CVI_BOOL MergeMtMaxMode;
	CVI_S32 WDRMotionCombineSlope;

	CVI_S16 WDRMtSlope[3];

	CVI_S32 WDRSeFixOffsetR; // CV1822 New Add
	CVI_S32 WDRSeFixOffsetG; // CV1822 New Add
	CVI_S32 WDRSeFixOffsetB; // CV1822 New Add

	CVI_U32 WDRCombineMinWeight;
	CVI_FLOAT WDRMinWeightSNRIIR;
};

CVI_S32 isp_algo_fswdr_main(
	struct fswdr_param_in *fswdr_param_in, struct fswdr_param_out *fswdr_param_out);
CVI_S32 isp_algo_fswdr_init(void);
CVI_S32 isp_algo_fswdr_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_FSWDR_H_
