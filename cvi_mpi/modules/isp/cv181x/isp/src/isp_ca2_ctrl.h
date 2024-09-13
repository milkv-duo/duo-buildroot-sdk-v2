/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ca2_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CA2_CTRL_H_
#define _ISP_CA2_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_ca2.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_ca2_ctrl_runtime {
	struct ca2_param_in ca2_param_in;
	struct ca2_param_out ca2_param_out;

	ISP_CA2_MANUAL_ATTR_S ca2_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_ca2_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_ca2_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_ca2_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_ca2_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_ca2_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);
CVI_S32 isp_ca2_ctrl_get_ca2_attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S **pstCA2Attr);
CVI_S32 isp_ca2_ctrl_set_ca2_attr(VI_PIPE ViPipe, const ISP_CA2_ATTR_S *pstCA2Attr);

extern const struct isp_module_ctrl ca2_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CA2_CTRL_H_
