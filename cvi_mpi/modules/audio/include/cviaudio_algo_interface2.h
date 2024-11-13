/* Include files */
#ifndef __CVIAUDIO_ALGO_INTERFACE2__
#define __CVIAUDIO_ALGO_INTERFACE2__
#include <stdio.h>

#ifndef AEC_PRO_DATA_LEN
#define AEC_PRO_DATA_LEN (160)
#endif

void *CviAud_DnAlgo_Init(int s32FunctMask, void *param_info);

int CviAud_DnAlgo_Process(void *pHandle, short *spk_in,
			short *spk_out, int iLength);

void CviAud_DnAlgo_DeInit(void *pHandle);

void  CviAud_DnAlgo_GetVersion(char *pstrVersion);

#endif
