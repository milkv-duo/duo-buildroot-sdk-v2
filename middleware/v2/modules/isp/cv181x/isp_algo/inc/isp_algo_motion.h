/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_motion.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_MOTION_H_
#define _ISP_ALGO_MOTION_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct motion_param_in {
	ISP_U8_PTR motionMapAddr;
	CVI_U32 motionStsBufSize;
	CVI_U8 motionThreshold;
	CVI_U8 gridWidth;
	CVI_U8 gridHeight;
	CVI_U32 imageWidth;
	CVI_U32 imageHeight;
	CVI_U32 frameCnt;
};

struct motion_param_out {
	CVI_U16 motionLevel;
	CVI_U32 frameCnt;
};

CVI_S32 isp_algo_motion_main(struct motion_param_in *motion_param_in, struct motion_param_out *motion_param_out);
CVI_S32 isp_algo_motion_init(void);
CVI_S32 isp_algo_motion_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_MOTION_H_
