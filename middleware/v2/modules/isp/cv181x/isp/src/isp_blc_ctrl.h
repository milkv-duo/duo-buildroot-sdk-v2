/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_blc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_BLC_CTRL_H_
#define _ISP_BLC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
//#include "isp_algo_blc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum _ISP_BLC_EXP_TYPE_E {
	ISP_BLC_EXP_LE,
	ISP_BLC_EXP_SE,
	ISP_BLC_EXP_MAX
} ISP_BLC_EXP_TYPE_E;

struct blc_info {
	CVI_U16 OffsetR;
	CVI_U16 OffsetGr;
	CVI_U16 OffsetGb;
	CVI_U16 OffsetB;
	CVI_U16 OffsetR2;
	CVI_U16 OffsetGr2;
	CVI_U16 OffsetGb2;
	CVI_U16 OffsetB2;
	CVI_U16 GainR;
	CVI_U16 GainGr;
	CVI_U16 GainGb;
	CVI_U16 GainB;
};

struct isp_blc_attr {
	CVI_U16 OffsetR;
	CVI_U16 OffsetGr;
	CVI_U16 OffsetGb;
	CVI_U16 OffsetB;
	CVI_U16 OffsetR2;
	CVI_U16 OffsetGr2;
	CVI_U16 OffsetGb2;
	CVI_U16 OffsetB2;
	CVI_U16 GainR;
	CVI_U16 GainGr;
	CVI_U16 GainGb;
	CVI_U16 GainB;
};

struct isp_blc_ctrl_runtime {
	// struct blc_param_in param_in;
	// struct blc_param_out param_out;

	struct isp_blc_attr pre_fe_blc_attr[ISP_BLC_EXP_MAX];
	struct isp_blc_attr pre_be_blc_attr[ISP_BLC_EXP_MAX];

	struct cvi_vip_isp_blc_config pre_fe_blc_cfg[ISP_BLC_EXP_MAX];
	struct cvi_vip_isp_blc_config pre_be_blc_cfg[ISP_BLC_EXP_MAX];

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_blc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_blc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_blc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_blc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_blc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);
CVI_S32 isp_blc_ctrl_set_blc_attr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr);
CVI_S32 isp_blc_ctrl_get_blc_attr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S **pstBlackLevelAttr);
CVI_S32 isp_blc_ctrl_get_blc_info(VI_PIPE ViPipe, struct blc_info *info);

extern const struct isp_module_ctrl blc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_BLC_CTRL_H_
