/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_fswdr_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_FSWDR_CTRL_H_
#define _ISP_FSWDR_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_fswdr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct fswdr_info {
	CVI_U16 WDRCombineLongThr;
	CVI_U16 WDRCombineShortThr;
	CVI_U16 WDRCombineMinWeight;
};

struct isp_fswdr_ctrl_runtime {
	struct fswdr_param_in fswdr_param_in;
	struct fswdr_param_out fswdr_param_out;

	ISP_FSWDR_MANUAL_ATTR_S fswdr_attr;

	CVI_U32 exp_ratio;
	CVI_U32 out_sel;
	CVI_U8 le_in_sel;
	CVI_U8 se_in_sel;

	struct cvi_vip_memblock fswdr_mem;

	CVI_BOOL initialized;
	CVI_BOOL mapped;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_BOOL is_init;
	struct isp_module_run_ctrl_mark run_ctrl_mark;
};

CVI_S32 isp_fswdr_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_fswdr_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_fswdr_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_fswdr_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_fswdr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_fswdr_ctrl_get_fswdr_attr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S **pstFSWDRAttr);
CVI_S32 isp_fswdr_ctrl_set_fswdr_attr(VI_PIPE ViPipe, const ISP_FSWDR_ATTR_S *pstFSWDRAttr);
CVI_S32 isp_fswdr_ctrl_get_fswdr_info(VI_PIPE ViPipe, struct fswdr_info *info);

extern const struct isp_module_ctrl fswdr_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_FSWDR_CTRL_H_
