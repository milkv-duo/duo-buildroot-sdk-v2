/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mono_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_MONO_CTRL_H_
#define _ISP_MONO_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_mono.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_mono_ctrl_runtime {
	//struct mono_param_in mono_param_in;
	//struct mono_param_out mono_param_out;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_mono_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_mono_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_mono_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_mono_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_mono_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_mono_ctrl_get_mono_attr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S **pstMonoAttr);
CVI_S32 isp_mono_ctrl_set_mono_attr(VI_PIPE ViPipe, const ISP_MONO_ATTR_S *pstMonoAttr);

extern const struct isp_module_ctrl mono_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MONO_CTRL_H_
