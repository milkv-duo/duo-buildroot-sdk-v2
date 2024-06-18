/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_proc_local.h
 * Description:
 *
 */

#ifndef _ISP_PROC_LOCAL_H_
#define _ISP_PROC_LOCAL_H_

#include <stdbool.h>
#include "cvi_comm_isp.h"
#include "isp_proc.h"
#include "isp_main_local.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef ENABLE_ISP_PROC_DEBUG
typedef struct _ISP_DEBUGINFO_PROC1_S {
	// FSWDR
	//common
	CVI_BOOL FSWDREnable;
	CVI_BOOL FSWDRisManualMode;
	CVI_BOOL FSWDRWDRCombineSNRAwareEn;
	CVI_U16 FSWDRWDRCombineSNRAwareLowThr;
	CVI_U16 FSWDRWDRCombineSNRAwareHighThr;
	CVI_U16 FSWDRWDRCombineSNRAwareSmoothLevel;
	//manual or auto
	CVI_U8 FSWDRWDRCombineSNRAwareToleranceLevel;

	// MeshShading
	//common
	CVI_BOOL MeshShadingEnable;
	CVI_BOOL MeshShadingisManualMode;
	CVI_BOOL MeshShadingGLSize;
	CVI_U16 MeshShadingGLColorTemperature[ISP_MLSC_COLOR_TEMPERATURE_SIZE];

	// RadialShading
	//common
	CVI_BOOL RadialShadingEnable;
	CVI_U16 RadialShadingCenterX;
	CVI_U16 RadialShadingCenterY;
	CVI_U16 RadialShadingStr;

	// DCI
	//common
	CVI_BOOL DCIEnable;
	CVI_BOOL DCIisManualMode;
	CVI_U16 DCISpeed;
	CVI_U16 DCIDciStrength;
	//manual or auto
	CVI_U16 DCIContrastGain;
	CVI_U8 DCIBlcThr;
	CVI_U8 DCIWhtThr;
	CVI_U16 DCIBlcCtrl;
	CVI_U16 DCIWhtCtrl;

	// LDCI
	//common
	CVI_BOOL LDCIEnable;

	// Dehaze
	//common
	CVI_BOOL DehazeEnable;
	CVI_BOOL DehazeisManualMode;

	// BlackLevel
	//common
	CVI_U8 BlackLevelEnable;
	CVI_BOOL BlackLevelisManualMode;

	// DPC Dynamic
	//common
	CVI_BOOL DPCEnable;
	CVI_BOOL DPCisManualMode;
	//manual or auto
	CVI_U8 DPCClusterSize;

	// TNR
	//common
	CVI_BOOL TNREnable;
	CVI_BOOL TNRisManualMode;
	CVI_U8 TNRDeflickerMode;
	CVI_U16 TNRDeflickerToleranceLevel;
	CVI_U8 TNRMtDetectUnit;
	CVI_U8 TNRMtDetectUnitList[ISP_AUTO_ISO_STRENGTH_NUM];
	//manual or auto
	CVI_S16 TNRBrightnessNoiseLevelLE;
	CVI_S16 TNRBrightnessNoiseLevelSE;
	CVI_U8 TNRRNoiseLevel0;
	CVI_U8 TNRRNoiseHiLevel0;
	CVI_U8 TNRGNoiseLevel0;
	CVI_U8 TNRGNoiseHiLevel0;
	CVI_U8 TNRBNoiseLevel0;
	CVI_U8 TNRBNoiseHiLevel0;
	CVI_U8 TNRRNoiseLevel1;
	CVI_U8 TNRRNoiseHiLevel1;
	CVI_U8 TNRGNoiseLevel1;
	CVI_U8 TNRGNoiseHiLevel1;
	CVI_U8 TNRBNoiseLevel1;
	CVI_U8 TNRBNoiseHiLevel1;

	// CAC
	//common
	CVI_BOOL CACEnable;

	// RGBCAC
	//common
	CVI_BOOL RGBCACEnable;

	// LCAC
	//common
	CVI_BOOL LCACEnable;

	// CNR
	//common
	CVI_BOOL CNREnable;
	CVI_BOOL CNRisManualMode;
	//manual or auto
	CVI_U8 CNRNoiseSuppressGain;
	CVI_U8 CNRFilterType;
	CVI_U8 CNRMotionNrStr;

	// Sharpen
	//common
	CVI_BOOL SharpenEnable;
	CVI_BOOL SharpenisManualMode;
	CVI_BOOL SharpenLumaAdpCoringEn;
	CVI_BOOL SharpenWdrCoringCompensationEn;
	CVI_U8 SharpenWdrCoringCompensationMode;
	CVI_U16 SharpenWdrCoringToleranceLevel;
	CVI_U8 SharpenWdrCoringHighThr;
	CVI_U8 SharpenWdrCoringLowThr;
	//manual or auto
	CVI_U8 SharpenEdgeFreq;
	CVI_U8 SharpenTextureFreq;
	CVI_U8 SharpenYNoiseLevel;

	// Gamma
	//common
	CVI_BOOL GammaEnable;
	ISP_GAMMA_CURVE_TYPE_E GammaenCurveType;

	// CCM
	//common
	CVI_BOOL CCMEnable;
	CVI_BOOL CCMisManualMode;
	//manual
	CVI_U8 CCMSatEnable;
	CVI_S16 CCMCCM[9];
	//auto
	CVI_BOOL CCMISOActEnable;
	CVI_BOOL CCMTempActEnable;
	CVI_U8 CCMTabNum;
	ISP_COLORMATRIX_ATTR_S CCMTab[7];
	//manual or auto
	CVI_U8 SaturationLE[ISP_AUTO_ISO_STRENGTH_NUM];
	CVI_U8 SaturationSE[ISP_AUTO_ISO_STRENGTH_NUM];

	// YNR
	//common
	CVI_BOOL YNREnable;
	CVI_BOOL YNRisManualMode;
	CVI_BOOL YNRCoringParamEnable;
	//manual or auto
	CVI_U8 YNRWindowType;
	CVI_U8 YNRFilterType;

	// ExposureInfo
	ISP_EXP_INFO_S tAEExp_Info;

	// WBInfo
	ISP_WB_INFO_S tWB_Info;

	// BNR
	//common
	CVI_BOOL BNREnable;
	CVI_BOOL BNRisManualMode;
	//manual or auto
	CVI_U8 BNRWindowType;
	CVI_U8 BNRFilterType;

	// CLut
	//common
	CVI_BOOL CLutEnable;

	// Crosstalk
	//common
	CVI_BOOL CrosstalkEnable;
	CVI_BOOL CrosstalkisManualMode;

	// Demosaic
	//common
	CVI_BOOL DemosaicEnable;
	CVI_BOOL DemosaicisManualMode;

	// DRC
	//common
	CVI_BOOL DRCEnable;
	CVI_BOOL DRCisManualMode;
	CVI_U32 DRCToneCurveSmooth;
	CVI_U16 DRCHdrStrength;
} ISP_DEBUGINFO_PROC_S;

typedef enum _ISP_PROC_LEVEL_E {
	PROC_LEVEL_0,//disable proc
	PROC_LEVEL_1,
	PROC_LEVEL_2,
	PROC_LEVEL_3,
	PROC_LEVEL_NUM
} ISP_PROC_LEVEL_E;

typedef enum _ISP_PROC_STATUS_E {
	PROC_STATUS_START,
	PROC_STATUS_READY,
	PROC_STATUS_RECEVENT,
	PROC_STATUS_UPDATEDATA,
	PROC_STATUS_NUM
} ISP_PROC_STATUS_E;

typedef enum _ISP_PROC_BLOCK_LIST_E {
	ISP_COMMON_START = 0,
	ISP_PROC_BLOCK_VERSION = ISP_COMMON_START,
	ISP_PROC_BLOCK_CTRLPARAM,
	ISP_COMMON_END,
	ISP_LEVEL1_START = ISP_COMMON_END,
	ISP_PROC_BLOCK_FSWDR = ISP_LEVEL1_START,
	ISP_PROC_BLOCK_SHADING,
	ISP_PROC_BLOCK_DCI,
	ISP_PROC_BLOCK_LDCI,
	ISP_PROC_BLOCK_DEHAZE,
	ISP_PROC_BLOCK_BLACKLEVEL,
	ISP_PROC_BLOCK_DPC,
	ISP_PROC_BLOCK_TNR,
	ISP_PROC_BLOCK_CAC,
	ISP_PROC_BLOCK_RGBCAC,
	ISP_PROC_BLOCK_LCAC,
	ISP_PROC_BLOCK_CNR,
	ISP_PROC_BLOCK_SHARPEN,
	ISP_PROC_BLOCK_SATURATION,
	ISP_PROC_BLOCK_GAMMA,
	ISP_PROC_BLOCK_CCM,
	ISP_PROC_BLOCK_YNR,
	ISP_PROC_BLOCK_EXPOSUREINFO,
	ISP_PROC_BLOCK_WBINFO,
	ISP_PROC_BLOCK_BNR,
	ISP_PROC_BLOCK_CLUT,
	ISP_PROC_BLOCK_CROSSTALK,
	ISP_PROC_BLOCK_DEMOSAIC,
	ISP_PROC_BLOCK_DRC,
	ISP_LEVEL1_END,
	ISP_LEVEL2_START = ISP_LEVEL1_END,
	ISP_LEVEL2_END = ISP_LEVEL2_START,
	ISP_LEVEL3_START = ISP_LEVEL2_END,
	ISP_LEVEL3_END = ISP_LEVEL3_START,
	ISP_PROC_BLOCK_MAX = ISP_LEVEL3_END,
	ISP_PROC_BLOCK_SIZE
} ISP_PROC_BLOCK_LIST_E;

typedef CVI_S32 (*ispProcFormatFunc)(VI_PIPE ViPipe, CVI_CHAR *buffer, CVI_S32 bufferSize, CVI_S32 *actualSize);

typedef struct _ISP_PROC_BLOCK {
	ispProcFormatFunc formatFunc;
	CVI_BOOL bFormat;
} ISP_PROC_BLOCK;

typedef struct _ISP_AAA_VERSION {
	CVI_U16 Ver;
	CVI_U16 SubVer;
} ISP_AAA_VERSION;

extern ISP_DEBUGINFO_PROC_S *g_pIspTempProcST[VI_MAX_PIPE_NUM];
extern ISP_CTX_S *g_astIspCtx[VI_MAX_PIPE_NUM];
#define ISP_GET_PROC_INFO(dev, pstIspProc) ((pstIspProc) = g_pIspTempProcST[(dev)])
#define ISP_GET_PROC_CTRL(dev, pstIspCtrl) ((pstIspCtrl) = &(g_astIspCtx[(dev)]->ispProcCtrl))
#define ISP_GET_PROC_ACTION(dev, procLevel) \
((g_astIspCtx[(dev)]->ispProcCtrl.u32ProcLevel >= (procLevel)) ? true : false)

CVI_S32 isp_proc_buf_init(VI_PIPE ViPipe);
CVI_S32 isp_proc_buf_deinit(VI_PIPE ViPipe);
CVI_S32 isp_proc_updateData2ProcST(VI_PIPE ViPipe);
CVI_S32 isp_proc_setProcStatus(ISP_PROC_STATUS_E eStatus);
ISP_PROC_STATUS_E isp_proc_getProcStatus(void);
CVI_S32 isp_proc_run(void);
CVI_S32 isp_proc_exit(void);

CVI_S32 isp_proc_collectAeInfo(VI_PIPE ViPipe, const ISP_EXP_INFO_S *pstAeExpInfo);
CVI_S32 isp_proc_collectAwbInfo(VI_PIPE ViPipe, const ISP_WB_INFO_S *pstAwbInfo);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_PROC_LOCAL_H_
