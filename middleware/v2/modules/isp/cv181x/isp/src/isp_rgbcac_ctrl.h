/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_rgbcac_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_RGBCAC_CTRL_H_
#define _ISP_RGBCAC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_rgbcac.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_rgbcac_ctrl_runtime {
	struct rgbcac_param_in rgbcac_param_in;
	struct rgbcac_param_out rgbcac_param_out;

	ISP_RGBCAC_MANUAL_ATTR_S rgbcac_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_rgbcac_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_rgbcac_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_rgbcac_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_rgbcac_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_rgbcac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_rgbcac_ctrl_get_rgbcac_attr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S **pstRGBCACAttr);
CVI_S32 isp_rgbcac_ctrl_set_rgbcac_attr(VI_PIPE ViPipe, const ISP_RGBCAC_ATTR_S *pstRGBCACAttr);

extern const struct isp_module_ctrl rgbcac_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_RGBCAC_CTRL_H_
