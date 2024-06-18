/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_drc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_DRC_CTRL_H_
#define _ISP_DRC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_drc.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct drc_algo_info {
	CVI_U32 globalToneBinNum;
	CVI_U32 globalToneSeStep;
	CVI_U32 *globalToneResult;
	CVI_U32 *darkToneResult;
	CVI_U32 *brightToneResult;
	CVI_U32 drc_g_curve_1_quan_bit;
};

struct isp_drc_ctrl_runtime {
	struct drc_param_in drc_param_in;
	struct drc_param_out drc_param_out;

	ISP_DRC_MANUAL_ATTR_S drc_attr;

	CVI_BOOL ldrc_mode;

	ISP_U8_PTR pLocalBuffer;
	ISP_U32_PTR preGlobalLut;
	ISP_U32_PTR preDarkLut;
	ISP_U32_PTR preBritLut;

	CVI_BOOL initialized;
	CVI_BOOL mapped;

	CVI_BOOL modify_rgbgamma_for_drc_tuning;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_BOOL reset_iir;
	CVI_S16 lv;
};

CVI_S32 isp_drc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_drc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_drc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_drc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_drc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_drc_ctrl_get_drc_attr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S **pstDRCAttr);
CVI_S32 isp_drc_ctrl_set_drc_attr(VI_PIPE ViPipe, const ISP_DRC_ATTR_S *pstDRCAttr);

CVI_S32 isp_drc_ctrl_get_algo_info(VI_PIPE ViPipe, struct drc_algo_info *info);

extern const struct isp_module_ctrl drc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DRC_CTRL_H_
