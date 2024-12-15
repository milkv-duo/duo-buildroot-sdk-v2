#ifndef _CVI_AUD_THREADS_H_
#define _CVI_AUD_THREADS_H_


#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdint.h>
#include <asoundlib.h>
#include "cvi_audio_arch.h"
#include "cvi_audio_common.h"
//#include "cvi_ai_internal.h"

typedef enum _AUDIO_INOUT {
	AUDIO_OUT   = 0,
	AUDIO_IN = 1,
	AUDIO_BUTT
} AUDIO_INOUT_E;

typedef struct _threadVqeInfo {
	bool bDevEnableVqe;
	bool bDevVqeWithAecOn;
} ST_THREAD_VQE_INFO;

typedef struct CVI_ST_THREAD_INFOtag {
	int card;
	int refCard;
	char *cCardName;
	struct pcm *pcmHandle;
	struct pcm_config stPcmCfg;
	pthread_t mThreadId;
	int b_enable_dump;
	int bStandby;
	int i32TrackRefCnt;
	CVI_U32 u32Count;
	struct CVI_ST_AUD_TRACK_INFO_tag *TrackVector[CVI_AUD_MAX_CHANNEL_NUM];
	int i32ExitPending;
} CVI_ST_THREAD_INFO, *CVI_ST_THREAD_INFO_PTR;

extern AI_CLI_THREAD_STATUS gstAiThreadStatus;
extern AO_CLI_THREAD_STATUS gstAoThreadStatus;
CVI_VOID *AudioPrimaryOutputThread(CVI_VOID *arg);
CVI_VOID *AudioPrimaryInputThread(CVI_VOID *arg);


#ifdef __cplusplus
}
#endif

#endif //_CVI_AUD_THREADS_H_


