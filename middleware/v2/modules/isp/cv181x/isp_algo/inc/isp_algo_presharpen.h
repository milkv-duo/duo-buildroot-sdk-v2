/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_presharpen.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_PRESHARPEN_H_
#define _ISP_ALGO_PRESHARPEN_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct presharpen_param_in {
	CVI_U8 LumaCorLutIn[EE_LUT_NODE];
	CVI_U8 LumaCorLutOut[EE_LUT_NODE];
	CVI_U8 MotionCorLutIn[EE_LUT_NODE];
	CVI_U8 MotionCorLutOut[EE_LUT_NODE];
	CVI_U8 MotionCorWgtLutIn[EE_LUT_NODE];
	CVI_U8 MotionCorWgtLutOut[EE_LUT_NODE];
	CVI_U8 MotionShtGainIn[EE_LUT_NODE];
	CVI_U8 MotionShtGainOut[EE_LUT_NODE];
	CVI_U8 SatShtGainIn[EE_LUT_NODE];
	CVI_U8 SatShtGainOut[EE_LUT_NODE];

	//CVI_U32 rsv[32];
};

struct presharpen_param_out {
	CVI_S16 LumaCorLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionCorLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionCorWgtLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionShtGainSlope[EE_LUT_NODE-1];
	CVI_S16 SatShtGainSlope[EE_LUT_NODE-1];
};

CVI_S32 isp_algo_presharpen_main(
	struct presharpen_param_in *presharpen_param_in, struct presharpen_param_out *presharpen_param_out);
CVI_S32 isp_algo_presharpen_init(void);
CVI_S32 isp_algo_presharpen_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_PRESHARPEN_H_
