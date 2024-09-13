/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_algo_lcac.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_LCAC_H_
#define _ISP_ALGO_LCAC_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct lcac_param_in {
	CVI_U8		u8FilterTypeBase;			// [0, 3]
	CVI_U8		u8FilterTypeAdv;			// [0, 5]
	CVI_U8		u8EdgeWgtBase_Wgt;			// [0, 128]
	CVI_U8		u8EdgeWgtBase_Sigma;		// [1, 255]
	CVI_U8		u8EdgeWgtAdv_Wgt;			// [0, 128]
	CVI_U8		u8EdgeWgtAdv_Sigma;			// [1, 255]
	ISP_VOID_PTR pvIntMemory;
};

struct lcac_param_out {
	CVI_S16		as16LTIFilterKernel[3];		// 10b
	CVI_U8		au8FCFFilterKernel[5];		// 7b
	CVI_U8		au8LTILumaLut[33];			// 8b
	CVI_U8		au8FCFLumaLut[33];			// 8b
};

CVI_S32 isp_algo_lcac_get_internal_memory(CVI_U32 *memory_size);

CVI_S32 isp_algo_lcac_init(void);
CVI_S32 isp_algo_lcac_uninit(void);

CVI_S32 isp_algo_lcac_main(
	struct lcac_param_in *lcac_param_in, struct lcac_param_out *lcac_param_out);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_LCAC_H_
