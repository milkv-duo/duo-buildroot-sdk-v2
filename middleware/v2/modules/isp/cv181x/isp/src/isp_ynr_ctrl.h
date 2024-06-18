/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ynr_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_YNR_CTRL_H_
#define _ISP_YNR_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_ynr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define YNR_NP_NODE_SIZE (6)

struct ynr_info {
	CVI_U8 NoiseCoringBaseLuma[YNR_NP_NODE_SIZE];
	CVI_U8 NoiseCoringBaseOffset[YNR_NP_NODE_SIZE]; // Q0.10
	CVI_S16 NpSlopeBase[YNR_NP_NODE_SIZE - 1]; // Q1.9
};

struct isp_ynr_ctrl_runtime {
	struct ynr_param_in ynr_param_in;
	struct ynr_param_out ynr_param_out;

	ISP_YNR_MANUAL_ATTR_S ynr_attr;
	ISP_YNR_FILTER_MANUAL_ATTR_S ynr_filter_attr;
	ISP_YNR_MOTION_NR_MANUAL_ATTR_S ynr_motion_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_ynr_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_ynr_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_ynr_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_ynr_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_ynr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_ynr_ctrl_get_ynr_attr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S **pstYNRAttr);
CVI_S32 isp_ynr_ctrl_set_ynr_attr(VI_PIPE ViPipe, const ISP_YNR_ATTR_S *pstYNRAttr);
CVI_S32 isp_ynr_ctrl_get_ynr_filter_attr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S **pstYNRFilterAttr);
CVI_S32 isp_ynr_ctrl_set_ynr_filter_attr(VI_PIPE ViPipe, const ISP_YNR_FILTER_ATTR_S *pstYNRFilterAttr);
CVI_S32 isp_ynr_ctrl_get_ynr_motion_attr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S **pstYNRMotionNRAttr);
CVI_S32 isp_ynr_ctrl_set_ynr_motion_attr(VI_PIPE ViPipe, const ISP_YNR_MOTION_NR_ATTR_S *pstYNRMotionNRAttr);
CVI_S32 isp_ynr_ctrl_get_ynr_info(VI_PIPE ViPipe, struct ynr_info *info);

extern const struct isp_module_ctrl ynr_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_YNR_CTRL_H_
