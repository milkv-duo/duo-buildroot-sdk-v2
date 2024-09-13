/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_gamma_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_GAMMA_CTRL_H_
#define _ISP_GAMMA_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

enum RGBGAMMA_CURVE_TYPE {
	CURVE_DEFAULT = 0,
	CURVE_DRC_TUNING_CURVE,
	CURVE_BYPASS
};

struct isp_gamma_ctrl_runtime {
	//struct gamma_param_in gamma_param_in;
	//struct gamma_param_out gamma_param_out;

	ISP_GAMMA_CURVE_ATTR_S gamma_attr;

	//struct cvi_vip_isp_gamma_config gamma_cfg;

	CVI_U16 au16RealGamma[GAMMA_NODE_NUM + 1];
	CVI_U16 au16RealGammaIIR[GAMMA_NODE_NUM + 1];
	CVI_U32 au32HistoryGamma[GAMMA_NODE_NUM + 1];
	CVI_U16 rgbgammacurve_index;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
	CVI_BOOL bResetYGammaIIR;
};

CVI_S32 isp_gamma_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_gamma_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_gamma_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_gamma_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_gamma_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_gamma_ctrl_get_gamma_attr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S **pstGammaAttr);
CVI_S32 isp_gamma_ctrl_set_gamma_attr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S *pstGammaAttr);
CVI_S32 isp_gamma_ctrl_get_gamma_curve_by_type(VI_PIPE ViPipe, ISP_GAMMA_ATTR_S *pstGammaAttr,
	const ISP_GAMMA_CURVE_TYPE_E curveType);
CVI_S32 isp_gamma_ctrl_get_auto_gamma_attr(VI_PIPE ViPipe,
	const ISP_AUTO_GAMMA_ATTR_S **pstAutoGammaAttr);
CVI_S32 isp_gamma_ctrl_set_auto_gamma_attr(VI_PIPE ViPipe,
	const ISP_AUTO_GAMMA_ATTR_S *pstAutoGammaAttr);
CVI_S32 isp_gamma_ctrl_get_real_gamma_lut(VI_PIPE ViPipe, CVI_U16 **pu16GammaLut);

CVI_S32 isp_gamma_ctrl_set_rgbgamma_curve(VI_PIPE ViPipe, enum RGBGAMMA_CURVE_TYPE eCurveType);

extern const struct isp_module_ctrl gamma_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_GAMMA_CTRL_H_
