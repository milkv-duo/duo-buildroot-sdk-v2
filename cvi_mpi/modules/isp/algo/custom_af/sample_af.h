/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: sample_af.h
 * Description:
 *
 */

#ifndef _SAMPLE_AF_H_
#define _SAMPLE_AF_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifndef UNUSED
#define UNUSED(x)	(void)(x)
#endif

#define AF_CHECK_POINTER(addr)


#define AF_CHECK_HANDLE_ID(s32Handle) \
	do { \
		if (((s32Handle) < 0) || ((s32Handle) >= VI_MAX_PIPE_NUM)) { \
			printf("Illegal handle id %d in %s!\n", (s32Handle), __func__); \
			return CVI_FAILURE; \
		} \
	} while (0)

void AF_GetAlgoVer(CVI_U16 *pVer, CVI_U16 *pSubVer);
CVI_S32 CVI_AF_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib);
CVI_S32 CVI_AF_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAfLib);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* _SAMPLE_AF_H_ */
