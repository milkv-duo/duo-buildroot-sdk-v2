/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ccm_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_CCM_CTRL_H_
#define _ISP_CCM_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_ccm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct ccm_info {
	CVI_BOOL ccm_en;
	CVI_S16 CCM[9];
};

struct isp_ccm_ctrl_runtime {
	struct ccm_param_in ccm_param_in;
	struct ccm_param_out ccm_param_out;

	ISP_CCM_ATTR_S ccm_attr[ISP_CHANNEL_MAX_NUM];
	ISP_SATURATION_ATTR_S saturation_attr[ISP_CHANNEL_MAX_NUM];
	struct cvi_vip_isp_ccm_config ccm_cfg[ISP_CHANNEL_MAX_NUM];

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_ccm_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_ccm_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_ccm_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_ccm_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_ccm_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_ccm_ctrl_get_ccm_attr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S **pstCCMAttr);
CVI_S32 isp_ccm_ctrl_set_ccm_attr(VI_PIPE ViPipe, const ISP_CCM_ATTR_S *pstCCMAttr);
CVI_S32 isp_ccm_ctrl_get_ccm_saturation_attr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S **pstCCMSaturationAttr);
CVI_S32 isp_ccm_ctrl_set_ccm_saturation_attr(VI_PIPE ViPipe, const ISP_CCM_SATURATION_ATTR_S *pstCCMSaturationAttr);
CVI_S32 isp_ccm_ctrl_get_saturation_attr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S **pstSaturationAttr);
CVI_S32 isp_ccm_ctrl_set_saturation_attr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSaturationAttr);

CVI_S32 isp_ccm_ctrl_get_ccm_info(VI_PIPE ViPipe, struct ccm_info *info);

extern const struct isp_module_ctrl ccm_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_CCM_CTRL_H_
