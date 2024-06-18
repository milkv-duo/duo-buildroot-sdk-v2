/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_lcac_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_LCAC_CTRL_H_
#define _ISP_LCAC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_lcac.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_lcac_ctrl_runtime {
	struct lcac_param_in lcac_param_in;
	struct lcac_param_out lcac_param_out;

	ISP_LCAC_MANUAL_ATTR_S lcac_attr;

	ISP_VOID_PTR pvAlgoMemory;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_lcac_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_lcac_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_lcac_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_lcac_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_lcac_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_lcac_ctrl_get_lcac_attr(VI_PIPE ViPipe, const ISP_LCAC_ATTR_S **pstLCACAttr);
CVI_S32 isp_lcac_ctrl_set_lcac_attr(VI_PIPE ViPipe, const ISP_LCAC_ATTR_S *pstLCACAttr);

extern const struct isp_module_ctrl lcac_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_LCAC_CTRL_H_
