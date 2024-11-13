#ifndef __CVI_AI_INTERNAL_H__
#define __CVI_AI_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
/* usage from cvi audio header */
#include "cvi_audio_arch.h"
#include "cvi_comm_adec.h"
#include "cvi_comm_aenc.h"
#include "cvi_comm_aio.h"
#include "cvi_resampler_api.h"
#include <linux/cvi_type.h>
#include "cvi_audio.h"
#include "cvi_aud_internal_tinyalsa.h"
#include "cvi_transcode_interface.h"
#include "cvi_audio.h"
#include "acodec.h" /* for ioctl control */
#include "asoundlib.h"
#include "cvi_audio_vqe.h"
#include "cyclebuffer.h"
#include "cvi_audio_sonic.h"
#include "cvi_audio_transcode.h"
#include "cvi_aud_threads.h"


#define CVI_MAX_INPUT_MIC_NUMBERS 8
typedef struct _AIN_MULTI_MIC {
	CVI_BOOL bEnableMultiMic;
	CVI_S32 s32MultiChn;
	cycleBuffer * pMultiMicCb[CVI_MAX_INPUT_MIC_NUMBERS];
	CVI_CHAR *pMultiMicBuffer[CVI_MAX_INPUT_MIC_NUMBERS];
	short *pMultiMicTmp[CVI_MAX_INPUT_MIC_NUMBERS];
} ST_AIN_MULTI_MIC;

typedef struct _AI_INSTANCE {
	//device config
	AIO_ATTR_S aio_attrs;
	//ST_TINYALSA_CONFIG stTinyAlsaCfg;
	CVI_BOOL bEnableAI;
	AUDIO_TRACK_MODE_E enTrackMode;
	//for device thread
	pthread_t AinThreadId;
	//channel config
	CVI_BOOL bThreadExist;
	AUDIO_DEV s32DevId;
	CVI_S32 s32VqeEnableCnt;
	struct CVI_ST_THREAD_INFOtag stThreadInfo;
	ST_BIND_CONFIG ChnBindCfg;
	CVI_ST_AUD_TRACK_INFO * ppastTrackInfo[CVI_AUD_MAX_CHANNEL_NUM];
	ST_AIN_MULTI_MIC stMultiMic;
	void *ppVolinstance[CVI_AUD_MAX_CHANNEL_NUM];
} ST_AI_INSTANCE;


typedef struct _AIN_THREAD_CFG {
	//ST_TINYALSA_CONFIG *pAlsaConfig;
//	ST_AI_CHANNEL_CONFIG *pChnConfig;
	CVI_BOOL *pbEnableThread;
	CVI_S32 DevId;
} ST_AIN_THREAD_CFG;
extern ST_AI_INSTANCE gstAiInstance[CVI_MAX_AI_DEVICE_ID_NUM];

#ifdef __cplusplus
}
#endif
#endif //__CVI_AI_INTERNAL_H__

