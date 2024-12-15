#ifndef __CVI_AUDIO_DNVQE_H__
#define __CVI_AUDIO_DNVQE_H__
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

CVI_BOOL CVI_AO_VQECheckEnable(AUDIO_DEV AoDevId, AO_CHN AoChn);
CVI_S32 CVI_AudOut_AlgoProcess(CVI_CHAR *datain,
				CVI_CHAR *dataout,
				CVI_S32 s32SizeInBytes,
				CVI_S32 *s32SizeOutBytes);
#endif
