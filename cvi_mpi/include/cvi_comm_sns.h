/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_common_sns.h
 * Description:
 */

#ifndef _CVI_COMM_SNS_H_
#define _CVI_COMM_SNS_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/cvi_type.h>
#include <linux/cvi_defines.h>
#include "cvi_debug.h"
#include "cvi_comm_isp.h"

// ++++++++ If you want to change these interfaces, please contact the isp team. ++++++++

#define NOISE_PROFILE_CHANNEL_NUM 4
#define NOISE_PROFILE_LEVEL_NUM 2
#define NOISE_PROFILE_ISO_NUM 16
#define USE_USER_SEN_DRIVER 1

/*ISP CMOS sensor image mode*/
typedef struct _ISP_CMOS_SENSOR_IMAGE_MODE_S {
	CVI_U16 u16Width;	/*image width*/
	CVI_U16 u16Height;	/*image height*/
	CVI_FLOAT f32Fps;	/*Sensor frame rate*/
	CVI_U8 u8SnsMode;	/*Sensor mode*/
} ISP_CMOS_SENSOR_IMAGE_MODE_S;

/*ISP CMOS sensor black level*/
typedef struct _ISP_CMOS_BLACK_LEVEL_S {
	CVI_BOOL bUpdate;				/*Black level configuration need to be updated or not*/
	ISP_BLACK_LEVEL_ATTR_S blcAttr;	/*Black level detailed attributes and configuration*/
} ISP_CMOS_BLACK_LEVEL_S;

/*Sensor attribute information*/
typedef struct _ISP_SNS_ATTR_INFO_S {
	CVI_U32 eSensorId;	/*Sensor id*/
} ISP_SNS_ATTR_INFO_S;

/*Noise calibration data for ISP CMOS sensors*/
typedef struct cviISP_CMOS_NOISE_CALIBRATION_S {
	CVI_FLOAT CalibrationCoef[NOISE_PROFILE_ISO_NUM][NOISE_PROFILE_CHANNEL_NUM][NOISE_PROFILE_LEVEL_NUM];	/*Noise calibration coefficient*/
} ISP_CMOS_NOISE_CALIBRATION_S;

/*Default settings for CMOS sensors*/
typedef struct _ISP_CMOS_DEFAULT_S {
	ISP_CMOS_NOISE_CALIBRATION_S stNoiseCalibration;
} ISP_CMOS_DEFAULT_S;

typedef struct _ISP_SENSOR_EXP_FUNC_S {
	/* Callback for init sensor IIC and register. */
	CVI_VOID (*pfn_cmos_sensor_init)(VI_PIPE ViPipe);
	/* Callback for exit sensor. */
	CVI_VOID (*pfn_cmos_sensor_exit)(VI_PIPE ViPipe);
	/* Callback for init sensor image mode. */
	CVI_VOID (*pfn_cmos_sensor_global_init)(VI_PIPE ViPipe);
	/* Callback for set sensor image mode. */
	CVI_S32 (*pfn_cmos_set_image_mode)(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode);
	/* Callback for set sensor wdr mode. */
	CVI_S32 (*pfn_cmos_set_wdr_mode)(VI_PIPE ViPipe, CVI_U8 u8Mode);

	/* the algs get data which is associated with sensor, except 3a */
	CVI_S32 (*pfn_cmos_get_isp_default)(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef);
	/* Callback for get sensor blc level. */
	CVI_S32 (*pfn_cmos_get_isp_black_level)(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel);
	/* Callback for get current sensor register info. */
	CVI_S32 (*pfn_cmos_get_sns_reg_info)(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *pstSnsRegsInfo);

	/* the function of sensor set pixel detect */
	//CVI_VOID (*pfn_cmos_set_pixel_detect)(VI_PIPE ViPipe, bool bEnable);
} ISP_SENSOR_EXP_FUNC_S;

typedef struct bmISP_SENSOR_REGISTER_S {
	ISP_SENSOR_EXP_FUNC_S stSnsExp;
} ISP_SENSOR_REGISTER_S;

/*Master Clock Frequency*/
typedef enum _MCLK_FREQ_E {
	MCLK_FREQ_NONE = 0,
	MCLK_FREQ_37P125M,	/*37.125M hz*/
	MCLK_FREQ_25M,		/*25M hz*/
	MCLK_FREQ_27M,		/*27M hz*/
	MCLK_FREQ_NUM
} MCLK_FREQ_E;

/*Sensor MCLK Configuration*/
typedef struct _SNS_MCLK_S {
	CVI_U32		u8Cam;
	MCLK_FREQ_E	enFreq;	/*Master Clock Frequency*/
} SNS_MCLK_S;

// -------- If you want to change these interfaces, please contact the isp team. --------

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _CVI_COMM_SNS_H_ */
