/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_ldci.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_LDCI_H_
#define _ISP_ALGO_LDCI_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct ldci_init_coef {
	CVI_U8	u8ImageSizeDivBy16x12;	// 8b
	CVI_U16	u16BlkSizeX;			// 13b
	CVI_U16	u16BlkSizeY;			// 13b
	CVI_U16	u16BlkSizeX1;			// 13b
	CVI_U16	u16BlkSizeY1;			// 13b
	CVI_U16	u16SubBlkSizeX;			// 13b
	CVI_U16	u16SubBlkSizeY;			// 13b
	CVI_U16	u16SubBlkSizeX1;		// 13b
	CVI_U16	u16SubBlkSizeY1;		// 13b
	CVI_U16	u16InterpNormUD;		// 16b
	CVI_U16	u16InterpNormUD1;		// 16b
	CVI_U16	u16SubInterpNormUD;		// 13b
	CVI_U16	u16SubInterpNormUD1;	// 13b
	CVI_U16	u16InterpNormLR;		// 16b
	CVI_U16	u16InterpNormLR1;		// 16b
	CVI_U16	u16SubInterpNormLR;		// 16b
	CVI_U16	u16SubInterpNormLR1;	// 16b
	CVI_U16	u16MeanNormX;			// 15b
	CVI_U16	u16MeanNormY;			// 14b
	CVI_U16	u16VarNormY;			// 14b
};

struct ldci_param_in {
	CVI_U8		u8GaussLPFSigma;
	CVI_U8		u8BrightContrastHigh;
	CVI_U8		u8BrightContrastLow;
	CVI_U8		u8DarkContrastHigh;
	CVI_U8		u8DarkContrastLow;
	CVI_U8		u8LumaPosWgt_Wgt;
	CVI_U8		u8LumaPosWgt_Sigma;
	CVI_U8		u8LumaPosWgt_Mean;
	ISP_VOID_PTR	pvIntMemory;
};

struct ldci_param_out {
	CVI_U16	u16IdxFilterNorm;			// 10b
	CVI_U16	u16VarFilterNorm;			// 9b
	CVI_U8	au8ToneCurveIdx[8];			// 4b
	CVI_U16	au16IdxFilterLut[15];		// 11b
	CVI_U16	au16ToneCurveLut[8][17];	// 10b
	CVI_U8	au8LumaWgtLut[33];			// 8b
	CVI_U16	au16VarFilterLut[6];		// 10b
};

CVI_S32 isp_algo_ldci_get_init_coef(int W, int H, struct ldci_init_coef *pldci_init_coef);
CVI_S32 isp_algo_ldci_get_internal_memory(CVI_U32 *memory_size);

CVI_S32 isp_algo_ldci_init(void);
CVI_S32 isp_algo_ldci_uninit(void);

CVI_S32 isp_algo_ldci_main(
	struct ldci_param_in *ldci_param_in, struct ldci_param_out *ldci_param_out);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_LDCI_H_
