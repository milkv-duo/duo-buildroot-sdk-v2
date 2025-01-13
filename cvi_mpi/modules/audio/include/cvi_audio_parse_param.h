#ifndef __CVI_AUDIO_PARSE_PARAM_H__
#define __CVI_AUDIO_PARSE_PARAM_H__

//#include "sample_comm.h"
#if defined(__CV181X__) || defined(__CV180X__)
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <linux/cvi_type.h>
#include <linux/cvi_common.h>
#include <string.h>
#else
#include "cvi_common.h"
#endif

#define CVIAUDIO_PARSE_FILE_LENGTH 128
#define CVIAUDIO_PARSE_NONE_SET (-1)
typedef struct {
	int sample_rate;
	int channel;
	int preiod_size;
	int codec;
	//PAYLOAD_TYPE_E eType;
	bool bVqeOn;
	bool bAecOn;
	bool bResmp;
	int bind_mode;
	int record_time;
	//use for long option
	char filename[CVIAUDIO_PARSE_FILE_LENGTH];
	int ain_volume;
	int aout_volume;
} stAudioPara;

typedef struct _AudioUnitTestCfg {
	CVI_S32 channels;
	CVI_S32 Time_in_second;
	CVI_S32 sample_rate;
	CVI_CHAR  format[64]; //pcm/ g711//g726...
	CVI_S32 period_size;
	CVI_S32 bitdepth;
	CVI_S32 unit_test;
	CVI_S32 s32TestMode;
	CVI_CHAR filename[256];
	CVI_BOOL bOptCfg;
} ST_AudioUnitTestCfg;

typedef struct _cvi_wavHEADER {
	/* RIFF string */
	CVI_U8 riff[4];
	// overall size of file in bytes
	CVI_U32 overall_size;
	// WAVE string
	CVI_U8 wave[4];
	// fmt string with trailing null char
	CVI_U8 fmt_chunk_marker[4];
	// length of the format data
	CVI_U32 length_of_fmt;
	// format type. 1-PCM, 3- IEEE float, 6 - 8bit A law, 7 - 8bit mu law
	CVI_U16 format_type;
	// no.of channels
	CVI_U16 channels;
	// sampling rate (blocks per second)
	CVI_U32 sample_rate;
	// SampleRate * NumChannels * BitsPerSample/8
	CVI_U32 byterate;
	// NumChannels * BitsPerSample/8
	CVI_U16 block_align;
	// bits per sample, 8- 8bits, 16- 16 bits etc
	CVI_U16 bits_per_sample;
	// DATA string or FLLR string
	CVI_U8 data_chunk_header[4];
	// NumSamples * NumChannels * BitsPerSample/8 - size of the next chunk that will be read
	CVI_U32 data_size;
} ST_CVI_WAV_HEADER;

int audio_parse(int argc, char **argv);
int _get_parseval(stAudioPara stAudparam);
int _parsing_audio_status(void);
int _parsing_request(char *printout, int default_val);
void *_parsing_request_name(char *printout, int default_val);

#endif


