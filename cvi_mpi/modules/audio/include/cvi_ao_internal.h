#ifndef __CVI_AO_INTERNAL_H__
#define __CVI_AO_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include<stdio.h>
#include<pthread.h>
#include "cvi_aud_threads.h"
#include "cvi_audio_dnvqe.h"
#include "cvi_audio_interface_tinyalsa.h"

typedef struct _AO_CHN_CONFIG {
	CVI_BOOL bChannelEnable;
	CVI_BOOL bResampleChn;
	CVI_BOOL bEnableVqe;
	AUDIO_SAMPLE_RATE_E inSmpRate;
	AUDIO_SAMPLE_RATE_E outSmpRate;
	/* link the resample function by ain_index and chn_index*/
	CVI_VOID  *cviRes;
	/* Resample buffer based on the max num output caculated */
	CVI_S16 *ps16ResBuffer;
} ST_AO_CHANNEL_CONFIG;


typedef struct _AO_INSTANCE {
	AIO_ATTR_S ao_attrs;
	CVI_BOOL bEnableAO;
	CVI_S32 s32PeriodMs;
	CVI_S32 s32PeriodFrameLen;
	CVI_BOOL bEnableResample;
	AUDIO_SAMPLE_RATE_E eInputSampleRate;
	AUDIO_SAMPLE_RATE_E eOutputSampleRate;
	//CVI_BOOL bBindMode_triggerAO;
	AUDIO_TRACK_MODE_E enTrackMode;
	CVI_BOOL bThreadExist;
	pthread_t	AoutThreadId;
	AUDIO_DEV s32DevId;
	CVI_ST_THREAD_INFO stThreadInfo;
	ST_BIND_CONFIG ChnBindCfg;
	struct CVI_ST_AUD_TRACK_INFO_tag *pastTrackInfo[CVI_AUD_MAX_CHANNEL_NUM];
} ST_AO_INSTANCE;

extern ST_AO_INSTANCE gstAoInstance[CVI_MAX_AO_DEVICE_ID_NUM];
extern CVI_S32 _audio_sendframe(CVI_ST_AUD_TRACK_INFO_PTR pstTrackInfo,
								const AUDIO_FRAME_S *pstData, CVI_S32 s32MilliSec);

#ifdef __cplusplus
}
#endif

#endif //__CVI_AO_INTERNAL_H__
