/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_audio_vqe_load.h
 * Description: audio vqe function define
 */
#ifndef __CVI_AUDIO_VQE_LOAD_H__
#define __CVI_AUDIO_VQE_LOAD_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <linux/cvi_type.h>
#include "cvi_comm_aio.h"

//customize parameter from algorithm client.h
extern AI_TALKVQE_CONFIG_S ZKT_VQE_CONFIG;
extern AI_TALKVQE_CONFIG_S CEOP_VQE_CONFIG;
extern AI_TALKVQE_CONFIG_S HT_VQE_CONFIG;
//api declaration
CVI_S32 CVI_AUD_VQE_SaveParamToCfg(FILE *fp, CVI_VOID *param);
CVI_S32 CVI_AUD_VQE_LoadParamFromCfg(FILE *fp, CVI_U8 *buf);

#ifdef __cplusplus
}
#endif
#endif

