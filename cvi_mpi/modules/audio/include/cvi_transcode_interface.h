/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: cvi_transcode_interface.h
 * Description:audio transcode api header
 */

#ifndef __CVI_TRANSCODE_INTERFACE_H__
#define __CVI_TRANSCODE_INTERFACE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <linux/cvi_type.h>

#define _AUDIO_TRANSCODE_VERSION_TAG_ "cviaudio_transcode20200916"
typedef enum _cvi_audiocodec {
	AUD_CODEC_NONE = 0x0,
	AUD_CODEC_G711_ALAW = 0x01,
	AUD_CODEC_G711_MLAW = 0x02,
	AUD_CODEC_G726 = 0x3,
	AUD_CODEC_ADPCM_IMA = 0x4,
	AUD_CODEC_ADPCM_DVI4 = 0x5,
	AUD_CODEC_ADPCM_VDVI = 0x6,
	AUD_CODEC_AAC = 0x7,//AAC support externally, please check sample/audio/AAC
	AUD_CODEC_END = 0x08,
} E_CVI_AUDIOCODEC;

typedef enum _eRate {
	Rate16kBits = 2,	/**< 16k bits per second (2 bits per ADPCM sample) */
	Rate24kBits = 3,	/**< 24k bits per second (3 bits per ADPCM sample) */
	Rate32kBits = 4,	/**< 32k bits per second (4 bits per ADPCM sample) */
	Rate40kBits = 5,	/**< 40k bits per second (5 bits per ADPCM sample) */
	RateNone = 9
} eRate;
typedef struct _cvi_audio_config {
	eRate bitrate;
	int sample_rate;
	int channel_num;
} S_CVI_AUDIO_CONFIG;

/* Decode(unsigned char* pout_buf, unsigned int* pout_len ,	*/
/* unsigned char* pin_buf, unsigned int in_len)*/
/* int g711_decode(void *pout_buf, int *pout_len, */
/* const void *pin_buf, const int in_len , int type)*/
CVI_VOID *CVI_AUDIO_Transcode_Init(CVI_VOID *inst,
				E_CVI_AUDIOCODEC eAudCodecType,
				S_CVI_AUDIO_CONFIG *pConfig);

CVI_S32 CVI_AUDIO_Transcode_DeInit(CVI_VOID *inst, E_CVI_AUDIOCODEC eAudCodecType);

CVI_S32 CVI_AUDIO_Encode(CVI_VOID *inst, E_CVI_AUDIOCODEC eACodecType, CVI_VOID *pInputBuf,
			 CVI_VOID *pOutputBUf, CVI_S32 s32InputLen, CVI_S32 *s32OutLen);

CVI_S32 CVI_AUDIO_Decode(CVI_VOID *inst, E_CVI_AUDIOCODEC eACodecType, CVI_VOID *pInputBuf,
			 CVI_VOID *pOutputBUf, CVI_S32 s32InputLen, CVI_S32 *s32OutLen);
CVI_S32 CVI_AUDIO_DecodeParseAAC(CVI_VOID *inst, E_CVI_AUDIOCODEC eACodecType, CVI_VOID *pInputBuf,
			 CVI_S32 s32InputLen, CVI_S32 *s32RequireBytes);


#ifdef __cplusplus
}
#endif
#endif

