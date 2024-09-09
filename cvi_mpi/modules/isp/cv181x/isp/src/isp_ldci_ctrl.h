/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_ldci_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_LDCI_CTRL_H_
#define _ISP_LDCI_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_ldci.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_ldci_ctrl_runtime {
	struct ldci_init_coef ldci_coef;
	struct ldci_param_in ldci_param_in;
	struct ldci_param_out ldci_param_out;

	ISP_LDCI_MANUAL_ATTR_S ldci_attr;

	ISP_VOID_PTR pvAlgoMemory;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_ldci_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_ldci_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_ldci_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_ldci_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_ldci_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_ldci_ctrl_get_ldci_attr(VI_PIPE ViPipe, const ISP_LDCI_ATTR_S **pstLDCIAttr);
CVI_S32 isp_ldci_ctrl_set_ldci_attr(VI_PIPE ViPipe, const ISP_LDCI_ATTR_S *pstLDCIAttr);

extern const struct isp_module_ctrl ldci_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_LDCI_CTRL_H_
