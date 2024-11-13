/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: minimad.c,v 1.4 2004/01/23 09:41:32 rob Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
//reference code1:https://blog.csdn.net/gepanqiang3020/article/details/73695483
//reference code2:https://chromium.googlesource.com/vcbox/third_party/libsox/+/d00047a1b38ac5f74d4b18ac4d843f0c73e58ff3/src/src/mp3.c

//internal header
#include "cvi_mp3_decode.h"
#include "mad.h"

#define EXAMPLE_FILE_MODE 0//for debug, the demo program from libmad
#define __CVIAUDIO_MP3_DEC_VERSION__  "cviaudio_mp3_20210817"

//error code --------start
/* using a NULL point */
#ifndef CVI_ERR_ADEC_NULL_PTR
#define CVI_ERR_ADEC_NULL_PTR          0xA3000006
#endif
//error code --------stop
#define MP3_UNUSED_REF(X)  ((X) = (X))
//#define CVI_MP3_BUF_SIZE (1024 * 6)
#define CVI_MP3_BUF_SIZE (1024 * 10)
#define CVI_PCM_OUT_BUFF_SIZE (1024 * 5)
#define CVI_MP3_FRAME_SIZE 2881
#define CVI_MP3_DROP_THRESHOLD (CVI_MP3_FRAME_SIZE * 2)

#ifndef MP3_SAFE_FREE_BUF
#define MP3_SAFE_FREE_BUF(OBJ) {if (NULL != OBJ) {free(OBJ); OBJ = NULL; } }
#endif

typedef struct _st_mp3_decoder_handler {
	//for buffer (frame by frame decode)
	unsigned char *mp3_inputbuffer;
	size_t mp3_inputbytes;
	size_t mp3_remainbytes;
	size_t mp3_buffersize;
	int total_frame_cnt;
	char *pcm_outbuffer;//TODO:remove
	unsigned long pcm_outbytes;

	//for libmad
	struct mad_stream stream;
	struct mad_frame frame;
	struct mad_synth synth;
	mad_timer_t             timer;
	//for libmad function pointer
	int (*mad_stream_sync)(struct mad_stream *stream);
	void (*mad_stream_buffer)(struct mad_stream *,
		unsigned char const *, unsigned long);
	void (*mad_stream_skip)(struct mad_stream *, unsigned long);
	void (*mad_stream_init)(struct mad_stream *);
	void (*mad_frame_init)(struct mad_frame *);
	void (*mad_synth_init)(struct mad_synth *);
	int (*mad_frame_decode)(struct mad_frame *, struct mad_stream *);
	void (*mad_timer_add)(mad_timer_t *, mad_timer_t);
	void (*mad_synth_frame)(struct mad_synth *, struct mad_frame const *);
	void (*mad_frame_finish)(struct mad_frame *);
	void (*mad_stream_finish)(struct mad_stream *);
	unsigned long (*mad_bit_read)(struct mad_bitptr *, unsigned int);
	int (*mad_header_decode)(struct mad_header *, struct mad_stream *);
	void (*mad_header_init)(struct mad_header *);
	signed long (*mad_timer_count)(mad_timer_t, enum mad_units);
	void (*mad_timer_multiply)(mad_timer_t *, signed long);
	char const *(*mad_stream_errorstr)(struct mad_stream const *);
	Decode_Cb pDecode_Cb;
	ST_MP3_DEC_INFO  stMp3DecInfo;
} st_mp3_dec_handler;
/*
 * This is perhaps the simplest example use of the MAD high-level API.
 * Standard input is mapped into memory via mmap(), then the high-level API
 * is invoked with three callbacks: input, output, and error. The output
 * callback converts MAD's high-resolution PCM samples to 16 bits, then
 * writes them to standard output in little-endian, stereo-interleaved
 * format.
 */

#if EXAMPLE_FILE_MODE
static int decode(unsigned char const *, unsigned long);
#endif

#ifdef TEST_IN_DECODE_UNIT
FILE *pfd_out = NULL;

static int cvimp3_decode_usage(void)
{
	printf("============================\n");
	printf("cvimp3 usage\n");
	printf("cvimp3 $(var1).mp3\n");
	printf("create mp3_output.pcm file after decoded complete\n");
	printf("============================\n");
	return 0;
}


//callback function should exist in sample code
static int _mp3_dec_callback_forApp(void *inst, ST_MP3_DEC_INFO *pMp3_DecInfo, char *pBuff, int size)
{
	if (inst == NULL) {
		printf("[v]Null pt [%s][%d]\n", __func__, __LINE__);
		printf("Null pt in callback function force return\n");
		return -1;
	}

	if (pMp3_DecInfo == NULL) {
		printf("[v]Null pt [%s][%d]\n", __func__, __LINE__);
		printf("Null pt in callback pMp3_DecInfo force return\n");
		return -1;
	}

	if (pBuff == NULL) {
		printf("[v]Null pt [%s][%d]\n", __func__, __LINE__);
		printf("Null pt in callback pBuff force return\n");
		return -1;
	}

	printf("========================================\n");
	printf("[channel]:[%d]\n", pMp3_DecInfo->channel_num);
	printf("[sample rate]:[%d]\n", pMp3_DecInfo->sample_rate);
	printf("[bit rate]:[%d]\n", pMp3_DecInfo->bit_rate);
	printf("[frame cnt]:[%d]\n", pMp3_DecInfo->frame_cnt);
	printf("[callback state] [0x%x]\n", pMp3_DecInfo->cbState);
	printf("[size in bytes]:[%d]\n", size);
	printf("=========================================\n");
	//save the data or play out to AO
#if 1
	FILE *fp_local;

	fp_local = fopen("mp3_dec_cbDump.pcm", "ab+");
	fwrite(pBuff, 1, size, fp_local);
	fclose(fp_local);
#endif
	return 0;
}

#endif

/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}


static CVI_S32 _update_handle_bufferinfo(CVI_VOID *inst)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	st_mp3_dec_handler *pmp3_dec_handler;

	if (inst == NULL) {
		printf("[error]Null pt [%s][%d]\n", __func__, __LINE__);
		return CVI_ERR_ADEC_NULL_PTR;
	} else
		pmp3_dec_handler = (st_mp3_dec_handler *)inst;

	//handle the buffer
	if (pmp3_dec_handler->stream.error == MAD_ERROR_BUFLEN) {
		//usage for debug
		//printf("_update_handle_bufferinfo: Required more data\n");
	}

	//calculate the remain bytes
	if (pmp3_dec_handler->stream.next_frame != NULL) {
		pmp3_dec_handler->mp3_remainbytes =
			pmp3_dec_handler->stream.bufend - pmp3_dec_handler->stream.next_frame;

	} else {
		if (pmp3_dec_handler->pcm_outbytes == 0) {
			//did not decode out any frame
			printf("[v]did not get any frame this round as input size[%d]\n",
				(int)pmp3_dec_handler->mp3_inputbytes);
			pmp3_dec_handler->mp3_remainbytes =
				pmp3_dec_handler->mp3_inputbytes;
		}
	}
	if (pmp3_dec_handler->mp3_remainbytes > ((pmp3_dec_handler->mp3_buffersize * 2) / 3)) {
		//force flush the input buffer
		printf("XXXXXXXXXXX[%s][%d]too many unsolved dataXXXXXXXXXXX\n", __func__, __LINE__);
		//TODO: FIFO strategy to push out old data ?
	}
	#if 0
	memmove(pmp3_dec_handler->mp3_inputbuffer,
		pmp3_dec_handler->stream.bufend - pmp3_dec_handler->mp3_remainbytes,
		pmp3_dec_handler->mp3_remainbytes);
	#endif
	memmove(pmp3_dec_handler->mp3_inputbuffer,
		pmp3_dec_handler->stream.next_frame,
		pmp3_dec_handler->mp3_remainbytes);
	//reset the indicator
	//reset the pcm_outputbytes
	//reset the mp3_inputbytes
	pmp3_dec_handler->pcm_outbytes = 0;
	pmp3_dec_handler->mp3_inputbytes = 0;

	return s32Ret;

}

static CVI_S32 _update_callback_data(CVI_VOID *inst,
					int nchannels,
					int samplerate,
					int bitrate,
					int frame_cnt,
					int totalbytes)
{

	st_mp3_dec_handler *pmp3_dec_handler;
	int cb_state = 0;

	if (inst != NULL)
		pmp3_dec_handler = (st_mp3_dec_handler *)inst;
	else {
		printf("[error]Null pt [%s][%d]\n", __func__, __LINE__);
		return CVI_ERR_ADEC_NULL_PTR;
	}

	if (pmp3_dec_handler->pDecode_Cb != NULL) {
		//step 1:update return state
		if (pmp3_dec_handler->stMp3DecInfo.channel_num == 0 &&
			pmp3_dec_handler->stMp3DecInfo.sample_rate == 0 &&
			pmp3_dec_handler->stMp3DecInfo.bit_rate == 0) {

			cb_state = CVI_MP3_DEC_CB_STATUS_GET_FIRST_INFO;

		} else {
			if (pmp3_dec_handler->stMp3DecInfo.sample_rate != samplerate)
				cb_state = CVI_MP3_DEC_CB_STATUS_SMP_RATE_CHG;
			else if (pmp3_dec_handler->stMp3DecInfo.bit_rate != bitrate)
				cb_state = CVI_MP3_DEC_CB_STATUS_BIT_RATE_CHG;
			else if (pmp3_dec_handler->stMp3DecInfo.channel_num != nchannels)
				cb_state = CVI_MP3_DEC_CB_STATUS_INFO_CHG;
			else
				cb_state = CVI_MP3_DEC_CB_STATUS_STABLE;
		}
		pmp3_dec_handler->stMp3DecInfo.cbState = cb_state;

		//step 2:copy data
		pmp3_dec_handler->stMp3DecInfo.channel_num = nchannels;
		pmp3_dec_handler->stMp3DecInfo.sample_rate = samplerate;
		pmp3_dec_handler->stMp3DecInfo.bit_rate = bitrate;
		pmp3_dec_handler->stMp3DecInfo.frame_cnt = frame_cnt;
		if (totalbytes == 0) {
			pmp3_dec_handler->pDecode_Cb(pmp3_dec_handler,
					&pmp3_dec_handler->stMp3DecInfo,
					pmp3_dec_handler->pcm_outbuffer,
					0);
		} else {
			if (pmp3_dec_handler->stMp3DecInfo.channel_num == 1)
				totalbytes = totalbytes / 2;//only single channel
			pmp3_dec_handler->pDecode_Cb(pmp3_dec_handler,
					&pmp3_dec_handler->stMp3DecInfo,
					pmp3_dec_handler->pcm_outbuffer,
					totalbytes);
		}
	} else {
		//no callback function, do nothing....
	}

	return 0;
}

static CVI_S32 _update_decodeout_buffer(CVI_VOID *inst,
					unsigned char *pOutputBUf,
					struct mad_pcm *pcm)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	st_mp3_dec_handler *pmp3_dec_handler;
	unsigned  char *pOutputIndex;
	//from decode.c sample
	unsigned int nchannels, nsamples, n_totalsamples;
	mad_fixed_t const *left_ch, *right_ch;

	if (inst != NULL)
		pmp3_dec_handler = (st_mp3_dec_handler *)inst;
	else {
		printf("[error]Null pt [%s][%d]\n", __func__, __LINE__);
		return CVI_ERR_ADEC_NULL_PTR;
	}

	nchannels = pcm->channels;
	n_totalsamples = nsamples  = pcm->length;
	left_ch   = pcm->samples[0];
	right_ch  = pcm->samples[1];
	pOutputIndex = pOutputBUf + pmp3_dec_handler->pcm_outbytes;

	while (nsamples--) {
		signed int sample;

		sample = scale(*left_ch++);
		*(pOutputIndex++) = sample >> 0;
		*(pOutputIndex++) = sample >> 8;
		if (nchannels == 2) {
			sample = scale(*right_ch++);
			*(pOutputIndex++) = sample >> 0;
			*(pOutputIndex++) = sample >> 8;

		}
	}
	//fwrite the output data before concatenate to larger buffer

	#if 0//EXAMPLE_FILE_MODE
	pOutputIndex = pOutputBUf + pmp3_dec_handler->pcm_outbytes;
	fwrite(pOutputIndex, 1, (n_totalsamples * 4), pfd_out);
	#endif

	#if 0//send data out using callback function
	if (pmp3_dec_handler->pDecode_Cb != NULL) {
		pmp3_dec_handler->stMp3DecInfo.channel_num = nchannels;
		pmp3_dec_handler->stMp3DecInfo.sample_rate = pcm->samplerate;
		pmp3_dec_handler->stMp3DecInfo.bit_rate = pmp3_dec_handler->frame.header.bitrate;
		pmp3_dec_handler->stMp3DecInfo.frame_cnt = pmp3_dec_handler->total_frame_cnt;
		if ((n_totalsamples * 4) > CVI_PCM_OUT_BUFF_SIZE) {
			printf("[v][error] dec out size[%d] > pcm_output buffersize[%d]\n",
				(n_totalsamples * 4), CVI_PCM_OUT_BUFF_SIZE);
			printf("[v][error][%s][%d]\n", __func__, __LINE__);
			pmp3_dec_handler->pDecode_Cb(pmp3_dec_handler,
					&pmp3_dec_handler->stMp3DecInfo,
					pmp3_dec_handler->pcm_outbuffer,
					0);
		} else {
			pOutputIndex = pOutputBUf + pmp3_dec_handler->pcm_outbytes;
			memcpy(pmp3_dec_handler->pcm_outbuffer, pOutputIndex, (n_totalsamples * 4));
			pmp3_dec_handler->pDecode_Cb(pmp3_dec_handler,
					&pmp3_dec_handler->stMp3DecInfo,
					pmp3_dec_handler->pcm_outbuffer,
					(n_totalsamples * 4));
		}
	}
	#else
	if ((n_totalsamples * 4) > CVI_PCM_OUT_BUFF_SIZE) {
		printf("[v][error] dec out size[%d] > pcm_output buffersize[%d]\n",
				(n_totalsamples * 4), CVI_PCM_OUT_BUFF_SIZE);
		printf("[v][error]Not Callback data[%s][%d]\n", __func__, __LINE__);
		_update_callback_data(pmp3_dec_handler,
				nchannels,
				pcm->samplerate,
				pmp3_dec_handler->frame.header.bitrate,
				pmp3_dec_handler->total_frame_cnt,
				0);
	} else {
		pOutputIndex = pOutputBUf + pmp3_dec_handler->pcm_outbytes;
		memcpy(pmp3_dec_handler->pcm_outbuffer, pOutputIndex, (n_totalsamples * 4));
		_update_callback_data(pmp3_dec_handler,
				nchannels,
				pcm->samplerate,
				pmp3_dec_handler->frame.header.bitrate,
				pmp3_dec_handler->total_frame_cnt,
				(n_totalsamples * 4));
	}
	#endif
	//update pcm_outputbytes
	if (nchannels == 1)
		pmp3_dec_handler->pcm_outbytes += (n_totalsamples * 2);
	else
		pmp3_dec_handler->pcm_outbytes += (n_totalsamples * 4);
	//printf("current decout pcm size bytes this[%d] totallu[%lu]\n",
	//	(n_totalsamples * 4), pmp3_dec_handler->pcm_outbytes);

	return s32Ret;

}


CVI_S32 CVI_MP3_Decode(CVI_VOID *inst, CVI_VOID *pInputBuf,
	CVI_VOID *pOutputBUf, CVI_S32 s32InputLen, CVI_S32 *s32OutLen)
{
	MP3_UNUSED_REF(inst);
	MP3_UNUSED_REF(pInputBuf);
	MP3_UNUSED_REF(pOutputBUf);
	MP3_UNUSED_REF(s32InputLen);
	MP3_UNUSED_REF(s32OutLen);
	//parse the handler

	CVI_S32 s32Ret = CVI_SUCCESS;
	st_mp3_dec_handler *pmp3_dec_handler;
	CVI_S32 s32RecoveryTryCnt = 0;
	CVI_S32 s32TotalSizeBytes = s32InputLen;
	struct mad_synth *synth;

	if (inst == NULL) {
		printf("[v]Null pt [%s][%d]\n", __func__, __LINE__);
		return CVI_ERR_ADEC_NULL_PTR;
	}

	pmp3_dec_handler = (st_mp3_dec_handler *)inst;

	//STEP 1:Read in the buffer
	//do the buffer handler
	if (s32InputLen > (CVI_S32)(pmp3_dec_handler->mp3_buffersize)) {
		printf("[error][%d] > [%d]\n", s32InputLen, (CVI_S32)pmp3_dec_handler->mp3_buffersize);
		printf("[error][%s][%d]size input explode!!!\n", __func__,  __LINE__);
		return CVI_FAILURE;
	}

	if (pmp3_dec_handler->mp3_remainbytes != 0) {
		if ((pmp3_dec_handler->mp3_remainbytes + s32InputLen) >
			pmp3_dec_handler->mp3_buffersize) {
			printf("[error]remain[%d] + input[%d] > bufsize[%d]",
				(CVI_S32)pmp3_dec_handler->mp3_remainbytes,
				s32InputLen,
				(CVI_S32)pmp3_dec_handler->mp3_buffersize);
			printf("force flush the buffer\n");
			memset(pmp3_dec_handler->mp3_inputbuffer, 0, pmp3_dec_handler->mp3_buffersize);
			memcpy(pmp3_dec_handler->mp3_inputbuffer, pInputBuf, s32InputLen);
			s32TotalSizeBytes = s32InputLen;
		} else {
			//pending with previous buffer
			memcpy((pmp3_dec_handler->mp3_inputbuffer + pmp3_dec_handler->mp3_remainbytes),
				pInputBuf,
				s32InputLen);
			s32TotalSizeBytes = pmp3_dec_handler->mp3_remainbytes + s32InputLen;
		}
	} else {
		printf("[v][%s][%d] inputlen[%d]\n", __func__, __LINE__, s32InputLen);
		memcpy(pmp3_dec_handler->mp3_inputbuffer, pInputBuf, s32InputLen);
		s32TotalSizeBytes = s32InputLen;
		printf("[v][%s][%d]\n", __func__, __LINE__);
	}
	pmp3_dec_handler->mp3_inputbytes = s32TotalSizeBytes;
	pmp3_dec_handler->mad_stream_buffer(&pmp3_dec_handler->stream,
			pmp3_dec_handler->mp3_inputbuffer, s32TotalSizeBytes);

	//STEP 2:Find and decode frame in buffer
	//TODO:
	//mad_frame_decode
	//return 0:success
	//return -1: failure , try recovery
	//return !=0 check error code, if mad_stream.error == MAD_ERROR_BUFLEN
	//no frame inside the input buffer
	s32RecoveryTryCnt = 0;
	do {
		if (pmp3_dec_handler->mad_frame_decode(&pmp3_dec_handler->frame,
						&pmp3_dec_handler->stream) != 0) {
			if (MAD_RECOVERABLE(pmp3_dec_handler->stream.error)) {

				if (s32RecoveryTryCnt < 2) {
					printf("go Retry\n");
					continue;
				} else {
					printf("Try recovery time out...go next round\n");
					break;
				}
				s32RecoveryTryCnt++;
			} else {
				if (pmp3_dec_handler->stream.error == MAD_ERROR_BUFLEN) {
					//need refill the buffer for more data
					//printf("refill the data request..require next round\n");

					break;

				} else {
					printf("unrecoverable frame level error (%s)\n",
						pmp3_dec_handler->mad_stream_errorstr(&pmp3_dec_handler->stream));
					break;
				}
			}
		} else {
			//decode frame success
			pmp3_dec_handler->total_frame_cnt++;
			//printf("[v][%s][%d] frm count[%d]\n",
			//	__func__, __LINE__, pmp3_dec_handler->total_frame_cnt);
			pmp3_dec_handler->mad_timer_add(&pmp3_dec_handler->timer,
						pmp3_dec_handler->frame.header.duration);
			pmp3_dec_handler->mad_synth_frame(&pmp3_dec_handler->synth,
						&pmp3_dec_handler->frame);
			//STEP3:update output buffer/accumalate the pcm_outbytes
			synth = &pmp3_dec_handler->synth;
			_update_decodeout_buffer(pmp3_dec_handler, pOutputBUf, &synth->pcm);
		}
	} while (1);
	//printf("get out while\n");

	// struct mad_pcm *pcm)
	//reset the pcm_outputbytes
	//reset the mp3_inputbytes
	//calculate the mp3_remainbytes
	*s32OutLen = pmp3_dec_handler->pcm_outbytes;
	s32Ret = _update_handle_bufferinfo(pmp3_dec_handler);

	return s32Ret;
}

CVI_VOID *CVI_MP3_Decode_Init(CVI_VOID *inst)
{
	MP3_UNUSED_REF(inst);
	st_mp3_dec_handler *pmp3_dec_handler;

	//init the buffer size
	pmp3_dec_handler = (st_mp3_dec_handler *)malloc(sizeof(st_mp3_dec_handler));
	memset(pmp3_dec_handler,0,sizeof(st_mp3_dec_handler));
	printf("version[%s]\n", __CVIAUDIO_MP3_DEC_VERSION__);


	//TODO: init the buffer
	pmp3_dec_handler->mp3_inputbuffer = (unsigned char *)malloc(CVI_MP3_BUF_SIZE);
	pmp3_dec_handler->pcm_outbuffer = (char *)malloc(CVI_PCM_OUT_BUFF_SIZE);
	pmp3_dec_handler->mp3_buffersize = CVI_MP3_BUF_SIZE;
	pmp3_dec_handler->mp3_remainbytes = 0;
	pmp3_dec_handler->mp3_inputbytes = 0;//each round
	pmp3_dec_handler->pcm_outbytes = 0;//each round
	pmp3_dec_handler->total_frame_cnt = 0;
	//init the struct
	pmp3_dec_handler->mad_stream_sync = mad_stream_sync;
	pmp3_dec_handler->mad_stream_buffer = mad_stream_buffer;
	pmp3_dec_handler->mad_stream_skip = mad_stream_skip;
	pmp3_dec_handler->mad_stream_init = mad_stream_init;
	pmp3_dec_handler->mad_frame_init = mad_frame_init;
	pmp3_dec_handler->mad_synth_init = mad_synth_init;
	pmp3_dec_handler->mad_frame_decode = mad_frame_decode;
	pmp3_dec_handler->mad_timer_add = mad_timer_add;
	pmp3_dec_handler->mad_synth_frame = mad_synth_frame;
	pmp3_dec_handler->mad_frame_finish = mad_frame_finish;
	pmp3_dec_handler->mad_stream_finish = mad_stream_finish;
	pmp3_dec_handler->mad_bit_read = mad_bit_read;
	pmp3_dec_handler->mad_header_decode = mad_header_decode;
	pmp3_dec_handler->mad_header_init = mad_header_init;
	pmp3_dec_handler->mad_timer_count = mad_timer_count;
	pmp3_dec_handler->mad_timer_multiply = mad_timer_multiply;
	pmp3_dec_handler->mad_stream_errorstr = mad_stream_errorstr;
	pmp3_dec_handler->pDecode_Cb = NULL;
	//init the value

	pmp3_dec_handler->mad_stream_init(&pmp3_dec_handler->stream);
	pmp3_dec_handler->mad_frame_init(&pmp3_dec_handler->frame);
	pmp3_dec_handler->mad_synth_init(&pmp3_dec_handler->synth);

	mad_stream_options(&pmp3_dec_handler->stream, 0);
	mad_timer_reset(&pmp3_dec_handler->timer);


	return (CVI_VOID *)pmp3_dec_handler;
}

CVI_S32 CVI_MP3_Decode_InstallCb(CVI_VOID *inst, Decode_Cb pCbFunc)
{
	if (inst == NULL) {
		printf("[error][%s][%d]\n", __func__, __LINE__);
		return CVI_ERR_ADEC_NULL_PTR;
	}

	st_mp3_dec_handler *pmp3_dec_handler;

	pmp3_dec_handler = (st_mp3_dec_handler *)inst;

	pmp3_dec_handler->pDecode_Cb = pCbFunc;
	//reset the mp3 dec info struct
	memset(&pmp3_dec_handler->stMp3DecInfo, 0, sizeof(ST_MP3_DEC_INFO));

	return CVI_SUCCESS;

}


CVI_S32 CVI_MP3_Decode_DeInit(CVI_VOID *inst)
{
	MP3_UNUSED_REF(inst);
	if (inst == NULL) {
		printf("[error][%s][%d]\n", __func__, __LINE__);
		return CVI_FAILURE;
	}

	st_mp3_dec_handler *pmp3_dec_handler;

	pmp3_dec_handler = (st_mp3_dec_handler *)inst;

	mad_synth_finish(&pmp3_dec_handler->synth);
	pmp3_dec_handler->mad_frame_finish(&pmp3_dec_handler->frame);
	pmp3_dec_handler->mad_stream_finish(&pmp3_dec_handler->stream);


	MP3_SAFE_FREE_BUF(pmp3_dec_handler->mp3_inputbuffer);
	MP3_SAFE_FREE_BUF(pmp3_dec_handler->pcm_outbuffer);
	MP3_SAFE_FREE_BUF(pmp3_dec_handler);
	return CVI_SUCCESS;
}


#ifdef TEST_IN_DECODE_UNIT
int main(int argc, char *argv[])
{
#define FILE_NAME_LEN	128
	printf("enter cvimp3 decoder exe\n");
	FILE *pfd_in = NULL;

	char *input_filename;
	char output_filename[FILE_NAME_LEN] = {0};
	unsigned char *input_buffer;
	unsigned char *output_buffer;
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32OutLen;

	if (argc < 2) {
		printf("Not enough input\n");
		cvimp3_decode_usage();
	}

	input_filename = argv[1];
	if (access(input_filename, 0) <  0) {
		printf("[Error]check file[%s] not exist!....error\n", input_filename);
		return (-1);
	}

	//step1:open file
	snprintf(output_filename, FILE_NAME_LEN, "%s.pcm", input_filename);
	pfd_in = fopen(input_filename, "rb");
	pfd_out = fopen(output_filename, "wb");
	fseek(pfd_in, 0, SEEK_END);
	int fsize = ftell(pfd_in);
	fseek(pfd_in, 0, SEEK_SET);
	printf("input file total size bytes[%d]\n", fsize);
	input_buffer = (unsigned char *)malloc(fsize);
	output_buffer = (unsigned char *)malloc(fsize * 6);
	printf("[v][v][%s][%d] total output buffer size[%d]\n", __func__, __LINE__, fsize);

	//step2:start init the libmad
	void *pMp3DecHandler;

	pMp3DecHandler = CVI_MP3_Decode_Init(NULL);
	if (pMp3DecHandler == NULL) {
		printf("[error][%s][%d]\n", __func__, __LINE__);
		return -1;
	}

	//optional:user call also get the data from call back function
	s32Ret = CVI_MP3_Decode_InstallCb(pMp3DecHandler, _mp3_dec_callback_forApp);

#if EXAMPLE_FILE_MODE
	if (fread(input_buffer,
			1,
			fsize, pfd_in) != (unsigned int)fsize) {
		printf("[Error][%s][%d] fread failure\n", __func__, __LINE__);
	}
	printf("test 1--->give it all\n");
	s32Ret = CVI_MP3_Decode((CVI_VOID *)pMp3DecHandler,
				(CVI_VOID *)input_buffer,
				(CVI_VOID *)output_buffer,
				(CVI_S32)fsize,
				(CVI_S32 *)&s32OutLen);

	if (s32Ret != CVI_SUCCESS)
		printf("[error][%s][%d]\n", __func__, __LINE__);

	printf("total output length\n");
	fwrite(output_buffer, 1, (s32OutLen), pfd_out);
#else
	int read_size = 0;
	while (1) {
		read_size = fread(input_buffer, 1, 1024, pfd_in);
		if (read_size != 1024) {
			printf("read file end ...break\n");
			break;
		}
		printf("test 2--->give it 1024\n");
		s32Ret = CVI_MP3_Decode((CVI_VOID *)pMp3DecHandler,
					(CVI_VOID *)input_buffer,
					(CVI_VOID *)output_buffer,
					(CVI_S32)1024,
					(CVI_S32 *)&s32OutLen);

		if (s32Ret != CVI_SUCCESS)
			printf("[error][%s][%d]\n", __func__, __LINE__);

		printf("output length s32OutLen[%d]\n", s32OutLen);
		fwrite(output_buffer, 1, (s32OutLen), pfd_out);
	}

#endif
	CVI_MP3_Decode_DeInit(pMp3DecHandler);


#if EXAMPLE_FILE_MODE
	decode(input_buffer, fsize);
#endif
	if (input_buffer != NULL)
		free(input_buffer);
	if (output_buffer != NULL)
		free(output_buffer);

	printf("Exit program!!!\n");
	return 0;

	//step3:do the decode

	//step4:write to file and deinit

#if EXAMPLE_FILE_MODE
  struct stat stat;
  void *fdm;

  if (argc != 1)
    return 1;

  if (fstat(STDIN_FILENO, &stat) == -1 ||
      stat.st_size == 0)
    return 2;

  fdm = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, STDIN_FILENO, 0);
  if (fdm == MAP_FAILED)
    return 3;

  decode(fdm, stat.st_size);

  if (munmap(fdm, stat.st_size) == -1)
    return 4;
#endif
  return 0;
}
#endif


/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */
#if EXAMPLE_FILE_MODE
static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples, n;
  mad_fixed_t const *left_ch, *right_ch;
  unsigned char Output[6912], *OutputPtr;
  int fmt = 0;
  int  wrote = 0;
  int  speed = 0;

	MP3_UNUSED_REF(speed);
	MP3_UNUSED_REF(wrote);
	MP3_UNUSED_REF(fmt);
	MP3_UNUSED_REF(data);
	MP3_UNUSED_REF(header);

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  n = nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  OutputPtr = Output;

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    //putchar((sample >> 0) & 0xff);
    //putchar((sample >> 8) & 0xff);
    *(OutputPtr++) = sample >> 0;
    *(OutputPtr++) = sample >> 8;

    if (nchannels == 2) {
      sample = scale(*right_ch++);
      //putchar((sample >> 0) & 0xff);
      //putchar((sample >> 8) & 0xff);
        *(OutputPtr++) = sample >> 0;
        *(OutputPtr++) = sample >> 8;
    }
  }
	n *= 4;
    OutputPtr = Output;
    while (n) {
    //wrote = write(sfd, OutputPtr, n);
    fwrite(OutputPtr, 1, n, pfd_out);
    OutputPtr += n;
    n -= n;
    }
    OutputPtr = Output;
  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */
static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = data;
	MP3_UNUSED_REF(stream);
	MP3_UNUSED_REF(frame);

  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

static
int decode(unsigned char const *start, unsigned long length)
{
  struct buffer buffer;
  struct mad_decoder decoder;
  int result;

  /* initialize our private message structure */

  buffer.start  = start;
  buffer.length = length;

  /* configure input, output, and error functions */

  mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);

  /* start decoding */

  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

  /* release the decoder */

  mad_decoder_finish(&decoder);

  return result;
}

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct buffer *buffer = data;
  printf("[v]input bufferlen[%d]\n", (int)buffer->length);
  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}
#endif