/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_param_header.h
 * Description:
 *
 */

#ifndef _ISP_PARAM_HEADER_H_
#define _ISP_PARAM_HEADER_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_3a.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
	//Pub attr
	ISP_PUB_ATTR_S pub_attr;

	// Pre-Raw
	ISP_BLACK_LEVEL_ATTR_S blc;
	ISP_DP_DYNAMIC_ATTR_S dpc_dynamic;
	ISP_DP_STATIC_ATTR_S dpc_static;
	ISP_DP_CALIB_ATTR_S DPCalib;
	ISP_CROSSTALK_ATTR_S crosstalk;

	// Raw-top
	ISP_NR_ATTR_S bnr;
	ISP_NR_FILTER_ATTR_S bnr_filter;
	ISP_DEMOSAIC_ATTR_S demosaic;
	ISP_DEMOSAIC_DEMOIRE_ATTR_S demosaic_demoire;
	ISP_RGBCAC_ATTR_S rgbcac;
	ISP_LCAC_ATTR_S lcac;
	ISP_DIS_ATTR_S disAttr;
	ISP_DIS_CONFIG_S disConfig;

	// RGB-top
	ISP_MESH_SHADING_ATTR_S mlsc;
	ISP_MESH_SHADING_GAIN_LUT_ATTR_S mlscLUT;
	ISP_SATURATION_ATTR_S Saturation;
	ISP_CCM_ATTR_S ccm;
	ISP_CCM_SATURATION_ATTR_S ccm_saturation;
	ISP_COLOR_TONE_ATTR_S colortone;
	ISP_FSWDR_ATTR_S fswdr;
	ISP_WDR_EXPOSURE_ATTR_S WDRExposure;
	ISP_DRC_ATTR_S drc;
	ISP_GAMMA_ATTR_S gamma;
	ISP_AUTO_GAMMA_ATTR_S autoGamma;
	ISP_DEHAZE_ATTR_S dehaze;
	ISP_CLUT_ATTR_S clut;
	ISP_CLUT_SATURATION_ATTR_S clut_saturation;
	ISP_CSC_ATTR_S csc;
	ISP_VC_ATTR_S vc_motion;

	// YUV-top
	ISP_DCI_ATTR_S dci;
	ISP_LDCI_ATTR_S ldci;
	ISP_PRESHARPEN_ATTR_S presharpen;
	ISP_TNR_ATTR_S tnr;
	ISP_TNR_NOISE_MODEL_ATTR_S tnr_noise_model;
	ISP_TNR_LUMA_MOTION_ATTR_S tnr_luma_motion;
	ISP_TNR_GHOST_ATTR_S tnr_ghost;
	ISP_TNR_MT_PRT_ATTR_S tnr_mt_prt;
	ISP_TNR_MOTION_ADAPT_ATTR_S tnr_motion_adapt;
	ISP_YNR_ATTR_S ynr;
	ISP_YNR_FILTER_ATTR_S ynr_filter;
	ISP_YNR_MOTION_NR_ATTR_S ynr_motion;
	ISP_CNR_ATTR_S cnr;
	ISP_CNR_MOTION_NR_ATTR_S cnr_motion;
	ISP_CAC_ATTR_S cac;
	ISP_SHARPEN_ATTR_S sharpen;
	ISP_CA_ATTR_S ca;
	ISP_CA2_ATTR_S ca2;
	ISP_YCONTRAST_ATTR_S ycontrast;

	// other
	ISP_CMOS_NOISE_CALIBRATION_S np;
	ISP_MONO_ATTR_S mono;
} ISP_Parameter_Structures;

typedef struct {
	ISP_WDR_EXPOSURE_ATTR_S WDRExpAttr;
	ISP_EXPOSURE_ATTR_S ExpAttr;
	ISP_AE_ROUTE_S AeRouteAttr;
	ISP_AE_ROUTE_EX_S AeRouteAttrEx;
	ISP_SMART_EXPOSURE_ATTR_S AeSmartExposureAttr;
	ISP_IRIS_ATTR_S AeIrisAttr;
	ISP_DCIRIS_ATTR_S AeDcirisAttr;
	ISP_AE_ROUTE_S AeRouteSFAttr;
	ISP_AE_ROUTE_EX_S AeRouteSFAttrEx;

	ISP_WB_ATTR_S WBAttr;
	ISP_AWB_ATTR_EX_S AWBAttrEx;
	ISP_AWB_Calibration_Gain_S WBCalib;
	ISP_AWB_Calibration_Gain_S_EX WBCalibEx;
	ISP_STATISTICS_CFG_S StatCfg;
} ISP_3A_Parameter_Structures;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_PARAM_HEADER_H_
