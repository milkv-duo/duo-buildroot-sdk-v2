#ifndef __CVI_AUDIO_DEC_H__
#define __CVI_AUDIO_DEC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <linux/cvi_type.h>
#include "cvi_comm_adec.h"
#include "cvi_comm_aenc.h"
#include "cvi_audio_common.h"
#include "cvi_transcode_interface.h"


//-----support AAC inside cvi_audio----start

#define AACLD_SAMPLES_PER_FRAME_DEFAULT 512
#define AACLC_SAMPLES_PER_FRAME_DEFAULT 1024
#define AACPLUS_SAMPLES_PER_FRAME_DEFAULT 2048
typedef enum _AAC_INTERNAL_TYPE_E {
	AAC_TYPE_AACLC = 0,    /* AAC LC */
	AAC_TYPE_EAAC = 1,	   /* eAAC	(HEAAC or AAC+	or aacPlusV1) */
	AAC_TYPE_EAACPLUS = 2, /* eAAC+ (AAC++ or aacPlusV2) */
	AAC_TYPE_AACLD = 3,
	AAC_TYPE_AACELD = 4,
	AAC_TYPE_BUTT,
} AAC_INTERNAL_TYPE_E;

typedef enum _AUDIO_DATA_TIMEMODE {
	E_AUDIO_DATA_BLOCK_MODE = (-1),
	E_AUDIO_DATA_NONE_BLOCK_MODE = (0),
	E_AUDIO_DATA_TIMEOUT_MODE = (2),
} E_AUDIO_DATA_TIMEMODE;
#define	CVI_AUDIO_DATA_TEN_SEC_TIMEOUT	(1000 * 15)
typedef enum _AAC_INTERNAL_BPS_E {
	AAC_BPS_8K = 8000,
	AAC_BPS_16K = 16000,
	AAC_BPS_22K = 22000,
	AAC_BPS_24K = 24000,
	AAC_BPS_32K = 32000,
	AAC_BPS_48K = 48000,
	AAC_BPS_64K = 64000,
	AAC_BPS_96K = 96000,
	AAC_BPS_128K = 128000,
	AAC_BPS_256K = 256000,
	AAC_BPS_320K = 320000,
	AAC_BPS_BUTT
} AAC_INTERNAL_BPS_E;


typedef enum _AAC_INTERNAL_TRANS_TYPE_E {
	AAC_TRANS_TYPE_ADTS = 0,
	AAC_TRANS_TYPE_LOAS = 1,
	AAC_TRANS_TYPE_LATM_MCP1 = 2,
	AAC_TRANS_TYPE_BUTT
} AAC_INTERNAL_TRANS_TYPE_E;

typedef struct _ST_AENC_ATTR_AAC_S {
	//AAC_TYPE_E enAACType;		/* AAC profile type */
	AAC_INTERNAL_TYPE_E  enAACType;		/* AAC profile type */
	//AAC_BPS_E enBitRate;
	AAC_INTERNAL_BPS_E enBitRate;
	/* AAC bitrate (LC:16~320, EAAC:24~128, EAAC+:16~64, AACLD:16~320, AACELD:32~320)*/
	AUDIO_SAMPLE_RATE_E enSmpRate;
	/* AAC sample rate (LC:8~48, EAAC:16~48, EAAC+:16~48, AACLD:8~48, AACELD:8~48)*/
	AUDIO_BIT_WIDTH_E enBitWidth;
	/* AAC bit width (only support 16bit)*/
	AUDIO_SOUND_MODE_E enSoundMode;
	/* sound mode of inferent audio frame */
	//AAC_TRANS_TYPE_E enTransType;
	AAC_INTERNAL_TRANS_TYPE_E enTransType;

	CVI_S16 s16BandWidth; /* targeted audio bandwidth in Hz (0 or 1000~enSmpRate/2), the default is 0*/
} ST_AENC_ATTR_AAC_S;

typedef struct _ST_AAC_ENC_INST {
	AAC_AENC_ENCODER_S *pstAACEncoder;
	ST_AENC_ATTR_AAC_S stAACEncConfig;
	CVI_VOID *aacenc_handle;
} ST_AAC_ENC_INST;

typedef struct cviAAC_FRAME_INFO_S {
	CVI_S32 s32Samplerate; /* sample rate*/
	CVI_S32 s32BitRate;    /* bitrate */
	CVI_S32 s32Profile;    /* profile*/
	CVI_S32 s32TnsUsed;    /* TNS Tools*/
	CVI_S32 s32PnsUsed;    /* PNS Tools*/
} AAC_FRAME_INFO_S;

typedef struct _ST_ADEC_ATTR_AAC_S {
	//AAC_TRANS_TYPE_E enTransType;
	AAC_INTERNAL_TRANS_TYPE_E enTransType;
	AUDIO_SOUND_MODE_E enSoundMode;
	AUDIO_SAMPLE_RATE_E enSmpRate;
} ST_ADEC_ATTR_AAC_S;
typedef struct _ST_AAC_DEC_INST {
	const ADEC_DECODER_S *pstAACDecoder;
	ST_ADEC_ATTR_AAC_S stAACDecConfig;
	CVI_VOID *aacdec_handle;
} ST_AAC_DEC_INST;


typedef struct _st_aac_dec {
	int sample_rate;
	int channel;
	int frame_byte;//ADTS+DATA
	unsigned char *aac_inputbuffer;
	unsigned  long long aac_remainbytes;
	unsigned  long long aac_bufferbyte;
	short *aac_dec_after_buf;
} ST_AAC_DEC;

typedef struct _ADEC_INSTANCE {
	ADEC_CHN_ATTR_S adec_attrs;
	ADEC_ATTR_ADPCM_S *stAdpcm;
	ADEC_ATTR_G711_S *stAdecG711;
	ADEC_ATTR_G726_S *stAdecG726;
	ADEC_ATTR_LPCM_S *stAdecLpcm;
	//ADEC_ATTR_AAC_S *stAdecAac;
	ST_ADEC_ATTR_AAC_S *stAdecAac;
	CVI_BOOL	bEnableAdec;
	CVI_VOID	*AdecHandle;
	CVI_U8		*DecBuff;
	CVI_S32 iShmMemIndex;
	CVI_CHAR	*pDecReadBuff;//for cyclebuffer read
	E_CVI_AUDIOCODEC eACodecType;
	S_CVI_AUDIO_CONFIG cviAdecConfig;
	CVI_S32 s32DecodedDataBytes;
	CVI_S32 fd_adec_input;
	CVI_S32 s32BytesOutPerDecode;
	CVI_BOOL bDecLoop;
	CVI_U64 u64TimeStamp;
	FILE *fd_adec_out;
	ST_AAC_DEC *pst_aac_dec;
	struct AUD_BIND_INFOtag stBindinfo;
	void *gpstCircleBuffer;//TODO:this should replace the global variable for adec
	CVI_S32 decInbyte;
	CVI_S32 decOutbyte;
} ST_ADEC_INSTANCE;

typedef struct _AENC_INSTANCE {
	AENC_CHN_ATTR_S aenc_attrs;
	AENC_ATTR_ADPCM_S *stAdpcmAenc;
	AENC_ATTR_G711_S *stAencG711;
	AENC_ATTR_G726_S *stAencG726;
	AENC_ATTR_LPCM_S *stAencLpcm;
	//AENC_ATTR_AAC_S  *stAencAac;
	ST_AENC_ATTR_AAC_S  *stAencAac;
	CVI_BOOL		 bEnableAenc;
	CVI_BOOL	bMute;
	CVI_VOID	*AencHandle;
	E_CVI_AUDIOCODEC eACodecType;
	S_CVI_AUDIO_CONFIG cviAencConfig;
	CVI_CHAR *EncInBuff;//for cyclebuffer read
	CVI_S32 EncInBuffSize;
	/* EncBuff used to save the encoded data*/
	CVI_U8		*EncBuff;
	CVI_U8		*EncBuff_aec;
	CVI_S32		s32EncodedDataBytes;
	CVI_S32		encInbyte;
	CVI_S32		encOutbyte;
	CVI_S32		fd_aenc_input;
	FILE		*fd_aenc_out;
	struct AUD_BIND_INFOtag stBindinfo;
	CVI_S32 iAencShmMemIndex;
	CVI_S32 s32SendBytePeriod;
	//setup for vqe config
	CVI_BOOL bVqeOn;
	CVI_BOOL bVqeWithAecOn;
	CVI_CHAR *EncVqeBuff;
} ST_AENC_INSTANCE;


extern ST_ADEC_INSTANCE gstAdecInstance[ADEC_MAX_CHN_NUM];


#ifdef __cplusplus
}
#endif

#endif //__CVI_AUDIO_DEC_H__

