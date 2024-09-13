/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_json_structure_local.h
 * Description:
 */

#ifndef _ISP_JSON_STRUCT_LOCAL_H_
#define _ISP_JSON_STRUCT_LOCAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "cvi_isp.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_af.h"
#include "json.h"

// -----------------------------------------------------------------------------
#define JSON	struct json_object

// -----------------------------------------------------------------------------
// ISP Pre-RAW
// -----------------------------------------------------------------------------
void ISP_BLACK_LEVEL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_BLACK_LEVEL_ATTR_S *data);
void ISP_DP_DYNAMIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_DYNAMIC_ATTR_S *data);
void ISP_DP_STATIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_STATIC_ATTR_S *data);
void ISP_CROSSTALK_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CROSSTALK_ATTR_S *data);

// -----------------------------------------------------------------------------
// ISP RAW-Top
// -----------------------------------------------------------------------------
void ISP_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_ATTR_S *data);
void ISP_NR_FILTER_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_FILTER_ATTR_S *data);
void ISP_DEMOSAIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_ATTR_S *data);
void ISP_DEMOSAIC_DEMOIRE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_DEMOIRE_ATTR_S *data);
void ISP_RGBCAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_RGBCAC_ATTR_S *data);
void ISP_LCAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LCAC_ATTR_S *data);
void ISP_DIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DIS_ATTR_S *data);
void ISP_DIS_CONFIG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DIS_CONFIG_S *data);

// -----------------------------------------------------------------------------
// ISP RGB-Top
// -----------------------------------------------------------------------------
void ISP_MESH_SHADING_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MESH_SHADING_ATTR_S *data);
void ISP_MESH_SHADING_GAIN_LUT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MESH_SHADING_GAIN_LUT_ATTR_S *data);
void ISP_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SATURATION_ATTR_S *data);
void ISP_CCM_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_ATTR_S *data);
void ISP_CCM_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_SATURATION_ATTR_S *data);
void ISP_COLOR_TONE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_COLOR_TONE_ATTR_S *data);
void ISP_FSWDR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_FSWDR_ATTR_S *data);
void ISP_WDR_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WDR_EXPOSURE_ATTR_S *data);
void ISP_DRC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DRC_ATTR_S *data);
void ISP_GAMMA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_GAMMA_ATTR_S *data);
void ISP_AUTO_GAMMA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AUTO_GAMMA_ATTR_S *data);
void ISP_DEHAZE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEHAZE_ATTR_S *data);
void ISP_CLUT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CLUT_ATTR_S *data);
void ISP_CLUT_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CLUT_SATURATION_ATTR_S *data);
void ISP_CSC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CSC_ATTR_S *data);
void ISP_VC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_VC_ATTR_S *data);

// -----------------------------------------------------------------------------
// ISP YUV-Top
// -----------------------------------------------------------------------------
void ISP_DCI_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCI_ATTR_S *data);
void ISP_LDCI_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LDCI_ATTR_S *data);
void ISP_PRESHARPEN_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_PRESHARPEN_ATTR_S *data);
void ISP_TNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_ATTR_S *data);
void ISP_TNR_NOISE_MODEL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_NOISE_MODEL_ATTR_S *data);
void ISP_TNR_LUMA_MOTION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_LUMA_MOTION_ATTR_S *data);
void ISP_TNR_GHOST_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_GHOST_ATTR_S *data);
void ISP_TNR_MT_PRT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MT_PRT_ATTR_S *data);
void ISP_TNR_MOTION_ADAPT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MOTION_ADAPT_ATTR_S *data);
void ISP_YNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_ATTR_S *data);
void ISP_YNR_FILTER_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_FILTER_ATTR_S *data);
void ISP_YNR_MOTION_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_MOTION_NR_ATTR_S *data);
void ISP_CNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_ATTR_S *data);
void ISP_CNR_MOTION_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_MOTION_NR_ATTR_S *data);
void ISP_CAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CAC_ATTR_S *data);
void ISP_SHARPEN_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SHARPEN_ATTR_S *data);
void ISP_CA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA_ATTR_S *data);
void ISP_CA2_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA2_ATTR_S *data);
void ISP_YCONTRAST_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YCONTRAST_ATTR_S *data);

// -----------------------------------------------------------------------------
// ISP Other
// -----------------------------------------------------------------------------
void ISP_CMOS_NOISE_CALIBRATION_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CMOS_NOISE_CALIBRATION_S *data);
void ISP_MONO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MONO_ATTR_S *data);

// -----------------------------------------------------------------------------
// AE
// -----------------------------------------------------------------------------
void ISP_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_EXPOSURE_ATTR_S *data);
void ISP_AE_ROUTE_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_S *data);
void ISP_AE_ROUTE_EX_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_EX_S *data);
void ISP_SMART_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SMART_EXPOSURE_ATTR_S *data);
void ISP_IRIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_IRIS_ATTR_S *data);
void ISP_DCIRIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCIRIS_ATTR_S *data);

// -----------------------------------------------------------------------------
// AWB
// -----------------------------------------------------------------------------
void ISP_WB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_ATTR_S *data);
void ISP_AWB_ATTR_EX_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_ATTR_EX_S *data);
void ISP_AWB_Calibration_Gain_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_Calibration_Gain_S *data);

// -----------------------------------------------------------------------------
// AF
// -----------------------------------------------------------------------------
// void ISP_AF_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_CFG_S *data);

// -----------------------------------------------------------------------------
// 3A
// -----------------------------------------------------------------------------
void ISP_STATISTICS_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_STATISTICS_CFG_S *data);

// -----------------------------------------------------------------------------
// Read only
// -----------------------------------------------------------------------------
void ISP_EXP_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_EXP_INFO_S *data);
void ISP_WB_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_INFO_S *data);

#endif // _ISP_JSON_STRUCT_LOCAL_H_
