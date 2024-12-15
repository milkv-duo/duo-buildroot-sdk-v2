/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_aud_internal.h
 * Description: internal audio function api
 */

/**					*/
/* @fi*/
/* Simple audio converter from transcode_aac*/


#ifndef __CVI_AUD_INTERNAL_H__
#define __CVI_AUD_INTERNAL_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
/*tiny_alsa usage */
#include "asoundlib.h"
/* cvi common headers */
#include "cvi_audio_arch.h"
#include "cvi_comm_aenc.h"
#include "cvi_comm_adec.h"
#include "cvi_comm_aio.h"
#include "cvi_transcode_interface.h"
#include "cvi_audio_common.h"

/* resample usage */
typedef struct _RES_INFO {
	AUDIO_BIT_WIDTH_E src_sample_fmt;
	AUDIO_BIT_WIDTH_E dst_sample_fmt;
	int src_rate;
	int dst_rate;
	int src_ch_layout;
	int dst_ch_layout;
} ST_RES_INFO;

int set_resample_info(ST_RES_INFO res_info);
ST_RES_INFO get_resample_info(void);

/* for saving downlink resample information */
int set_resample_info_aout(ST_RES_INFO res_info);
ST_RES_INFO get_resample_info_aout(void);


/* audio in usage */

/* for DC filter -----------------------------------------------------start*/
/* DC Filter */
typedef struct {
	CVI_FLOAT notch_radius;
	CVI_FLOAT notch_mem[2];    /* assign two elements per channel */
} stDCFilterState;

/* for DC filter -------------------------------------------------------end*/



#ifdef __cplusplus
}
#endif
#endif



