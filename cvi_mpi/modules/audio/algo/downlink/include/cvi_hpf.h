/**
 *Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 * \brief Application interface for cvitek audio
 * \author yike <ke.yi@cvitek.com>
 * \author cvitek
 * \date 2021
 */
#ifndef __CVITEK_HPF_H__
#define __CVITEK_HPF_H__


typedef struct {
	int type;
	float f0;
	float Q;
	float gainDb;
} CVI_ST_FILTER_PARAM;

typedef void *CVI_HPF_HANDLE;
CVI_HPF_HANDLE cvitek_hpfilter_create(int samplerate, int channels, CVI_ST_FILTER_PARAM *pstFilterParams);
int cvitek_hpfilter_process(CVI_HPF_HANDLE handle, float *data, int frames);
void cvitek_hpfilter_destroy(CVI_HPF_HANDLE handle);
int cvitek_hpfilter_get_freq(CVI_HPF_HANDLE handle);
int cvitek_hpfilter_set_params(CVI_HPF_HANDLE handle, unsigned int freq);
int cvitek_hpfilter_get_freq(CVI_HPF_HANDLE handle);

#endif //__CVITEK_EQ_H__
