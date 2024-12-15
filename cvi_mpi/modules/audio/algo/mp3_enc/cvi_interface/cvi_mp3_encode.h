/*
 *	cvi_audio_uac.h  --  USB Audio Class Gadget API
 *
 */

#ifndef _CVI_MP3_ENCODE_H_
#define _CVI_MP3_ENCODE_H_
#include <stdio.h>
#include <linux/cvi_type.h>
/* ------------------------------------------------------------------------
 * Debugging, printing and logging
 */

typedef struct _ST_CVI_MP3_ENC_INIT {
	int bitrate;
	int sample_rate;
	int channel_num;
	int quality;//2, 5, 9 [2:best but slow, 9:worst but quick]
} ST_CVI_MP3_ENC_INIT;



#define CVI_MP3_ENC_CB_REQUIRE_MORE_INPUT  0X42

#if 0
#define CVI_MP3_ENC_CB_STATUS_INIT 0
#define CVI_MP3_ENC_CB_STATUS_GET_FIRST_INFO  0x41
#define CVI_MP3_ENC_CB_STATUS_INFO_CHG 0x42
#define CVI_MP3_ENC_CB_STATUS_BIT_RATE_CHG 0x43
#define CVI_MP3_ENC_CB_STATUS_STABLE 0x40
#endif

typedef struct _st_mp3_enc_cbinfo {
	int channels;
	int samplerate;
	int bitrate;
	int mode;
	int cbState;
} ST_MP3_ENC_CBINFO;

typedef int (*Encode_Cb)(void *, ST_MP3_ENC_CBINFO *, char *, int );


CVI_S32 CVI_MP3_Encode(CVI_VOID *inst,CVI_VOID *pInputBuf,
	CVI_VOID *pOutputBUf, CVI_S32 s32InputLen, CVI_S32 *s32OutLen);

CVI_VOID *CVI_MP3_Encode_Init(CVI_VOID *inst, ST_CVI_MP3_ENC_INIT stConfig);

CVI_S32 CVI_MP3_Encode_DeInit(CVI_VOID *inst);

CVI_S32 CVI_MP3_Encode_InstallCb(CVI_VOID *inst, Encode_Cb pCbFunc);

#endif /* _CVI_AUDIO_UAC_H_ */

