/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_awb.h
 * Description:
 *
 */

#ifndef _SAMPLE_AWB_H_
#define _SAMPLE_AWB_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef UNUSED
#define UNUSED(x)	(void)(x)
#endif

#define SAMPLE_AWB_INTERVAL	(6)

#define SAMPLE_AWB_COL	(64)	//Don't modify
#define SAMPLE_AWB_ROW	(32)	//Don't modify

#define AWB_GAIN_BASE	(1024)	//Don't modify

#define AWB_GET_CTX(ViPipe)	(&g_astAwbCtx[ViPipe])

#define AWB_CHECK_DEV			ISP_CHECK_PIPE
#define AWB_CHECK_POINTER(addr)	((void)(addr))


#define AWB_CHECK_HANDLE_ID(s32Handle) \
	do { \
		if (((s32Handle) < 0) || ((s32Handle) >= MAX_AWB_LIB_NUM)) { \
			printf("Illegal handle id %d in %s!\n", (s32Handle), __func__); \
			return CVI_FAILURE; \
		} \
	} while (0)


#define AWB_CHECK_LIB_NAME(acName) \
	do { \
		if (strncmp((acName), CVI_AWB_LIB_NAME, ALG_LIB_NAME_SIZE_MAX) != 0)	{ \
			printf("Illegal lib name %s in %s!\n", (acName), __func__); \
			return CVI_FAILURE; \
		} \
	} while (0)


typedef struct cviSAMPLE_AWB_CTX_S {
	/* usr var */
	CVI_U16	u16DetectTemp;
	CVI_U8	u8WbType;
	/* communicate with isp */
	ISP_AWB_PARAM_S stAwbParam;
	CVI_U32	u32FrameCnt;
	ISP_AWB_INFO_S	stAwbInfo;
	ISP_AWB_RESULT_S stAwbResult;
	VI_PIPE	IspBindDev;
	/* communicate with sensor, defined by user. */
	CVI_BOOL		bSnsRegister;
	//SENSOR_ID	SensorId;
	ISP_SNS_ATTR_INFO_S stSnsAttrInfo;
	AWB_SENSOR_DEFAULT_S stSnsDft;
	AWB_SENSOR_REGISTER_S stSnsRegister;
	/* global variables of awb algorithm */
} SAMPLE_AWB_CTX_S;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _SAMPLE_AWB_H_ */
