/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: sample/cvi_sample_audio.c
 * Description:example for audio api flow
 * such as audio in, audio out, audio trancode flow
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
//#include "cvi_sample_comm.h"
//#include "sample_comm.h"
#include "cvi_audio_dl_adp.h"
#include "cvi_audio_parse_param.h"
//#ifdef SUPPORT_EXTERNAL_AAC
#include "cvi_audio_aac_adp.h"
//#endif

#define ACODEC_ADC	"/dev/cvitekaadc"
#define ACODEC_DAC	"/dev/cvitekadac"

#define SMP_AUD_UNUSED_REF(X)  ((X) = (X))
#define AUDIO_ADPCM_TYPE ADPCM_TYPE_DVI4/* ADPCM_TYPE_IMA, ADPCM_TYPE_DVI4*/
#define G726_BPS MEDIA_G726_32K         /* MEDIA_G726_16K, MEDIA_G726_24K ... */

static AAC_TYPE_E     gs_enAacType = AAC_TYPE_AACLC;
static AAC_BPS_E     gs_enAacBps  = AAC_BPS_32K;
static AAC_TRANS_TYPE_E gs_enAacTransType = AAC_TRANS_TYPE_ADTS;

#define FILE_NAME_LEN 128
#define AEC_LOOP_RECORD_RUN 1
#define	AEC_LOOP_RECORD_STOP 0

/* WAV */
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT	0x20746d66
#define ID_DATA 0x61746164

struct riff_wave_header {
	unsigned int riff_id;
	unsigned int riff_sz;
	unsigned int wave_id;
};

struct chunk_header {
	unsigned int id;
	unsigned int sz;
};

struct chunk_fmt {
	unsigned short audio_format;
	unsigned short num_channels;
	unsigned int sample_rate;
	unsigned int byte_rate;
	unsigned short block_align;
	unsigned short bits_per_sample;
};


typedef struct {
	int AiDev;
	int AiChn;
	bool bVqe;
	int cap_loop;
	int ChnCnt;
	int record_status;
} ST_VQE_RECORD_TEST_STRUCT;

typedef struct {
	int AoDev;
	int AoChn;
	int ChnCnt;
	unsigned int u32PtNumPerFrm;
	FILE *fp_playfile;
	int stop_flag;
} ST_VQE_PLAY_TEST_STRUCT;

typedef struct {
	int sample_rate;
	int channel;
	int preiod_size;
	int codec;
	//PAYLOAD_TYPE_E eType;
	bool bVqeOn;
	char filename[FILE_NAME_LEN];
	int Chnsample_rate;
	int record_time;
} stAudPara;


static void dump_audiodata(char *filename, char *buf, unsigned int len)
{
	FILE *fp;

	if (filename == NULL) {
		return;
	}

	fp = fopen(filename, "ab+");
	fwrite(buf, 1, len, fp);
	fclose(fp);

}

int running = 1;
void sigint_handler_sample(int signal)
{
	SMP_AUD_UNUSED_REF(signal);
	running = 0;
}

void register_inthandler(void)
{
	signal(SIGINT, sigint_handler_sample);
	signal(SIGHUP, sigint_handler_sample);
	signal(SIGTERM, sigint_handler_sample);
}


static void _update_down_vqe_setting(AO_VQE_CONFIG_S *pstVqeConfig, int samplerate, int channels)
{
	CVI_HPF_CONFIG_S stHpfParam;
	CVI_EQ_CONFIG_S stEqParam;
	CVI_DRC_COMPRESSOR_PARAM stDrcCompressor;
	CVI_DRC_LIMITER_PARAM stDrcLimiter;
	CVI_DRC_EXPANDER_PARAM stDrcExpander;

	pstVqeConfig->u32OpenMask = DNVQE_HPFILTER;
	pstVqeConfig->s32WorkSampleRate = samplerate;
	pstVqeConfig->s32channels = channels;
	//HPF
	stHpfParam.type = E_FILTER_HPF;
	stHpfParam.f0 = 70;
	stHpfParam.Q = 0.707;
	stHpfParam.gainDb = 0;
	pstVqeConfig->stHpfParam = stHpfParam;

	//EQ
	stEqParam.bandIdx = 5;	/*bandIdx	0-5*/
	stEqParam.freq = 150;	/*freq		100-20000*/
	stEqParam.QValue = 0.707;	/*QValue	0.01-10*/
	stEqParam.gainDb = 6;	/*gainDb	-30-30*/
	pstVqeConfig->stEqParam = stEqParam;

	//DRC-Limiter
	stDrcLimiter.attackTimeMs = 10;
	stDrcLimiter.releaseTimeMs = 100;
	stDrcLimiter.thresholdDb = -2;
	stDrcLimiter.postGain = 4.0f;
	pstVqeConfig->stDrcLimiter = stDrcLimiter;

	//DRC-Compressor
	stDrcCompressor.attackTimeMs = 5;
	stDrcCompressor.releaseTimeMs = 200;
	stDrcCompressor.thresholdDb = -2;
	stDrcCompressor.ratio = 10;
	pstVqeConfig->stDrcCompressor = stDrcCompressor;

	//DRC-pExpander
	stDrcExpander.attackTimeMs = 10;
	stDrcExpander.releaseTimeMs = 200;
	stDrcExpander.thresholdDb = -20;
	stDrcExpander.minDb = -60;
	stDrcExpander.ratio = 5;
	stDrcExpander.holdTimeMs = 20;
	pstVqeConfig->stDrcExpander = stDrcExpander;

}


static CVI_BOOL _update_agc_anr_setting(AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr)
{
	if (pstAiVqeTalkAttr == NULL)
		return CVI_FALSE;

	pstAiVqeTalkAttr->u32OpenMask |= (NR_ENABLE | AGC_ENABLE | DCREMOVER_ENABLE);

	AUDIO_AGC_CONFIG_S st_AGC_Setting;
	AUDIO_ANR_CONFIG_S st_ANR_Setting;

	st_AGC_Setting.para_agc_max_gain = 0;
	st_AGC_Setting.para_agc_target_high = 2;
	st_AGC_Setting.para_agc_target_low = 72;
	st_AGC_Setting.para_agc_vad_ena = CVI_TRUE;
	st_ANR_Setting.para_nr_snr_coeff = 15;
	st_ANR_Setting.para_nr_init_sile_time = 0;



	pstAiVqeTalkAttr->stAgcCfg = st_AGC_Setting;
	pstAiVqeTalkAttr->stAnrCfg = st_ANR_Setting;

	pstAiVqeTalkAttr->para_notch_freq = 0;
	printf("pstAiVqeTalkAttr:u32OpenMask[0x%x]\n", pstAiVqeTalkAttr->u32OpenMask);
	return CVI_TRUE;
}

static CVI_BOOL _update_aec_setting(AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr)
{
	if (pstAiVqeTalkAttr == NULL)
		return CVI_FALSE;

	AI_AEC_CONFIG_S default_AEC_Setting;

	memset(&default_AEC_Setting, 0, sizeof(AI_AEC_CONFIG_S));
	default_AEC_Setting.para_aec_filter_len = 13;
	default_AEC_Setting.para_aes_std_thrd = 37;
	default_AEC_Setting.para_aes_supp_coeff = 60;
	pstAiVqeTalkAttr->stAecCfg = default_AEC_Setting;
	pstAiVqeTalkAttr->u32OpenMask = LP_AEC_ENABLE | NLP_AES_ENABLE |
					NR_ENABLE | AGC_ENABLE;
	printf("pstAiVqeTalkAttr:u32OpenMask[0x%x]\n", pstAiVqeTalkAttr->u32OpenMask);
	return CVI_FALSE;
}


static CVI_S32  _update_aenc_params(AENC_CHN_ATTR_S *pAencAttrs,
				    AIO_ATTR_S *pAioAttrs,
				    PAYLOAD_TYPE_E enType)
{
	CVI_S32 s32Ret = CVI_SUCCESS;

	if (!pAencAttrs || !pAioAttrs) {
		printf("[fatal error][%p, %p] ptr is NULL,fuc:%s,line:%d\n",
			pAencAttrs, pAioAttrs, __func__, __LINE__);
		return -1;
	}

	memset(pAencAttrs, 0, sizeof(AENC_CHN_ATTR_S));
	pAencAttrs->enType = enType;
	pAencAttrs->u32BufSize = 30;
	pAencAttrs->u32PtNumPerFrm = pAioAttrs->u32PtNumPerFrm;

	if (pAencAttrs->enType == PT_ADPCMA) {
		AENC_ATTR_ADPCM_S *pstAdpcmAenc = (AENC_ATTR_ADPCM_S *)malloc(sizeof(
				AENC_ATTR_ADPCM_S));

		pstAdpcmAenc->enADPCMType = AUDIO_ADPCM_TYPE;
		pAencAttrs->pValue       = (CVI_VOID *)pstAdpcmAenc;
	} else if (pAencAttrs->enType == PT_G711A || pAencAttrs->enType == PT_G711U) {
		AENC_ATTR_G711_S *pstAencG711 = (AENC_ATTR_G711_S *)malloc(sizeof(
							AENC_ATTR_G711_S));

		pAencAttrs->pValue = (CVI_VOID *)pstAencG711;
	} else if (pAencAttrs->enType == PT_G726) {
		AENC_ATTR_G726_S *pstAencG726 = (AENC_ATTR_G726_S *)malloc(sizeof(
							AENC_ATTR_G726_S));
		pstAencG726->enG726bps = G726_BPS;
		pAencAttrs->pValue = (CVI_VOID *)pstAencG726;

	} else if (pAencAttrs->enType == PT_LPCM) {
		AENC_ATTR_LPCM_S *pstAencLpcm = (AENC_ATTR_LPCM_S *)malloc(sizeof(
							AENC_ATTR_LPCM_S));

		pAencAttrs->pValue = (CVI_VOID *)pstAencLpcm;
	} else if (pAencAttrs->enType == PT_AAC) {
		printf("Need update detail external AAC function params\n");
		//need update AAC if supported
	} else {
		printf("[Error]Not support codec type[%d]\n", enType);
		s32Ret = CVI_FAILURE;
	}

	return s32Ret;
}

CVI_S32 _update_Aenc_setting(AIO_ATTR_S *pstAioAttr,
			     AENC_CHN_ATTR_S *pstAencAttr,
			     PAYLOAD_TYPE_E enType,
			     int Sample_rate, bool bVqe)
{

	CVI_S32 s32Ret;
	SMP_AUD_UNUSED_REF(bVqe);

	s32Ret = _update_aenc_params(pstAencAttr, pstAioAttr, enType);
	if (s32Ret != CVI_SUCCESS) {
		printf("[Error][%s]failure in params\n", __func__);
		return CVI_FAILURE;
	}

	if (enType == PT_AAC) {
		AENC_ATTR_AAC_S  *pstAencAac = (AENC_ATTR_AAC_S *)malloc(sizeof(
							AENC_ATTR_AAC_S));
		pstAencAac->enAACType = gs_enAacType;
		pstAencAac->enBitRate = gs_enAacBps;
		pstAencAac->enBitWidth = AUDIO_BIT_WIDTH_16;

		pstAencAac->enSmpRate = Sample_rate;
		printf("[cvi_info] AAC enc[%s][%d]smp-rate[%d]\n",
		       __func__,
		       __LINE__,
		       pstAencAac->enSmpRate);

		pstAencAac->enSoundMode = pstAioAttr->enSoundmode;
		pstAencAac->enTransType = gs_enAacTransType;
		pstAencAac->s16BandWidth = 0;
		pstAencAttr->pValue = pstAencAac;
		s32Ret = CVI_MPI_AENC_AacInit();
		printf("[cvi_info] CVI_MPI_AENC_AacInit s32Ret:%d\n", s32Ret);
	}

	pstAencAttr->bFileDbgMode = CVI_FALSE;

	return CVI_SUCCESS;
}

int _destroy_Aenc_setting(AENC_CHN_ATTR_S *pstAencAttr)
{
	if (!pstAencAttr || !pstAencAttr->pValue) {
		printf("[fatal error][%p, %p] ptr is NULL,fuc:%s,line:%d\n",
			pstAencAttr, pstAencAttr->pValue, __func__, __LINE__);
		return -1;
	}
	if (pstAencAttr->enType == PT_AAC)
		CVI_MPI_AENC_AacDeInit();
	if (pstAencAttr->pValue) {
		free(pstAencAttr->pValue);
		pstAencAttr->pValue = NULL;
	}
	memset(pstAencAttr, 0, sizeof(AENC_CHN_ATTR_S));

	return 0;
}


void *AudioAigetdata(void *argv)
{
	int s32Ret;
	AUDIO_FRAME_S stFrame;
	AEC_FRAME_S   stAecFrm;
	int AiChn = 1;
	int AiDev = 0;
	int channels = 0;
	struct timespec end;
	struct timespec now;

	int record_time = *(int *)argv;

	s32Ret = CVI_AI_EnableChn(AiDev, AiChn);
	if (s32Ret) {
		printf("[error] ai enable chn error\n");
		return NULL;
	}

	printf("AudAigetdata   in.\n");
	clock_gettime(CLOCK_MONOTONIC, &now);
	end.tv_sec = now.tv_sec + record_time;
	end.tv_nsec = now.tv_nsec;

	while (running) {
		if (record_time) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec > end.tv_sec ||
			    (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
				break;
		}
		s32Ret = CVI_AI_GetFrame(AiDev, AiChn, &stFrame, &stAecFrm, -1);
		if (s32Ret) {
			printf("[error] ai getframe error\n");
			return NULL;
		}

		channels = (stFrame.enSoundmode == AUDIO_SOUND_MODE_MONO) ? 1 : 2;
		if (stFrame.u32Len)
			dump_audiodata("AiDataChn1.raw", (char *)stFrame.u64VirAddr[0],
				       stFrame.u32Len * channels * 2);
		else
			printf("[warn] AudAigetdata get data 0\n");

		s32Ret = CVI_AI_ReleaseFrame(AiDev, AiChn, &stFrame, &stAecFrm);
		if (s32Ret) {
			printf("[error] ai releaseframe error\n");
			return NULL;
		}
	}

	printf("AudAigetdata   out.\n");
	CVI_AI_DisableChn(AiDev, AiChn);
	return NULL;
}

static PAYLOAD_TYPE_E _sample_audio_get_codec(int s32Opt)
{
	PAYLOAD_TYPE_E eType = PT_G726;

	if (s32Opt == 0) {
		//printf("[cvi_info] Codec G726\n");
		eType = PT_G726;
	} else if (s32Opt == 1) {
		printf("[cvi_info] Codec G711A\n");
		eType = PT_G711A;
	} else if (s32Opt == 2) {
		printf("[cvi_info] Codec G711Mu\n");
		eType = PT_G711U;
	} else if (s32Opt == 3) {
		printf("[cvi_info] Codec PT_ADPCMA\n");
		eType = PT_ADPCMA;
	} else if (s32Opt == 4) {
		printf("[cvi_info] Codec AAC_LC\n");
		eType = PT_AAC;
	} else {
		printf("[cvi_info] Enter invalid num ....select g726\n");
		eType = PT_G726;
	}

	return eType;
}

int printf_sample_usage(void)
{
	printf("\n/Usage:./sample_audio <index>/\n");
	printf("\t0:Ai bind Aenc, save as file\n");
	printf("\t1:Ai/Aenc user-get-mode, save as file\n");
	printf("\t2:Adec bind ao, save as file\n");
	printf("\t3:Adec/Ao user-get-mode, save as file\n");
	printf("\t4:recording frame by frame\n");
	printf("\t5:playing audio frame by frame\n");
	printf("\t6:SetVolume db test\n");
	printf("\t7:Audio Version\n");
	printf("\t8:GetVolume db test\n");
	printf("\t9:ioctl test\n");
	printf("\t10:Aec test\n");

	return 0;
}


int printf_parse_usage(void)
{

	printf("[---------Sample Audio USE-------]\n");
	printf("use: sample_audio 1 --list\n");
	printf("-r [sample_rate] -R [Chnsample_rate]\n");
	printf("-c [channel] -p [preiod_size][*aac enc must 1024]\n");
	printf("-C [codec 0:g726 1:g711A 2:g711Mu 3: adpcm 4.AAC]\n");
	printf("-V [bVqeOn] -F [In/Out filename] -T [record time]\n");
	printf("Aenc eg:./sample_audio 0 --list -r 8000 -R 8000 -c 2 -p 320 -C 1 -V 0 -F Cvi_8k_2chn.g711a -T 10\n");
	printf("       :./sample_audio 0 --list -r 8000 -R 8000 -c 2 -p 1024 -C 4 -V 0 -F Cvi_8k_2chn.aac -T 10\n");
	printf("Adec eg:./sample_audio 2 --list -r 8000 -R 8000 -c 2 -p 320 -C 1 -V 0 -F Cvi_8k_2chn.g711a -T 10\n");
	printf("Ai eg:./sample_audio 4 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 0 -F Cvi_8k_2chn.raw -T 10\n");
	printf("Ao eg:./sample_audio 5 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 0 -F Cvi_8k_2chn.raw -T 10\n");
	printf("SetVol eg:./sample_audio 6\n");
	printf("GetVol eg:./sample_audio 8\n");
	printf("AECtest eg:./sample_audio 10 --list -r 8000 -R 8000 -c 2 -p 320 -C 0 -V 1 -F play.wav -T 10\n");
	printf("[----------------------------------]\n");
	return 0;
}

int get_audio_parse(int argc, char **argv, stAudPara *pstAudioparam)
{
	int i, ret;

	memset(pstAudioparam, 0, sizeof(stAudPara));
	for (i = 1; i < argc - 2; i++) {
		argv[i] = argv[i + 2];
	}
	argv[argc - 2] = "\n";
	argv[argc - 1] = "\n";
	while ((ret = getopt(argc, argv, "r:c:p:C:V:F:R:T:")) != -1) {
		switch (ret) {
		case 'r':
			pstAudioparam->sample_rate = atoi(optarg);
			break;
		case 'c':
			pstAudioparam->channel = atoi(optarg);
			break;
		case 'p':
			pstAudioparam->preiod_size = atoi(optarg);
			break;
		case 'C':
			pstAudioparam->codec = atoi(optarg);
			break;
		case 'V':
			pstAudioparam->bVqeOn = (bool)atoi(optarg);
			break;
		case 'F':
			snprintf(pstAudioparam->filename, strlen(optarg) + 1, "%s", optarg);
			break;
		case 'R':
			pstAudioparam->Chnsample_rate = atoi(optarg);
			break;
		case 'T':
			pstAudioparam->record_time = atoi(optarg);
			break;
		default:
			printf_usage();
			return -1;
		}
	}
	return 0;
}




CVI_S32 _updata_Adec_setting(ADEC_CHN_ATTR_S *pAdecAttr)
{
	if (!pAdecAttr) {
		printf("[fatal error] ptr is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}
	pAdecAttr->u32BufSize = 20;
	pAdecAttr->enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */
	pAdecAttr->bFileDbgMode = CVI_FALSE;

	if (pAdecAttr->enType == PT_ADPCMA) {
		ADEC_ATTR_ADPCM_S *pstAdpcm = malloc(sizeof(ADEC_ATTR_ADPCM_S));

		pAdecAttr->pValue = pstAdpcm;
		pstAdpcm->enADPCMType = AUDIO_ADPCM_TYPE;
	} else if (pAdecAttr->enType == PT_G711A || pAdecAttr->enType == PT_G711U) {
		ADEC_ATTR_G711_S *pstAdecG711 = malloc(sizeof(ADEC_ATTR_G711_S));

		pAdecAttr->pValue = pstAdecG711;
	} else if (pAdecAttr->enType == PT_G726) {
		ADEC_ATTR_G726_S *pstAdecG726 = malloc(sizeof(ADEC_ATTR_G726_S));

		pAdecAttr->pValue = pstAdecG726;
		pstAdecG726->enG726bps = G726_BPS;
	} else if (pAdecAttr->enType == PT_LPCM) {
		ADEC_ATTR_LPCM_S *pstAdecLpcm = malloc(sizeof(ADEC_ATTR_LPCM_S));

		pAdecAttr->pValue = pstAdecLpcm;
		pAdecAttr->enMode = ADEC_MODE_PACK;/* lpcm must use pack mode */
	}

	if (pAdecAttr->enType == PT_AAC) {
		CVI_MPI_ADEC_AacInit();
		ADEC_ATTR_AAC_S *pstAdecAac = malloc(sizeof(ADEC_ATTR_AAC_S));

		pstAdecAac->enTransType = gs_enAacTransType;
		pstAdecAac->enSoundMode = (pAdecAttr->s32ChannelNums == 2 ?
					   AUDIO_SOUND_MODE_STEREO : AUDIO_SOUND_MODE_MONO);
		pstAdecAac->enSmpRate = pAdecAttr->s32Sample_rate;
		pAdecAttr->pValue = pstAdecAac;
		pAdecAttr->enMode = ADEC_MODE_STREAM;   /* aac should be stream mode */
		pAdecAttr->s32frame_size = 1024;
	}

	return 0;
}

int _destroy_Adec_setting(ADEC_CHN_ATTR_S *pAdecAttr)
{
	if (!pAdecAttr || !pAdecAttr->pValue) {
		printf("[fatal error][%p, %p] ptr is NULL,fuc:%s,line:%d\n",
			pAdecAttr, pAdecAttr->pValue, __func__, __LINE__);
		return -1;
	}
	if (pAdecAttr->enType == PT_AAC)
		CVI_MPI_ADEC_AacDeInit();

	if (pAdecAttr->pValue) {
		free(pAdecAttr->pValue);
		pAdecAttr->pValue = NULL;
	}
	memset(pAdecAttr, 0, sizeof(ADEC_CHN_ATTR_S));

	return 0;
}

void *play_audio_file(AIO_ATTR_S *pstAioAttr, FILE *fp, int AoChn, int AoDev)
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	AUDIO_FRAME_S stFrame;
	char szThreadName[20] = "downlink";
	int num_readbytes;
	int channel_count = pstAioAttr->enSoundmode + 1;

	CVI_S32 s32FrameBytes = channel_count * pstAioAttr->u32PtNumPerFrm * 2;

	char *pBuffer = malloc(s32FrameBytes);
	char *pVqeBuffer = malloc(s32FrameBytes);//2ch 16bit 160 samples

	prctl(PR_SET_NAME, szThreadName, 0, 0, 0);
	memset(pBuffer, 0, s32FrameBytes);
	memset(pVqeBuffer, 0, s32FrameBytes);

	printf("[cvi_info] sr:%d,channels:%d,period:%d,s32FrameBytes:%d.\n",
	       pstAioAttr->enSamplerate,
	       channel_count, pstAioAttr->u32PtNumPerFrm, s32FrameBytes);

	while (running) {
		memset(pBuffer, 0, s32FrameBytes);
		num_readbytes = fread(pBuffer, 1, s32FrameBytes, fp);
		if (num_readbytes > 0) {
			stFrame.u64VirAddr[0] = (CVI_U8 *)pBuffer;
			stFrame.u32Len = pstAioAttr->u32PtNumPerFrm;//samples size for each channel
			stFrame.u64TimeStamp = 0;
			stFrame.enSoundmode = pstAioAttr->enSoundmode;
			stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;

			s32Ret = CVI_AO_SendFrame(AoDev, AoChn, (const AUDIO_FRAME_S *)&stFrame, 1000);
			if (s32Ret != CVI_SUCCESS)
				printf("[cvi_info] CVI_AO_SendFrame failed with %#x!\n", s32Ret);
		} else {
			printf("num_framebytes %d.\n", num_readbytes);
			break;
		}
	}

	if (!pBuffer)
		free(pBuffer);
	if (!pVqeBuffer)
		free(pVqeBuffer);
	if (!fp)
		fclose(fp);

	return 0;
}

static CVI_BOOL _cvi_checkname_iswav(char *infilename)
{
	CVI_S32 s32InputFileLen = 0;

	s32InputFileLen = strlen(infilename);
	if (s32InputFileLen == 0) {
		printf("No Input File Name..force return\n");
		return 0;
	}
	if (infilename[s32InputFileLen - 4] == '.' &&
	    (infilename[s32InputFileLen - 3] == 'W' ||
	     infilename[s32InputFileLen - 3] == 'w') &&
	    (infilename[s32InputFileLen - 2] == 'A' ||
	     infilename[s32InputFileLen - 2] == 'a') &&
	    (infilename[s32InputFileLen - 1] == 'V' ||
	     infilename[s32InputFileLen - 1] == 'v')) {
		printf("Enter wav file\n");
		return 0;
	} else
		return -1;
}

static FILE *audio_open_wavfile(const char *filename, int *channels,
				int *sample_rate)
{
	FILE *file;
	struct riff_wave_header riff_wave_header;
	struct chunk_header chunk_header;
	struct chunk_fmt chunk_fmt;
	int more_chunks = 1;

	file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		return NULL;
	}

	fread(&riff_wave_header, sizeof(riff_wave_header), 1, file);
	if ((riff_wave_header.riff_id != ID_RIFF) ||
	    (riff_wave_header.wave_id != ID_WAVE)) {
		fprintf(stderr, "Error: '%s' is not a riff/wave file\n", filename);
		fclose(file);
		return NULL;
	}

	do {
		fread(&chunk_header, sizeof(chunk_header), 1, file);

		switch (chunk_header.id) {
		case ID_FMT:
			fread(&chunk_fmt, sizeof(chunk_fmt), 1, file);
			*sample_rate = chunk_fmt.sample_rate;
			*channels = chunk_fmt.num_channels;
			/* If the format header is larger, skip the rest */
			if (chunk_header.sz > sizeof(chunk_fmt))
				fseek(file, chunk_header.sz - sizeof(chunk_fmt), SEEK_CUR);
			break;
		case ID_DATA:
			/* Stop looking for chunks */
			more_chunks = 0;
			chunk_header.sz = le32toh(chunk_header.sz);
			break;
		default:
			/* Unknown chunk, skip bytes */
			fseek(file, chunk_header.sz, SEEK_CUR);
		}
	} while (more_chunks);
	return file;
}



CVI_S32 SAMPLE_AUDIO_AI_BIND_AENC(void *argv)
{
	int AiDev = 0;/*only support 0 dev */
	int AiChn = 0;
	int AeChn = 0;
	int s32Ret = 0;
	int AudMaxChn = 3;
	struct timespec end;
	struct timespec now;
	bool ReSam_flag = false;
	AUDIO_STREAM_S stStream;
	AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
	AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr = (AI_TALKVQE_CONFIG_S *)&stAiVqeTalkAttr;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("[fatal error] ptr is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}
	int sample_rate = pstAudioparam->sample_rate;
	unsigned int Chnsample_rate = pstAudioparam->Chnsample_rate;
	int channel = pstAudioparam->channel;
	int u32PtNumPerFrm = pstAudioparam->preiod_size;
	bool bVqe = pstAudioparam->bVqeOn;
	int record_time = pstAudioparam->record_time;
	PAYLOAD_TYPE_E enType =	_sample_audio_get_codec(
					pstAudioparam->codec);/*PT_G711A,PT_G711U,PT_G726 */


	register_inthandler();
	FILE *fpAenc = fopen(pstAudioparam->filename, "ab+");

	if (!fpAenc) {
		printf("[fatal error] ptr is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}

	if (enType == PT_AAC && u32PtNumPerFrm != 1024) {
		printf("[warn] aac encode u32PtNumPerFrm must 1024\n");
		u32PtNumPerFrm = 1024;
	}
//STEP 1:set ai and vqe attr
	AIO_ATTR_S AudinAttr;

	AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)sample_rate;
	AudinAttr.u32ChnCnt = AudMaxChn;
	AudinAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	AudinAttr.u32EXFlag = 0;
	AudinAttr.u32FrmNum = 10; /* only use in bind mode */
	AudinAttr.u32PtNumPerFrm = u32PtNumPerFrm; /* sample_rate/fps */
	AudinAttr.u32ClkSel = 0;
	AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	/*if you want to use vqe ,ai chn must 2chn.*/
	/*if you don't need to use vqe, you can skip this step*/

	if (bVqe) {
		memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
		if (((AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_8000) ||
			(AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_16000)) &&
			channel == 2) {

			pstAiVqeTalkAttr->s32WorkSampleRate = AudinAttr.enSamplerate;
			_update_agc_anr_setting(pstAiVqeTalkAttr);
			_update_aec_setting(pstAiVqeTalkAttr);

		} else {
			printf("[error] AEC will need to setup record in to channel Count = 2\n");
			printf("[error] VQE only support on 8k/16k sample rate. current[%d]\n",
				AudinAttr.enSamplerate);
		}
	}

	s32Ret = CVI_AUDIO_INIT();
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}


//STEP 2:start ai and aenc
	s32Ret = CVI_AI_SetPubAttr(AiDev, &AudinAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AI_Enable(AiDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AI_EnableChn(AiDev, AiChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (bVqe == true) {

		s32Ret = CVI_AI_SetTalkVqeAttr(AiDev, AiChn, 0, 0,
					       &stAiVqeTalkAttr);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

		s32Ret = CVI_AI_EnableVqe(AiDev, AiChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

	}

	if ((enType == PT_G711A || enType == PT_G711U) && (Chnsample_rate != 8000)) {
		Chnsample_rate = 8000;
		printf("G711 only support sr 8000,change to 8000.\n");
	}

	if ((Chnsample_rate != (unsigned int)AudinAttr.enSamplerate)) {
		s32Ret = CVI_AI_EnableReSmp(AiDev, AiChn, Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
	}

	AENC_CHN_ATTR_S stAencAttr;

	_update_Aenc_setting(&AudinAttr, &stAencAttr, enType,
			     Chnsample_rate, bVqe);

	s32Ret = CVI_AENC_CreateChn(AeChn, &stAencAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("s32Ret=%#x,\n", s32Ret);
		goto ERROR;
	}

//STEP 3:ai bind aenc
	MMF_CHN_S stSrcChn, stDestChn;

	stSrcChn.enModId = CVI_ID_AI;
	stSrcChn.s32DevId = AiDev;
	stSrcChn.s32ChnId = AiChn;
	stDestChn.enModId = CVI_ID_AENC;
	stDestChn.s32DevId = 0;
	stDestChn.s32ChnId = AeChn;

	CVI_AUD_SYS_Bind(&stSrcChn, &stDestChn);

	/*Create this thread if you want to fetch the source data at the same time as the encoded data */
	pthread_t t_dp;

	pthread_create(&t_dp, NULL,
		       (void *(*)(void *))AudioAigetdata, (void *)&record_time);

	clock_gettime(CLOCK_MONOTONIC, &now);
	end.tv_sec = now.tv_sec + record_time;
	end.tv_nsec = now.tv_nsec;

	while (running) {
		if (record_time) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec > end.tv_sec ||
			    (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
				break;
		}

		s32Ret = CVI_AENC_GetStream(AeChn, &stStream, 0);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			break;
		}
		if (!stStream.u32Len) {
			usleep(10 * 1000);
			continue;
		}

		fwrite(stStream.pStream, 1, stStream.u32Len, fpAenc);
	}

	CVI_AUD_SYS_UnBind(&stSrcChn, &stDestChn);

	pthread_join(t_dp, NULL);
ERROR:
	CVI_AENC_DestroyChn(AeChn);
	_destroy_Aenc_setting(&stAencAttr);

ERROR1:
	if (bVqe == true)
		CVI_AI_DisableVqe(AiDev, AiChn);
	if (ReSam_flag == true)
		CVI_AI_DisableReSmp(AiDev, AiChn);
	CVI_AI_DisableChn(AiDev, AiChn);
ERROR2:
	CVI_AI_Disable(AiDev);
ERROR3:
	fclose(fpAenc);
	CVI_AUDIO_DEINIT();
	return 0;
}


CVI_S32 SAMPLE_AUDIO_AI_AENC_USER_GET_MODE(void *argv)
{
	int AiDev = 0;/*only support 0 dev */
	int AiChn = 0;
	int AeChn = 0;
	int s32Ret = 0;
	int AudMaxChn = 3;
	bool ReSam_flag = false;
	struct timespec end;
	struct timespec now;
	AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
	AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr = (AI_TALKVQE_CONFIG_S *)&stAiVqeTalkAttr;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("[fatal error] ptr is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}

	int sample_rate = pstAudioparam->sample_rate;
	unsigned int Chnsample_rate = pstAudioparam->Chnsample_rate;
	int channel = pstAudioparam->channel;
	int u32PtNumPerFrm = pstAudioparam->preiod_size;
	bool bVqe = pstAudioparam->bVqeOn;
	int record_time = pstAudioparam->record_time;
	PAYLOAD_TYPE_E enType =	_sample_audio_get_codec(
					pstAudioparam->codec);/*PT_G711A,PT_G711U,PT_G726 */

	register_inthandler();
	FILE *fpAenc = fopen(pstAudioparam->filename, "ab+");

	if (!fpAenc) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}

	if (enType == PT_AAC && u32PtNumPerFrm != 1024) {
		printf("[warn] aac encode u32PtNumPerFrm must 1024\n");
		u32PtNumPerFrm = 1024;
	}

//STEP 1:set ai and vqe attr
	AIO_ATTR_S AudinAttr;

	AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)sample_rate;
	AudinAttr.u32ChnCnt = AudMaxChn;
	AudinAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	AudinAttr.u32EXFlag = 0;
	AudinAttr.u32FrmNum = 10; /* only use in bind mode */
	AudinAttr.u32PtNumPerFrm = u32PtNumPerFrm; /* sample_rate/fps */
	AudinAttr.u32ClkSel = 0;
	AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	/*if you want to use vqe ,ai chn must 2chn.*/
	/*if you don't need to use vqe, you can skip this step*/

	if (bVqe) {
		memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
		if (((AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_8000) ||
			(AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_16000)) &&
			channel == 2) {

			pstAiVqeTalkAttr->s32WorkSampleRate = AudinAttr.enSamplerate;
			_update_agc_anr_setting(pstAiVqeTalkAttr);
			_update_aec_setting(pstAiVqeTalkAttr);
		} else {
			printf("[error] AEC will need to setup record in to channel Count = 2\n");
			printf("[error] VQE only support on 8k/16k sample rate. current[%d]\n",
				AudinAttr.enSamplerate);
		}
	}


	s32Ret = CVI_AUDIO_INIT();
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}


//STEP 2:start ai and aenc
	s32Ret = CVI_AI_SetPubAttr(AiDev, &AudinAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AI_Enable(AiDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AI_EnableChn(AiDev, AiChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (bVqe == true) {
		s32Ret = CVI_AI_SetTalkVqeAttr(AiDev, AiChn, 0, 0,
					       &stAiVqeTalkAttr);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

		s32Ret = CVI_AI_EnableVqe(AiDev, AiChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

	}

	if ((enType == PT_G711A || enType == PT_G711U) && (Chnsample_rate != 8000)) {
		Chnsample_rate = 8000;
		printf("G711 only support sr 8000,change to 8000.\n");
	}

	if ((Chnsample_rate != (unsigned int)AudinAttr.enSamplerate)) {
		s32Ret = CVI_AI_EnableReSmp(AiDev, AiChn, Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
		ReSam_flag = true;
	}

	AENC_CHN_ATTR_S stAencAttr;

	_update_Aenc_setting(&AudinAttr, &stAencAttr, enType,
			     Chnsample_rate, bVqe);

	s32Ret = CVI_AENC_CreateChn(AeChn, &stAencAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("s32Ret=%#x,\n", s32Ret);
		goto ERROR;
	}

	AUDIO_STREAM_S stStream;
	AEC_FRAME_S   stAecFrm;
	AUDIO_FRAME_S stFrame;

	clock_gettime(CLOCK_MONOTONIC, &now);
	end.tv_sec = now.tv_sec + record_time;
	end.tv_nsec = now.tv_nsec;


	while (running) {
		if (record_time) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec > end.tv_sec ||
			    (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
				break;
		}

		s32Ret = CVI_AI_GetFrame(AiDev, AiChn, &stFrame, &stAecFrm, -1);
		if (s32Ret) {
			printf("[error] ai getframe error\n");
			break;
		}

		s32Ret = CVI_AENC_SendFrame(AeChn, &stFrame, &stAecFrm);
		if (s32Ret) {
			printf("[error] CVI_AENC_SendFrame error\n");
			break;
		}

		s32Ret = CVI_AENC_GetStream(AeChn, &stStream, 0);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR;
		}
		if (!stStream.u32Len) {
			continue;
		}

		fwrite(stStream.pStream, 1, stStream.u32Len, fpAenc);

		s32Ret = CVI_AI_ReleaseFrame(AiDev, AiChn, &stFrame, &stAecFrm);
		if (s32Ret) {
			printf("[error] ai releaseframe error\n");
			break;
		}
	}

ERROR:
	CVI_AENC_DestroyChn(AeChn);
	_destroy_Aenc_setting(&stAencAttr);
ERROR1:
	if (bVqe == true)
		CVI_AI_DisableVqe(AiDev, AiChn);
	if (ReSam_flag == true)
		CVI_AI_DisableReSmp(AiDev, AiChn);
	CVI_AI_DisableChn(AiDev, AiChn);
ERROR2:
	CVI_AI_Disable(AiDev);
ERROR3:
	fclose(fpAenc);
	CVI_AUDIO_DEINIT();

	return 0;
}


CVI_S32 SAMPLE_AUDIO_ADEC_BIND_AO(void *argv)
{
	CVI_S32 s32Ret = 0;
	CVI_S32 AdChn = 0;
	CVI_S32 AdDev = 0;
	CVI_S32 AoChn = 0;
	CVI_S32 AoDev = 0;
	int AudMaxChn = 3;
	int length_bytes = 640;//2ch 16bit 160 samples
	PAYLOAD_TYPE_E enType;
	AIO_ATTR_S AudoutAttr;
	ADEC_CHN_ATTR_S stAdecAttr;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}
	register_inthandler();
	CVI_AUDIO_INIT();
	int channel = pstAudioparam->channel;

//STEP 1:set and enable ao
	AudoutAttr.u32ChnCnt = AudMaxChn;
	AudoutAttr.enSamplerate   = pstAudioparam->sample_rate;
	AudoutAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudoutAttr.enWorkmode	  = AIO_MODE_I2S_MASTER;
	AudoutAttr.u32EXFlag	  = 0;
	AudoutAttr.u32FrmNum	  = 10; /* only use in bind mode */
	AudoutAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudoutAttr.u32PtNumPerFrm =
		pstAudioparam->preiod_size;/* 20*targetsamplerate/1000 */
	//CVI_S32 s32FrameBytes = channel * AudoutAttr.u32PtNumPerFrm * 2;
	AudoutAttr.u32ClkSel	  = 0;
	AudoutAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	s32Ret = CVI_AO_SetPubAttr(AoDev, &AudoutAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AO_Enable(AoDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AO_EnableChn(AoDev, AoChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate) {//  /chn 16k/chn 2 48k  ->8k
		s32Ret = CVI_AO_EnableReSmp(AoDev, AoChn, pstAudioparam->Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
	}

	enType = _sample_audio_get_codec(pstAudioparam->codec);


//STEP 2:set and enable adec
	memset(&stAdecAttr, 0, sizeof(ADEC_CHN_ATTR_S));
	stAdecAttr.s32Sample_rate = pstAudioparam->Chnsample_rate;
	stAdecAttr.s32ChannelNums = pstAudioparam->channel;
	stAdecAttr.s32BytesPerSample = 2;
	stAdecAttr.s32frame_size = pstAudioparam->preiod_size;
	stAdecAttr.enType = enType;
	if (stAdecAttr.enType == PT_AAC)
		stAdecAttr.s32frame_size = 1024;

	_updata_Adec_setting(&stAdecAttr);

	s32Ret = CVI_ADEC_CreateChn(AdChn, &stAdecAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("%s: CVI_ADEC_CreateChn(%d) failed with %#x!\n", __func__,
		       AdChn, s32Ret);
		goto ERROR;
	}


//STEP 3:adec bind ao
	MMF_CHN_S stSrcChn, stDestChn;

	stSrcChn.enModId = CVI_ID_ADEC;
	stSrcChn.s32DevId = AdDev;
	stSrcChn.s32ChnId = AdChn;
	stDestChn.enModId = CVI_ID_AO;
	stDestChn.s32DevId = AoDev;
	stDestChn.s32ChnId = AoChn;

	CVI_AUD_SYS_Bind(&stSrcChn, &stDestChn);

	int num_readbytes = 0;
	AUDIO_STREAM_S stAudioStream;

	length_bytes = stAdecAttr.s32frame_size * stAdecAttr.s32ChannelNums *
		       stAdecAttr.s32BytesPerSample;
	char *pBuffer = malloc(length_bytes);

	memset(pBuffer, 0, length_bytes);
	FILE *fpAdec = fopen(pstAudioparam->filename, "rb+");

	if (!fpAdec) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}

	while (running) {
		stAudioStream.pStream = (CVI_U8 *)pBuffer;
		num_readbytes = fread(stAudioStream.pStream, 1, length_bytes, fpAdec);
		if (num_readbytes <= 0) {
			s32Ret = CVI_ADEC_SendEndOfStream(AdChn, CVI_FALSE);
			if (s32Ret != CVI_SUCCESS)
				printf("[cvi_error] CVI_ADEC_SendEndOfStream failed!\n");
			break;
		}
		stAudioStream.u32Len = num_readbytes; //640 bytes
		//printf("[cvi_info] num_readbytes = %d\n", num_readbytes);
		s32Ret = CVI_ADEC_SendStream(AdChn, &stAudioStream, CVI_TRUE);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error] adec send stream failed Len[%d]ret[%#x]!\n",
			       stAudioStream.u32Len, s32Ret);
			break;
		}

	}

	CVI_AUD_SYS_UnBind(&stSrcChn, &stDestChn);


	if (fpAdec)
		fclose(fpAdec);
	if (pBuffer)
		free(pBuffer);

	CVI_ADEC_DestroyChn(AdChn);
	_destroy_Adec_setting(&stAdecAttr);
ERROR:
	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate)
		CVI_AO_DisableReSmp(AoDev, AoChn);
ERROR1:
	CVI_AO_DisableChn(AoDev, AoChn);
ERROR2:
	CVI_AO_Disable(AoDev);
ERROR3:
	CVI_AUDIO_DEINIT();

	return 0;
}


CVI_S32 SAMPLE_AUDIO_ADEC_AO_USER_GET_MODE(void *argv)
{
	CVI_S32 s32Ret = 0;
	CVI_S32 AdChn = 0;
	CVI_S32 AoChn = 0;
	CVI_S32 AoDev = 0;
	int length_bytes = 640;//2ch 16bit 160 samples
	PAYLOAD_TYPE_E enType;
	AIO_ATTR_S AudoutAttr;
	ADEC_CHN_ATTR_S stAdecAttr;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}
	register_inthandler();
	CVI_AUDIO_INIT();
	int channel = pstAudioparam->channel;

//STEP 1:set and enable ao
	AudoutAttr.u32ChnCnt = channel;
	AudoutAttr.enSamplerate   = pstAudioparam->sample_rate;
	AudoutAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudoutAttr.enWorkmode	  = AIO_MODE_I2S_MASTER;
	AudoutAttr.u32EXFlag	  = 0;
	AudoutAttr.u32FrmNum	  = 10; /* only use in bind mode */
	AudoutAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudoutAttr.u32PtNumPerFrm =
		pstAudioparam->preiod_size;/* 20*targetsamplerate/1000 */
	//CVI_S32 s32FrameBytes = channel * AudoutAttr.u32PtNumPerFrm * 2;
	AudoutAttr.u32ClkSel	  = 0;
	AudoutAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	s32Ret = CVI_AO_SetPubAttr(AoDev, &AudoutAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AO_Enable(AoDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AO_EnableChn(AoDev, AoChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate) {
		s32Ret = CVI_AO_EnableReSmp(AoDev, AoChn, pstAudioparam->Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
	}

	enType = _sample_audio_get_codec(pstAudioparam->codec);


//STEP 2:set and enable adec
	memset(&stAdecAttr, 0, sizeof(ADEC_CHN_ATTR_S));
	stAdecAttr.s32Sample_rate = pstAudioparam->Chnsample_rate;
	stAdecAttr.s32ChannelNums = pstAudioparam->channel;
	stAdecAttr.s32BytesPerSample = 2;
	stAdecAttr.s32frame_size = pstAudioparam->preiod_size;
	stAdecAttr.enType = enType;
	if (stAdecAttr.enType == PT_AAC)
		stAdecAttr.s32frame_size = 1024;

	_updata_Adec_setting(&stAdecAttr);

	s32Ret = CVI_ADEC_CreateChn(AdChn, &stAdecAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("%s: CVI_ADEC_CreateChn(%d) failed with %#x!\n", __func__,
		       AdChn, s32Ret);
		goto ERROR;
	}

//STEP 2: adec send data to ao
	int num_readbytes = 0;
	AUDIO_STREAM_S stAudioStream;

	length_bytes = stAdecAttr.s32frame_size * stAdecAttr.s32ChannelNums *
		       stAdecAttr.s32BytesPerSample;
	char *pBuffer = malloc(length_bytes);

	memset(pBuffer, 0, length_bytes);
	FILE *fpAdec = fopen(pstAudioparam->filename, "rb+");

	if (!fpAdec) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}
	AUDIO_FRAME_S stFrame;
	AUDIO_FRAME_S *pstFrame = &stFrame;
	AUDIO_FRAME_INFO_S sDecOutFrm;

	sDecOutFrm.pstFrame = (AUDIO_FRAME_S *)&stFrame;


	while (running) {
		stAudioStream.pStream = (CVI_U8 *)pBuffer;
		num_readbytes = fread(stAudioStream.pStream, 1, length_bytes, fpAdec);
		if (num_readbytes <= 0) {
			s32Ret = CVI_ADEC_SendEndOfStream(AdChn, CVI_FALSE);
			if (s32Ret != CVI_SUCCESS)
				printf("[cvi_error] CVI_ADEC_SendEndOfStream failed!\n");
			break;
		}
		stAudioStream.u32Len = num_readbytes; //640 bytes
		//printf("[cvi_info] num_readbytes = %d\n", num_readbytes);
		s32Ret = CVI_ADEC_SendStream(AdChn, &stAudioStream, CVI_TRUE);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error] adec send stream failed Len[%d]ret[%#x]!\n",
			       stAudioStream.u32Len, s32Ret);
			break;
		}

		s32Ret = CVI_ADEC_GetFrame(AdChn, &sDecOutFrm, CVI_TRUE);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error] adec get stream failed Len[%d]ret[%#x]!\n",
			       stAudioStream.u32Len, s32Ret);
			break;
		}

		if (pstFrame->u32Len != 0) {
			s32Ret = CVI_AO_SendFrame(AoDev, AoChn, pstFrame, 5000);
			if (s32Ret != CVI_SUCCESS) {
				printf("%s: AoDev(%d), failed with %#x!\n", __func__, AoDev, s32Ret);
				break;
			}
		} else
			printf("[cvi_error] dec out frame size 0\n");


	}


	if (fpAdec)
		fclose(fpAdec);
	if (pBuffer)
		free(pBuffer);

	CVI_ADEC_DestroyChn(AdChn);
	_destroy_Adec_setting(&stAdecAttr);
ERROR:
	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate)
		CVI_AO_DisableReSmp(AoDev, AoChn);
ERROR1:
	CVI_AO_DisableChn(AoDev, AoChn);
ERROR2:
	CVI_AO_Disable(AoDev);
ERROR3:
	CVI_AUDIO_DEINIT();
	return 0;
}


CVI_S32 SAMPLE_AUDIO_RECORD_PCM_FORMAT_FILE(void *argv)
{
	int AiDev = 0;/*only support 0 dev */
	int AiChn = 0;
	int s32Ret = 0;
	bool ReSam_flag = false;
	struct timespec end;
	struct timespec now;
	int AudMaxChn = 3;
	AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
	AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr = (AI_TALKVQE_CONFIG_S *)&stAiVqeTalkAttr;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("[fatal error] fpAenc is NULL,fuc:%s,line:%d\n", __func__, __LINE__);
		return -1;
	}

	int sample_rate = pstAudioparam->sample_rate;
	unsigned int Chnsample_rate = pstAudioparam->Chnsample_rate;
	int channel = pstAudioparam->channel;
	int u32PtNumPerFrm = pstAudioparam->preiod_size;
	bool bVqe = pstAudioparam->bVqeOn;
	int record_time = pstAudioparam->record_time;
	PAYLOAD_TYPE_E enType =	_sample_audio_get_codec(
					pstAudioparam->codec);/*PT_G711A,PT_G711U,PT_G726 */

	register_inthandler();
	FILE *fpAi = fopen(pstAudioparam->filename, "ab+");

	if (!fpAi) {
		printf("fpAi open fail\n");
		return -1;
	}

//STEP 1:set ai and vqe attr
	AIO_ATTR_S AudinAttr;

	AudinAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)sample_rate;
	AudinAttr.u32ChnCnt = AudMaxChn;
	AudinAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	AudinAttr.u32EXFlag = 0;
	AudinAttr.u32FrmNum = 10; /* only use in bind mode */
	AudinAttr.u32PtNumPerFrm = u32PtNumPerFrm; /* sample_rate/fps */
	AudinAttr.u32ClkSel = 0;
	AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	/*if you want to use vqe ,ai chn must 2chn.*/
	/*if you don't need to use vqe, you can skip this step*/
	if (bVqe) {
		memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
		if (((AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_8000) ||
			(AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_16000)) &&
			channel == 2) {

			pstAiVqeTalkAttr->s32WorkSampleRate = AudinAttr.enSamplerate;
			_update_agc_anr_setting(pstAiVqeTalkAttr);
			_update_aec_setting(pstAiVqeTalkAttr);
		} else {
			printf("[error] AEC will need to setup record in to channel Count = 2\n");
			printf("[error] VQE only support on 8k/16k sample rate. current[%d]\n",
				AudinAttr.enSamplerate);
		}
	}

	s32Ret = CVI_AUDIO_INIT();
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}


//STEP 2:start ai and aenc
	s32Ret = CVI_AI_SetPubAttr(AiDev, &AudinAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AI_Enable(AiDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AI_EnableChn(AiDev, AiChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (bVqe == true) {
		s32Ret = CVI_AI_SetTalkVqeAttr(AiDev, AiChn, 0, 0,
					       &stAiVqeTalkAttr);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

		s32Ret = CVI_AI_EnableVqe(AiDev, AiChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}

	}

	if ((enType == PT_G711A || enType == PT_G711U) && (Chnsample_rate != 8000)) {
		Chnsample_rate = 8000;
		printf("G711 only support sr 8000,change to 8000.\n");
	}

	if ((Chnsample_rate != (unsigned int)AudinAttr.enSamplerate)) {
		s32Ret = CVI_AI_EnableReSmp(AiDev, AiChn, Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
		ReSam_flag = true;
	}

	AUDIO_FRAME_S stFrame;
	AEC_FRAME_S   stAecFrm;

	clock_gettime(CLOCK_MONOTONIC, &now);
	end.tv_sec = now.tv_sec + record_time;
	end.tv_nsec = now.tv_nsec;

	int s32OutputChnCnt = channel;

	while (running) {
		if (record_time) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			if (now.tv_sec > end.tv_sec ||
			    (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
				break;
		}

		s32Ret = CVI_AI_GetFrame(AiDev, AiChn, &stFrame, &stAecFrm, -1);
		if (s32Ret) {
			printf("[error] ai getframe error\n");
			break;
		}

		fwrite(stFrame.u64VirAddr[0], 1, stFrame.u32Len * s32OutputChnCnt * 2, fpAi);

		s32Ret = CVI_AI_ReleaseFrame(AiDev, AiChn, &stFrame, &stAecFrm);
		if (s32Ret) {
			printf("[error] ai releaseframe error\n");
			break;
		}
	}


ERROR1:
	if (bVqe == true)
		CVI_AI_DisableVqe(AiDev, AiChn);
	if (ReSam_flag == true)
		CVI_AI_DisableReSmp(AiDev, AiChn);
	CVI_AI_DisableChn(AiDev, AiChn);
ERROR2:
	CVI_AI_Disable(AiDev);
ERROR3:
	fclose(fpAi);
	CVI_AUDIO_DEINIT();

	return 0;
}

CVI_S32 SAMPLE_AUDIO_PLAY_PCM_FORMAT_FILE(void *argv)
{
	CVI_S32 s32Ret = 0;
	CVI_S32 AoChn = 0;
	CVI_S32 AoDev = 0;
	AIO_ATTR_S AudoutAttr;
	FILE *fpAo = NULL;
	int AudMaxChn = 3;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("pstAudioparam is null\n");
		return -1;
	}

	register_inthandler();
	CVI_AUDIO_INIT();

	if (_cvi_checkname_iswav(pstAudioparam->filename) == CVI_SUCCESS) {
		fpAo = audio_open_wavfile(pstAudioparam->filename,
					  (int *)&pstAudioparam->channel,
					  (int *)&pstAudioparam->sample_rate);
	} else
		fpAo = fopen(pstAudioparam->filename, "rb");

	if (!fpAo) {
		printf("fpAo open fail\n");
		return -1;
	}

	int channel = pstAudioparam->channel;
//STEP 1:set and enable ao
	AudoutAttr.u32ChnCnt = AudMaxChn;
	AudoutAttr.enSamplerate   = pstAudioparam->sample_rate;
	AudoutAttr.enSoundmode = (channel == 2 ? AUDIO_SOUND_MODE_STEREO :
				 AUDIO_SOUND_MODE_MONO);
	AudoutAttr.enWorkmode	  = AIO_MODE_I2S_MASTER;
	AudoutAttr.u32EXFlag	  = 0;
	AudoutAttr.u32FrmNum	  = 10; /* only use in bind mode */
	AudoutAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudoutAttr.u32PtNumPerFrm =
		pstAudioparam->preiod_size;/* 20*targetsamplerate/1000 */
	AudoutAttr.u32ClkSel	  = 0;
	AudoutAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	s32Ret = CVI_AO_SetPubAttr(AoDev, &AudoutAttr);
	if (s32Ret != CVI_SUCCESS) {
		printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}
	s32Ret = CVI_AO_Enable(AoDev);
	if (s32Ret != CVI_SUCCESS) {
		printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR3;
	}

	s32Ret = CVI_AO_EnableChn(AoDev, AoChn);
	if (s32Ret != CVI_SUCCESS) {
		printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
		goto ERROR2;
	}

	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate) {
		s32Ret = CVI_AO_EnableReSmp(AoDev, AoChn, pstAudioparam->Chnsample_rate);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
	}

	if (pstAudioparam->bVqeOn) {
		AO_VQE_CONFIG_S stVqeConfig;
		AO_VQE_CONFIG_S *pstVqeConfig = &stVqeConfig;

		_update_down_vqe_setting(pstVqeConfig, AudoutAttr.enSamplerate, channel);
		s32Ret = CVI_AO_SetVqeAttr(AoDev, AoChn, pstVqeConfig);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
		s32Ret = CVI_AO_EnableVqe(AoDev, AoChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("[cvi_error],[%s],[line:%d],\n", __func__, __LINE__);
			goto ERROR1;
		}
	}


	play_audio_file(&AudoutAttr, fpAo, AoChn, AoDev);


	if (pstAudioparam->Chnsample_rate != pstAudioparam->sample_rate)
		CVI_AO_DisableReSmp(AoDev, AoChn);

	if (pstAudioparam->bVqeOn)
		CVI_AO_DisableVqe(AoDev, AoChn);

ERROR1:
	CVI_AO_DisableChn(AoDev, AoChn);
ERROR2:
	CVI_AO_Disable(AoDev);
ERROR3:
	CVI_AUDIO_DEINIT();
	return 0;
}


CVI_S32 SAMPLE_AUDIO_DEBUG_SET_VOLUME( )
{
	printf("Enter %s\n", __func__);

	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 err;
	CVI_S32 idevid = 0;
	CVI_S32 volumedb = 0;
	CVI_S32 s32SetInputOrOutput = 0;

	printf("----------------------cvi check------------------------\n");
	printf("\n Enter output card id: \n");
	err = scanf("%d", &idevid);
	printf("\n Enter volume \n");
	err = scanf("%d", &volumedb);
	printf("\n enter card[%d] vol[%d]\n", idevid, volumedb);
	if (err == EOF)
		printf("[Error][%s][%d]\n", __func__, __LINE__);
	printf("\n Set [Ain]Volume:1  Set [Aout]Volume:0 ? [0 or 1]\n");
	err = scanf("%d", &s32SetInputOrOutput);
	printf("select [%d]\n", s32SetInputOrOutput);
	if (s32SetInputOrOutput == 0) {
		s32Ret = CVI_AO_SetVolume(idevid, volumedb);
		if (s32Ret != CVI_SUCCESS) {
			printf("[Error][%s][%d]\n", __func__, __LINE__);
			return CVI_FAILURE;
		}
	} else {
		s32Ret = CVI_AI_SetVolume(idevid, volumedb);
		if (s32Ret != CVI_SUCCESS) {
			printf("[Error][%s][%d]\n", __func__, __LINE__);
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

CVI_S32 SAMPLE_AUDIO_DEBUG_GET_VOLUME( )
{
	printf("Enter %s\n", __func__);
	CVI_S32 idevid = 0;
	CVI_S32 volume = 0;
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_S32 err;

	printf("Enter output card id: \n");
	err = scanf("%d", &idevid);
	if (err == EOF)
		printf("[Error][%s][%d]\n", __func__, __LINE__);
	printf("\n enter card[%d]\n", idevid);
	s32Ret = CVI_AO_GetVolume(idevid, &volume);
	printf("Get Volume Aout[%d]\n", volume);
	if (s32Ret != CVI_SUCCESS) {
		printf("[Error][%s][%d]\n", __func__, __LINE__);
		return CVI_FAILURE;
	}
	s32Ret = CVI_AI_GetVolume(idevid, &volume);
	printf("Get Volume Ain[%d]\n", volume);
	if (s32Ret != CVI_SUCCESS) {
		printf("[Error][%s][%d]\n", __func__, __LINE__);
		return CVI_FAILURE;
	}

	return CVI_SUCCESS;
}

void *VQE_RECORD_GET_FRAME(void *parg)
{

	AUDIO_FRAME_S stFrame;
	AEC_FRAME_S   stAecFrm;
	int s32Ret = 0;
	int s32ChnCnt = 2;

	ST_VQE_RECORD_TEST_STRUCT *pstVqeRecord = (ST_VQE_RECORD_TEST_STRUCT *)parg;

	s32ChnCnt = (pstVqeRecord->bVqe)?1:2;


	FILE *fp_rec = fopen("sample_record.raw", "wb");

	if (!fp_rec) {
		perror("sample_record.raw open fail");
		return NULL;
	}
	printf("[cvi_info] cap_loop:%d\n", pstVqeRecord->cap_loop);
	while (running && pstVqeRecord->cap_loop--) {

		s32Ret = CVI_AI_GetFrame(pstVqeRecord->AiDev, pstVqeRecord->AiChn,
					 &stFrame,
					 &stAecFrm,
					 -1);

		if (s32Ret != CVI_SUCCESS) {
			printf("[Error]CVI_AI_GetFrame none!!\n");
			break;
		}

		if (stFrame.u32Len == 0)
			printf("[cvi_warrn] block mode return size 0...\n");


		fwrite(stFrame.u64VirAddr[0], 1, (stFrame.u32Len * s32ChnCnt * 2), fp_rec);

		s32Ret = CVI_AI_ReleaseFrame(pstVqeRecord->AiDev, pstVqeRecord->AiChn,
					 &stFrame,
					 &stAecFrm);
		if (s32Ret) {
			printf("[error] ai releaseframe error\n");
			break;
		}
	}

	pstVqeRecord->record_status = AEC_LOOP_RECORD_STOP;
	fclose(fp_rec);
	return NULL;
}



void *VQE_PLAY_SEND_FRAME(void *parg)
{

	CVI_S32 num_readbytes = 0;
	AUDIO_FRAME_S stFrameSend;
	CVI_S32 s32Ret = CVI_SUCCESS;

	ST_VQE_PLAY_TEST_STRUCT *pstVqePlay = (ST_VQE_PLAY_TEST_STRUCT *)parg;

	CVI_S32 s32FrameBytes = pstVqePlay->ChnCnt * pstVqePlay->u32PtNumPerFrm * 2;

	char *pBuffer = malloc(s32FrameBytes);

	while (running && !pstVqePlay->stop_flag) {

		memset(pBuffer, 0, s32FrameBytes);
		num_readbytes = fread(pBuffer, 1, s32FrameBytes, pstVqePlay->fp_playfile);

		if (num_readbytes > 0) {
			stFrameSend.u64VirAddr[0] = (CVI_U8 *)pBuffer;
			stFrameSend.u32Len = pstVqePlay->u32PtNumPerFrm;//samples size for each channel
			stFrameSend.u64TimeStamp = 0;
			stFrameSend.enBitwidth = AUDIO_BIT_WIDTH_16;

			if (pstVqePlay->ChnCnt == 1)
				stFrameSend.enSoundmode = AUDIO_SOUND_MODE_MONO;
			else
				stFrameSend.enSoundmode = AUDIO_SOUND_MODE_STEREO;

			s32Ret = CVI_AO_SendFrame(pstVqePlay->AoDev, pstVqePlay->AoChn,
								(const AUDIO_FRAME_S *)&stFrameSend, 1000);
			if (s32Ret != CVI_SUCCESS)
				printf("[error] CVI_AO_SendFrame failed with %#x!\n", s32Ret);
		} else {
			printf("[cvi_info] End of the play !!\n");
			break;
		}
	}

	if (!pBuffer) {
		free(pBuffer);
		pBuffer = NULL;
	}
	if (!pstVqePlay->fp_playfile) {
		fclose(pstVqePlay->fp_playfile);
		pstVqePlay->fp_playfile = NULL;
	}
	return NULL;
}


CVI_S32 SAMPLE_AUDIO_AEC_LOOP_TEST(void *argv)
{
	AIO_ATTR_S	AudinAttr;
	AIO_ATTR_S AudoutAttr;
	CVI_S32 s32Ret = CVI_SUCCESS;
	pthread_t st_RecThread;
	pthread_t st_PlayThread;
	FILE *fp_play = NULL;
	int AoDev = 0;
	int AoChn = 0;
	int AiDev = 0;
	int AiChn = 0;
	int AudMaxChn = 3;
	stAudPara *pstAudioparam = (stAudPara *)argv;

	if (!pstAudioparam) {
		printf("pstAudioparam is null\n");
		return -1;
	}

	CVI_AUDIO_INIT();
	int channel = pstAudioparam->channel;

	AudinAttr.enSamplerate = pstAudioparam->sample_rate;
	AudinAttr.u32ChnCnt = AudMaxChn;
	AudinAttr.enSoundmode = (channel == 1)?AUDIO_SOUND_MODE_MONO:AUDIO_SOUND_MODE_STEREO;
	AudinAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudinAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	AudinAttr.u32EXFlag = 0;
	AudinAttr.u32FrmNum = 10; /* only use in bind mode */
	AudinAttr.u32PtNumPerFrm = pstAudioparam->preiod_size;
	AudinAttr.u32ClkSel = 0;
	AudinAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
	AI_TALKVQE_CONFIG_S *pstAiVqeTalkAttr = (AI_TALKVQE_CONFIG_S *)&stAiVqeTalkAttr;

	memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
	CVI_BOOL bVqeOn = true;

	bVqeOn = pstAudioparam->bVqeOn;
	printf("[cvi_info] VQE:%d\n", bVqeOn);

	if (bVqeOn == CVI_FALSE)
		pstAiVqeTalkAttr = NULL;
	else {
		if (((AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_8000) ||
			(AudinAttr.enSamplerate == AUDIO_SAMPLE_RATE_16000)) &&
			channel == 2) {

			pstAiVqeTalkAttr->s32WorkSampleRate = AudinAttr.enSamplerate;
			_update_agc_anr_setting(pstAiVqeTalkAttr);
			_update_aec_setting(pstAiVqeTalkAttr);
		} else {
			printf("[error] AEC will need to setup record in to channel Count = 2\n");
			printf("[error] VQE only support on 8k/16k sample rate. current[%d]\n",
				AudinAttr.enSamplerate);
		}
	}

	s32Ret = CVI_AI_SetPubAttr(AiDev, &AudinAttr);
	if (bVqeOn == CVI_TRUE) {
		s32Ret = CVI_AI_SetTalkVqeAttr(
				 AiDev,
				 0,
				 0,
				 0,
				 (AI_TALKVQE_CONFIG_S *)pstAiVqeTalkAttr);
		s32Ret = CVI_AI_EnableVqe(AiDev, AiChn);
		if (s32Ret != CVI_SUCCESS) {
			printf("%s: CVI_AI_EnableVqe(%d,0) failed\n",
			       __func__,
			       AiDev);
			return s32Ret;
		}

	}
	s32Ret = CVI_AI_Enable(AiDev);
	if (s32Ret == CVI_FAILURE)
		printf("[error] uplink audio setup failure\n");

	s32Ret = CVI_AI_EnableChn(AiDev, AiChn);

	ST_VQE_RECORD_TEST_STRUCT st_RecThreadStruct;

	st_RecThreadStruct.AiDev = AiDev;
	st_RecThreadStruct.AiChn = AiChn;
	st_RecThreadStruct.bVqe = bVqeOn;
	st_RecThreadStruct.cap_loop = (pstAudioparam->record_time * AudinAttr.enSamplerate)/AudinAttr.u32PtNumPerFrm;
	st_RecThreadStruct.ChnCnt = channel;
	st_RecThreadStruct.record_status = AEC_LOOP_RECORD_RUN;
	pthread_create(&st_RecThread, 0, VQE_RECORD_GET_FRAME,
		       &st_RecThreadStruct);
	pthread_detach(st_RecThread);

	AudoutAttr.u32ChnCnt = AudMaxChn;
	AudoutAttr.enSamplerate   = pstAudioparam->sample_rate;
	AudoutAttr.enSoundmode = (channel == 1)?AUDIO_SOUND_MODE_MONO:AUDIO_SOUND_MODE_STEREO;
	AudoutAttr.enWorkmode	  = AIO_MODE_I2S_MASTER;
	AudoutAttr.u32EXFlag	  = 0;
	AudoutAttr.u32FrmNum	  = 10; /* only use in bind mode */
	AudoutAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	AudoutAttr.u32PtNumPerFrm =
		pstAudioparam->preiod_size;/* 20*targetsamplerate/1000 */
	AudoutAttr.u32ClkSel	  = 0;
	AudoutAttr.enI2sType = AIO_I2STYPE_INNERCODEC;

	if (access(pstAudioparam->filename, 0) < 0) {
		printf("Error filename not exist\n");
		return CVI_FAILURE;

	} else {
		if (_cvi_checkname_iswav(pstAudioparam->filename) == CVI_SUCCESS) {
			fp_play = audio_open_wavfile(pstAudioparam->filename,
						(int *)&channel,
						(int *)&AudoutAttr.enSamplerate);

			if (!fp_play) {
				printf("[error] open fail\n");
				return -1;
			}
		} else {
			printf("[error] not wav\n");
			return -1;
		}
	}


	s32Ret |= CVI_AO_SetPubAttr(AoDev, &AudoutAttr);
	s32Ret |= CVI_AO_Enable(AoDev);
	s32Ret |= CVI_AO_EnableChn(AoDev, AoChn);
	if (s32Ret == CVI_FAILURE)
		printf("[Error]in downlink audio setup\n");

	ST_VQE_PLAY_TEST_STRUCT st_PlayThreadStruct;

	st_PlayThreadStruct.AoDev = AoDev;
	st_PlayThreadStruct.AoChn = AoChn;
	st_PlayThreadStruct.ChnCnt = channel;
	st_PlayThreadStruct.u32PtNumPerFrm = AudoutAttr.u32PtNumPerFrm;
	st_PlayThreadStruct.fp_playfile = fp_play;
	st_PlayThreadStruct.stop_flag = 0;

	pthread_create(&st_PlayThread, 0, VQE_PLAY_SEND_FRAME,
		       &st_PlayThreadStruct);
	pthread_detach(st_PlayThread);


	while (st_RecThreadStruct.record_status == AEC_LOOP_RECORD_RUN) {
		usleep(1000*30);
	}

	if (bVqeOn == CVI_FALSE)
		CVI_AI_DisableVqe(AiDev, AiChn);

	st_PlayThreadStruct.stop_flag = 1;
	CVI_AI_DisableChn(AiDev, AiChn);
	CVI_AI_Disable(AiDev);
	s32Ret = CVI_AO_DisableChn(AoDev, AoChn);
	s32Ret = CVI_AO_Disable(AoDev);
	CVI_AUDIO_DEINIT();

	return CVI_SUCCESS;
}
CVI_S32 SAMPLE_AUDIO_VERSION(CVI_VOID)
{
	return CVI_AUDIO_DEBUG();
}

CVI_S32 main(int argc, char *argv[])
{
	CVI_S32 s32Ret = CVI_SUCCESS;
	CVI_U32 u32Index = 0;
	stAudPara stAudioparam;

	if (argc  <  2) {
		printf_sample_usage();
		printf_parse_usage();
		return CVI_FAILURE;
	}

	u32Index = atoi(argv[1]);

	if (u32Index > 10) {
		printf_sample_usage();
		return CVI_FAILURE;
	}

	if (u32Index < 6 || u32Index == 10) {
		if (argc != 19) {
			printf_parse_usage();
			return CVI_FAILURE;
		}
		memset(&stAudioparam, 0, sizeof(stAudPara));
		get_audio_parse(argc, argv, &stAudioparam);
	}

	printf("[cvi_info] cvi_sample_audio:Enter command id =[%d]\n", u32Index);

	switch (u32Index) {
	case 0: {
		SAMPLE_AUDIO_AI_BIND_AENC(&stAudioparam);
		break;
	}
	case 1: {
		SAMPLE_AUDIO_AI_AENC_USER_GET_MODE(&stAudioparam);
		break;
	}
	case 2: {
		SAMPLE_AUDIO_ADEC_BIND_AO(&stAudioparam);
		break;
	}
	case 3: {
		SAMPLE_AUDIO_ADEC_AO_USER_GET_MODE(&stAudioparam);
		break;
	}
	case 4: {
		SAMPLE_AUDIO_RECORD_PCM_FORMAT_FILE(&stAudioparam);
		break;
	}
	case 5: {
		SAMPLE_AUDIO_PLAY_PCM_FORMAT_FILE(&stAudioparam);
		break;
	}
	case 6: {
		printf("[cviaudio] Set Volume!\n");
		SAMPLE_AUDIO_DEBUG_SET_VOLUME();
		printf("[cviaudio]SET VOLUME!...end\n");
		break;

	}
	case 7: {
		printf("[sample code]Cviaudio Version\n");
		SAMPLE_AUDIO_VERSION();
		break;
	}
	case 8: {
		printf("[cviaudio] Get Volume!\n");
		SAMPLE_AUDIO_DEBUG_GET_VOLUME();
		printf("[cviaudio]GET VOLUME!...end\n");
		break;
	}
	case 9: {
		printf("[cviaudio] ioctl_test!\n");
		printf("IOCTL Test is no longer supported\n");
		printf("[cviaudio] ioctl_test!...end\n");
		break;
	}
	case 10: {
		printf("[sample code]AEC self loop test\n");
		SAMPLE_AUDIO_AEC_LOOP_TEST(&stAudioparam);
		break;
	}
	default: {
		break;
	}
	}

	return s32Ret;
}















