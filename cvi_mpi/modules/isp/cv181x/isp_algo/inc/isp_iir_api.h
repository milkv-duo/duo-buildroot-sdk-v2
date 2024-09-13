/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: isp_iir_api.h
 * Description:
 *
 */

#ifndef _ISP_IIR_API_H_
#define _ISP_IIR_API_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
	uint8_t		*pu8LutIn;
	uint32_t	*pu32LutHistory;
	uint8_t		*pu8LutOut;
	uint16_t	u16LutSize;
	uint16_t	u16IIRWeight;		// max 10b @ u8 input
} TIIR_U8_Ctrl;

typedef struct {
	uint16_t	*pu16LutIn;
	uint32_t	*pu32LutHistory;
	uint16_t	*pu16LutOut;
	uint16_t	u16LutSize;
	uint16_t	u16IIRWeight;		// max 10b @ u10 input
} TIIR_U10_Ctrl;

typedef struct {
	uint16_t	*pu16LutIn;
	uint32_t	*pu32LutHistory;
	uint16_t	*pu16LutOut;
	uint16_t	u16LutSize;
	uint16_t	u16IIRWeight;		// max 16b @ u16 input
} TIIR_U16_Ctrl;

int IIR_U8_Once(TIIR_U8_Ctrl *ptIIRCoef);
int IIR_U8_UpdateIIROutFromHistory(TIIR_U8_Ctrl *ptIIRCoef);

int IIR_U10_Once(TIIR_U10_Ctrl *ptIIRCoef);
int IIR_U10_UpdateIIROutFromHistory(TIIR_U10_Ctrl *ptIIRCoef);

int IIR_U16_Once(TIIR_U16_Ctrl *ptIIRCoef);
int IIR_U16_UpdateIIROutFromHistory(TIIR_U16_Ctrl *ptIIRCoef);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_IIR_API_H_
