/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mlsc_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_MLSC_CTRL_H_
#define _ISP_MLSC_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_mlsc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct mlsc_info {
	CVI_U16 *lut_r;
	CVI_U16 *lut_g;
	CVI_U16 *lut_b;
	CVI_FLOAT mlsc_compensate_gain;
};

struct isp_mlsc_ctrl_runtime {
	struct mlsc_param_in mlsc_param_in;
	struct mlsc_param_out mlsc_param_out;

	ISP_MESH_SHADING_MANUAL_ATTR_S mlsc_attr;
	ISP_MESH_SHADING_GAIN_LUT_S mlsc_lut_attr;

	struct cvi_vip_memblock mlsc_mem_info;

	CVI_U16 unit_gain_table[CVI_ISP_LSC_GRID_POINTS];
	CVI_BOOL mapped;

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;

	CVI_BOOL is_init;
	struct isp_module_run_ctrl_mark run_ctrl_mark;
};

CVI_S32 isp_mlsc_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_mlsc_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_mlsc_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_mlsc_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_mlsc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_mlsc_ctrl_set_run_ctrl_mark(VI_PIPE ViPipe, const struct isp_module_run_ctrl_mark *mark);

CVI_S32 isp_mlsc_ctrl_get_mlsc_attr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S **pstMeshShadingAttr);
CVI_S32 isp_mlsc_ctrl_set_mlsc_attr(VI_PIPE ViPipe, const ISP_MESH_SHADING_ATTR_S *pstMeshShadingAttr);
CVI_S32 isp_mlsc_ctrl_get_mlsc_lut_attr(VI_PIPE ViPipe,
	const ISP_MESH_SHADING_GAIN_LUT_ATTR_S **pstMeshShadingGainLutAttr);
CVI_S32 isp_mlsc_ctrl_set_mlsc_lut_attr(VI_PIPE ViPipe
	, const ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pstMeshShadingGainLutAttr);
CVI_S32 isp_mlsc_ctrl_get_mlsc_info(VI_PIPE ViPipe, struct mlsc_info *info);

extern const struct isp_module_ctrl mlsc_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MLSC_CTRL_H_
