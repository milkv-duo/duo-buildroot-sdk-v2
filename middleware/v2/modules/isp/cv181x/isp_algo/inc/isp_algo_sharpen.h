/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_sharpen.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_SHARPEN_H_
#define _ISP_ALGO_SHARPEN_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct sharpen_param_in {
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

struct sharpen_param_out {
	CVI_S16 LumaCorLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionCorLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionCorWgtLutSlope[EE_LUT_NODE-1];
	CVI_S16 MotionShtGainSlope[EE_LUT_NODE-1];
	CVI_S16 SatShtGainSlope[EE_LUT_NODE-1];
};

CVI_S32 isp_algo_sharpen_main(struct sharpen_param_in *sharpen_param_in, struct sharpen_param_out *sharpen_param_out);
CVI_S32 isp_algo_sharpen_init(void);
CVI_S32 isp_algo_sharpen_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_SHARPEN_H_
