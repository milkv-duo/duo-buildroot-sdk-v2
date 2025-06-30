/*
 * Copyright (C) Cvitek Co., Ltd. 2023-2024. All rights reserved.
 *
 * File Name: teaisp_bnr_ctrl.h
 * Description:
 *
 */

#ifndef _TEAISP_BNR_CTRL_H_
#define _TEAISP_BNR_CTRL_H_

#include "isp_feature_ctrl.h"
#include "isp_algo_teaisp_bnr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct teaisp_bnr_config {
	CVI_U8  update;
	CVI_U32 blend;
	CVI_U32 blc;
	CVI_U32 coeff_a;
	CVI_U32 coeff_b;
	CVI_U32 filter_motion_str_2d;
	CVI_U32 filter_static_str_2d;
	CVI_U32 filter_str_3d;
	CVI_U32 filter_str_2d;
	CVI_U16 lblcOffsetR[ISP_LBLC_GRID_POINTS];
	CVI_U16 lblcOffsetGr[ISP_LBLC_GRID_POINTS];
	CVI_U16 lblcOffsetGb[ISP_LBLC_GRID_POINTS];
	CVI_U16 lblcOffsetB[ISP_LBLC_GRID_POINTS];
};

struct teaisp_bnr_ctrl_runtime {
	struct teaisp_bnr_param_in bnr_param_in;
	struct teaisp_bnr_param_out bnr_param_out;
	struct teaisp_bnr_config bnr_cfg;
	TEAISP_BNR_MANUAL_ATTR_S bnr_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_BOOL is_teaisp_bnr_enable;
	CVI_BOOL is_teaisp_bnr_running;
	CVI_U32 u32CurrentISO;
	ISP_VOID_PTR handle;
	CVI_FLOAT afAEEVRatio;
};

CVI_S32 teaisp_bnr_ctrl_init(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 teaisp_bnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 teaisp_bnr_ctrl_set_model(VI_PIPE ViPipe, const TEAISP_BNR_MODEL_INFO_S *pstModelInfo);
CVI_S32 teaisp_bnr_ctrl_get_bnr_attr(VI_PIPE ViPipe, const TEAISP_BNR_ATTR_S **pstNRAttr);
CVI_S32 teaisp_bnr_ctrl_set_bnr_attr(VI_PIPE ViPipe, const TEAISP_BNR_ATTR_S *pstNRAttr);
CVI_S32 teaisp_bnr_ctrl_get_np_attr(VI_PIPE ViPipe, const TEAISP_BNR_NP_S **np);
CVI_S32 teaisp_bnr_ctrl_set_np_attr(VI_PIPE ViPipe, const TEAISP_BNR_NP_S *np);

extern const struct isp_module_ctrl teaisp_bnr_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_BNR_CTRL_H_
