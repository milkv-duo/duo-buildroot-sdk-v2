#ifndef __CVI_AAC_ENC_H__
#define __CVI_AAC_ENC_H__

#define MAX_CHANNELS 2
#define AACENC_BLOCKSIZE 1024
#define _VERSION_TAG_  "cvi_fdkaac_enc_20210723"
#define AACLC_ENC_AOT 2
#define HEAAC_ENC_AOT 5
#define HEAAC_PLUS_ENC_AOT 29
#define AACLD_ENC_AOT 23
#define AACELD_ENC_AOT 39

int CodecArray[5] = {
	AACLC_ENC_AOT,
	HEAAC_ENC_AOT,
	HEAAC_PLUS_ENC_AOT,
	AACLD_ENC_AOT,
	AACELD_ENC_AOT
};

typedef enum {
	AACLC = 0,/**<AAC-LC format*/
	EAAC = 1,/**<HEAAC or AAC+  or aacPlusV1*/
	EAACPLUS = 2,/**<AAC++ or aacPlusV2*/
	AACLD = 3,/**<AAC LD(Low Delay)*/
	AACELD = 4,/**<AAC ELD(Low Delay)*/
} EncFormat;

typedef struct _aac_enc_info {
	int channel_num;
	int sample_rate;
	int frame_size;
	int bitrate;//16000
	//int aot;//2
	EncFormat aacformat;//0
} AAC_ENC_INFO;

typedef struct _st_aac_enc_info {
	int channel_num;
	int sample_rate;
	int frame_size;
	int cbState;
} ST_AAC_ENC_INFO;

typedef int (*pEncode_Cb)(void *, ST_AAC_ENC_INFO *, char *, int);

void *CVI_AAC_Encode_Init(AAC_ENC_INFO aac_enc);
int CVI_AAC_Encode(void *paac_handle, short *pInputBuf,
		   unsigned char *pu8Outbuf, int s32InputBytes, int *ps32NumOutBytes);
int CVI_AAC_Decode_Dinit(void *paac_handle);
int CVI_AAC_Encode_InstallCb(void *paac_handle, pEncode_Cb pCbFunc);


#endif


