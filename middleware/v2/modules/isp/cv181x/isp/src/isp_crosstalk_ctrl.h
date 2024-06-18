/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_crosstalk_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CROSSTALK_CTRL_H_
#define _ISP_CROSSTALK_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_crosstalk.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_crosstalk_ctrl_runtime {
	struct crosstalk_param_in crosstalk_param_in;
	struct crosstalk_param_out crosstalk_param_out;

	ISP_CROSSTALK_MANUAL_ATTR_S crosstalk_attr;

	struct cvi_vip_isp_ge_config ge_cfg;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_crosstalk_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_crosstalk_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_crosstalk_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_crosstalk_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_crosstalk_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_crosstalk_ctrl_get_crosstalk_attr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S **pstCrosstalkAttr);
CVI_S32 isp_crosstalk_ctrl_set_crosstalk_attr(VI_PIPE ViPipe, const ISP_CROSSTALK_ATTR_S *pstCrosstalkAttr);

extern const struct isp_module_ctrl crosstalk_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CROSSTALK_CTRL_H_
