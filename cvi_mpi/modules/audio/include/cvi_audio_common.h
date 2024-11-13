#ifndef _CVI_AUDIO_COMMON_H_
#define _CVI_AUDIO_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include<stdio.h>
#include"alog.h"
#include<cvi_comm_aio.h>
#ifdef RPC_MULTI_PROCESS_AUDIO
#include "cvi_audio_rpc.h"
#endif
//#include "cvi_ai_internal.h"


#ifndef TRACK_MAX
#define TRACK_MAX  8
#endif

#define AUDIO_ALIGN(x, a)      (((x) + ((a)-1)) & ~((a)-1))
#define WORD2INT(x) ((x) < -32767.5f ? -32768 : ((x) > 32766.5f ? 32767 : floor(.5+(x))))

#define _VERSION_TAG_ "master_20220429_tiny0323newsdk_ver5_downRes"

#ifndef SAFE_FREE_BUF
#define SAFE_FREE_BUF(OBJ) {if (NULL != OBJ) {free(OBJ); OBJ = NULL; } }
#endif

#define PERIOD_MS 20
#define DEFAULT_BYTES_PER_SAMPLE 2
#define CYCLE_BUFFER_SIZE (19200)
#define CVI_PROCESS_AACLC_FRMAE_LEN (1024)
#define CVI_AUDIO_AAC_DECODE 1
#define CVI_AAC_BUF_SIZE (1024 * 6)


typedef enum _AUD_E_BIND_CFG {
	AUD_E_AI_BIND_AO = 0,
	AUD_E_AI_BIND_AENC,
	AUD_E_ADEC_BIND_AO,
	AUD_E_BIND_BUTT
} AUD_E_BIND_CFG;


typedef enum AUD_ROLE_E_INFOtag {
	ROLE_E_SOURCE = 0,
	ROLE_E_DEST = 1,
	ROLE_E_BUTT
} AUD_ROLE_E_INFO;

typedef struct AUD_DEV_CHN_INFOtag {
	CVI_S32 s32DevId;
	CVI_S32 s32ChnId;
} AUD_DEV_CHN_INFO;

typedef struct AUD_BIND_INFOtag {
	CVI_S32 bBind;
	AUD_ROLE_E_INFO role;
	union {
		AUD_DEV_CHN_INFO srcinfo;
		AUD_DEV_CHN_INFO dstinfo;
	};
} AUD_BIND_INFO;


typedef struct _AUD_BIND_IO_INFO {
	CVI_S32  SrcDevId;
	CVI_S32  DestDevId;
	CVI_S32  SrcChnId;
	CVI_S32  DestChnId;
} AUD_BIND_IO_INFO;


typedef struct _ST_BIND_CONFIG {
	CVI_BOOL bAIbindAO;
	CVI_BOOL bAIbindAENC;
	CVI_BOOL bADECbindAO;
	CVI_S32  SrcDevId;
	CVI_S32  DestDevId;
	CVI_S32  SrcChnId;
	CVI_S32  DestChnId;
	AUD_BIND_IO_INFO astBindIoInfo[AUD_E_BIND_BUTT];
} ST_BIND_CONFIG;



enum {
	STATE_IDLE,
	STATE_READY,
	STATE_RUNNING,
	STATE_PAUSE,
	STATE_STOP,
	STATE_CLOSE
};

#if 0
typedef struct _AI_CHN_CONFIG {
	//ain channel parameters
	CVI_BOOL bChannelEnable;
	AI_CHN_PARAM_S  stChnParams;
	//ain vqe parameters
	CVI_BOOL bEnableVqe;
	CVI_BOOL bVqeWithAecOn;
	CVI_BOOL bindModeVQEOn;
	CVI_CHAR *pVqeBuff;
	//ain resample parameters
	CVI_BOOL bResampleChn;
	AUDIO_SAMPLE_RATE_E inSmpRate;
	AUDIO_SAMPLE_RATE_E outSmpRate;
	CVI_S32 s32ResFrameLen;
	CVI_S32 s32VqeChannelCnt;
	CVI_VOID  *cviRes;
	CVI_S16 *ps16ResBuffer;
	CVI_U32 u32AecShareBufSize;
	CVI_S16 *pRawData;
	CVI_S16 *pAecData;
	CVI_S16 *pRawResData;
	CVI_S16 *pAecResData;
	CVI_U32 u32RawPeriodBytes;
	CVI_U32 u32AecPeriodBytes;
	//ain mode, cyclebuffer parameters
	CVI_S32 iAecShmMemIndex;
} ST_AI_CHANNEL_CONFIG, *ST_AI_CHANNEL_CONFIG_PTR;
#else
typedef struct _AI_CHN_CONFIG {
	//ain channel parameters
	//CVI_BOOL bChannelEnable;
	AI_CHN_PARAM_S  stChnParams;
	//ain vqe parameters
	CVI_BOOL bEnableVqe;
	CVI_BOOL bVqeWithAecOn;
	//CVI_BOOL bindModeVQEOn;
	//CVI_CHAR *pVqeBuff;
	//ain resample parameters
	//CVI_BOOL bResampleChn;
	//AUDIO_SAMPLE_RATE_E inSmpRate;
	//AUDIO_SAMPLE_RATE_E outSmpRate;
	//CVI_S32 s32ResFrameLen;
	//CVI_S32 s32VqeChannelCnt;
	//CVI_VOID  *cviRes;
	//CVI_S16 *ps16ResBuffer;
	//CVI_U32 u32AecShareBufSize;//aec
	CVI_S16 *pRawData;
	//CVI_S16 *pAecData;//aec
	//CVI_S16 *pRawResData;
	//CVI_S16 *pAecResData;//aec
	CVI_U32 u32RawPeriodBytes;
	//CVI_U32 u32AecPeriodBytes;//aec
	//ain mode, cyclebuffer parameters
	//CVI_S32 iAecShmMemIndex;
} ST_AI_CHANNEL_CONFIG;//stAiChnCfg


#endif


typedef struct CVI_ST_AUD_TRACK_INFO_tag {
	CVI_S32 direction;
	CVI_S32 chnid;
	CVI_S32 bInitOk;
	CVI_S32 refFlag;
	CVI_S32 iChannels;
	CVI_S32 iSampleRate;
	CVI_S32 iShmMemIndex;
	CVI_U32 u32ShareBufSize;
	CVI_S32 b_dump_enable;
	CVI_S32 state;
	CVI_S32 lastState;
	CVI_U32 u32SeqCnt;
	CVI_U32 dataByte;
	long allDataByte;
	void *pResHandle;
	CVI_S32 b_need_resample;
	AUD_BIND_INFO stBindinfo;
	void *pBasePtr;
	struct CVI_ST_THREAD_INFOtag *ThreadInfo;
	union {
		ST_AI_CHANNEL_CONFIG stAiChnCfg;
	};
} CVI_ST_AUD_TRACK_INFO, *CVI_ST_AUD_TRACK_INFO_PTR;


typedef enum _CLI_INPUT_STATUS_E {
	AUD_PCM_READLY = 1,
	AUD_VQE_READLY = 2,
	AUD_TRACK_READLY = 3,
	AUD_TRACK_RUNINNG = 4,
	AUD_SHATEMEM_READLY = 5,
	AUD_AI_THREAD_EXIT = 6,
	AUD_AI_BUTT
} CLI_INPUT_STATUS_E;

typedef enum _CLI_OUT_STATUS_E {
	AUD_READLY = 2,
	AUD_SHARE_READLY = 3,
	AUD_RUNINNG = 4,
	AUD_WRITE_READLY = 5,
	AUD_AO_THREAD_EXIT = 6,
	AUD_AO_BUTT
} CLI_OUT_STATUS_E;

typedef struct _AI_CLI_THREAD_STATUS {
	CLI_INPUT_STATUS_E audThreadStatus[CVI_AUD_MAX_CHANNEL_NUM];
} AI_CLI_THREAD_STATUS;

typedef struct _AO_CLI_THREAD_STATUS {
	CLI_OUT_STATUS_E aoThreadStatus[CVI_AUD_MAX_CHANNEL_NUM];
} AO_CLI_THREAD_STATUS;


extern ST_BIND_CONFIG gstBindConfig;
extern void *gpstCircleBuffer_AiEnc;

extern CVI_S32 CVI_AO_Init(void);
extern CVI_VOID CVI_AO_Deinit(void);
extern CVI_S32 CVI_AI_Init(void);
extern void CVI_AI_Deinit(void);
extern CVI_S32 CVI_AENC_Init(void);
extern void CVI_AENC_Deinit(void);
extern CVI_S32 CVI_ADEC_Init(void);
extern CVI_VOID CVI_ADEC_Deinit(void);
extern void _check_and_dump(const char *filename, char *buf, unsigned int sizebytes);
extern unsigned long long _get_current_pts(void);
extern unsigned long long _get_current_time(void);

#ifdef __cplusplus
}
#endif

#endif //_CVI_AUDIO_COMMON_H_
