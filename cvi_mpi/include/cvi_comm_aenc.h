/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_aenc.h
 * Description: basic audio audio
 * encoder api for application layer
 */
#ifndef __CVI_COMM_AENC_H__
#define __CVI_COMM_AENC_H__

#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include "cvi_comm_aio.h"



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _AENC_ATTR_G711_S {
	CVI_U32 resv; /* reserve item */
} AENC_ATTR_G711_S;

typedef struct _AENC_ATTR_G726_S {
	G726_BPS_E enG726bps; /* G.726 bitrate setting */
} AENC_ATTR_G726_S;

typedef struct _AENC_ATTR_ADPCM_S {
	ADPCM_TYPE_E enADPCMType; /* ADPCM type setting */
} AENC_ATTR_ADPCM_S;

typedef struct _AENC_ATTR_LPCM_S {
	CVI_U32 resv; /* reserve item */
} AENC_ATTR_LPCM_S;

typedef struct _AAC_AENC_ENCODER_S {
	PAYLOAD_TYPE_E  enType; /* payload type () */
	CVI_U32		u32MaxFrmLen; /* maximum frame length */
	CVI_CHAR	aszName[17];  /* encoder name */
	/* encoder type,be used to print proc information */
	CVI_S32 (*pfnOpenEncoder)(CVI_VOID *pEncoderAttr, CVI_VOID **ppEncoder); /* Open the encoder */
	/* pEncoder is the handle to control the encoder */
#if 0
	CVI_S32 (*pfnEncodeFrm)(CVI_VOID *pEncoder, const AUDIO_FRAME_S *pstData,
		CVI_U8 *pu8Outbuf, CVI_U32 *pu32OutLen); /* Encode a frame of audio data */
#endif
CVI_S32  (*pfnEncodeFrm)(CVI_VOID *pEncoder, CVI_S16 * inputdata, CVI_U8 * pu8Outbuf,
							CVI_S32 s32InputSizeBytes, CVI_U32 *pu32OutLen); /* Encode a frame of audio data */
	CVI_S32 (*pfnCloseEncoder)(CVI_VOID *pEncoder); /* Close the encoder */
} AAC_AENC_ENCODER_S;

typedef struct _AENC_CHN_ATTR_S {
	PAYLOAD_TYPE_E      enType;         /*payload type ()*/
	CVI_U32		u32PtNumPerFrm;         /*payload number per frame*/
	CVI_U32              u32BufSize;      /*buf size [2~CVI_MAX_AUDIO_FRAME_NUM]*/
	/* CVI_VOID ATTRIBUTE   *pValue;  point to attribute of definite audio encoder*/
	CVI_VOID    *pValue; /*Specific protocol attribute pointer*/
	CVI_BOOL bFileDbgMode; /*Open the save file mode*/
} AENC_CHN_ATTR_S;



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __CVI_COMM_AENC_H__*/

