/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_audio_vqe.h
 * Description: audio vqe function define
 */
#ifndef __CVI_AUDIO_VQE_H__
#define __CVI_AUDIO_VQE_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#if defined(__CV181X__) || defined(__CV180X__)
#include <linux/cvi_type.h>
#else
#include "cvi_type.h"
#endif
#include "cvi_comm_aio.h"


//control for AO_VQE
/*  AGC Control in SPK Path */
#define SPK_AGC_ENABLE 0x1  /* bit 0 */
#define SPK_EQ_ENABLE 0x2  /* bit 1 */

/* Extern variable for cvi_audio_interface.c */
typedef struct _AI_CHANNEL_VQE_CONFIG {
	AI_RECORDVQE_CONFIG_S reocrdvqe;
	AI_TALKVQE_CONFIG_S   talkvqe;
	CVI_BOOL benablevqe;
} AI_CHANNEL_VQE_CONFIG;


typedef struct _AO_VQE_CONFIG {
	AI_TALKVQE_CONFIG_S talkvqe;
	AI_RECORDVQE_CONFIG_S reocrdvqe;
	AO_VQE_CONFIG_S  stVqeConf;
	CVI_BOOL bAoVqeEnable;
	CVI_S32 s32BytesPerSample;
} AO_VQE_CONFIG;


typedef struct _AI_VQE_CONFIG {
	/* AI_CHANNEL_VQE_CONFIG  chnvqe[MAX_CHANNEL_NUM]; */
	AI_TALKVQE_CONFIG_S talkvqe;
	AI_RECORDVQE_CONFIG_S reocrdvqe;
	CVI_BOOL bEnableRefAEC;
	CVI_BOOL bAiVqeEnable;
	CVI_S32 s32BytesPerSample;
} AI_VQE_CONFIG;

#ifdef AUD_SUPPORT_KERNEL_MODE
typedef enum _E_VQE_PATH {
	E_VQE_USER_SPACE_LIB = 0,
	E_VQE_KERNEL_BLOCK_MODE = 1,
	E_VQE_KERNEL_CO_BUFF_MODE = 2,
	E_VQE_PATH_MAX
} E_VQE_ALGO_PATH;
#endif

extern AUDIO_VQE_REGISTER_S gstVqeReg;
//extern AI_VQE_CONFIG gstAiVQE;
// extern AO_VQE_CONFIG gstAoVQE;
extern CVI_S32 s32CurrAiSampleRate;
extern CVI_S32 s32CurrAoutSampleRate;


CVI_S32 CVI_AudIn_AlgoInit(const AI_TALKVQE_CONFIG_S *pstVqeConfig);
CVI_S32 CVI_AudIn_AlgoProcess_AnrAgc(CVI_CHAR *datain, CVI_CHAR *dataout,
	 CVI_S32 s32SizeInBytes,
	 CVI_S32 *s32SizeOutBytes);
CVI_S32 CVI_AudIn_AlgoProcess_AEC(CVI_CHAR *datain, CVI_CHAR *dataout,
	CVI_S32 s32SizeInBytes,
	CVI_S32 *s32SizeOutBytes);
CVI_S32 CVI_AudIn_AlgoDeInit(void);
CVI_S32 CVI_AudIn_AlgoFreeBuffer(void);
CVI_S32 CVI_AudIn_AlgoFunConfig(CVI_S32 u32OpenMask);
CVI_S32 CVI_AI_DevVQECheckEnable(AUDIO_DEV AiDevId);
CVI_BOOL CVI_AI_VQECheckEnable(AUDIO_DEV AiDevId, AI_CHN AiChn);
CVI_S32 CVI_AI_VQECheckFlag(AUDIO_DEV AiDevId, AI_CHN AiChn);
#ifdef CVIAUDIO_SOFTWARE_AEC
//software AEC interface
//Be aware that the software AEC performance only guaranteed when algorithm is ready .
//Otherwise, cvi_audio_vqe only provide the api interface for algorithm
CVI_S32 CVI_VQE_EnableSwAEC(CVI_S32 s32SampleRate, CVI_S32 s32CoutChannel, CVI_S32 perido_size);
CVI_S32 CVI_VQE_DisableSwAEC(void);
CVI_S32 CVI_VQE_CheckSwAECEnable(void);
CVI_S32 CVI_VQE_SwAECWrite(char *pbuffer, unsigned int size_bytes);//save the data need to playout to speaker
CVI_S32 CVI_VQE_SwAECRead(char *pbuffer, unsigned int size_bytes);//read the data for the mic in
CVI_S32 CVI_VQE_CloseSwAEC(void);
#endif
#ifdef __cplusplus
}
#endif
#endif

