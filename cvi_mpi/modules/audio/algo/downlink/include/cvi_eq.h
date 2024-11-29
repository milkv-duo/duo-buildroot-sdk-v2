/**
 *Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 * \brief Application interface for cvitek audio
 * \author yike <ke.yi@cvitek.com>
 * \author cvitek
 * \date 2021
 */
#ifndef __CVITEK_EQ_H__
#define __CVITEK_EQ_H__
#include <stdio.h>
#include <stdint.h>
typedef void *EQ_HANDLE;


typedef struct CVI_ST_EQ_PARAMStag {
	int enable;
	int bandIdx;
	CVI_U32 freq;
	float QValue;
	float gainDb;
} CVI_ST_EQ_PARAMS, *CVI_ST_EQ_PARAMS_PTR;


EQ_HANDLE cvitek_eq_create(int samplerate, int channels);
int cvitek_eq_set_params(EQ_HANDLE EqHandle, CVI_ST_EQ_PARAMS_PTR pstEqParams);
int cvitek_eq_get_params(EQ_HANDLE EqHandle, CVI_ST_EQ_PARAMS_PTR pstEqParams);
int cvitek_eq_process(EQ_HANDLE EqHandle, float *data, int frames);
void cvitek_eq_destroy(EQ_HANDLE EqHandle);
int cvitek_eq_get_bind_cnt(void);

#endif //__CVITEK_EQ_H__
