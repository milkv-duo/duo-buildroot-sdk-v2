/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_tnr_ctrl.h
 * Description:
 *
 */

#ifndef _ISP_TNR_CTRL_H_
#define _ISP_TNR_CTRL_H_

#include "cvi_comm_isp.h"
#include "isp_feature_ctrl.h"
#include "isp_algo_tnr.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _ISP_TNR_INTER_ATTR_S {
	CVI_BOOL bUpdateMotionUnit;
} ISP_TNR_INTER_ATTR_S;

struct isp_tnr_ctrl_runtime {
	struct tnr_param_in tnr_param_in;
	struct tnr_param_out tnr_param_out;

	ISP_TNR_MANUAL_ATTR_S tnr_attr;
	ISP_TNR_NOISE_MODEL_MANUAL_ATTR_S tnr_noise_model_attr;
	ISP_TNR_LUMA_MOTION_MANUAL_ATTR_S tnr_luma_motion_attr;
	ISP_TNR_GHOST_MANUAL_ATTR_S tnr_ghost_attr;
	ISP_TNR_MT_PRT_MANUAL_ATTR_S tnr_mt_prt_attr;
	ISP_TNR_MOTION_ADAPT_MANUAL_ATTR_S tnr_motion_adapt_attr;
	ISP_TNR_INTER_ATTR_S tnr_internal_attr;

	CVI_BOOL avg_mode_write;
	CVI_BOOL drop_mode_write;

	CVI_U8 au8MtDetectUnit[ISP_AUTO_ISO_STRENGTH_NUM];
	CVI_U16 GainCompensateRatio[ISP_CHANNEL_MAX_NUM][ISP_BAYER_CHN_NUM];

	CVI_BOOL preprocess_updated;
	CVI_BOOL process_updated;
	CVI_BOOL postprocess_updated;
	CVI_BOOL is_module_bypass;
};

CVI_S32 isp_tnr_ctrl_init(VI_PIPE ViPipe);
CVI_S32 isp_tnr_ctrl_uninit(VI_PIPE ViPipe);
CVI_S32 isp_tnr_ctrl_suspend(VI_PIPE ViPipe);
CVI_S32 isp_tnr_ctrl_resume(VI_PIPE ViPipe);
CVI_S32 isp_tnr_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input);

CVI_S32 isp_tnr_ctrl_get_tnr_internal_attr(VI_PIPE ViPipe, ISP_TNR_INTER_ATTR_S *pstTNRInterAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_internal_attr(VI_PIPE ViPipe, const ISP_TNR_INTER_ATTR_S *pstTNRInterAttr);

CVI_S32 isp_tnr_ctrl_get_tnr_attr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S **pstTNRAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_attr(VI_PIPE ViPipe, const ISP_TNR_ATTR_S *pstTNRAttr);
CVI_S32 isp_tnr_ctrl_get_tnr_noise_model_attr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S **pstTNRNoiseModelAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_noise_model_attr(VI_PIPE ViPipe, const ISP_TNR_NOISE_MODEL_ATTR_S *pstTNRNoiseModelAttr);
CVI_S32 isp_tnr_ctrl_get_tnr_luma_motion_attr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S **pstTNRLumaMotionAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_luma_motion_attr(VI_PIPE ViPipe, const ISP_TNR_LUMA_MOTION_ATTR_S *pstTNRLumaMotionAttr);
CVI_S32 isp_tnr_ctrl_get_tnr_ghost_attr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S **pstTNRGhostAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_ghost_attr(VI_PIPE ViPipe, const ISP_TNR_GHOST_ATTR_S *pstTNRGhostAttr);
CVI_S32 isp_tnr_ctrl_get_tnr_mt_prt_attr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S **pstTNRMtPrtAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_mt_prt_attr(VI_PIPE ViPipe, const ISP_TNR_MT_PRT_ATTR_S *pstTNRMtPrtAttr);
CVI_S32 isp_tnr_ctrl_get_tnr_motion_adapt_attr(VI_PIPE ViPipe,
	const ISP_TNR_MOTION_ADAPT_ATTR_S **pstTNRMotionAdaptAttr);
CVI_S32 isp_tnr_ctrl_set_tnr_motion_adapt_attr(VI_PIPE ViPipe,
	const ISP_TNR_MOTION_ADAPT_ATTR_S *pstTNRMotionAdaptAttr);

extern const struct isp_module_ctrl tnr_mod;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_TNR_CTRL_H_
