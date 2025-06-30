/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: isp_lblc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_LBLC_CTRL_H_
#define _ISP_LBLC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_lblc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct lblc_info {
	CVI_U16 *lblcOffsetR;
	CVI_U16 *lblcOffsetGr;
	CVI_U16 *lblcOffsetGb;
	CVI_U16 *lblcOffsetB;
};

struct isp_lblc_ctrl_runtime {
	struct lblc_param_in lblc_param_in;
	struct lblc_param_out lblc_param_out;

	ISP_LBLC_MANUAL_ATTR_S lblc_attr;
	ISP_LBLC_LUT_S lblc_lut_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_lblc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_lblc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_lblc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_lblc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_lblc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_lblc_ctrl_get_lblc_attr(VI_PIPE ViPipe, const ISP_LBLC_ATTR_S **pstLblcAttr);
CVI_S32 isp_lblc_ctrl_set_lblc_attr(VI_PIPE ViPipe, const ISP_LBLC_ATTR_S *pstLblcAttr);
CVI_S32 isp_lblc_ctrl_get_lblc_lut_attr(VI_PIPE ViPipe,
	const ISP_LBLC_LUT_ATTR_S **pstLblcLutAttr);
CVI_S32 isp_lblc_ctrl_set_lblc_lut_attr(VI_PIPE ViPipe
	, const ISP_LBLC_LUT_ATTR_S *pstLblcLutAttr);
CVI_S32 isp_lblc_ctrl_get_lblc_info(VI_PIPE ViPipe, struct lblc_info *info);

extern const struct isp_module_ctrl lblc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_LBLC_CTRL_H_
