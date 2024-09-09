/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_main.h
 * Description:
 */

#ifndef _ISP_MAIN_H_
#define _ISP_MAIN_H_

#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_comm_3a.h"
#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef enum _AAA_LIB_TYPE_E {
	AAA_TYPE_AE = 0x0,
	AAA_TYPE_AWB,
	AAA_TYPE_AF,
	AAA_TYPE_MAX,
} AAA_LIB_TYPE_E;

typedef enum _ISP_IQ_BLOCK_LIST_E {
	// If you want to add modules, follow the order of isp pipeline
	ISP_PRE_RAW_START = 0,
	ISP_IQ_BLOCK_GMS = ISP_PRE_RAW_START,
	ISP_IQ_BLOCK_BLC,
	ISP_IQ_BLOCK_RLSC,
	ISP_IQ_BLOCK_FPN,
	ISP_IQ_BLOCK_DPC,
	ISP_IQ_BLOCK_CROSSTALK,
	ISP_IQ_BLOCK_WBGAIN,
	ISP_IQ_BLOCK_3AWIN,
	ISP_PRE_RAW_END,
	ISP_RAW_TOP_START = ISP_PRE_RAW_END,
	ISP_IQ_BLOCK_BNR = ISP_RAW_TOP_START,
	ISP_IQ_BLOCK_DEMOSAIC,
	ISP_IQ_BLOCK_RGBCAC,
	ISP_IQ_BLOCK_LCAC,
	ISP_RAW_TOP_END,
	ISP_RGB_TOP_START = ISP_RAW_TOP_END,
	ISP_IQ_BLOCK_MLSC,
	ISP_IQ_BLOCK_CCM,
	ISP_IQ_BLOCK_FUSION,
	ISP_IQ_BLOCK_DRC,
	ISP_IQ_BLOCK_GAMMA,
	ISP_IQ_BLOCK_DEHAZE,
	ISP_IQ_BLOCK_CLUT,
	ISP_IQ_BLOCK_CSC,
	ISP_IQ_BLOCK_DCI,
	ISP_IQ_BLOCK_LDCI,
	ISP_IQ_BLOCK_CA,
	ISP_IQ_BLOCK_PREYEE,
	ISP_IQ_BLOCK_MOTION,
	ISP_RGB_TOP_END,
	ISP_YUV_TOP_START = ISP_RGB_TOP_END,
	ISP_IQ_BLOCK_3DNR,
	ISP_IQ_BLOCK_YNR,
	ISP_IQ_BLOCK_CNR,
	ISP_IQ_BLOCK_CAC,
	ISP_IQ_BLOCK_CA2,
	ISP_IQ_BLOCK_YEE,
	ISP_IQ_BLOCK_YCONTRAST,
	ISP_IQ_BLOCK_MONO,
	ISP_YUV_TOP_END,
	ISP_IQ_BLOCK_MAX = ISP_YUV_TOP_END,
} ISP_IQ_BLOCK_LIST_E;

typedef enum _ISP_TEST_PARAM_E {
	ISP_TEST_PARAM_DEBUGLOG_LEVEL = 1,
	ISP_TEST_PARAM_DEBUGLOG_INTERVAL = 2,
	ISP_TEST_PARAM_AE_LSCR_ENABLE = 3,
	ISP_TEST_PARAM_AWB_LSCR_ENABLE = 4,
	ISP_TEST_PARAM_DRC_DEBUGLOG_LEVEL = 5
} ISP_TEST_PARAM_E;

CVI_S32 isp_iqParam_addr_initDefault(VI_PIPE ViPipe);
CVI_S32 isp_sensor_getId(VI_PIPE ViPipe, SENSOR_ID *pSensorId);
CVI_S32 isp_snsSync_cfg_set(VI_PIPE ViPipe);
CVI_S32 isp_sensor_updateBlc(VI_PIPE ViPipe, ISP_BLACK_LEVEL_ATTR_S **ppstSnsBlackLevel);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MAIN_H_
