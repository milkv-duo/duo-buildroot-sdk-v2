/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dci_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_DCI_CTRL_H_
#define _ISP_DCI_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_dci.h"
#include "isp_sts_ctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_dci_ctrl_runtime {
	struct dci_param_in dci_param_in;
	struct dci_param_out dci_param_out;

	ISP_DCI_STATISTICS_S dciStatsInfoBuf;
	ISP_DCI_MANUAL_ATTR_S dci_attr;

	CVI_U16 map_lut[DCI_BINS_NUM];
	CVI_U16 pre_dci_data[DCI_BINS_NUM];
	CVI_U32 pre_dci_lut[DCI_BINS_NUM];

	CVI_BOOL reset_iir;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_dci_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_dci_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_dci_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_dci_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_dci_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_dci_ctrl_get_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S **pstDciAttr);
CVI_S32 isp_dci_ctrl_set_dci_attr(VI_PIPE ViPipe, const ISP_DCI_ATTR_S *pstDciAttr);

extern const struct isp_module_ctrl dci_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_DCI_CTRL_H_
