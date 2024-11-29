/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_ae.h
 * Description:
 *
 */

#ifndef _SAMPLE_AE_H_
#define _SAMPLE_AE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define AE_SENSOR_NUM (2)



#ifndef UNUSED
#define UNUSED(x)	(void)(x)
#endif

#define AE_GET_CTX(s32Handle)           (&g_astAeCtx_sample[s32Handle])

#define AE_CHECK_DEV			ISP_CHECK_PIPE
#define AE_CHECK_POINTER(addr)	((void)(addr))

#define AE_CHECK_HANDLE_ID(s32Handle) \
		do { \
			if (((s32Handle) < 0) || ((s32Handle) >= MAX_AE_LIB_NUM)) { \
				printf("Illegal handle id %d in %s!\n", (s32Handle), __func__); \
				return CVI_FAILURE; \
			} \
		} while (0)

#define AE_CHECK_LIB_NAME(acName) \
		do { \
			if (strcmp((acName), CVI_AE_LIB_NAME) != 0)	{ \
				printf("Illegal lib name %s in %s!\n", (acName), __func__); \
				return CVI_FAILURE; \
			} \
		} while (0)

typedef struct _SAMPLE_AE_CTX_S {
	/* usr var */
	CVI_U8	u8AeMode;
	CVI_BOOL bWDRMode;
	/* communicate with isp */
	ISP_AE_PARAM_S	stAeParam;
	CVI_U32	u32FrameCnt;
	ISP_AE_INFO_S	stAeInfo;
	ISP_AE_RESULT_S	stAeResult;
	VI_PIPE	IspBindDev;
	/* communicate with sensor, defined by user. */
	CVI_BOOL	bSnsRegister;
	ISP_SNS_ATTR_INFO_S	stSnsAttrInfo;
	AE_SENSOR_DEFAULT_S	stSnsDft;
	AE_SENSOR_REGISTER_S	stSnsRegister;
	/* global variables of ae algorithm */
} SAMPLE_AE_CTX_S;


extern CVI_FLOAT sample_ae_fps;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _SAMPLE_AE_H_ */
