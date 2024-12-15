#ifndef __CVI_AUDIO_LOADCHECK_H__
#define __CVI_AUDIO_LOADCHECK_H__

#ifdef __cplusplus
extern	"C"	{
#endif

#include <stdio.h>


void *CVI_AUDIO_LoadingCheck_Init(int samplerate,
				int channel_cnt,
				int measure_interval_sec, char *name);
int CVI_AUDIO_LoadingCheck_Begin(void *phandle);
int CVI_AUDIO_LoadingCheck_End(void *phandle, unsigned int bytes_proceed);
int CVI_AUDIO_LoadingCheck_DeInit(void *phandle);

#ifdef __cplusplus
}
#endif

#endif

