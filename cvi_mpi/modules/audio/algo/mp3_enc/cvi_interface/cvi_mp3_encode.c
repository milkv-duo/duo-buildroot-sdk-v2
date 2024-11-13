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
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//internal header
#include "cvi_mp3_encode.h"
#include "lame.h"

#define EXAMPLE_FILE_MODE 0
#define MP3ENC_UNUSED_REF(X)  ((X) = (X))

#define CVI_MP3_ENC_INPUT_BUFFER_SIZE (1152 * 6)
#define CVI_MP3_ENC_BASIC_INPUT_LEVEL (1152 * 2) //single channel required 1152*2 bytes
#define CVI_MP3_ENC_BASIC_INPUT_LEVEL_STEREO (1152 * 2 * 2)//dual channel required 1152*2*2 bytes
#define CVI_MP3_ENC_OUTPUT_BUFFER_SIZE (144 * 1024)
#ifndef CVI_ERR_AENC_NULL_PTR
#define CVI_ERR_AENC_NULL_PTR         0xA2000006
#endif

#ifdef TEST_IN_ENCODE_UNIT
static int _console_request(char *printout, int defalut_val);
#define _CONSOLE_REQ(X, Y) _console_request((char *)X, (int)Y)
#endif

#ifndef MP3_ENC_CHECK_NULL
#define MP3_ENC_CHECK_NULL(ptr) \
	do {                                                                                                           \
		if (!(ptr)) {                                                                                     \
			printf("func:%s,line:%d, NULL pointer\n", __func__, __LINE__);                                 \
			return CVI_ERR_AENC_NULL_PTR;                                                                            \
		}                                                                                                      \
	} while (0)
#endif

typedef struct _st_mp3_encoder_handler {
	//basic params from input
	int channels;
	int samplerate;
	int bitrate;
	int mode;
	int quality;//2,5,9  9:quality worst but fast, 2:quality good but slow
	int vbr_on;
	//for buffer (frame by frame encode)
	unsigned char *inputbuffer;
	int input_sizebytes;//total input size bytes each round
	char *enc_outputbuffer;
	unsigned char mp3buffer[LAME_MAXMP3BUFFER];
	unsigned long output_sizebytes; //total output size bytes each round
	int remain_sizebytes;
	Encode_Cb pEncode_Cb;
	ST_MP3_ENC_CBINFO  stMp3EncInfo;
	//lame basic structure/handler
	lame_global_flags *gfp;
	//lame function pointer
	lame_global_flags * (*lame_init)(void);
	int (*lame_set_in_samplerate)(lame_global_flags *, int);
	int (*lame_set_out_samplerate)(lame_global_flags *, int);
	int (*lame_set_num_channels)(lame_global_flags *, int);
	int (*lame_set_num_samples)(lame_global_flags *, unsigned long);
	int (*lame_set_brate)(lame_global_flags *, int);
	int (*lame_set_quality)(lame_global_flags *, int);
	int (*lame_set_VBR)(lame_global_flags *, vbr_mode);
	int (*lame_init_params)(lame_global_flags *);
	int (*lame_set_mode)(lame_global_flags *, MPEG_mode);
	int (*lame_encode_buffer_float)(
		lame_global_flags*  gfp,
		const float         pcm_l [],
		const float         pcm_r [],
		const int           nsamples,
		unsigned char*      mp3buf,
		const int           mp3buf_size);

	int (*lame_encode_buffer)(
		lame_global_flags*  gfp,
		const short int     buffer_l [],
		const short int     buffer_r [],
		const int           nsamples,
		unsigned char*      mp3buf,
		const int           mp3buf_size);

	int (*lame_encode_buffer_interleaved)(
		lame_global_flags*  gfp,
		short int           pcm[],
		int                 num_samples,
		unsigned char*      mp3buf,
		int                 mp3buf_size);
	int (*lame_encode_buffer_int)(
		lame_global_flags*  gfp,
		const int      buffer_l [],
		const int      buffer_r [],
		const int           nsamples,
		unsigned char*      mp3buf,
		const int           mp3buf_size);

} ST_MP3_EncHandler;


#ifdef TEST_IN_ENCODE_UNIT
static int _console_request(char *printout, int default_val)
{
	char s_option[128];
	int s32Ret = -1;

	printf("\e[0;32m=====================================\n");
	fflush(stdout);
	fflush(stdin);
	if (printout != NULL) {
		printf(printout);
		fgets(s_option, 10, stdin);
		if (s_option[0] == '\n') {
			s32Ret = default_val;
			printf("input default val[%d]\n", s32Ret);
		} else {
			s32Ret = atoi(s_option);
			printf("input [%d]\n", s32Ret);
		}
	} else
		printf("Error input type[%s][%d]\n", __func__, __LINE__);

	printf("\e[0;32m=====================================\n");
	fflush(stdin);
	return s32Ret;
}
#endif

#ifdef TEST_IN_ENCODE_UNIT
FILE *pfd_out;

static int cvimp3enc_encode_usage(void)
{
	printf("============================\n");
	printf("cvimp3enc  usage\n");
	printf("cvimp3enc $(var1).mp3\n");
	printf("create $(input).mp3 file after decoded complete\n");
	printf("============================\n");
	return 0;
}


//callback function should exist in sample code
static int _mp3_enc_callback_forApp(void *inst, char *pBuff, int size)
{
	MP3ENC_UNUSED_REF(inst);
	MP3ENC_UNUSED_REF(pBuff);
	MP3ENC_UNUSED_REF(size);
	return 0;
}

#endif

static int _check_lame_ret_value(int ret_bytes)
{
	int ret = 0;

	if(ret_bytes == -1) {
		printf("[Error]mp3buf was too small\n");
		ret = -1;
	} else if(ret_bytes == -2) {
		printf("[Error]There was a malloc problem\n");
		ret = -2;
	} else if(ret_bytes == -3) {
		printf("[Error]lame_init_params() not called\n");
		ret = -3;
	} else if(ret_bytes == -4) {
		printf("[Error]Psycho acoustic problems\n");
		ret = -4;
	}

	return ret;

}


CVI_S32 CVI_MP3_Encode(CVI_VOID *inst, CVI_VOID *pInputBuf,
	CVI_VOID *pOutputBUf, CVI_S32 s32InputLen, CVI_S32 *s32OutLen)
{
	MP3ENC_UNUSED_REF(inst);
	MP3ENC_UNUSED_REF(pInputBuf);
	MP3ENC_UNUSED_REF(pOutputBUf);
	MP3ENC_UNUSED_REF(s32InputLen);
	MP3ENC_UNUSED_REF(s32OutLen);
	//parse the handler
	MP3_ENC_CHECK_NULL(inst);
	ST_MP3_EncHandler *pmp3_enc_handler;
	CVI_S32 s32TotalSizeBytes = s32InputLen;


	pmp3_enc_handler = (ST_MP3_EncHandler *)inst;
	MP3_ENC_CHECK_NULL(pmp3_enc_handler->gfp);
	//step1:hander the input buffer
	s32TotalSizeBytes = s32InputLen + pmp3_enc_handler->remain_sizebytes;
	if (pmp3_enc_handler->remain_sizebytes == 0) {
		if (s32TotalSizeBytes <= CVI_MP3_ENC_INPUT_BUFFER_SIZE) {
			//without remain size bytes
			//buffer is safe in size
			memcpy(pmp3_enc_handler->inputbuffer, pInputBuf, s32InputLen);
			pmp3_enc_handler->input_sizebytes = s32TotalSizeBytes;
		} else {
			printf("[Error]Input size over buffer limit [%d] > [%d]\n",
				s32InputLen, CVI_MP3_ENC_INPUT_BUFFER_SIZE);
			return -1;
		}

	} else {
		//concatenate the buffer
		if (s32TotalSizeBytes > CVI_MP3_ENC_INPUT_BUFFER_SIZE) {
			printf("[Warning]TotalSize over buffer size [%d] > [%d]\n",
				s32TotalSizeBytes, CVI_MP3_ENC_INPUT_BUFFER_SIZE);
			printf("force flush....\n");
			CVI_S32 s32FlushBytes = s32TotalSizeBytes - CVI_MP3_ENC_INPUT_BUFFER_SIZE;

			memmove(pmp3_enc_handler->inputbuffer,
				(pmp3_enc_handler->inputbuffer + s32FlushBytes),
				 pmp3_enc_handler->remain_sizebytes - s32FlushBytes);
			memcpy((pmp3_enc_handler->inputbuffer + CVI_MP3_ENC_INPUT_BUFFER_SIZE - s32InputLen),
				pInputBuf,
				s32InputLen);
			pmp3_enc_handler->input_sizebytes = CVI_MP3_ENC_INPUT_BUFFER_SIZE;

		} else {
			memcpy((pmp3_enc_handler->inputbuffer + pmp3_enc_handler->remain_sizebytes),
				pInputBuf,
				s32InputLen);
			pmp3_enc_handler->input_sizebytes = s32TotalSizeBytes;
		}
	}

	//step2:do the encode if the data input over the CVI_MP3_ENC_BASIC_INPUT_LEVEL
	CVI_S32 s32BasicSizeLevel = 0;

	if (pmp3_enc_handler->channels == 1)
		s32BasicSizeLevel = CVI_MP3_ENC_BASIC_INPUT_LEVEL;
	else if (pmp3_enc_handler->channels == 2)
		s32BasicSizeLevel = CVI_MP3_ENC_BASIC_INPUT_LEVEL_STEREO;
	else {
		printf("Error in channel numbers[%d] ...force return\n",
			pmp3_enc_handler->channels);
		*s32OutLen = 0;
		return CVI_FAILURE;
	}

	if (pmp3_enc_handler->input_sizebytes < s32BasicSizeLevel) {
		printf("Require more bytes...[%d]\n",
			s32BasicSizeLevel - pmp3_enc_handler->input_sizebytes);
		*s32OutLen = 0;
		pmp3_enc_handler->remain_sizebytes = pmp3_enc_handler->input_sizebytes;
		return CVI_MP3_ENC_CB_REQUIRE_MORE_INPUT;
	}


	CVI_S32 s32RemainBytes = pmp3_enc_handler->input_sizebytes;
	short *buffer = (short *)pmp3_enc_handler->inputbuffer;
	short *leftBuffer = malloc(CVI_MP3_ENC_BASIC_INPUT_LEVEL);
	short *rightBuffer = malloc(CVI_MP3_ENC_BASIC_INPUT_LEVEL);
	//unsigned char mp3buffer[LAME_MAXMP3BUFFER];
	int outputbytes = 0;

	do {
		if (pmp3_enc_handler->channels == 1) {
			memcpy(leftBuffer, buffer, CVI_MP3_ENC_BASIC_INPUT_LEVEL);
			outputbytes = lame_encode_buffer(pmp3_enc_handler->gfp,
				leftBuffer,
				leftBuffer,
				(CVI_MP3_ENC_BASIC_INPUT_LEVEL/2),
				pmp3_enc_handler->mp3buffer,
				LAME_MAXMP3BUFFER);
			if (_check_lame_ret_value(outputbytes) < 0) {
				printf("[Error][%s][%d]\n", __func__, __LINE__);
				return CVI_FAILURE;
			}

			s32RemainBytes -= CVI_MP3_ENC_BASIC_INPUT_LEVEL;
			buffer = buffer + 1152;

			memcpy ((pmp3_enc_handler->enc_outputbuffer + pmp3_enc_handler->output_sizebytes),
				pmp3_enc_handler->mp3buffer, outputbytes);
			pmp3_enc_handler->output_sizebytes += outputbytes;

		} else {
			for (int i = 0; i < CVI_MP3_ENC_BASIC_INPUT_LEVEL; i++) {
				if ((i % 2) == 0)
					leftBuffer[i/2] = buffer[i];
				else
					rightBuffer[i/2] = buffer[i];

			}

			outputbytes = lame_encode_buffer(pmp3_enc_handler->gfp,
				leftBuffer,
				rightBuffer,
				(CVI_MP3_ENC_BASIC_INPUT_LEVEL/2),
				pmp3_enc_handler->mp3buffer,
				LAME_MAXMP3BUFFER);
			if (_check_lame_ret_value(outputbytes) < 0) {
				printf("[Error][%s][%d]\n", __func__, __LINE__);
				return CVI_FAILURE;
			}

			s32RemainBytes -= CVI_MP3_ENC_BASIC_INPUT_LEVEL * 2;
			buffer = buffer + (1152 * 2);
			//step3:output to output buffer,
			memcpy ((pmp3_enc_handler->enc_outputbuffer + pmp3_enc_handler->output_sizebytes),
				pmp3_enc_handler->mp3buffer,
				outputbytes);
			pmp3_enc_handler->output_sizebytes += outputbytes;
		}

	} while (s32RemainBytes >= s32BasicSizeLevel);

	//step3:handle the remain size
	//reset the output size bytes
	//reset the input size bytes
	//return the sizebytes;

	*s32OutLen = pmp3_enc_handler->output_sizebytes;
	memcpy(pOutputBUf, pmp3_enc_handler->enc_outputbuffer, *s32OutLen);
	pmp3_enc_handler->remain_sizebytes = s32RemainBytes;
	if (s32RemainBytes != 0) {
		memmove(pmp3_enc_handler->inputbuffer,
			pmp3_enc_handler->inputbuffer + pmp3_enc_handler->input_sizebytes - s32RemainBytes,
			s32RemainBytes);
	}
	printf("Get out of loop---->remain[%d]output[%d]\n", s32RemainBytes, *s32OutLen);
	pmp3_enc_handler->output_sizebytes = 0;
	pmp3_enc_handler->input_sizebytes = 0;


	return CVI_SUCCESS;
}

CVI_VOID *CVI_MP3_Encode_Init(CVI_VOID *inst, ST_CVI_MP3_ENC_INIT stConfig)
{
	MP3ENC_UNUSED_REF(inst);
	ST_MP3_EncHandler *pmp3_enc_handler;
	int ret;

	printf("CVI_MP3_Encode_Init b_rate[%d] smp_rate[%d] chn[%d] q[%d]\n",
		stConfig.bitrate,
		stConfig.sample_rate,
		stConfig.channel_num,
		stConfig.quality);
	//init the buffer size
	pmp3_enc_handler = (ST_MP3_EncHandler *)malloc(sizeof(ST_MP3_EncHandler));
	memset(pmp3_enc_handler, 0, sizeof(ST_MP3_EncHandler));

	//init the buffer //how much size for buffer is reasonable?
	pmp3_enc_handler->inputbuffer = malloc(CVI_MP3_ENC_INPUT_BUFFER_SIZE);
	pmp3_enc_handler->enc_outputbuffer = malloc(CVI_MP3_ENC_OUTPUT_BUFFER_SIZE);
	pmp3_enc_handler->pEncode_Cb = NULL;
	//init the function pt
	pmp3_enc_handler->lame_init = lame_init;
	pmp3_enc_handler->lame_set_in_samplerate = lame_set_in_samplerate;
	pmp3_enc_handler->lame_set_out_samplerate = lame_set_out_samplerate;
	pmp3_enc_handler->lame_set_num_channels = lame_set_num_channels;
	pmp3_enc_handler->lame_set_num_samples = lame_set_num_samples;
	pmp3_enc_handler->lame_set_brate = lame_set_brate;
	pmp3_enc_handler->lame_set_quality = lame_set_quality;
	pmp3_enc_handler->lame_set_VBR = lame_set_VBR;
	pmp3_enc_handler->lame_init_params = lame_init_params;
	pmp3_enc_handler->lame_set_mode = lame_set_mode;
	pmp3_enc_handler->lame_encode_buffer_float = lame_encode_buffer_float;
	pmp3_enc_handler->lame_encode_buffer = lame_encode_buffer;
	pmp3_enc_handler->lame_encode_buffer_interleaved = lame_encode_buffer_interleaved;
	pmp3_enc_handler->lame_encode_buffer_int = lame_encode_buffer_int;

	//init the value
	//sample rate, channel, bitrate, quality
	if (pmp3_enc_handler->lame_init != NULL)
		pmp3_enc_handler->gfp = pmp3_enc_handler->lame_init();
	else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->gfp != NULL)
		printf("enc handler create success!!\n");

	if (pmp3_enc_handler->lame_set_in_samplerate != NULL)
		pmp3_enc_handler->lame_set_in_samplerate(pmp3_enc_handler->gfp,
							stConfig.sample_rate);
	else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->lame_set_out_samplerate != NULL)
		pmp3_enc_handler->lame_set_out_samplerate(pmp3_enc_handler->gfp,
							stConfig.sample_rate);
	else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->lame_set_num_channels != NULL) {
		pmp3_enc_handler->lame_set_num_channels(pmp3_enc_handler->gfp,
							stConfig.channel_num);
		if (stConfig.channel_num == 1) {
			printf("Set in the mono channel mode for encode mp3\n");
			pmp3_enc_handler->lame_set_mode(pmp3_enc_handler->gfp,
							MONO);
		}
	} else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->lame_set_brate != NULL)
		pmp3_enc_handler->lame_set_brate(pmp3_enc_handler->gfp,
						stConfig.bitrate / 1000);
	else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->lame_set_quality != NULL)
		pmp3_enc_handler->lame_set_quality(pmp3_enc_handler->gfp,
						stConfig.quality);
	else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}

	if (pmp3_enc_handler->lame_init_params != NULL) {
		ret = pmp3_enc_handler->lame_init_params(pmp3_enc_handler->gfp);
		if (ret < 0)
			printf("[Error]cvi mp3 encoder init params failure\n");

	} else {
		printf("[Error][%s][%d]...function pt NULL\n", __func__, __LINE__);
		return NULL;
	}
	pmp3_enc_handler->channels = stConfig.channel_num;
	pmp3_enc_handler->samplerate = stConfig.sample_rate;
	pmp3_enc_handler->bitrate = stConfig.bitrate;
	pmp3_enc_handler->quality = stConfig.quality;//TODO: check quality range and do the check
	pmp3_enc_handler->vbr_on = 0;

	return (CVI_VOID *)pmp3_enc_handler;
}

CVI_S32 CVI_MP3_Encode_InstallCb(CVI_VOID *inst, Encode_Cb pCbFunc)
{

	MP3ENC_UNUSED_REF(inst);
	MP3ENC_UNUSED_REF(pCbFunc);
	MP3_ENC_CHECK_NULL(inst);
	ST_MP3_EncHandler *pmp3_enc_handler;

	pmp3_enc_handler = (ST_MP3_EncHandler *)inst;
	MP3_ENC_CHECK_NULL(pmp3_enc_handler->gfp);
#if 0
	st_mp3_dec_handler *pmp3_dec_handler;

	pmp3_dec_handler = (st_mp3_dec_handler *)inst;

	pmp3_dec_handler->pDecode_Cb = pCbFunc;
	//reset the mp3 dec info struct
	memset(&pmp3_dec_handler->stMp3DecInfo, 0, sizeof(ST_MP3_DEC_INFO));
#endif
	return CVI_SUCCESS;

}


CVI_S32 CVI_MP3_Encode_DeInit(CVI_VOID *inst)
{
	MP3ENC_UNUSED_REF(inst);
	MP3_ENC_CHECK_NULL(inst);
	ST_MP3_EncHandler *pmp3_enc_handler;

	pmp3_enc_handler = (ST_MP3_EncHandler *)inst;
	MP3_ENC_CHECK_NULL(pmp3_enc_handler->gfp);

	int read;
	//flush mp3 buffer
	read = lame_encode_flush(pmp3_enc_handler->gfp,
				pmp3_enc_handler->mp3buffer,
				LAME_MAXMP3BUFFER);

	if (read < 0) {
		if(read == -1) {
			printf("[Error]mp3buffer is probably not big enough.\n");
		} else {
			printf("[Error]MP3 internal error.\n");
		}
	} else {
		printf("Flushing stage yielded %d frames.\n", read);
	}
	//write in mp3 tag:song information


	//close lame handler
	lame_close(pmp3_enc_handler->gfp);

	free(pmp3_enc_handler);
#if 0
  /*samples have been written. write ID3 tag.*/
  read = lame_get_id3v1_tag(gfp, mp3buffer, sizeof(mp3buffer));
  if(sizeof(read) > sizeof(mp3buffer)) {
    printf("Buffer too small to write ID3v1 tag.\n");
  } else {
    if(read > 0) {
      write = (int) fwrite(mp3buffer, 1, read, mp3);
      if(read != write) {
        printf("more errors in writing id tag to mp3 file.\n");
      }
    }
  }

  lame_close(gfp);
  fclose(pcm);
  fclose(mp3);
#endif
	return CVI_SUCCESS;

}


#ifdef TEST_IN_ENCODE_UNIT
int main(int argc, char *argv[])
{
	printf("enter cvimp3enc exe\n");
	FILE *pfd_in = NULL;

	char *input_filename;
	char output_filename[128] = {0};
	unsigned char *input_buffer;
	unsigned char *output_buffer;
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 s32OutLen;

	if (argc < 2) {
		printf("Not enough input\n");
		cvimp3enc_encode_usage();
	}

	input_filename = argv[1];
	if (access(input_filename, 0) <  0) {
		printf("[Error]check file[%s] not exist!....error\n", input_filename);
		return (-1);
	}

	//step1:open file
	snprintf(output_filename, 128, "%s.mp3", input_filename);
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
	void *pMp3EncHandler;
	ST_CVI_MP3_ENC_INIT stMp3EncConfig;

	//TODO: this should be optional
	stMp3EncConfig.bitrate = 128000;
	stMp3EncConfig.sample_rate = _CONSOLE_REQ("Enter sample rate:(default:32000)\n", 32000);
	stMp3EncConfig.channel_num = _CONSOLE_REQ("Enter channel num:(default:2)\n", 2);
	stMp3EncConfig.quality = 9;
	pMp3EncHandler = CVI_MP3_Encode_Init(NULL, stMp3EncConfig);
	if (pMp3EncHandler == NULL) {
		printf("[error][%s][%d]\n", __func__, __LINE__);
		return -1;
	}

	//optional:user call also get the data from call back function
	//s32Ret = CVI_MP3_Decode_InstallCb(pMp3DecHandler, _mp3_dec_callback_forApp);
	_mp3_enc_callback_forApp(NULL, NULL, 0);
#if EXAMPLE_FILE_MODE
	if (fread(input_buffer,
			1,
			fsize, pfd_in) != (unsigned int)fsize) {
		printf("[Error][%s][%d] fread failure\n", __func__, __LINE__);
	}
	printf("test 1--->give it all\n");
	s32Ret = CVI_MP3_Encode((CVI_VOID *)pMp3EncHandler,
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
		printf("test 222--->give it 1024\n");
		s32Ret = CVI_MP3_Encode((CVI_VOID *)pMp3EncHandler,
					(CVI_VOID *)input_buffer,
					(CVI_VOID *)output_buffer,
					(CVI_S32)1024,
					(CVI_S32 *)&s32OutLen);

		if (s32Ret != CVI_SUCCESS) {
			if (s32Ret == CVI_MP3_ENC_CB_REQUIRE_MORE_INPUT)
				printf("Require more bytes\n");
		} else
			printf("[error][%s][%d]\n", __func__, __LINE__);

		printf("output length s32OutLen[%d]\n", s32OutLen);
		if (s32OutLen != 0)
			fwrite(output_buffer, 1, (s32OutLen), pfd_out);
	}

#endif
	CVI_MP3_Encode_DeInit(pMp3EncHandler);

	if (input_buffer != NULL)
		free(input_buffer);
	if (output_buffer != NULL)
		free(output_buffer);

	printf("Exit program!!!\n");
	return 0;
}
#endif