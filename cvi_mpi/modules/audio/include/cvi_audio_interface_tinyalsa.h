#ifndef __CVI_AUDIO_INTERFACE_TINYALSA_H__
#define __CVI_AUDIO_INTERFACE_TINYALSA_H__

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
//#include "cvi_sys.h"
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

#ifdef USE_CYCLE_BUFFER
#define USE_CYCLE_FIFO_BUFFER 1	//if this define is 0, it will using ring buffer instead
#else
//using ring buffer
#undef USE_CYCLE_FIFO_BUFFER
#endif

extern CVI_S32 cviaud_dbg;
/* userful macro */
#ifndef UNUSED_REF
#define UNUSED_REF(X)  ((X) = (X))
#endif

#ifndef SAFE_FREE_BUF
#define SAFE_FREE_BUF(OBJ) {if (NULL != OBJ) {free(OBJ); OBJ = NULL; } }
#endif

#ifndef CHECK_CVIAUDIO_DEV_INVALID
#define CHECK_CVIAUDIO_DEV_INVALID(X)  ((X) <= 0 ? 1 : 0)
#endif


#ifndef CVIAUDIO_UNUSED_REF
#define CVIAUDIO_UNUSED_REF(X)  ((X) = (X))
#endif


#ifndef CVIAUDIO_CALLOC
#define CVIAUDIO_CALLOC(TYPE, COUNT) ((TYPE *)calloc(COUNT, sizeof(TYPE)))
#endif

/* version tag */
/*cviaudio_20201105_tinyalsa turn on cycle buffer*/


#define RES_LIB_NAME "libcvi_RES1.so"
#define DEFAULT_FORMAT	SND_PCM_FORMAT_S16
#define DEFAULT_MAX_FRM_SIZE (2048)
#define DEFAULT_CHANNEL_NUMBER 2
#define DEFAULT_BYTES_PER_SAMPLE 2//16bit = 2 bytes
#define DEFAULT_AEC_OUT_CHN_COUNT 1//AEC with 2 chn input, and 1 chn out
#define MAX_BYTES_PER_SAMPLE  4 //32bit = 4 bytes
#define BASIC_BUFFER_DEPTH 4
#define MAX_BUFFERING_DEPTH  300  /* #define CVI_MAX_AUDIO_FRAME_NUM		300 */
#define MAX_BUFFER_SETTING	(1 * 1024 * 1024)
#define MAX_BUFFER_FRAME_COUNT 300
#define MAX_AAC_DEC_INPUT_SIZE_BYTES  6144
//for ain_getframe / aout_send frame time mode usage

//------patch flag definition
#define ACODEC_ADC	"/dev/cvitekaadc"
#define ACODEC_DAC	"/dev/cvitekadac"

#define PCM_DEVICE_CARD0 "plughw:0,0"
#define PCM_DEVICE_CARD1 "plughw:1,0"
#define PCM_DEVICE_CARD2 "plughw:2,0"
#define pcm_external "plughw:0,0"
#define PCM_STOP_THRESHOD (2147483647)

/* global variable */
typedef unsigned long snd_pcm_uframes_t;
typedef long snd_pcm_sframes_t;
typedef struct _ST_SAVEFILE_INFO {
	AUDIO_SAVE_FILE_INFO_S	stFileInfo;
	CVI_BOOL bSaveing; /* File saving to the required size */
	CVI_BOOL bTrigger; /* User Toggle the save file  */
	CVI_BOOL bInit; /* variable been CreateChn */
	FILE *fd_save_file; /* Target file pointer */
} ST_SAVEFILE_INFO;



typedef struct _AinProcParam {
	CVI_S32  frames_per_period;
	CVI_S32  bytes_per_period;
	CVI_S32  bytes_per_frame;
	CVI_S32 bytes_per_sample;
	CVI_S32  buffersize;
	CVI_S32  buffering_frame;
	CVI_BOOL bUpdated;
	CVI_BOOL bInterleave;
} ST_AINPROC_PARAM;

typedef struct _AoutProcParam {
	snd_pcm_uframes_t frames_per_period;
	CVI_S32  bytes_per_period;
	CVI_S32  bytes_per_frame;
	CVI_S32 bytes_per_sample;
	CVI_S32  buffersize;
	CVI_S32  buffering_frame;
	CVI_BOOL bUpdated;
	CVI_BOOL bInterleave;
	CVI_S32 s32BitDepth_PerSample;
} ST_AOUTPROC_PARAM;

typedef struct _CodecParam {
	ACODEC_VOL_CTRL vol_ctrl;
	CVI_BOOL bMute;
	AUDIO_FADE_S stFade;
} ST_CODEC_CONFIG;

typedef struct _AudInnerStatus {
	CVI_S32 AiIdxNow;
	CVI_S32 AoIdxNow;
} AudInnerStatus;

#define PCM_IO_IN 0
#define PCM_IO_OUT 1
typedef struct pcm_config   st_pcm_config;
typedef struct pcm  st_pcm;

typedef struct _TINYALSA_CONFIG {
	st_pcm_config tinyconfig;
	CVI_S32 s32cardid;
	st_pcm *pcm;
	CVI_S32  periodbytes;
	char  *pPcmReadBuf;
} ST_TINYALSA_CONFIG;

typedef enum _E_CVIAUDIO_MEM_MODE {
	E_CVIAUDIO_MEM_USER_MODE = 0,
	E_CVIAUDIO_MEM_KERNEL_MODE = 1,
	E_CVIAUDIO_MEM_INVALID
} E_CVIAUDIO_MEM_MODE;


extern ST_AOUTPROC_PARAM gstAoutProcParam;
extern AudInnerStatus gstAudStatus;
extern ST_CODEC_CONFIG gstCodecConfig;

//for cli
//extern ST_AI_INSTANCE gstAiInstance[CVI_MAX_AI_DEVICE_ID_NUM];
extern ST_AENC_INSTANCE gstAencInstance[AENC_MAX_CHN_NUM];
extern unsigned int audoutThrstatus;
extern unsigned int audoutThrcount;
extern unsigned int audinThrstatus;
extern unsigned int audinThrcount;


//Decoder cycle buffer(decode -> Aout)
extern void *gpstCircleBuffer;

extern void _cvi_audio_getDbgMask(void);
extern void audio_register_cmd(void);

#endif //__CVI_AUDIO_INTERFACE_TINYALSA_H__
