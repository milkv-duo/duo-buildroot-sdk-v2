/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_presharpen_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_PRESHARPEN_CTRL_H_
#define _ISP_PRESHARPEN_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_presharpen.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_presharpen_ctrl_runtime {
	struct presharpen_param_in presharpen_param_in;
	struct presharpen_param_out presharpen_param_out;

	ISP_PRESHARPEN_MANUAL_ATTR_S presharpen_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_presharpen_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_presharpen_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_presharpen_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_presharpen_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_presharpen_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_presharpen_ctrl_get_presharpen_attr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S **pstPreSharpenAttr);
CVI_S32 isp_presharpen_ctrl_set_presharpen_attr(VI_PIPE ViPipe, const ISP_PRESHARPEN_ATTR_S *pstPreSharpenAttr);

extern const struct isp_module_ctrl presharpen_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_PRESHARPEN_CTRL_H_
