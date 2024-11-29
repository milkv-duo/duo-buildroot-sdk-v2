/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: awbalgo.h
 * Description:
 *
 */

#ifndef _AWB_ALG_H_
#define _AWB_ALG_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

#define AWB_LIMIT(var, min, max)    ((var) = ((var) < (min)) ? (min) : (((var) > (max)) ? (max) : (var)))
#define AWB_ABS(a)                  ((a) > 0 ? (a) : -(a))
#define AWB_MIN(a, b)               ((a) < (b) ? (a) : (b))
#define AWB_MAX(a, b)               ((a) > (b) ? (a) : (b))
#define AWB_DIV_0_TO_1(a)           ((0 == (a)) ? 1 : (a))
#define AWB_LIMIT_UPPER(var, max_v) ((var) = ((var) > (max_v)) ? (max_v) : (var))

#ifndef MIN2
#define MIN2(a, b)                                                                                                     \
	({                                                                                                             \
		__typeof__(a) _a = (a);                                                                                \
		__typeof__(b) _b = (b);                                                                                \
		_a < _b ? _a : _b;                                                                                     \
	})
#endif

#ifndef MAX2
#define MAX2(a, b)                                                                                                     \
	({                                                                                                             \
		__typeof__(a) _a = (a);                                                                                \
		__typeof__(b) _b = (b);                                                                                \
		_a > _b ? _a : _b;                                                                                     \
	})
#endif

#ifndef LIMIT_RANGE
#define LIMIT_RANGE(v, min, max) ({ MIN2(MAX2((v), (min)), max); })
#endif


#define AWB_GAIN_SMALL_BASE     (256)
#define AWB_GAIN_BASE           (1024)
#define AWB_STEP_BIT            (4)
#define AWB_STEP                (1 << AWB_STEP_BIT)//16,Build Curve Step ,AWB_CURVE_SIZE
#define AWB_CURVE_SIZE          (1024)//AWB_STEP*1024 = Max Rgain :16384 ==>16X gain
#define AWB_MAX_GAIN            (16 * AWB_GAIN_BASE - 1)

#define AWB_MAX_SPEED           (4096)
#define AWB_PERIOD_NUM          (6)
#define AWB_CHANNEL_NUM         (3)
#define AWB_BOOT_MAX_FRAME      (3)

#define CURVE_COEF_NUM          (AWB_CURVE_PARA_NUM)
#define WB_FIXED_LV_HIGH        (1600)
#define WB_MIXED_LV_HIGH        (1300)

#define MIN_COLOR_TEMPERATURE   (1500)
#define MAX_COLOR_TEMPERATURE   (10000)

#define SMOOTH_GAIN_RANGE       (100)

#define TOTAL_SAMPLE_NUM        (2)

#define AWB_BIN_STEP            (32)
#define AWB_BIN_MAX_RGAIN       (16384)//16X
#define AWB_BIN_ZONE_NUM        (AWB_BIN_MAX_RGAIN / AWB_BIN_STEP)//512

#define MAX_AWB_LOG_PATH_LENGTH (128)
#define MAX_LOG_FILENAME_LENGTH (MAX_AWB_LOG_PATH_LENGTH * 2 + 32)

#define AWB_GAIN_NORM_EN_MAX    (AWB_GAIN_BASE * 4)

#define OUT_DOOR_WT_NORMAL      (32)
#define OUT_DOOR_WT_HIGH        (256)

#define AWB_CURVE_BASE          (100000)//Old Calib method
#define TEMP_CURVE_BASE         (10000)//Old Calib method

#define MIX_NATURAL_LOW_TEMP    (3500)
#define HISTO_WEIGHT_SIZE       (256)
#define HISTO_WT_MAX            (512)

#define AWB_ZONE_WT_DEF         (8)

#define AWB_FAST_CONV_BV_STEP   (1.0)

#define AWB_WIDE_RANGE_TOP      (300)
#define AWB_WIDE_RANGE_BOT      (100)

#define AWB_GRAY_THRES          (300)

#define AWB_BOUND_LOW_R         (500)//TODO@CLIFF
#define AWB_BOUND_HIGH_R        (16384)
#define AWB_BOUND_LOW_B         (1400)
#define AWB_BOUND_HIGH_B        (3200)

//Just for init
#define CT_TO_RGAIN_CURVE_K1   (0.000055)
#define CT_TO_RGAIN_CURVE_K2   (-0.254903)
#define CT_TO_RGAIN_CURVE_K3   (1692.074097)

#define CT_SHIFT_LIMIT_REGION1 (3900)//ColorTemp
#define CT_SHIFT_LIMIT_REGION2 (5300)
#define CT_SHIFT_LIMIT_REGION3 (6600)

#define DEFAULT_RGAIN_RATIO    (1.789062)
#define DEFAULT_BGAIN_RATIO    (1.639648)

#define AWB_LAST_LV_SIZE (30)

typedef struct cviAWB_CTX_S {
	/* usr var */
	CVI_U8 u8AwbMode;

	/* communicate with isp */
	ISP_AWB_PARAM_S stAwbParam;
	CVI_U32 u32FrameCnt;
	VI_PIPE IspBindDev;

	/* communicate with sensor, defined by user. */
	CVI_BOOL bSnsRegister;
	ISP_SNS_ATTR_INFO_S stSnsAttrInfo;
	AWB_SENSOR_DEFAULT_S stSnsDft;
	AWB_SENSOR_REGISTER_S stSnsRegister;

	/* global variables of awb algorithm */
} AWB_CTX_S;

typedef enum _AWB_RGB {
	AWB_R = 0,
	AWB_G = 1,
	AWB_B = 2,
	AWB_MAX = 3,
} AWB_RGB;

typedef enum _WDR_AWB_FRAME {
	WB_WDR_LE = 0,
	WB_WDR_SE,
	WB_WDR_FRAME_MAX,
} WDR_AWB_FRAME;

typedef enum _WB_CONVERGED_MODE {
	WB_CONVERGED_NORMAL = 0,
	WB_CONVERGED_FAST = 1,
} WB_CONVERGED_MODE;

typedef enum _MWB_MODE {
	MWB_NORMAL = 0,
	MWB_DAYLIGHT,
	MWB_CLOUDY,
	MWB_SHADE,
	MWB_FLUORESCENT,
	MWB_FLUORESCENT_H,
	MWB_FLUORESCENT_L,
	MWB_TUNGSTEN,
} MWB_MODE;

typedef enum _AWB_PARAMETER_UPDATE {
	AWB_ATTR_UPDATE,
	AWB_ATTR_EX_UPDATE,
	AWB_CALIB_UPDATE,
	AWB_UPDATE_TOTAL,
} AWB_PARAMETER_UPDATE;

typedef struct _WbEnvironmentInfo {
	CVI_S16 s16LvX100;
	CVI_U32 u32ISONum;
	CVI_FLOAT fBVstep;
} sWbEnvironmentInfo;

typedef struct _WBGain {
	CVI_U16 u16RGain;
	CVI_U16 u16GGain;
	CVI_U16 u16BGain;
} sWBGain;

typedef struct _WBBoundary {
	CVI_U16 u16RLowBound;
	CVI_U16 u16RHighBound;
	CVI_U16 u16BLowBound;
	CVI_U16 u16BHighBound;
} sWBBoundary;

typedef struct _WBCurveInfo {
	CVI_U16 u16CurveWhiteTopRange;
	CVI_U16 u16CurveWhiteBotRange;
	CVI_U16 u16LowTempTopRange;
	CVI_U16 u16LowTempBotRange;
	CVI_U16 u16MidTempTopRange;
	CVI_U16 u16MidTempBotRange;
	CVI_U16 u16HighTempTopRange;
	CVI_U16 u16HighTempBotRange;
	CVI_U16 u16Region1_R;
	CVI_U16 u16Region2_R;
	CVI_U16 u16Region3_R;
	CVI_U16 u16CurveWildTopRange;
	CVI_U16 u16CurveWildBotRange;
	CVI_U16 u16LowTempWildTopRange;
	CVI_U16 u16LowTempWildBotRange;
	CVI_U16 u16MidTempWildTopRange;
	CVI_U16 u16MidTempWildBotRange;
	CVI_U16 u16HighTempWildTopRange;
	CVI_U16 u16HighTempWildBotRange;
} sWBCurveInfo;

typedef struct _WBSampleInfo {
	CVI_U16 u16GrayCnt;
	CVI_U8 u8GrayRatio;
	CVI_U16 u16CoolWhiteGrayCnt;
	CVI_U8 u8CoolWhiteGrayRatio;
	CVI_U16 u16FlourGrayCnt;
	CVI_U16 u16ConvegerCnt;
	CVI_U8 u8ConvegerRatio;
	CVI_U16 u16GreenCnt;
	CVI_U8 u8GreenRatio;
	CVI_BOOL bUsePreBalanceGain;
	CVI_U16 u16TotalSampleCnt;
	CVI_U16 u16EffectiveSampleCnt;
	CVI_U64 u64TotalPixelSum[TOTAL_SAMPLE_NUM][AWB_MAX];
	CVI_U16 u16CentRawG;
	CVI_U16 u16WDRSECnt;
	CVI_U32 u32GrayWeightCnt;
	CVI_U64 u64PixelSum[AWB_BIN_ZONE_NUM][AWB_MAX];
	CVI_U64 u64GrayCnt[AWB_BIN_ZONE_NUM];
	CVI_U64 u64CT_BinPixelSum[AWB_BIN_ZONE_NUM][AWB_MAX];
	CVI_U64 u64CT_BinGrayCnt[AWB_BIN_ZONE_NUM];
	CVI_U64 u64OutDoorBinPixelSum[AWB_BIN_ZONE_NUM][AWB_MAX];
	CVI_U64 u64OutDoorBinGrayCnt[AWB_BIN_ZONE_NUM];

	CVI_U32 u32WildCnt;
	CVI_U32 u32WildSum[AWB_MAX];
	CVI_U16 u16CenterBinCnt[AWB_BIN_ZONE_NUM];
	CVI_U32 u32SE_Cnt;
	CVI_U32 u32SE_Sum[AWB_MAX];
	CVI_U16 u16SE_BinCnt[AWB_BIN_ZONE_NUM];
} sWBSampleInfo;

typedef struct _WBInfo {
	CVI_U32 u32FrameID;
	CVI_U32 u32RunCnt;
	CVI_U16 u16WBRowSize;
	CVI_U16 u16WBColumnSize;
	sWBGain stCurrentWB;
	sWBGain stDefaultWB;
	sWBGain stBalanceWB;
	sWBGain stGoldenWB;
	sWBGain stFinalWB;
	sWBGain stAssignedWB;
	sWBGain stGrayWorldWB;
	sWBBoundary stAWBBound;
	sWbEnvironmentInfo stEnvInfo;
	CVI_U16 u16FixedWBOffsetR;
	CVI_U16 u16FixedWBOffsetB;
	CVI_U8 u8WBMode;
	CVI_U8 u8WBConvergeMode;
	CVI_U8 u8ALightRMixRatio;
	CVI_U8 u8ALightBMixRatio;
	CVI_U8 u8NatureDaylightRMixRatio;
	CVI_U8 u8NatureDaylightBMixRatio;
	CVI_BOOL bALightGray;
	CVI_BOOL bSkinMixDaylight;
	CVI_U16 u16ColorTemp;
	CVI_U16 u16SEColorTemp;
	CVI_U16 u16ManualColorTemp;
	CVI_U32 u32FirstStableTime;
	CVI_U16 u16CTCurve[AWB_CURVE_SIZE];
	CVI_U16 *pu16AwbStatistics[WB_WDR_FRAME_MAX][AWB_CHANNEL_NUM];
	CVI_FLOAT fCurveCoei[CURVE_COEF_NUM];
	sWBGain stCalibWBGain[AWB_CALIB_PTS_NUM];
	CVI_U16 u16CalibColorTemp[AWB_CALIB_PTS_NUM];
	sWBGain stBootGain[AWB_BOOT_MAX_FRAME];
	CVI_U16 u16BootColorTemp[AWB_BOOT_MAX_FRAME];
	CVI_BOOL bBootConvergeStatus[AWB_BOOT_MAX_FRAME];
	CVI_BOOL bUseGrayWorld;
	CVI_BOOL bQuadraticPolynomial;
	CVI_U8 u8AwbMaxFrameNum;
	CVI_BOOL bStable;
	CVI_BOOL bEnableSmoothAWB;
	CVI_U8 u8Saturation[4];
	CVI_U8 u8AdjCASaturation;
	CVI_U8 u8AdjCASatLuma;
} sWBInfo;

typedef enum _AWB_POINT_TYPE {
	AWB_POINT_EXTRA_NONE,
	//The point that are in extra region((ExpQuant < 50) || (ExpQuant > 100 && ExpQuant <150))
	//Reference lv only
	AWB_POINT_EXTRA_LOOSE,
	//The point that are in extra region((ExpQuant > 50 && ExpQuant < 100) || (ExpQuant > 150))
	//Reference lv and pixel count
	AWB_POINT_EXTRA_STRICT,
} AWB_POINT_TYPE;

typedef struct _POINT_WB_INFO {
	CVI_U8 u8X;
	CVI_U8 u8Y;
	CVI_U16 u16RGain;
	CVI_U16 u16BGain;
	CVI_U16 u16GValue;
	CVI_U16 u16CurveBGain;
	AWB_POINT_TYPE enPointType;
	CVI_FLOAT fStrictExtraRatio;
	CVI_BOOL bEnableStrictExtra;
	CVI_BOOL isSE_OrSDR;
} sPOINT_WB_INFO;

typedef enum _AWB_DBG_PT {
	AWB_DBG_PT_NO_USE = 0,
	AWB_DBG_PT_OUT_CURVE,
	AWB_DBG_PT_IN_CURVE,
} AWB_DBG_PT;

typedef enum _AWB_DBG_BIN_FLAG {
	AWB_DBG_BIN_FLOW_INIT = 0,
	AWB_DBG_BIN_FLOW_TRIG,
	AWB_DBG_BIN_FLOW_UPDATE,
} AWB_DBG_BIN_FLAG;

typedef enum _AWB_EXTRA_WB_LT_MODE {
	AWB_EXTRA_LS_DONT_CARE = 0,
	AWB_EXTRA_LS_ADD,
	AWB_EXTRA_LS_REMOVE,
} AWB_EXTRA_WB_LT_MODE;

typedef struct _SFACE_DETECT_WB_INFO {
	CVI_U8 u8FDNum;
	CVI_U16 u16StX;
	CVI_U16 u16StY;
	CVI_U16 u16EndX;
	CVI_U16 u16EndY;
} SFACE_DETECT_WB_INFO;

#define CT_RB_SIZE          (8)
#define CT_RB_PACK_CT_START (2000)
#define CT_RB_PACK_CT_STEP  (1000)
#define CT_RB_PACK_CT_ADJ   (100)
#define CT_RB_PACK_RB_BASE  (64)

typedef struct _SCT_RB_CLBT {
	CVI_U16 ct[CT_RB_SIZE];
	CVI_FLOAT rb[CT_RB_SIZE];
} SCT_RB_CLBT;

typedef struct _SCT_RB_CLBT_PACK {
	CVI_U8 ct[CT_RB_SIZE / 2];
	CVI_U8 rb[CT_RB_SIZE];//2.6
} SCT_RB_CLBT_PACK;

typedef struct _SEXTRA_LIGHT_RB {
	CVI_U16 ext_ls_r_max[AWB_SENSOR_NUM][AWB_LS_NUM];
	CVI_U16 ext_ls_r_min[AWB_SENSOR_NUM][AWB_LS_NUM];
	CVI_U16 ext_ls_b_max[AWB_SENSOR_NUM][AWB_LS_NUM];
	CVI_U16 ext_ls_b_min[AWB_SENSOR_NUM][AWB_LS_NUM];
} SEXTRA_LIGHT_RB;

typedef enum _AWB_SKY_MODE {
	AWB_SKY_OFF = 0,
	AWB_SKY_REMOVE,
	AWB_SKY_MAPPING,
} AWB_SKY_MODE;

struct ST_SSKY_RB {
	CVI_U16 r_max;
	CVI_U16 r_min;
	CVI_U16 b_max;
	CVI_U16 b_min;
};

typedef struct _ZONE_INFO {
	CVI_U64 u64MaxZoneRgain;
	CVI_U64 u64MaxZoneGgain;
	CVI_U64 u64MaxZoneBgain;
	CVI_U64 u64MaxBinTotalWeight;
	CVI_U64 u64MaxGrayTotalCnt;
	CVI_U64 u64MaxSumInx;
	CVI_U64 u64SubZoneRgain;
	CVI_U64 u64SubZoneGgain;
	CVI_U64 u64SubZoneBgain;
	CVI_U64 u64SubBinTotalWeight;
	CVI_U64 u64SubGrayTotalCnt;
	CVI_U64 u64SubSumInx;
} stZone_Info;

CVI_S32 awbInit(VI_PIPE ViPipe, const ISP_AWB_PARAM_S *pstAwbParam);

#ifndef AAA_PC_PLATFORM
CVI_S32 awbRun(VI_PIPE ViPipe, const ISP_AWB_INFO_S *pstAwbInfo, ISP_AWB_RESULT_S *pstAwbResult, CVI_S32 s32Rsv);
CVI_S32 awbCtrl(VI_PIPE ViPipe, CVI_U32 u32Cmd, CVI_VOID *pValue);
CVI_S32 awbExit(VI_PIPE ViPipe);

void AWB_GetSnapLogBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize);
void AWB_GetDbgBinBuf(CVI_U8 sID, CVI_U8 buf[], CVI_U32 bufSize);

CVI_S32 AWB_DumpLog(CVI_U8 sID);
CVI_S32 AWB_SetDumpLogPath(const char *szPath);
CVI_S32 AWB_SetDumpLogName(const char *szName);
#endif

void AWB_BuildCurve(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S *psWBGain, ISP_WB_ATTR_S *pwbAttr);
void AWB_GetCurrentInfo(CVI_U8 sID, sWBInfo *pstAwbInfo);
CVI_U8 AWB_CheckSensorID(CVI_U8 sID);
void AWB_GetCurrentSampleInfo(CVI_U8 sID, sWBSampleInfo *pstSampeInfo);
CVI_U16 AWB_GetCurColorTemperatureRB(CVI_U8 sID, CVI_U16 curRGain, CVI_U16 curBGain);
CVI_U16 AWB_GetRBGainByColorTemperature(CVI_U8 sID, CVI_U16 u16CT, CVI_S16 s16Shift, CVI_U16 *pu16AWBGain);

void AWB_SetAttr(CVI_U8 sID, const ISP_WB_ATTR_S *pstWBAttr);
void AWB_GetAttr(CVI_U8 sID, ISP_WB_ATTR_S *pstWBAttr);
void AWB_SetAttrEx(CVI_U8 sID, const ISP_AWB_ATTR_EX_S *pstAWBAttrEx);
void AWB_GetAttrEx(CVI_U8 sID, ISP_AWB_ATTR_EX_S *pstAWBAttrEx);
void AWB_GetQueryInfo(CVI_U8 sID, ISP_WB_Q_INFO_S *pstWB_Q_Info);
void AWB_SetManualGain(CVI_U8 sID, CVI_U16 RGain, CVI_U16 GGain, CVI_U16 BGain);
void AWB_SetByPass(CVI_U8 sID, CVI_BOOL enalbe);
void AWB_SetCurveBoundary(CVI_U8 sID, CVI_U16 lowCT, CVI_U16 highCT);
void AWB_SetRGStrength(CVI_U8 sID, CVI_U8 RG, CVI_U8 BG);
void AWB_GetCurveBoundary(CVI_U8 sID, sWBBoundary *psWBBound);
void AWB_GetCurveRange(CVI_U8 sID, sWBCurveInfo *psWBCurve);
void AWB_SetCalibration(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S *pstWBCali);
void AWB_GetCalibration(CVI_U8 sID, ISP_AWB_Calibration_Gain_S *pstWBCali);
void AWB_SetCalibrationEx(CVI_U8 sID, const ISP_AWB_Calibration_Gain_S_EX *pstWBCali);
void AWB_GetCalibrationEx(CVI_U8 sID, ISP_AWB_Calibration_Gain_S_EX *pstWBCali);
CVI_U8 AWB_ViPipe2sID(VI_PIPE ViPipe);
void AWB_GetCurveWhiteZone(CVI_U8 sID, ISP_WB_CURVE_S *pstWBCurve);
void AWB_SetGrayWorld(CVI_U8 sID, CVI_BOOL enable);
void AWB_ShowWhiteZone(CVI_U8 sID, CVI_U8 mode);

void AWB_DumpDbgBin(CVI_U8 sID);
CVI_U32 AWB_GetDbgBinSize(void);
void AWB_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer);
void AWB_Delay1ms(CVI_U32 ms);
void *AWB_Malloc(size_t size);
void AWB_Free(void *ptr);
void AWB_GetGrayWorldWB(CVI_U8 sID, CVI_U16 *pRgain, CVI_U16 *pBgain);
void AWB_SetAwbSimMode(CVI_BOOL bMode);
CVI_BOOL AWB_GetAwbSimMode(void);
AWB_CTX_S *AWB_GET_CTX(VI_PIPE ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _AWB_ALG_H_ */
