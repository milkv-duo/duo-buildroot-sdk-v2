/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ccm.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CCM_H_
#define _ISP_ALGO_CCM_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct ccm_param_in {
	//ISP_CCM_ATTR_S *ccm_attr[ISP_CHANNEL_MAX_NUM];
	//ISP_SATURATION_ATTR_S *saturation_attr[ISP_CHANNEL_MAX_NUM];
	ISP_VOID_PTR ccm_attr[ISP_CHANNEL_MAX_NUM];
	ISP_VOID_PTR saturation_attr[ISP_CHANNEL_MAX_NUM];
	CVI_U32 iso;
	CVI_U32 color_temp[ISP_CHANNEL_MAX_NUM];

	//CVI_U32 rsv[32];
};

struct ccm_param_out {
	ISP_S16_PTR CCM[ISP_CHANNEL_MAX_NUM];
};

CVI_S32 isp_algo_ccm_main(struct ccm_param_in *ccm_param_in, struct ccm_param_out *ccm_param_out);
CVI_S32 isp_algo_ccm_init(void);
CVI_S32 isp_algo_ccm_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CCM_H_
