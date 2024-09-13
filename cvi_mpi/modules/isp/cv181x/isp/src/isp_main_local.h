/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_main_local.h
 * Description:
 */

#ifndef _ISP_MAIN_LOCAL_H_
#define _ISP_MAIN_LOCAL_H_

#include <pthread.h>
#include "isp_main.h"
#include "isp_comm_inc.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define ISP_TUNING_BUF_NULL		(-2)

// TODO@CF remove this if marco for wdr ready
#define IS_2to1_WDR_MODE(mode)	(\
		(WDR_MODE_2To1_FRAME == (mode))\
		|| (WDR_MODE_2To1_FRAME_FULL_RATE == (mode))\
		|| (WDR_MODE_2To1_LINE == (mode))\
		|| (WDR_MODE_QUDRA == (mode))\
		)

typedef enum _WDR_TYPE_E {
	WDR_TYPE_NONE = 0x0,
	WDR_TYPE_TWO_2_ONE = 0x1,
	WDR_TYPE_MAX,
} WDR_TYPE_E;

typedef struct _ISP_LIB_INFO_S {
	CVI_U32 used;
	ALG_LIB_S libInfo;
	union {
		ISP_AE_EXP_FUNC_S aeFunc;
		ISP_AWB_EXP_FUNC_S awbFunc;
		ISP_AF_EXP_FUNC_S afFunc;
	} algoFunc;
} ISP_LIB_INFO_S;

typedef struct _ISP_SENSOR_IMAGE_MODE_S {
	CVI_U16 u16Width;
	CVI_U16 u16Height;
	CVI_FLOAT f32Fps;
	CVI_U8 u8SnsMode;
} ISP_SENSOR_IMAGE_MODE_S;

typedef struct _ISP_CTX_S {
	/* 1. ctrl param */
	CVI_BOOL bSnsReg;
	CVI_U32 ispDevFd;
	CVI_U32 snsDevFd;
	RECT_S stSysRect;
	CVI_BOOL bIspRun;
	CVI_BOOL bWDRSwitchFinish;
	ISP_CTRL_PARAM_S ispProcCtrl;
	enum ISP_SCENE_INFO scene;
	enum VI_EVENT enPreViEvent;

	/* 2. algorithm ctrl param */
	CVI_U32 frameCnt;
	CVI_BOOL wdrLinearMode;
	WDR_MODE_E u8SnsWDRMode;
	ISP_BAYER_FORMAT_E enBayer;

	ISP_AE_RESULT_S stAeResult;
	ISP_AWB_RESULT_S stAwbResult;

	/* 3. 3a register info */
	ISP_BIND_ATTR_S stBindAttr;
	int activeLibIdx[AAA_TYPE_MAX];
	CVI_U8 u8AERunInterval;
	CVI_U8 u8AEWaitFrame;
	CVI_BOOL bAeLscRCompensate;
	CVI_BOOL bAwbLscRCompensate;

	ISP_SENSOR_IMAGE_MODE_S stSnsImageMode;

	ISP_FRAME_INFO_S frameInfo;
	ISP_STATISTICS_CFG_S stsCfgInfo;

	pthread_mutex_t ispEventLock;
	pthread_cond_t ispEventCond[ISP_VD_MAX];

} ISP_CTX_S;

typedef struct {
	// If you want to add modules, follow the order of isp pipeline
	// PRE_RAW
	ISP_BLACK_LEVEL_ATTR_S blc;
	//ISP_RADIAL_SHADING_ATTR_S rlsc;
	//ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S rlscLUT;
	ISP_DP_DYNAMIC_ATTR_S dpc_dynamic;
	ISP_DP_STATIC_ATTR_S dpc_static;
	ISP_DP_CALIB_ATTR_S DPCalib;
	ISP_CROSSTALK_ATTR_S crosstalk;
	// RAW_TOP
	ISP_NR_ATTR_S bnr;
	ISP_NR_FILTER_ATTR_S bnr_filter;
	ISP_DEMOSAIC_ATTR_S demosaic;
	ISP_DEMOSAIC_DEMOIRE_ATTR_S demosaic_demoire;
	ISP_RGBCAC_ATTR_S rgbcac;
	ISP_LCAC_ATTR_S lcac;
	// RGB_TOP
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
	ISP_DCI_ATTR_S dci;
	ISP_LDCI_ATTR_S ldci;
	ISP_CA_ATTR_S ca;
	ISP_PRESHARPEN_ATTR_S presharpen;
	ISP_VC_ATTR_S vc_motion;
	// YUV_TOP
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
	ISP_CA2_ATTR_S ca2;
	ISP_SHARPEN_ATTR_S sharpen;
	ISP_YCONTRAST_ATTR_S ycontrast;
	ISP_MONO_ATTR_S mono;

	ISP_CMOS_NOISE_CALIBRATION_S np;
	ISP_DIS_ATTR_S disAttr;
	ISP_DIS_CONFIG_S disConfig;
} ISP_PARAMETER_BUFFER;

//extern ISP_PARAMETER_BUFFER g_param[VI_MAX_PIPE_NUM];

extern ISP_CTX_S *g_astIspCtx[VI_MAX_PIPE_NUM];
extern CVI_S32 isp_mgr_buf_init(VI_PIPE ViPipe);
extern CVI_U8 g_ispCtxBufCnt;

#define ISP_GET_CTX(dev, pstIspCtx) ({    \
	if (g_astIspCtx[dev] == CVI_NULL) {   \
		isp_mgr_buf_init(dev);            \
	}                                     \
	pstIspCtx = g_astIspCtx[dev];         \
})

#define IS_MULTI_CAM() (g_ispCtxBufCnt > 1 ? 1 : 0)

/*isp_3a.c*/
CVI_S32 isp_3aLib_find(VI_PIPE ViPipe, const ALG_LIB_S *pstAeLib, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_reg(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, void *pstRegister, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_unreg(VI_PIPE ViPipe, ALG_LIB_S *pstLib, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_get(VI_PIPE ViPipe, ALG_LIB_S *pstAlgoLib, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_init(VI_PIPE ViPipe, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_run(VI_PIPE ViPipe, AAA_LIB_TYPE_E type);
CVI_S32 isp_3aLib_exit(VI_PIPE ViPipe, AAA_LIB_TYPE_E type);
CVI_S32 isp_3a_frameCnt_set(VI_PIPE ViPipe, CVI_U32 frameCnt);
CVI_S32 isp_3a_update_proc_info(VI_PIPE ViPipe);

CVI_S32 isp_run(VI_PIPE ViPipe);

/*isp_sensor.c*/
CVI_S32 isp_sensor_register(CVI_S32 ViPipe, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo, ISP_SENSOR_REGISTER_S *pstRegister);
CVI_S32 isp_sensor_unRegister(CVI_S32 ViPipe);
CVI_S32 isp_sensor_init(CVI_S32 ViPipe);
CVI_S32 isp_sensor_exit(CVI_S32 ViPipe);
CVI_S32 isp_sensor_setWdrMode(CVI_S32 ViPipe, WDR_MODE_E wdrMode);
CVI_S32 isp_sensor_switchMode(CVI_S32 ViPipe);
CVI_S32 isp_sns_info_init(VI_PIPE ViPipe);
CVI_S32 isp_sensor_default_get(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S **ppstSnsDft);
CVI_S32 isp_sensor_get_crop_info(CVI_S32 ViPipe, ISP_SNS_SYNC_INFO_S *snsCropInfo);

/*isp_flow_ctrl.c*/
CVI_S32 isp_flow_event_siganl(VI_PIPE ViPipe, ISP_VD_TYPE_E eventType);

// implement in sys/src/cvi_base.c
extern int open_device(const char *dev_name, CVI_S32 *fd);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MAIN_LOCAL_H_
