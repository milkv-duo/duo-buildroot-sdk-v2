/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_demosaic_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_DEMOSAIC_CTRL_H_
#define _ISP_DEMOSAIC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_demosaic.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_demosaic_ctrl_runtime {
	struct demosaic_param_in demosaic_param_in;
	struct demosaic_param_out demosaic_param_out;

	ISP_DEMOSAIC_MANUAL_ATTR_S demosaic_attr;
	ISP_DEMOSAIC_DEMOIRE_MANUAL_ATTR_S demosaic_demoire_attr;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_demosaic_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_demosaic_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_demosaic_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_demosaic_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_demosaic_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_demosaic_ctrl_get_demosaic_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_ATTR_S **pstDemosaicAttr);
CVI_S32 isp_demosaic_ctrl_set_demosaic_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_ATTR_S *pstDemosaicAttr);
CVI_S32 isp_demosaic_ctrl_get_demosaic_demoire_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S **pstDemosaicDemoireAttr);
CVI_S32 isp_demosaic_ctrl_set_demosaic_demoire_attr(VI_PIPE ViPipe,
	const ISP_DEMOSAIC_DEMOIRE_ATTR_S *pstDemosaicDemoireAttr);

extern const struct isp_module_ctrl demosaic_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DEMOSAIC_CTRL_H_
