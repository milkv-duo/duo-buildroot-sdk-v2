/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_audio_sonic.h
 * Description: This header is not for release, only declare internally for sonic lib
 * sonic lib is used to play out audio with different speed
 */
#ifndef __CVI_AUDIO_SONIC_H__
#define __CVI_AUDIO_SONIC_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <linux/cvi_type.h>
#include "cvi_comm_aio.h"
#define CVIAUDIO_SPEED_PLAY_LIB "libsonic.so"



struct sonicStreamStruct {
#ifdef SONIC_SPECTROGRAM
	sonicSpectrogram spectrogram;
#endif  /* SONIC_SPECTROGRAM */
	short *inputBuffer;
	short *outputBuffer;
	short *pitchBuffer;
	short *downSampleBuffer;
	float speed;
	float volume;
	float pitch;
	float rate;
	int oldRatePosition;
	int newRatePosition;
	int useChordPitch;
	int quality;
	int numChannels;
	int inputBufferSize;
	int pitchBufferSize;
	int outputBufferSize;
	int numInputSamples;
	int numOutputSamples;
	int numPitchSamples;
	int minPeriod;
	int maxPeriod;
	int maxRequired;
	int remainingInputToCopy;
	int sampleRate;
	int prevPeriod;
	int prevMinDiff;
};

struct sonicStreamStruct;
typedef struct sonicStreamStruct *sonicStream;

typedef void* (*funcp_sonicCreateStream)(int sampleRate, int numChannels);
typedef void (*funcp_sonicDestroyStream)(sonicStream stream);
typedef int (*funcp_sonicWriteFloatToStream)(sonicStream stream, float *samples, int numSamples);
typedef int (*funcp_sonicWriteShortToStream)(sonicStream stream, short *samples, int numSamples);
typedef int (*funcp_sonicWriteUnsignedCharToStream)(sonicStream stream, unsigned char *samples,
						int numSamples);
typedef int (*funcp_sonicReadFloatFromStream)(sonicStream stream, float *samples,
						int maxSamples);
typedef int (*funcp_sonicReadShortFromStream)(sonicStream stream, short *samples,
						int maxSamples);
typedef int (*funcp_sonicReadUnsignedCharFromStream)(sonicStream stream, unsigned char *samples,
						int maxSamples);
typedef int (*funcp_sonicFlushStream)(sonicStream stream);
typedef int (*funcp_sonicSamplesAvailable)(sonicStream stream);
typedef float (*funcp_sonicGetSpeed)(sonicStream stream);
typedef void (*funcp_sonicSetSpeed)(sonicStream stream, float speed);
typedef float (*funcp_sonicGetPitch)(sonicStream stream);
typedef void (*funcp_sonicSetPitch)(sonicStream stream, float pitch);
typedef float (*funcp_sonicGetRate)(sonicStream stream);
typedef void (*funcp_sonicSetRate)(sonicStream stream, float rate);
typedef float (*funcp_sonicGetVolume)(sonicStream stream);
typedef void (*funcp_sonicSetVolume)(sonicStream stream, float volume);
typedef int (*funcp_sonicGetChordPitch)(sonicStream stream);
typedef void (*funcp_sonicSetChordPitch)(sonicStream stream, int useChordPitch);
typedef int (*funcp_sonicGetQuality)(sonicStream stream);
typedef void (*funcp_sonicSetQuality)(sonicStream stream, int quality);
typedef int (*funcp_sonicGetSampleRate)(sonicStream stream);
typedef void (*funcp_sonicSetSampleRate)(sonicStream stream, int sampleRate);
typedef int (*funcp_sonicGetNumChannels)(sonicStream stream);
typedef void (*funcp_sonicSetNumChannels)(sonicStream stream, int numChannels);
typedef int (*funcp_sonicChangeFloatSpeed)(float *samples, int numSamples, float speed,
				float pitch, float rate, float volume,
				int useChordPitch, int sampleRate, int numChannels);
typedef int (*funcp_sonicChangeShortSpeed)(short *samples, int numSamples, float speed,
				float pitch, float rate, float volume,
				int useChordPitch, int sampleRate, int numChannels);



typedef struct _ST_CVIAUDIO_SPEED_CTRL_FUNC {
	//saving dlopen handle, only need to dlopen once. should destroy when cviaudio deinit
	CVI_VOID *pLibHandle;
	funcp_sonicCreateStream	psonicCreateStream;
	funcp_sonicDestroyStream	psonicDestroyStream;
	funcp_sonicWriteFloatToStream	psonicWriteFloatToStream;
	funcp_sonicWriteShortToStream	psonicWriteShortToStream;
	funcp_sonicWriteUnsignedCharToStream	psonicWriteUnsignedCharToStream;
	funcp_sonicReadFloatFromStream	psonicReadFloatFromStream;
	funcp_sonicReadShortFromStream	psonicReadShortFromStream;
	funcp_sonicReadUnsignedCharFromStream	psonicReadUnsignedCharFromStream;
	funcp_sonicFlushStream	psonicFlushStream;
	funcp_sonicSamplesAvailable	psonicSamplesAvailable;
	funcp_sonicGetSpeed	psonicGetSpeed;
	funcp_sonicSetSpeed	psonicSetSpeed;
	funcp_sonicGetPitch	psonicGetPitch;
	funcp_sonicSetPitch	psonicSetPitch;
	funcp_sonicGetRate	psonicGetRate;
	funcp_sonicSetRate	psonicSetRate;
	funcp_sonicGetVolume	psonicGetVolume;
	funcp_sonicSetVolume	psonicSetVolume;
	funcp_sonicGetChordPitch	psonicGetChordPitch;
	funcp_sonicSetChordPitch	psonicSetChordPitch;
	funcp_sonicGetQuality		psonicGetQuality;
	funcp_sonicSetQuality		psonicSetQuality;
	funcp_sonicGetSampleRate	psonicGetSampleRate;
	funcp_sonicSetSampleRate  psonicSetSampleRate;
	funcp_sonicGetNumChannels psonicGetNumChannels;
	funcp_sonicSetNumChannels psonicSetNumChannels;
	funcp_sonicChangeFloatSpeed psonicChangeFloatSpeed;
	funcp_sonicChangeShortSpeed psonicChangeShortSpeed;
} ST_CVIAUDIO_SPEED_CTRL_FUNC;

typedef struct _ST_CVIAUDIO_SPEEDPLAY_USR_CONFIG {
	int sampleRate;
	int channels;
	float speed;
	float pitch;
	float rate;
	float volume;
} ST_CVIAO_SPEEDPLAY_CONFIG;

//API declaration here ------------------------->[start]
CVI_S32 CVI_AO_EnableSpeedPlay(AUDIO_DEV AoDevId, ST_CVIAO_SPEEDPLAY_CONFIG  AoSpeedCfg);
CVI_S32 CVI_AO_DisableSpeedPlay(AUDIO_DEV AoDevId);
CVI_S32 CVI_AO_DestroySpeedPlay(AUDIO_DEV AoDevId);
//below API customer will not be able to see
//at least 2048 pts to do the speed play ...need a cycle buffer
CVI_S32 CVI_AO_SpeedPlayProc(AUDIO_DEV AoDevId,
				short *input_buffer,
				CVI_S32 s32InputBytes,
				short **output_buffer,
				CVI_S32 s32OutputBufferSizeBytes);
//API declaration here ------------------------->[end]

#ifdef __cplusplus
}
#endif
#endif

