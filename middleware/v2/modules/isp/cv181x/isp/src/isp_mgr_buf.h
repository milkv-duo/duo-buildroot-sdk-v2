/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mgr_buf.h
 * Description:
 *
 */

#ifndef _ISP_MGR_BUF_H_
#define _ISP_MGR_BUF_H_

#include "cvi_comm_isp.h"

#include "isp_mlsc_ctrl.h"
#include "isp_dpc_ctrl.h"
#include "isp_dci_ctrl.h"
#include "isp_demosaic_ctrl.h"
#include "isp_fswdr_ctrl.h"
#include "isp_drc_ctrl.h"
#include "isp_ycontrast_ctrl.h"
#include "isp_gamma_ctrl.h"
#include "isp_ldci_ctrl.h"
#include "isp_dehaze_ctrl.h"
#include "isp_lcac_ctrl.h"
#include "isp_rgbcac_ctrl.h"
#include "isp_tnr_ctrl.h"
#include "isp_bnr_ctrl.h"
#include "isp_presharpen_ctrl.h"
#include "isp_sharpen_ctrl.h"
#include "isp_blc_ctrl.h"
#include "isp_cnr_ctrl.h"
#include "isp_ca_ctrl.h"
#include "isp_cac_ctrl.h"
#include "isp_motion_ctrl.h"
#include "isp_csc_ctrl.h"
#include "isp_crosstalk_ctrl.h"
#include "isp_ca2_ctrl.h"
#include "isp_clut_ctrl.h"
#include "isp_dis_ctrl.h"
#include "isp_wb_ctrl.h"
#include "isp_mono_ctrl.h"
#include "isp_ccm_ctrl.h"
#include "isp_ynr_ctrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct isp_mlsc_shared_buffer {
	ISP_MESH_SHADING_ATTR_S mlsc;
	ISP_MESH_SHADING_GAIN_LUT_ATTR_S mlscLUT;
	struct isp_mlsc_ctrl_runtime runtime;
};

struct isp_dpc_shared_buffer {
	ISP_DP_DYNAMIC_ATTR_S stDPCDynamicAttr;
	ISP_DP_STATIC_ATTR_S stDPStaticAttr;
	ISP_DP_CALIB_ATTR_S stDPCalibAttr;
	struct isp_dpc_ctrl_runtime runtime;
};

struct isp_dci_shared_buffer {
	ISP_DCI_ATTR_S stDciAttr;
	struct isp_dci_ctrl_runtime runtime;
};

struct isp_demosaic_shared_buffer {
	ISP_DEMOSAIC_ATTR_S stDemosaicAttr;
	ISP_DEMOSAIC_DEMOIRE_ATTR_S stDemoireAttr;
	struct isp_demosaic_ctrl_runtime runtime;
};

struct isp_fswdr_shared_buffer {
	ISP_FSWDR_ATTR_S stFSWDRAttr;
	struct isp_fswdr_ctrl_runtime runtime;
};

struct isp_drc_shared_buffer {
	ISP_DRC_ATTR_S stDRCAttr;
	struct isp_drc_ctrl_runtime runtime;
};

struct isp_ycur_shared_buffer {
	ISP_YCONTRAST_ATTR_S stYContrastAttr;
	struct isp_ycontrast_ctrl_runtime runtime;
};

struct isp_gamma_shared_buffer {
	ISP_GAMMA_ATTR_S stGammaAttr;
	ISP_AUTO_GAMMA_ATTR_S stAutoGammaAttr;
	struct isp_gamma_ctrl_runtime runtime;
};

struct isp_ldci_shared_buffer {
	ISP_LDCI_ATTR_S stLdciAttr;
	struct isp_ldci_ctrl_runtime runtime;
};

struct isp_tnr_shared_buffer {
	ISP_TNR_ATTR_S stTNRAttr;
	ISP_TNR_NOISE_MODEL_ATTR_S stTNRNoiseModelAttr;
	ISP_TNR_LUMA_MOTION_ATTR_S stTNRLumaMotionAttr;
	ISP_TNR_GHOST_ATTR_S stTNRGhostAttr;
	ISP_TNR_MT_PRT_ATTR_S stTNRMtPrtAttr;
	ISP_TNR_MOTION_ADAPT_ATTR_S stTNRMotionAdaptAttr;
	struct isp_tnr_ctrl_runtime runtime;
};

struct isp_dehaze_shared_buffer {
	ISP_DEHAZE_ATTR_S stDehazeAttr;
	struct isp_dehaze_ctrl_runtime runtime;
};

struct isp_lcac_shared_buffer {
	ISP_LCAC_ATTR_S stLCACAttr;
	struct isp_lcac_ctrl_runtime runtime;
};

struct isp_rgbcac_shared_buffer {
	ISP_RGBCAC_ATTR_S stRGBCACAttr;
	struct isp_rgbcac_ctrl_runtime runtime;
};

struct isp_bnr_shared_buffer {
	ISP_NR_ATTR_S stNRAttr;
	ISP_NR_FILTER_ATTR_S stNRFilterAttr;
	struct isp_bnr_ctrl_runtime runtime;
};

struct isp_presharpen_shared_buffer {
	ISP_PRESHARPEN_ATTR_S stPreSharpenAttr;
	struct isp_presharpen_ctrl_runtime runtime;
};

struct isp_sharpen_shared_buffer {
	ISP_SHARPEN_ATTR_S stSharpenAttr;
	struct isp_sharpen_ctrl_runtime runtime;
};

struct isp_blc_shared_buffer {
	ISP_BLACK_LEVEL_ATTR_S stBlackLevelAttr;
	struct isp_blc_ctrl_runtime runtime;
};

struct isp_cnr_shared_buffer {
	ISP_CNR_ATTR_S stCNRAttr;
	ISP_CNR_MOTION_NR_ATTR_S stCNRMotionNRAttr;
	struct isp_cnr_ctrl_runtime runtime;
};

struct isp_ca_shared_buffer {
	ISP_CA_ATTR_S stCAAttr;
	struct isp_ca_ctrl_runtime runtime;
};

struct isp_cac_shared_buffer {
	ISP_CAC_ATTR_S stCACAttr;
	struct isp_cac_ctrl_runtime runtime;
};

struct isp_motion_shared_buffer {
	ISP_VC_ATTR_S stMotionAttr;
	struct isp_motion_ctrl_runtime runtime;
};

struct isp_3a_shared_buffer {
	ISP_EXPOSURE_ATTR_S stExpAttr;
	ISP_EXP_INFO_S stExpInfo;
	ISP_WDR_EXPOSURE_ATTR_S stWDRExpAttr;
	ISP_AE_ROUTE_S stAERouteAttr;
	ISP_AE_ROUTE_EX_S stAERouteAttrEx;
	ISP_SMART_EXPOSURE_ATTR_S stSmartExpAttr;
	ISP_AE_STATISTICS_CFG_S stAeStatCfg;
	ISP_AE_ROUTE_S stAERouteSFAttr;
	ISP_AE_ROUTE_EX_S stAERouteSFAttrEx;
	ISP_IRIS_ATTR_S stIrisAttr;
	ISP_DCIRIS_ATTR_S stDcirisAttr;
	CVI_U32 u32AEParamUpdateFlag;

	ISP_WB_Q_INFO_S stWB_Q_Info;
	ISP_WB_ATTR_S stWBAttr;
	ISP_AWB_ATTR_EX_S stAWBAttrEx;
	ISP_AWB_Calibration_Gain_S stWBCalib;
	CVI_U32 u32AWBParamUpdateFlag;
};

struct isp_csc_shared_buffer {
	ISP_CSC_ATTR_S stCSCAttr;
	struct isp_csc_ctrl_runtime runtime;
};

struct isp_crosstalk_shared_buffer {
	ISP_CROSSTALK_ATTR_S stCrosstalkAttr;
	struct isp_crosstalk_ctrl_runtime runtime;
};

struct isp_ca2_shared_buffer {
	ISP_CA2_ATTR_S stCA2Attr;
	struct isp_ca2_ctrl_runtime runtime;
};

struct isp_clut_shared_buffer {
	ISP_CLUT_ATTR_S stCLUTAttr;
	ISP_CLUT_SATURATION_ATTR_S stClutSaturationAttr;
	struct isp_clut_ctrl_runtime runtime;
};

struct isp_dis_shared_buffer {
	ISP_DIS_ATTR_S stDisAttr;
	ISP_DIS_CONFIG_S stDisConfig;
	struct isp_dis_ctrl_runtime runtime;
};

struct isp_wb_shared_buffer {
	ISP_COLOR_TONE_ATTR_S stColorToneAttr;
	struct isp_wb_ctrl_runtime runtime;
};

struct isp_ccm_shared_buffer {
	ISP_CCM_ATTR_S stCCMAttr;
	ISP_CCM_SATURATION_ATTR_S stCCMSaturationAttr;
	ISP_SATURATION_ATTR_S stSaturationAttr;
	struct isp_ccm_ctrl_runtime runtime;
};

struct isp_ynr_shared_buffer {
	ISP_YNR_ATTR_S stYNRAttr;
	ISP_YNR_FILTER_ATTR_S stYNRFilterAttr;
	ISP_YNR_MOTION_NR_ATTR_S stYNRMotionNRAttr;
	struct isp_ynr_ctrl_runtime runtime;
};

struct isp_mono_shared_buffer {
	ISP_MONO_ATTR_S stMonoAttr;
	struct isp_mono_ctrl_runtime runtime;
};

CVI_S32 isp_mgr_buf_init(VI_PIPE ViPipe);
CVI_S32 isp_mgr_buf_uninit(VI_PIPE ViPipe);

CVI_S32 isp_mgr_buf_get_event_addr(VI_PIPE ViPipe, CVI_U64 *paddr, CVI_VOID **vaddr);
CVI_S32 isp_mgr_buf_get_addr(VI_PIPE ViPipe, ISP_IQ_BLOCK_LIST_E block, CVI_VOID **addr);
CVI_S32 isp_mgr_buf_get_ctx_addr(VI_PIPE ViPipe, CVI_VOID **addr);
CVI_S32 isp_mgr_buf_get_tun_buf_ctrl_runtime_addr(VI_PIPE ViPipe, CVI_VOID **addr);
CVI_S32 isp_mgr_buf_get_sts_ctrl_runtime_addr(VI_PIPE ViPipe, CVI_VOID **addr);

CVI_S32 isp_mgr_buf_get_np_addr(VI_PIPE ViPipe, CVI_VOID **addr);
CVI_S32 isp_mgr_buf_get_3a_addr(VI_PIPE ViPipe, CVI_VOID **addr);

CVI_S32 isp_mgr_buf_invalid_cache(VI_PIPE ViPipe);
CVI_S32 isp_mgr_buf_flush_cache(VI_PIPE ViPipe); // Conflict with C906L !!!

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MGR_BUF_H_
