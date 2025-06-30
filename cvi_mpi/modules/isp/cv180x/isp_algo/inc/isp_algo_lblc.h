/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_lblc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_LBLC_H_
#define _ISP_ALGO_LBLC_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

struct lblc_param_in {
	CVI_U32 iso;
	CVI_U16 iso_tbl_size;

	CVI_U32 iso_tbl[ISP_LBLC_ISO_SIZE];
	ISP_U16_PTR lblc_lut_r[ISP_LBLC_ISO_SIZE];
	ISP_U16_PTR lblc_lut_gr[ISP_LBLC_ISO_SIZE];
	ISP_U16_PTR lblc_lut_gb[ISP_LBLC_ISO_SIZE];
	ISP_U16_PTR lblc_lut_b[ISP_LBLC_ISO_SIZE];

	CVI_U16 strength;
};

struct lblc_param_out {
	ISP_U16_PTR lblc_lut_r;
	ISP_U16_PTR lblc_lut_gr;
	ISP_U16_PTR lblc_lut_gb;
	ISP_U16_PTR lblc_lut_b;
};

CVI_S32 isp_algo_lblc_main(struct lblc_param_in *lblc_param_in, struct lblc_param_out *lblc_param_out);
CVI_S32 isp_algo_lblc_init(void);
CVI_S32 isp_algo_lblc_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_lblc_H_
