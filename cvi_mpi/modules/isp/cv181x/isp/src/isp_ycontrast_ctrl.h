/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ycontrast_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_YCONTRAST_CTRL_H_
#define _ISP_YCONTRAST_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_ycontrast.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_ycontrast_ctrl_runtime {
	struct ycontrast_param_in ycontrast_param_in;
	struct ycontrast_param_out ycontrast_param_out;

	CVI_U32 au32YCurveLutIIRHist[YCURVE_LUT_ENTRY_NUM];

	CVI_U8 u8ContrastLow;
	CVI_U8 u8ContrastHigh;
	CVI_U8 u8CenterLuma;

	CVI_BOOL bResetYCurveIIR;
	CVI_U16 u16YCurveIIRSpeed;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_ycontrast_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_ycontrast_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_ycontrast_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_ycontrast_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_ycontrast_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_ycontrast_ctrl_get_ycontrast_attr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S **pstYContrastAttr);
CVI_S32 isp_ycontrast_ctrl_set_ycontrast_attr(VI_PIPE ViPipe, const ISP_YCONTRAST_ATTR_S *pstYContrastAttr);

extern const struct isp_module_ctrl ycontrast_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_YCONTRAST_CTRL_H_
