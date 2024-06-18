/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_motion_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_MOTION_CTRL_H_
#define _ISP_MOTION_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_motion.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_motion_ctrl_runtime {
	struct motion_param_in motion_param_in;
	struct motion_param_out motion_param_out;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_motion_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_motion_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_motion_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_motion_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_motion_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_motion_ctrl_get_motion_attr(VI_PIPE ViPipe, const ISP_VC_ATTR_S **pstMotionAttr);
CVI_S32 isp_motion_ctrl_set_motion_attr(VI_PIPE ViPipe, const ISP_VC_ATTR_S *pstMotionAttr);

extern const struct isp_module_ctrl motion_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MOTION_CTRL_H_
