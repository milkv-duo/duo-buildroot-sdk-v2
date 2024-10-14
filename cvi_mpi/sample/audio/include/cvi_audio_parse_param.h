#ifndef __CVI_AUDIO_PARSE_PARAM_H__
#define __CVI_AUDIO_PARSE_PARAM_H__

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

int audio_parse(int argc, char **argv);
int _get_parseval(stAudioPara stAudparam);
int _parsing_audio_status(void);
int _parsing_request(char *printout, int default_val);

#endif


