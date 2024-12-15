/**
 *Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 * \brief Application interface for cvitek audio
 * \author yike <ke.yi@cvitek.com>
 * \author cvitek
 * \date 2021
 */
#ifndef __CVITEK_DRC_H__
#define __CVITEK_DRC_H__
#include <stdio.h>
#include <stdint.h>

typedef void *DRC_HANDLE;


typedef struct CVI_ST_DRC_COMPRESSOR_PARAMtag {
	CVI_U32 attackTimeMs;
	CVI_U32 releaseTimeMs;
	CVI_U16 ratio;
	float thresholdDb;
} CVI_ST_DRC_COMPRESSOR_PARAM, *CVI_ST_DRC_COMPRESSOR_PARAM_PTR;

typedef struct {
	CVI_U32 attackTimeMs;
	CVI_U32 releaseTimeMs;
	float thresholdDb;
	float postGain;
} CVI_ST_DRC_LIMITER_PARAM, *CVI_ST_DRC_LIMITER_PARAM_PTR;

typedef struct {
	CVI_U32 attackTimeMs;
	CVI_U32 releaseTimeMs;
	CVI_U32 holdTimeMs;
	CVI_U16 ratio;
	float thresholdDb;
	float minDb;
} CVI_ST_DRC_EXPANDER_PARAM, *CVI_ST_DRC_EXPANDER_PARAM_PTR;

typedef enum {
	CVI_E_DRC_TYPE_COMPRESSOR,
	CVI_E_DRC_TYPE_LIMITER,
	CVI_E_DRC_TYPE_EXPANDER,
	CVI_E_DRC_TYPE_BUTT,
} CVI_E_DRC_TYPE;

DRC_HANDLE cvitek_drc_create(int channel, int samplerate, void *pDrcParams, CVI_E_DRC_TYPE eDrcType);
int cvitek_drc_process(DRC_HANDLE DrcHandle, float *data, int frames);
void cvitek_drc_destroy(DRC_HANDLE DrcHandle);
int cvitek_set_drc_params(DRC_HANDLE DrcHandle, void *pDrcParams, CVI_E_DRC_TYPE eDrcType);
int cvitek_get_drc_params(DRC_HANDLE DrcHandle, void *pDrcParams, CVI_E_DRC_TYPE eDrcType);
CVI_E_DRC_TYPE cvitek_get_drc_type(DRC_HANDLE DrcHandle);




#endif //__CVITEK_DRC_H__
