/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_wb_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_WB_CTRL_H_
#define _ISP_WB_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_wb_ctrl_runtime {
	// struct wb_param_in param_in;
	// struct wb_param_out param_out;

	ISP_MWB_ATTR_S wb_attr;

	struct cvi_vip_isp_wbg_config wbg_cfg;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
};

CVI_S32 isp_wb_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_wb_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_wb_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_wb_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_wb_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_wb_ctrl_set_wb_colortone_attr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S *pstColorToneAttr);
CVI_S32 isp_wb_ctrl_get_wb_colortone_attr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S **pstColorToneAttr);

extern const struct isp_module_ctrl wb_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_WB_CTRL_H_
