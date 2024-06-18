/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dci.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DCI_H_
#define _ISP_ALGO_DCI_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct dci_param_in {
	ISP_U16_PTR pHist;
	ISP_U16_PTR pCntCurve;		// DCI curve
	ISP_U32_PTR pPreDCILut;	// IIR curve lut
	CVI_U32 dci_bins_num;
	CVI_U16 img_width;
	CVI_U16 img_height;
	CVI_U16 ContrastGain;
	CVI_U16 DciStrength;
	CVI_U16 DciGamma;
	CVI_U8 DciOffset;
	CVI_U8 BlcThr;
	CVI_U8 WhtThr;
	CVI_U16 BlcCtrl;
	CVI_U16 WhtCtrl;
	CVI_U32 Speed;
	CVI_U8 ToleranceY;
	CVI_U16 DciGainMax;
	CVI_U8 Method;
	CVI_BOOL bUpdateCurve;
	//CVI_U32 rsv[32];
};

struct dci_param_out {
	ISP_U16_PTR map_lut;
};

CVI_S32 isp_algo_dci_main(
	struct dci_param_in *dci_param_in, struct dci_param_out *dci_param_out);
CVI_S32 isp_algo_dci_init(void);
CVI_S32 isp_algo_dci_uninit(void);

CVI_S32 DCI_wbuf_alloc(uint32_t num);
CVI_S32 DCI_wbuf_free(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DCI_H_
