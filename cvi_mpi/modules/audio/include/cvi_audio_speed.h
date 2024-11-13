#ifndef __CVI_AUDIO_SPEED_H__
#define __CVI_AUDIO_SPEED_H__

#ifdef __cplusplus
extern "C" {
#endif

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
#include "cvi_type.h"
#include "cvi_audio.h"
#include "cvi_aud_internal_tinyalsa.h"
#include "cvi_transcode_interface.h"
#include "cvi_audio.h"
#include "acodec.h" /* for ioctl control */
#include "asoundlib.h"
#include "cvi_audio_vqe.h"
#include "cyclebuffer.h"
#include "cvi_audio_sonic.h"




#define CVIAUDIO_AO_SPEEDOUT_BUF_SIZE  (1024*5)
typedef struct _ST_CVIAUDIO_SPEED_PLAY_INSTANCE {
	CVI_BOOL	bEnable;
	sonicStream	stream;
	cycleBuffer *ao_speedfifo;
	short *before_buffer;
	short *speedout_buffer;
	int speedout_buffersize;
	ST_CVIAO_SPEEDPLAY_CONFIG cfg;

} ST_CVIAUDIO_SPEED_PLAY_INSTANCE;
#define DEFAULT_AO_SPEED_PROCESS_UNIT 2048
#define DEFAULT_AO_SPEED_PROCESS_BUFFER (2048*2)
#define AO_SPEED_PROC_THRESHOLD_BYTES (2048) //this value can be tuned, it will effect the delay effect
extern ST_CVIAUDIO_SPEED_PLAY_INSTANCE gstAoSpeedInstance[CVI_MAX_AO_DEVICE_ID_NUM];

#ifdef __cplusplus
}
#endif

#endif //__CVI_AUDIO_SPEED_H__
