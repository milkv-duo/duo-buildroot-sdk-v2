/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: aealgo.h
 * Description:
 *
 */

#ifndef _AE_ALG_H_
#define _AE_ALG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define AE_LIB_VER     (1)//U8
#define AE_LIB_SUBVER  (53)//U8

#ifndef AAA_PC_PLATFORM

#include <stdio.h>
#include <string.h>
#include "cvi_comm_3a.h"
#include "cvi_ae_comm.h"
#include "cvi_ae.h"
#include "ae_common.h"
#include "cvi_comm_isp.h"

#else

#include <stdio.h>
#include <string.h>
#include "cvi_type.h"
#include "cvi_comm_3a.h"
#include "cvi_ae_comm.h"
#include "cvi_ae.h"
#include "cvi_comm_isp.h"
#endif

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define AE_SENSOR_NUM (VI_MAX_PIPE_NUM)

#define AE_GAIN_BASE 1024
#define AE_EVBIAS_BASE 1024
#define AE_WDR_RATIO_BASE 64
#define EXP_COEF_BASE	1024

#define MAX_WDR_FRAME_NUM 2
#define AE_CHANNEL_NUM 4
#define AE_MAX_PERIOD_NUM 5//4 //3

#define AE_DEFAULT_TARGET_LUMA	56
#define AE_MAX_LUMA 1023
#define AE_METER_MIN_LUMA	5

#define HIGH_LIGHT_ZONE_WEIGHT_RATIO_THR	4	// 1/255 = 0. 4%

#define MIN_EVBIAS	(AE_GAIN_BASE / 4)
#define MAX_EVBIAS	(AE_GAIN_BASE * 4)

#define MIN_ISPDGAIN 1024
#define MAX_ISPDGAIN 32767

#define SENSOR_DGAIN_MAX_NODE_NUM	7

#define MAX_AE_LOG_PATH_LENGTH	128
#define MAX_AE_LOG_FILENAME_LENGTH	(MAX_AE_LOG_PATH_LENGTH * 2 + 32)
#define	FLOAT_TO_INT_NUMBER		1000000

#define LINEAR_MIN_LVX100 (0)
#define LINEAR_MAX_LVX100 (1500)
#define WDR_MIN_LVX100 (0)
#define WDR_MAX_LVX100 (1500)

#define SUNSET_LV	(600)
#define NIGHT_LV	(400)

#define TV_ENTRY_WITH_ISPDGAIN_COMPENSATION		(EVTT_ENTRY_1_960SEC)

#define AAA_LIMIT(var, min, max) ((var) = ((var) < (min)) ? (min) : (((var) > (max)) ? (max) : (var)))
#define AAA_ABS(a) ((a) > 0 ? (a) : -(a))
#define AAA_MIN(a, b) ((a) < (b) ? (a) : (b))
#define AAA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define AAA_DIV_0_TO_1(a) ((0 == (a)) ? 1 : (a))

typedef enum _CONVERGE_MODE {
	CONVERGE_FAST,
	CONVERGE_NORMAL,
} CONVERGE_MODE;

typedef enum _ADJUST_ITEM {
	ADJ_TV,
	ADJ_SV,
} ADJUST_ITEM;


typedef enum _AE_WDR_FRAME {
	AE_LE = 0,
	AE_SE,
	AE_MAX_WDR_FRAME_NUM,
} AE_WDR_FRAME;

typedef enum _AE_SENSOR_MODE {
	AE_LINEAR_MODE = 0,
	AE_WDR_MODE,
	AE_TOTAL_SENSOR_MODE,
} AE_SENSOR_MODE;


typedef enum _AE_ROUTE_CHANGE_ITEM {
	AE_CHANGE_NONE = 0,
	AE_CHANGE_SHUTTER,
	AE_CHANGE_ISO,
	AE_CHANGE_AGAIN,
	AE_CHANGE_DGAIN,
	AE_CHANGE_ISPDGAIN,
	AE_CHANGE_SHUTTER_ISO,
} AE_ROUTE_CHANGE_ITEM;

typedef enum _AE_PARAMETER_UPDATE {
	AE_EXPOSURE_ATTR_UPDATE,
	AE_WDR_EXPOSURE_ATTR_UPDATE,
	AE_ROUTE_UPDATE,
	AE_ROUTE_EX_UPDATE,
	AE_STATISTICS_CONFIG_UPDATE,
	AE_SMART_EXPOSURE_ATTR_UPDATE,
	AE_IRIS_ATTR_UPDATE,
	AE_DC_IRIS_ATTR_UPDATE,
	AE_ROUTE_SF_UPDATE,
	AE_ROUTE_SF_EX_UPDATE,
	AE_UPDATE_ALL,
	AE_UPDATE_TOTAL,
} AE_PARAMETER_UPDATE;

typedef enum _AE_METER_WEIGHT {
	AE_RGB_WEIGHT,
	AE_IR_WEIGHT,
} AE_METER_WEIGHT;

typedef enum _AE_IRIS_STEP {
	AE_AV_ENTRY_0,
	AE_AV_ENTRY_1,
	AE_AV_ENTRY_2,
	AE_AV_ENTRY_3,
	AE_AV_ENTRY_4,
	AE_AV_ENTRY_5,
	AE_AV_ENTRY_6,
	AE_AV_ENTRY_7,
	AE_AV_ENTRY_8,
	AE_AV_ENTRY_9,
	AE_AV_ENTRY_10,
} AE_IRIS_STEP;

typedef enum _AE_SLOW_SHUTTER_STATUS {
	AE_LEAVE_SLOW_SHUTTER,
	AE_ENTER_SLOW_SHUTTER,
} AE_SLOW_SHUTTER_STATUS;

typedef enum _AE_GAIN_TYPE {
	AE_AGAIN,
	AE_DGAIN,
	AE_ISPDGAIN,
	AE_GAIN_TOTAL,
} AE_GAIN_TYPE;

typedef enum _AE_ISPDGAIN_COMPENSATION {
	DISABLE_COMPENSATION,
	ENABLE_COMPENSATION,
} AE_ISPDGAIN_COMPENSATION;

typedef enum _AE_ADJUST_TARGET_STATUS {
	AE_ADJUST_NONE,
	AE_ADJUST_TO_MAX,
	AE_ADJUST_TO_MIN,
	AE_ADJUST_MIN_TO_MED,
	AE_ADJUST_MAX_TO_MED,
} AE_ADJUST_TARGET_STATUS;

typedef enum _AE_MEMORY_ITEM {
	AE_BOOT_ITEM,
	AE_LOG_ITEM,
	AE_TOOL_PARAMETER_ITEM,
} AE_MEMORY_ITEM;

typedef enum _AE_RGB_CHANNEL {
	AE_CHANNEL_B,
	AE_CHANNEL_GB,
	AE_CHANNEL_GR,
	AE_CHANNEL_R,
} AE_RGB_CHANNEL;

typedef enum _AE_LOG_LEVEL {
	AE_LOG_ALL,
	AE_LOG_NO_HISTOGRAM,
	AE_LOG_NO_STATISTICS,
} AE_LOG_LEVEL;

typedef enum _DELAY_ITEM {
	WHITE_DELAY,
	BLACK_DELAY,
} DELAY_ITEM;

typedef enum _LUMA_BASE_E {
	GRID_BASE,
	HISTOGRAM_BASE,
	TOTAL_BASE,
} LUMA_BASE_E;


typedef struct _AE_GAIN {
	CVI_U32 u32AGain;
	CVI_U32 u32DGain;
	CVI_U32 u32ISPDGain;
} AE_GAIN;

typedef struct _AE_FASTBOOT_EXPOSURE {
	CVI_U32 u32ExpLine;
	AE_GAIN	stGain;
} AE_FASTBOOT_EXPOSURE;


typedef struct _SEXPOSURE_TV_SV_CURVE {
	CVI_U16 NodeTvEntry;
	CVI_U16 NodeSvEntry;
	ADJUST_ITEM AdjItem;
	AE_GAIN	stNodeGain;
	CVI_BOOL isMaxTime;
} SEXPOSURE_TV_SV_CURVE, *PSEXPOSURE_TV_SV_CURVE;


typedef struct _AE_EXPOSURE {
	CVI_FLOAT fIdealExpTime;
	CVI_U32 u32ExpTime;
	CVI_FLOAT fIdealExpTimeLine;
	CVI_U32 u32ExpTimeLine;
	CVI_U32 u32SensorExpTime;
	CVI_U32 u32SensorExpTimeLine;
	AE_GAIN stExpGain;
	AE_GAIN stSensorExpGain;
} AE_EXPOSURE;

typedef struct _AE_APEX {
	CVI_S16 s16BVEntry;
	CVI_S16 s16TVEntry;
	CVI_S16 s16AVEntry;
	CVI_S16 s16SVEntry;
} AE_APEX;

typedef struct _AE_RAW_REPLAY_EXP_INFO {
	CVI_U32 u32FrameID;
	CVI_U32 u32Time[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 u32AGain[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 u32DGain[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 u32ISPDGain[AE_MAX_WDR_FRAME_NUM];
	CVI_U32	u32ISO;
	CVI_S16	s16LvX100;
} AE_RAW_REPLAY_EXP_INFO;


typedef struct _SENSOR_EXP_INFO {
	CVI_U32 u32LinesPer500ms;
	CVI_U32 u32FrameLine;
	CVI_U32 u32MaxExpLine;
	CVI_U32 u32MinExpLine;
	CVI_FLOAT f32Accuracy;
	CVI_FLOAT f32Offset;
	CVI_U32 u32MaxAgain;
	CVI_U32 u32MinAgain;
	CVI_U32 u32MaxDgain;
	CVI_U32 u32MinDgain;
	CVI_U8 u8AERunInterval;
	CVI_FLOAT f32MaxFps;
	CVI_FLOAT f32MinFps;
	CVI_U8 u8SensorPeriod;
	AE_BLC_TYPE_E enBlcType;
	CVI_U8 u8DGainAccuType;
} SENSOR_EXP_INFO;


typedef struct _SAE_INFO {
	CVI_U32 u32frmCnt;
	CVI_U8 u8GridHNum;
	CVI_U8 u8GridVNum;
	AE_APEX stApex[AE_MAX_WDR_FRAME_NUM];
	AE_APEX stInitApex[AE_MAX_WDR_FRAME_NUM];
	AE_APEX stAssignApex[AE_MAX_WDR_FRAME_NUM];
	AE_APEX stSmoothApex[AE_MAX_WDR_FRAME_NUM][AE_MAX_PERIOD_NUM];
	CVI_S16 s16BvStepEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16SmoothBvStepEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16ConvBvStepEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16lumaBvStepEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16WindowBvStepEntry;
	CVI_S16	s16LumaReduceBvStepEntry;
	CVI_U8	u8BvStepWeight[AE_MAX_WDR_FRAME_NUM];
	CVI_U8	u8LumaReduceWeight[AE_MAX_WDR_FRAME_NUM];
	CVI_S16	s16PreBvEntry[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16AssignEVBIAS[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 (*pu16AeStatistics)[AE_ZONE_ROW][AE_ZONE_COLUMN][AE_CHANNEL_NUM];
	CVI_U32 (*pu32Histogram)[HIST_NUM];
	CVI_U32 u32HistogramTotalCnt[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16CenterG[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16MeterLuma[AE_MAX_WDR_FRAME_NUM][AE_ZONE_ROW][AE_ZONE_COLUMN];
	CVI_U16 u16MeterMaxLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16MeterMinLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16FrameLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16BrightQuaG[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8MeterMode[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8ConvergeMode[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8ConvergeSpeed[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL bConvergeStable[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16TargetLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16AdjustTargetLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U8	u8AdjustTargetStatus[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 u32ISONum[AE_MAX_WDR_FRAME_NUM];
	CVI_U32 u32BLCISONum[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16LvX100[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL bIsStable[AE_MAX_WDR_FRAME_NUM];
	AE_EXPOSURE stExp[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16ConvergeFrameCnt[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8GridLowRatio[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8GridLowPredictRatio[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16LowRatio[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE];
	CVI_U16 u16LowBufRatio[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE];
	CVI_U16 u16HighRatio[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE];
	CVI_U16 u16HighBufRatio[AE_MAX_WDR_FRAME_NUM][TOTAL_BASE];
	CVI_U8 u8MeterGridCnt[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8RouteNum[AE_MAX_WDR_FRAME_NUM];
	SEXPOSURE_TV_SV_CURVE *pstAeRoute[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16MinTVEntry;
	CVI_S16 s16MaxTVEntry;
	CVI_S16 s16MinFpsSensorTVEntry;
	CVI_S16 s16MinSensorTVEntry;
	CVI_S16 s16MaxSensorTVEntry;
	CVI_S16 s16MinRouteTVEntry;
	CVI_S16 s16MaxRouteTVEntry;
	CVI_S16 s16MinRouteExTVEntry;
	CVI_S16 s16MaxRouteExTVEntry;
	CVI_S16 s16MinRangeTVEntry;
	CVI_S16 s16MaxRangeTVEntry;
	CVI_S16 s16MinISOEntry;
	CVI_S16 s16MaxISOEntry;
	CVI_S16 s16MinSensorISOEntry;
	CVI_S16 s16MaxSensorISOEntry;
	CVI_S16 s16MinRouteISOEntry;
	CVI_S16 s16MaxRouteISOEntry;
	CVI_S16 s16MinRouteExISOEntry;
	CVI_S16 s16MaxRouteExISOEntry;
	CVI_S16 s16MinRangeISOEntry;
	CVI_S16 s16MaxRangeISOEntry;
	CVI_S16 s16BLCMaxISOEntry;
	CVI_S16	s16ManualBvEntry;
	CVI_U16 u16LvFrameLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16LvBvStep[AE_MAX_WDR_FRAME_NUM];
	CVI_U8 u8AeMaxFrameNum;
	CVI_U32 u32FrameLine;
	CVI_U32 u32ExpLineMin;
	CVI_U32 u32ExpLineMax;
	CVI_U32 u32ExpTimeMin;
	CVI_U32 u32ExpTimeMax;
	CVI_U32 u32MinFpsExpTimeMax;
	CVI_U32 u32WDRSEExpLineMax;
	CVI_U32 u32WDRSEExpTimeMax;
	CVI_S16 s16WDRSEMinTVEntry;
	CVI_U32 u32AGainMin;
	CVI_U32 u32AGainMax;
	CVI_U32 u32DGainMin;
	CVI_U32 u32DGainMax;
	CVI_U32 u32ISPDGainMin;
	CVI_U32 u32ISPDGainMax;
	CVI_U32 u32TotalGainMin;
	CVI_U32 u32TotalGainMax;
	CVI_S16 s16AGainMaxEntry;
	CVI_S16 s16DGainMaxEntry;
	CVI_S16 s16ISPDGainMaxEntry;
	CVI_U32 u32BLCSysGainMax;
	CVI_U32 u32ISONumMin;
	CVI_U32 u32ISONumMax;
	CVI_U32 u32BLCISONumMax;
	CVI_U32 u32LinePer500ms;
	CVI_U32 u32FirstStableTime;
	CVI_FLOAT fMaxFps;
	CVI_FLOAT fMinFps;
	CVI_FLOAT fFps;
	CVI_FLOAT fDefaultFps;
	CVI_BOOL bIsMaxExposure;
	CVI_BOOL bISPDGainFirst;
	CVI_FLOAT fExpLineTime;
	CVI_BOOL bWDRMode;
	CVI_BOOL bWDRLEOnly;
	CVI_U32 u32WDRExpRatio;
	CVI_BOOL bEnableSmoothAE;
	ISP_SNS_GAIN_MODE_E enWDRGainMode;
	CVI_BOOL bWDRUseSameGain;
	CVI_BOOL bISOLimitByBLC;
	CVI_U32 u32ManualWDRSETime;
	CVI_U32 u32ManualWDRSEISPDgain;
	CVI_U8	u8DGainAccuType;
	CVI_U8	u8MeterFramePeriod;
	CVI_U8 u8SensorDgainNodeNum;
	CVI_U32 u32SensorDgainNode[SENSOR_DGAIN_MAX_NODE_NUM];
	CVI_BOOL	bParameterUpdate[AE_UPDATE_TOTAL];
	CVI_S16 s16SlowShutterISOEntry;
	CVI_FLOAT fSlowShutterFps;
	CVI_U16 u16WeightSum;
	CVI_U16	u16AvX100;
	CVI_U8	u8BootWaitFrameNum;
	CVI_S16 s16AEBracketing0EvBv[AE_MAX_WDR_FRAME_NUM];
	AE_SLOW_SHUTTER_STATUS enSlowShutterStatus;
	CVI_FLOAT fIspPubAttrFps;
	CVI_U8	u8His255Ratio[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16TargetLumaLowBound[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16TargetLumaHighBound[AE_MAX_WDR_FRAME_NUM];
	CVI_S16 s16TargetlumaOffset[AE_MAX_WDR_FRAME_NUM];
	CVI_BOOL	bPubFpsChange;
	CVI_BOOL	bAeFpsChange;
	AE_BLC_TYPE_E	enBlcType;
	CVI_BOOL	bDGainSwitchToAGain;
	CVI_FLOAT	fExpTimeAccu;
	CVI_BOOL	bEnableISPDgainCompensation;
	CVI_U16	u16FrameAvgLuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16 u16ROILuma[AE_MAX_WDR_FRAME_NUM];
	CVI_U16	u16ROIWeightThr;
	CVI_U8	u8SensorPeriod;
	CVI_U8	u8SensorUsePeriod;
	CVI_U8	u8SensorRunInterval;
	CVI_U8	u8AERunInterval;
	CVI_U16	u16FramePeriodTime;
	CVI_U8	u8LimitExposurePath[AE_MAX_WDR_FRAME_NUM];
	CVI_S16	s16FrameAvgBvStep;
	CVI_U32	u32MeterFrameCnt;
	CVI_BOOL	bMeterEveryFrame;
	CVI_S16	as16SmoothBv[AE_MAX_WDR_FRAME_NUM][2];
	CVI_FLOAT	fExpTimeOffset;
	LUMA_BASE_E	enLumaBase;
	CVI_U32	u32RawId;
	CVI_U8	u8RawNum;
	CVI_U8	u8RawBufCnt;
	CVI_BOOL	bIsRawReplay;
	CVI_BOOL	bRegWeightUpdate;
	CVI_U8	u8FastConvergeFrmCnt;
} SAE_INFO;

#define AE_FD_GRID_LUMA_SIZE				64
typedef struct _SFACE_DETECT_INFO {
	CVI_BOOL	bMode;
	CVI_U16 u16FDLuma;
	CVI_U16 u16FDTargetLuma;
	CVI_S16 s16FDEVStep;
	CVI_U16 u16FDPosX;
	CVI_U16 u16FDPosY;
	CVI_U16 u16FDWidth;
	CVI_U16 u16FDHeight;
	CVI_U16	u16FrameWidth;
	CVI_U16	u16FrameHeight;
	CVI_U8	u8GridSizeX;
	CVI_U8	u8GridSizeY;
	CVI_U16	u16FDGridStart;
	CVI_U16	u16FDGridEnd;
	CVI_U16 u16AEFDPosX;
	CVI_U16 u16AEFDPosY;
	CVI_U16 u16AEFDWidth;
	CVI_U16 u16AEFDHeight;
	CVI_U16	u16AEFDGridStart;
	CVI_U16	u16AEFDGridEnd;
	CVI_U8	u8AEFDGridWidthCount;
	CVI_U8	u8AEFDGridHeightCount;
	CVI_U16	u16AEFDGridCount;
	CVI_S16	s16EnvBvStep;
	CVI_S16	s16FinalBvStep;
	ISP_SMART_ROI_S	stSmartInfo;
} SFACE_DETECT_INFO;


#define AE_BOOT_MAX_FRAME 30
typedef struct _AE_BOOT_INFO {
	CVI_U8 u8FrmID[AE_BOOT_MAX_FRAME];
	CVI_U16 u16FrmLuma[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
	CVI_U16 u16AdjustTargetLuma[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
	CVI_S16 s16FrmBvStep[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
	CVI_S16	s16LvX100[AE_BOOT_MAX_FRAME];
	CVI_BOOL bFrmConvergeMode[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
	AE_APEX stApex[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
	CVI_BOOL bStable[AE_MAX_WDR_FRAME_NUM][AE_BOOT_MAX_FRAME];
} SAE_BOOT_INFO;


#define AE_GET_CTX(ViPipe) (&g_astAeCtx[ViPipe])

typedef struct cviAE_CTX_S {
	/* usr var */
	CVI_U8 u8AeMode;

	/* communicate with isp */
	ISP_AE_PARAM_S stAeParam;
	CVI_U32 u32FrameCnt;
	VI_PIPE IspBindDev;

	/* communicate with sensor, defined by user. */
	CVI_BOOL bSnsRegister;
	ISP_SNS_ATTR_INFO_S stSnsAttrInfo;
	AE_SENSOR_DEFAULT_S stSnsDft;
	AE_SENSOR_REGISTER_S stSnsRegister;

	/* global variables of ae algorithm */
} AE_CTX_S;

#define AE_DBG_BIN_VER	(3)
typedef struct _AE_DBG_S {
	CVI_U32 u32Date;
	CVI_U16 u8AlgoVer;
	CVI_U16 u8AlgoSubVer;
	CVI_U16 u8DbgVer;
	CVI_U32 u32BinSize;
	ISP_EXPOSURE_ATTR_S stExpAtt[AE_SENSOR_NUM];
	ISP_WDR_EXPOSURE_ATTR_S stWdrAtt[AE_SENSOR_NUM];
	ISP_AE_ROUTE_S stRoute[AE_SENSOR_NUM][MAX_WDR_FRAME_NUM];
	ISP_AE_ROUTE_EX_S stRouteEx[AE_SENSOR_NUM][MAX_WDR_FRAME_NUM];
	ISP_SMART_EXPOSURE_ATTR_S	stSmartAtr[AE_SENSOR_NUM];
	ISP_AE_STATISTICS_CFG_S stStatCfg[AE_SENSOR_NUM];
	ISP_SMART_ROI_S	stSmartExpInfo[AE_SENSOR_NUM];
	SENSOR_EXP_INFO stSensorExpInfo[AE_SENSOR_NUM];
} s_AE_DBG_S;

typedef struct _AE_SIM_S {
	CVI_BOOL bIsSimAe;
	CVI_S32  ae_wantTv[2];
	CVI_S32  ae_wantSv[2];
	CVI_U32  genRawCnt;
	CVI_U32  genRawCntLast;
	CVI_U8   waitCnt;
} s_AE_SIM_S;

extern AE_CTX_S g_astAeCtx[MAX_AE_LIB_NUM];
extern s_AE_SIM_S sAe_sim;

CVI_S32 aeInit(VI_PIPE ViPipe, const ISP_AE_PARAM_S *pstAeParam);
CVI_S32 aeRun(VI_PIPE ViPipe, const ISP_AE_INFO_S *pstAeInfo, ISP_AE_RESULT_S *pstAeResult, CVI_S32 s32Rsv);
CVI_S32 aeCtrl(VI_PIPE ViPipe, CVI_U32 u32Cmd, CVI_VOID *pValue);
CVI_S32 aeExit(VI_PIPE ViPipe);
CVI_S32 AE_SetExposure(CVI_U8 sID, AE_EXPOSURE *exp);
void AE_SetExpToResult(CVI_U8 sID, ISP_AE_RESULT_S *pstAeResult, const AE_EXPOSURE *pStExp);
void AE_Delay1s(CVI_S32 s);
void AE_Delay1ms(CVI_S32 ms);
CVI_U32 AE_GetMSTimeDiff(const struct timeval *before, const struct timeval *after);
CVI_U32 AE_GetUSTimeDiff(const struct timeval *before, const struct timeval *after);
void AE_Function_Init(CVI_U8 sID);
CVI_S32 AE_DumpLog(void);
void AE_DumpSnapLog(CVI_U8 sID);
void AE_SaveSnapLog(CVI_U8 sID);
CVI_S32 AE_SetDumpLogPath(const char *szPath);
CVI_S32 AE_GetDumpLogPath(char *szPath, CVI_U32 pathSize);
CVI_S32 AE_SetDumpLogName(const char *szName);
CVI_S32 AE_GetDumpLogName(char *szName, CVI_U32 nameSize);
void AE_LogInit(void);
void AE_LogPrintf(const char *szFmt, ...);




CVI_U32 AE_GetFrameLine(CVI_U8 sID);
void AE_CalculateFrameLuma(CVI_U8 sID);
void AE_AdjustTargetY(CVI_U8 sID);
CVI_U8 AE_CheckSensorID(CVI_U8 sID);
void AE_CalculateConverge(CVI_U8 sID);
CVI_U16 AE_GetCenterG(CVI_U8 sID, CVI_U8 WDR_SE);
CVI_U16 AE_GetBrightQuaG(CVI_U8 sID, CVI_U8 WDR_SE);
void AE_SetManualExposure(CVI_U8 sID);
void AE_GetExpTimeByEntry(CVI_U8 sID, CVI_S16 tvEntry, CVI_U32 *ptime);
void AE_GetIdealExpTimeByEntry(CVI_U8 sID, CVI_S16 tvEntry, CVI_FLOAT *ptime);
void AE_GetExpGainByEntry(CVI_U8 sID, CVI_S16 SvEntry, AE_GAIN *pstGain);
void AE_GetRoutExGainByEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 SvEntry, AE_GAIN *pstGain);

CVI_S32 AE_SetExposureAttr(CVI_U8 sID, const ISP_EXPOSURE_ATTR_S *pstExpAttr);
CVI_S32 AE_GetExposureAttr(CVI_U8 sID, ISP_EXPOSURE_ATTR_S *pstExpAttr);
CVI_S32 AE_SetWDRExposureAttr(CVI_U8 sID, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr);
CVI_S32 AE_GetWDRExposureAttr(CVI_U8 sID, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr);
CVI_S32 AE_SetSmartExposureAttr(CVI_U8 sID, const ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr);
CVI_S32 AE_GetSmartExposureAttr(CVI_U8 sID, ISP_SMART_EXPOSURE_ATTR_S *pstSmartExpAttr);
CVI_S32 AE_SetRouteAttr(CVI_U8 sID, const ISP_AE_ROUTE_S *pstRouteAttr);
CVI_S32 AE_GetRouteAttr(CVI_U8 sID, ISP_AE_ROUTE_S *pstRouteAttr);
CVI_S32 AE_SetRouteAttrEx(CVI_U8 sID, const ISP_AE_ROUTE_EX_S *pstRouteAttrEx);
CVI_S32 AE_GetRouteAttrEx(CVI_U8 sID, ISP_AE_ROUTE_EX_S *pstRouteAttrEx);
CVI_S32 AE_SetRouteSFAttr(CVI_U8 sID, const ISP_AE_ROUTE_S *pstRouteSFAttr);
CVI_S32 AE_GetRouteSFAttr(CVI_U8 sID, ISP_AE_ROUTE_S *pstRouteSFAttr);
CVI_S32 AE_SetRouteSFAttrEx(CVI_U8 sID, const ISP_AE_ROUTE_EX_S *pstRouteSFAttrEx);
CVI_S32 AE_GetRouteSFAttrEx(CVI_U8 sID, ISP_AE_ROUTE_EX_S *pstRouteSFAttrEx);
CVI_S32 AE_SetStatisticsConfig(CVI_U8 sID, const ISP_AE_STATISTICS_CFG_S *pstAeStatCfg);
CVI_S32 AE_GetStatisticsConfig(CVI_U8 sID, ISP_AE_STATISTICS_CFG_S *pstAeStatCfg);
CVI_S32 AE_SetIrisAttr(CVI_U8 sID, const ISP_IRIS_ATTR_S *pstIrisAttr);
CVI_S32 AE_GetIrisAttr(CVI_U8 sID, ISP_IRIS_ATTR_S *pstIrisAttr);
CVI_S32 AE_SetDcIrisAttr(CVI_U8 sID, const ISP_DCIRIS_ATTR_S *pstDcirisAttr);
CVI_S32 AE_GetDcIrisAttr(CVI_U8 sID, ISP_DCIRIS_ATTR_S *pstDcirisAttr);
CVI_S32 AE_GetExposureInfo(CVI_U8 sID, ISP_EXP_INFO_S *pstExpInfo);


void AE_ConfigExposureRoute(CVI_U8 sID);
void AE_ConfigExposure(CVI_U8 sID);
void AE_GetExposureTimeLine(CVI_U8 sID, CVI_U32 time, CVI_U32 *line);
void AE_GetExposureTimeIdealLine(CVI_U8 sID, CVI_FLOAT time, CVI_FLOAT *line);
void AE_GetCurrentInfo(CVI_U8 sID, SAE_INFO **stAeInfo);
void AE_GetHistogram(CVI_U8 sID, CVI_U32 *histogram);
CVI_U32 AE_GetISONum(CVI_U8 sID);
CVI_U32 AE_GetFrameID(CVI_U8 sID, CVI_U32 *frameID);
void AE_SetTvEntryLimit(CVI_U8 sID, CVI_S16 MinEntry, CVI_S16 MaxEntry);
void AE_SetAssignISOEntry(CVI_U8 sID, CVI_S16 ISOEntry);
void AE_SetAssignTvEntry(CVI_U8 sID, CVI_S16 TvEntry);
void AE_SetAssignEvBias(CVI_U8 sID, CVI_S16 EvBias);
void AE_SetConvergeSpeed(CVI_U8 sID, CVI_U8 speed, CVI_U16 b2dSpeed);
CVI_S16 AE_LimitBVEntry(CVI_U8 sID, CVI_S16 BvEntry);
void AE_GetRouteTVAVSVEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 BvEntry, CVI_S16 *pTvEntry,
	CVI_S16 *pAvEntry, CVI_S16 *pSvEntry);
void AE_GetTVSVExposureRoute(CVI_U8 sID, AE_WDR_FRAME wdrFrm, PSEXPOSURE_TV_SV_CURVE *pPreviewTVSVExposureCurve,
	CVI_U8 *curveSize);
void AE_ConfigManualExposure(CVI_U8 sID);
void AE_GetBVEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry);
void AE_SetByPass(CVI_U8 sID, CVI_U8 enable);
void AE_SetISPDGainPriority(CVI_U8 sID, CVI_U8 enable);
void AE_GetTVEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry);
void AE_GetISOEntryLimit(CVI_U8 sID, CVI_S16 *MinEntry, CVI_S16 *MaxEntry);
void AE_GetTvRangeEntryLimit(CVI_U8 sID, CVI_S16 *minEntry, CVI_S16 *maxEntry);
void AE_GetISORangeEntryLimit(CVI_U8 sID, CVI_S16 *minEntry, CVI_S16 *maxEntry);
CVI_U32 AE_CalTotalGain(CVI_U32 AGain, CVI_U32 DGain, CVI_U32 ISPDGain);
void AE_EnableSmooth(CVI_U8 sID, CVI_BOOL enable);
void AE_ConfigSmoothExposure(CVI_U8 sID);
void AE_GetSmoothTvAvSvEntry(CVI_U8 sID, CVI_U32 frmNo);
void AE_CalCurrentLvX100(CVI_U8 sID);
CVI_S16 AE_GetCurrentLvX100(CVI_U8 sID);
void AE_SetAdjustTargetLuma(CVI_U8 sID, CVI_BOOL enable);
CVI_U8 AE_CheckWDRIdx(CVI_U8 WDRIdx);

void AE_SetWDRSETime(CVI_U8 sID, CVI_U32 time);
CVI_U32 AE_GetWDRSEMaxTime(CVI_U8 sID);
CVI_U32 AE_GetWDRSEMinTime(CVI_U8 sID);
void AE_SetWDRSEISPDGain(CVI_U8 sID, CVI_U32 ispDgain);
void AE_GetISOEntryByISONum(CVI_U32 ISONum, CVI_S16 *pISOEntry);
void AE_SaveBootLog(CVI_U8 sID);
void AE_SaveBootSensorRegInfo(CVI_U8 sID, CVI_U32 frameCnt);
void AE_GetBootInfo(CVI_U8 sID, SAE_BOOT_INFO *bootInfo);
void AE_GetISONumByEntry(CVI_U8 sID, CVI_S16 ISOEntry, CVI_U32 *ISONum);
void AE_ShowConvergeStatus(CVI_U8 sID);
void AE_GetRouteTVSVEntry(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_S16 BvEntry, CVI_S16 *pTvEntry, CVI_S16 *pSvEntry);
void AE_GetExposureLineByTime(CVI_U8 sID, CVI_U32 time, CVI_U32 *line);
void AE_GetExpTimeByLine(CVI_U8 sID, CVI_U32 line, CVI_U32 *time);
void AE_GetTvEntryByTime(CVI_U8 sID, CVI_U32 time, CVI_S16 *ptvEntry);
void AE_GetISOEntryByGain(CVI_U32 Gain, CVI_S16 *pISOEntry);
void AE_GetGainBySvEntry(CVI_U8 sID, AE_GAIN_TYPE gainType, CVI_S16 SvEntry, CVI_U32 *Gain);
void AE_GetISONumByGain(CVI_U32 gain, CVI_U32 *ISONum);
void AE_GetGainByISONum(CVI_U32 ISONum, CVI_U32 *Gain);
CVI_U16 AE_CalGain2Entry(CVI_U32 gain);
void AE_SetDebugMode(CVI_U8 sID, CVI_U8 mode);
CVI_U8 AE_GetDebugMode(CVI_U8 sID);
CVI_U16 AE_GetFrameLuma(CVI_U8 sID);
CVI_BOOL AE_GetLogBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize);
CVI_BOOL AE_GetLogBufSize(CVI_U8 sID, CVI_U32 *bufSize);
CVI_BOOL AE_GetBinBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize);
CVI_BOOL AE_GetBinBufSize(CVI_U8 sID, CVI_U32 *bufSize);
CVI_U32 AE_LimitExpTime(CVI_U8 sID, CVI_U32 expTime);
CVI_U32 AE_LimitISONum(CVI_U8 sID, CVI_U32 ISONum);
CVI_U32 AE_LimitWDRSEExpTime(CVI_U8 sID, CVI_U32 expTime);
CVI_U32 AE_LimitAGain(CVI_U8 sID, CVI_U32 AGain);
CVI_U32 AE_LimitDGain(CVI_U8 sID, CVI_U32 DGain);
CVI_U32 AE_LimitISPDGain(CVI_U8 sID, CVI_U32 ISPDGain);
CVI_U32 AE_LimitTimeRange(CVI_U8 sID, CVI_U32 expTime);
CVI_U32 AE_LimitISORange(CVI_U8 sID, CVI_U32 ISONum);
CVI_U32 AE_LimitAGainRange(CVI_U8 sID, CVI_U32 AGain);
CVI_U32 AE_LimitDGainRange(CVI_U8 sID, CVI_U32 DGain);
CVI_U32 AE_LimitISPDGainRange(CVI_U8 sID, CVI_U32 ISPDGain);
CVI_U32 AE_LimitManualTime(CVI_U8 sID, CVI_U32 expTime);
CVI_U32 AE_LimitManualISONum(CVI_U8 sID, CVI_U32 ISONum);
CVI_U32 AE_LimitManualAGain(CVI_U8 sID, CVI_U32 AGain);
CVI_U32 AE_LimitManualDGain(CVI_U8 sID, CVI_U32 DGain);
CVI_U32 AE_LimitManualISPDGain(CVI_U8 sID, CVI_U32 ISPDGain);
void AE_SetWDRLEOnly(CVI_U8 sID, CVI_BOOL wdrLEOnly);
void AE_SetMeterMode(CVI_U8 sID, ISP_AE_METER_MODE_E meterMode);
void AE_GetWDRSETimeRange(CVI_U8 sID, CVI_U32 *minTime, CVI_U32 *maxTime);
CVI_BOOL AE_IsWDRMode(CVI_U8 sID);

void AE_BracketingStart(CVI_U8 sID);
CVI_S32 AE_BracketingSetExposure(CVI_U8 sID, CVI_S16 leEv, CVI_S16 seEv);
CVI_S32 AE_BracketingSetIndex(CVI_U8 sID, CVI_S16 index);
void AE_BracketingSetSimple(CVI_BOOL bEnable);
void AE_BracketingSetExposureFinish(CVI_U8 sID);



void AE_CalFaceDetectLuma(CVI_U8 sID);
void AE_CalFaceDetectBvStep(CVI_U8 sID);
CVI_U8 AE_ViPipe2sID(VI_PIPE ViPipe);
VI_PIPE AE_sID2ViPipe(CVI_U8 sID);
void AE_GetBootExpousreInfo(CVI_U8 sID, AE_APEX *bootApex);
void AE_SetAuto(CVI_U8 sID);
void AE_GetLimitGain(CVI_U8 sID, AE_GAIN *pstGain);
void AE_GetWDRLESEEntry(CVI_U8 sID, AE_APEX *LE, AE_APEX *SE);
CVI_U8 AE_GetSensorDGainType(CVI_U8 sID);
CVI_U32 AE_GetDGainNodeValue(CVI_U8 sID, CVI_U8 nodeNum);
CVI_S32 AE_SetFps(CVI_U8 sID, CVI_FLOAT fps);
void AE_GetFps(CVI_U8 sID, CVI_FLOAT *pFps);
void AE_ShowTvSvCompensateTable(CVI_U8 sID);
void AE_SaveSmoothExpSettingLog(CVI_U8 sID);
void AE_GetSensorExposureSetting(CVI_U8 sID, AE_EXPOSURE *expSet);
void AE_GetWDRSEMinTvEntry(CVI_U8 sID, CVI_S16 *tvEntry);
CVI_BOOL AE_GetWDRExpLineRange(CVI_U8 sID, CVI_U32 expRatio, CVI_U32 *leExpLine, CVI_U32 *seExpLine,
	CVI_BOOL getMaxLine);
CVI_U8 AE_GetMeterPeriod(CVI_U8 sID);
CVI_U8 AE_GetSensorPeriod(CVI_U8 sID);
CVI_U8 AE_GetSensorExpGainPeriod(CVI_U8 sID);
CVI_BOOL AE_DumpBootLog(CVI_U8 sID);
void AE_GetExpGainInfo(CVI_U8 sID, AE_WDR_FRAME wdrFrm, AE_GAIN *expGain);
void AE_SetAPEXExposure(CVI_U8 sID, AE_WDR_FRAME wdrFrm, const AE_APEX *papex);
void AE_GetAPEXExposure(CVI_U8 sID, AE_WDR_FRAME wdrFrm, AE_APEX *papex);
void AE_GetTargetRange(CVI_U8 sID, CVI_S16 LvX100, CVI_S32 *ptargetMin, CVI_S32 *ptargetMax);

void AE_GetMaxGain(CVI_U8 sID, AE_GAIN *pstGain);
CVI_U16 AE_GetAdjustTarget(CVI_U8 sID);
void AE_GetExpMinTime(CVI_U8 sID, CVI_U32 *pTime);
void AE_GetExpMaxTime(CVI_U8 sID, CVI_U32 *pTime);
void AE_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer);
void AE_SetIspDgainCompensation(CVI_U8 sID, CVI_BOOL enable);
CVI_U32 AE_CalculateISONum(CVI_U8 sID, const AE_GAIN *pStGain);
CVI_U32 AE_CalculateBLCISONum(CVI_U8 sID, const AE_GAIN *pStGain);
void AE_CalculateManualBvEntry(CVI_U8 sID, CVI_U32 exposure, CVI_S16 *bvEntry);
void AE_GetExpLineRange(CVI_U8 sID, CVI_U32 *minExpLine, CVI_U32 *maxExpLine);
void AE_GetWDRSEExpLineRange(CVI_U8 sID, CVI_U32 *minExpLine, CVI_U32 *maxExpLine);
void AE_ShutterGainTest(CVI_U8 sID, AE_EXPOSURE *pStExp);
void AE_MemoryAlloc(CVI_U8 sID, AE_MEMORY_ITEM item);
void AE_MemoryFree(CVI_U8 sID, AE_MEMORY_ITEM item);
void AE_SetFastBootExposure(VI_PIPE ViPipe, CVI_U32 expLine, CVI_U32 again, CVI_U32 dgain, CVI_U32 ispdgain);
void *AE_Malloc(CVI_U8 sID, size_t size);
void AE_Free(CVI_U8 sID, void *ptr);

void AE_ConfigExposureTime_ISO(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_FLOAT time, CVI_U32 ISO);
void AE_ConfigExposureTime_Gain(CVI_U8 sID, AE_WDR_FRAME wdrFrm, CVI_FLOAT time, AE_GAIN *pstGain);
void AE_SetLumaBase(CVI_U8 sID, LUMA_BASE_E type);
void AE_SetParameterUpdate(CVI_U8 sID, AE_PARAMETER_UPDATE paraItem);

CVI_S32 AE_SetRawDumpFrameID(CVI_U8 sID, CVI_U32 fid, CVI_U8 frmNum);
CVI_S32 AE_GetRawReplayExpBuf(CVI_U8 sID, CVI_U8 *buf, CVI_U32 *bufSize);
CVI_S32 AE_SetRawReplayExposure(CVI_U8 sID, const ISP_EXP_INFO_S *pstExpInfo);
void AE_SetRawReplayMode(CVI_U8 sID, CVI_BOOL mode);
CVI_S32 AE_isRawReplayMode(CVI_U8 sID);
CVI_S32 AE_GetRawReplayFrmNum(CVI_U8 sID, CVI_U8 *bootFrmNum, CVI_U8 *ispDgainPeriodNum);



void GenNewRaw(void *pDstOri, void *pSrcOri, CVI_U32 sizeBk, CVI_U32 raw_mode,
	CVI_U32 raw_w, CVI_U32 raw_h, CVI_U32 nRawBlc);
void AE_SetAeSimMode(CVI_BOOL bMode);
CVI_BOOL AE_IsAeSimMode(void);
void AE_SetAeSimEv(CVI_U8 sID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _AE_ALG_H_
