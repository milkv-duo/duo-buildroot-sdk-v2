/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: dpcm_api.h
 * Description:
 *
 */

#ifndef _DPCM_API_H_
#define _DPCM_API_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
	CVI_U32 width;
	CVI_U32 height;
	CVI_U32 stride;
	CVI_U8 *buffer;
} RAW_INFO;

CVI_S32 decoderRaw(RAW_INFO rawInfo, CVI_U8 *outPut);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _DPCM_API_H_
