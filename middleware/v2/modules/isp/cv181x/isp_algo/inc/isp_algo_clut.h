/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_clut.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_CLUT_H_
#define _ISP_ALGO_CLUT_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define MAX_UPDATE_ITEM			(1024)

typedef uint32_t CLUT_DATA;
typedef uint32_t CLUT_ADDRESS;


// uint32_t lut[256][2] in driver
typedef struct {
	CLUT_ADDRESS addr;
	CLUT_DATA data;
} CLUT_UPDATE_ITEM;

typedef struct {
	uint32_t length;
	CLUT_UPDATE_ITEM items[MAX_UPDATE_ITEM];
} CLUT_UPDATE_LIST;

struct clut_param_in {
	CVI_BOOL is_table_update;
	CVI_BOOL is_updating;
	CVI_BOOL is_lut_update;

	CVI_U32 iso;

	ISP_U16_PTR ClutR;
	ISP_U16_PTR ClutG;
	ISP_U16_PTR ClutB;
	ISP_VOID_PTR saturation_attr;
	//const ISP_CLUT_SATURATION_ATTR_S *saturation_attr;
};

struct clut_param_out {
	CLUT_UPDATE_LIST updateList;
};

CVI_S32 isp_algo_clut_main(CVI_U32 ViPipe, struct clut_param_in *clut_param_in, struct clut_param_out *clut_param_out);
CVI_S32 isp_algo_clut_init(CVI_U32 ViPipe);
CVI_S32 isp_algo_clut_uninit(CVI_U32 ViPipe);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_CLUT_H_
