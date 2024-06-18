/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dis_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_DIS_CTRL_H_
#define _ISP_DIS_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_algo_dis.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_dis_ctrl_runtime {
	DIS_MAIN_INPUT_S inPtr;
	DIS_MAIN_OUTPUT_S outPtr;

	ISP_PUB_ATTR_S pstPubAttr;

	CVI_BOOL isEnable;
	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_dis_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_dis_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_dis_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_dis_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_dis_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_dis_ctrl_set_dis_attr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S *pstDisAttr);
CVI_S32 isp_dis_ctrl_get_dis_attr(VI_PIPE ViPipe, const ISP_DIS_ATTR_S **pstDisAttr);
CVI_S32 isp_dis_ctrl_set_dis_config(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S *pstDisConfig);
CVI_S32 isp_dis_ctrl_get_dis_config(VI_PIPE ViPipe, const ISP_DIS_CONFIG_S **pstDisConfig);
CVI_S32 isp_dis_ctrl_get_gms_attr(VI_PIPE ViPipe, struct cvi_vip_isp_gms_config *gmsCfg);

extern const struct isp_module_ctrl dis_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DIS_CTRL_H_
