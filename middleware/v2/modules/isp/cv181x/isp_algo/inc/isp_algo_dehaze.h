/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dehaze.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DEHAZE_H_
#define _ISP_ALGO_DEHAZE_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define DEHAZE_SKIN_LUT_SIZE 16
#define DEHAZE_TMAP_GAIN_LUT_SIZE 129

struct dehaze_param_in {
	CVI_U16 DehazeLumaCOEFFI;
	CVI_U16 DehazeSkinCOEFFI;
	CVI_U8 TransMapWgtWgt;
	CVI_U8 TransMapWgtSigma;
	//CVI_U32 rsv[32];
};

struct dehaze_param_out {
	CVI_U8 DehazeLumaLut[DEHAZE_SKIN_LUT_SIZE];
	CVI_U8 DehazeSkinLut[DEHAZE_SKIN_LUT_SIZE];
	CVI_U8 DehazeTmapGainLut[DEHAZE_TMAP_GAIN_LUT_SIZE];
};

CVI_S32 isp_algo_dehaze_main(
	struct dehaze_param_in *dehaze_param_in, struct dehaze_param_out *dehaze_param_out);
CVI_S32 isp_algo_dehaze_init(void);
CVI_S32 isp_algo_dehaze_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DEHAZE_H_
