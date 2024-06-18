/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_csc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CSC_CTRL_H_
#define _ISP_CSC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_csc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_csc_ctrl_runtime {
	struct csc_param_in csc_param_in;
	struct csc_param_out csc_param_out;

	struct cvi_vip_isp_csc_config csc_cfg;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_csc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_csc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_csc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_csc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_csc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_csc_ctrl_get_csc_attr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S **pstCSCAttr);
CVI_S32 isp_csc_ctrl_set_csc_attr(VI_PIPE ViPipe, const ISP_CSC_ATTR_S *pstCSCAttr);

extern const struct isp_module_ctrl csc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CSC_CTRL_H_
