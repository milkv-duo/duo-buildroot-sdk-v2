/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dpc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_DPC_CTRL_H_
#define _ISP_DPC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_dpc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_dpc_ctrl_runtime {
	struct dpc_param_in dpc_param_in;
	struct dpc_param_out dpc_param_out;

	ISP_DP_DYNAMIC_MANUAL_ATTR_S dpc_dynamic_attr;

	struct cvi_vip_isp_dpc_config dpc_cfg;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_dpc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_dpc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_dpc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_dpc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_dpc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_dpc_ctrl_get_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S **pstDPCDynamicAttr);
CVI_S32 isp_dpc_ctrl_set_dpc_dynamic_attr(VI_PIPE ViPipe, const ISP_DP_DYNAMIC_ATTR_S *pstDPCDynamicAttr);
CVI_S32 isp_dpc_ctrl_get_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S **pstDPStaticAttr);
CVI_S32 isp_dpc_ctrl_set_dpc_static_attr(VI_PIPE ViPipe, const ISP_DP_STATIC_ATTR_S *pstDPStaticAttr);
CVI_S32 isp_dpc_ctrl_get_dpc_calibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S **pstDPCalibAttr);
CVI_S32 isp_dpc_ctrl_set_dpc_calibrate(VI_PIPE ViPipe, const ISP_DP_CALIB_ATTR_S *pstDPCalibAttr);

extern const struct isp_module_ctrl dpc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DPC_CTRL_H_
