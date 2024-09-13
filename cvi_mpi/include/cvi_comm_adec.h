/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_adec.h
 * Description: basic audio decoder  api for application layer
 */

#ifndef __CVI_COMM_ADEC_H__
#define __CVI_COMM_ADEC_H__


#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include "cvi_comm_aio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct _ADEC_ATTR_G711_S {
	CVI_U32 resv;            /*reserve item*/
} ADEC_ATTR_G711_S;

typedef struct _ADEC_ATTR_G726_S {
	G726_BPS_E enG726bps; /*G.726 bitrate setting*/
} ADEC_ATTR_G726_S;

typedef struct _ADEC_ATTR_ADPCM_S {
	ADPCM_TYPE_E enADPCMType; /*ADPCM type setting*/
} ADEC_ATTR_ADPCM_S;

typedef struct _ADEC_ATTR_LPCM_S {
	CVI_U32 resv;            /*reserve item*/
} ADEC_ATTR_LPCM_S;

typedef enum _ADEC_MODE_E {
	ADEC_MODE_PACK = 0, /*Pack method decoding*/
	/*require input is valid dec pack(a		*/
	/*complete frame encode result),*/
	/*e.g.the stream get from AENC is a*/
	/*valid dec pack, the stream know actually*/
	/*pack len from file is also a dec pack.*/
	/*this mode is high-performative*/
	ADEC_MODE_STREAM, /*Stream method decoding*/
	/*input is stream,low-performative,		*/
	/*if you couldn't find out whether a stream is*/
	/*valid dec pack,you could use*/
	/*this mode*/
	ADEC_MODE_BUTT
} ADEC_MODE_E;

typedef struct _ADEC_CH_ATTR_S {
	PAYLOAD_TYPE_E enType; /*payload type*/
	CVI_U32         u32BufSize; /*buf size[2~CVI_MAX_AUDIO_FRAME_NUM]*/
	ADEC_MODE_E	enMode;/*decode mode*/
	/* CVI_VOID ATTRIBUTE      *pValue;*/
	CVI_VOID *pValue; /*Specific protocol attribute pointer*/
	CVI_BOOL bFileDbgMode; /*Open the save file mode*/
	/*------user should update these information if*/
	/*ao not enable*/
	CVI_S32 s32BytesPerSample; /*Unit sampling and using bytes*/
	CVI_S32 s32frame_size; /*in samples*/
	CVI_S32 s32ChannelNums; /* 1 or 2*/
	CVI_S32 s32Sample_rate; /*Sampling frequency*/
} ADEC_CHN_ATTR_S;

typedef struct _ADEC_DECODER_S {
	PAYLOAD_TYPE_E  enType; /*payload type*/
	CVI_CHAR	aszName[17]; /*decoder name*/

	CVI_S32 (*pfnOpenDecoder)(CVI_VOID *pDecoderAttr, CVI_VOID **ppDecoder); /*open decoder*/
	CVI_S32 (*pfnDecodeFrm)(CVI_VOID *pDecoder, CVI_U8 **pu8Inbuf,
				CVI_S32 *ps32LeftByte,
				CVI_U16 *pu16Outbuf, CVI_U32 *pu32OutLen, CVI_U32 *pu32Chns); /*Decode a frame of data*/
	CVI_S32 (*pfnGetFrmInfo)(CVI_VOID *pDecoder, CVI_VOID *pInfo); /*get frame info*/
	CVI_S32 (*pfnCloseDecoder)(CVI_VOID *pDecoder); /*close decoder*/
	CVI_S32 (*pfnResetDecoder)(CVI_VOID *pDecoder); /*reset decoder*/
} ADEC_DECODER_S;


  
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif/* End of #ifndef __CVI_COMM_ADEC_H__*/

