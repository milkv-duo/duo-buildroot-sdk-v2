/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_tnr.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_TNR_H_
#define _ISP_ALGO_TNR_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define TNR_MOTION_BLOCK_SIZE_REF_LUMA		(2048)
#define TNR_MOTION_BLOCK_SIZE_MOTION_TH		(128.0)
#define TNR_MOTION_BLOCK_SIZE_TOLERANCE		(2)
#define TNR_PATH_NUM 2
#define TNR_CHAN_NUM 3

struct tnr_param_in {
	CVI_U16 L2mIn0[4];
	CVI_U16 L2mIn1[4];
	CVI_U8 L2mOut0[4];
	CVI_U8 L2mOut1[4];
	CVI_U8 PrvMotion0[4];
	CVI_U8 PrtctWgt0[4];
	CVI_U8 LowMtPrtInY[4];
	CVI_U8 LowMtPrtInU[4];
	CVI_U8 LowMtPrtInV[4];
	CVI_U8 LowMtPrtOutY[4];
	CVI_U8 LowMtPrtOutU[4];
	CVI_U8 LowMtPrtOutV[4];
	CVI_U8 LowMtPrtAdvIn[4];
	CVI_U8 LowMtPrtAdvOut[4];
	CVI_U8 LowMtPrtAdvDebugIn[4];
	CVI_U8 LowMtPrtAdvDebugOut[4];
	CVI_U8 AdaptNrLumaStrIn[4];
	CVI_U8 AdaptNrLumaStrOut[4];
	CVI_U8 AdaptNrChromaStrIn[4];
	CVI_U8 AdaptNrChromaStrOut[4];

	ISP_CMOS_NOISE_CALIBRATION_S np;
	CVI_U32 iso;
	CVI_U32 ispdgain;
	CVI_U32 wb_rgain;
	CVI_U32 wb_bgain;
	CVI_U8 gbmap_input_sel;
	CVI_U8 rgbmap_w_bit;
	CVI_U8 rgbmap_h_bit;

	CVI_U8 noiseLowLv[TNR_PATH_NUM][TNR_CHAN_NUM];
	CVI_U8 noiseHiLv[TNR_PATH_NUM][TNR_CHAN_NUM];
	CVI_S32 BrightnessNoiseLevelLE[2];

	//CVI_U32 rsv[32];
};

struct tnr_param_out {
	CVI_S16 L2mSlope0[3];
	CVI_S16 L2mSlope1[3];
	CVI_S16 PrtctSlope[3];
	CVI_S16 LowMtPrtSlopeY[3];
	CVI_S16 LowMtPrtSlopeU[3];
	CVI_S16 LowMtPrtSlopeV[3];
	CVI_S16 LowMtPrtAdvSlope[3];
	CVI_S16 LowMtPrtAdvDebugSlope[3];
	CVI_S16 AdaptNrLumaSlope[3];
	CVI_S16 AdaptNrChromaSlope[3];
	CVI_S32 slope[TNR_PATH_NUM][TNR_CHAN_NUM];
	CVI_S32 luma[TNR_PATH_NUM][TNR_CHAN_NUM];
	CVI_S32 low[TNR_PATH_NUM][TNR_CHAN_NUM];
	CVI_S32 high[TNR_PATH_NUM][TNR_CHAN_NUM];

	CVI_U16 SharpenCompGain;
	CVI_U16 LumaCompGain[17];
	CVI_U16 TnrLscCenterX;
	CVI_U16 TnrLscCenterY;
	CVI_U16 TnrLscNorm;
	CVI_U8 TnrLscDYGain;
	CVI_U16 TnrLscCompGain[17];
	CVI_U16 MtLscCenterX;
	CVI_U16 MtLscCenterY;
	CVI_U16 MtLscNorm;
	CVI_U8 MtLscDYGain;
	CVI_U16 MtLscCompGain[17];
};

struct tnr_motion_block_size_in {
	CVI_FLOAT fSlopeR;
	CVI_FLOAT fSlopeG;
	CVI_FLOAT fSlopeB;
	CVI_FLOAT fInterceptR;
	CVI_FLOAT fInterceptG;
	CVI_FLOAT fInterceptB;
	CVI_U32 u32RefLuma;		// 12bit
	CVI_FLOAT fMotionTh;	// 12bit
	CVI_U32 u32Tolerance;
};

struct tnr_motion_block_size_out {
	CVI_U8 u8BlockValue;
};

CVI_S32 isp_algo_tnr_main(struct tnr_param_in *tnr_param_in, struct tnr_param_out *tnr_param_out);
CVI_S32 isp_algo_tnr_init(void);
CVI_S32 isp_algo_tnr_uninit(void);

CVI_S32 isp_algo_tnr_generateblocksize(
	struct tnr_motion_block_size_in *ptIn, struct tnr_motion_block_size_out *ptOut);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_TNR_H_
