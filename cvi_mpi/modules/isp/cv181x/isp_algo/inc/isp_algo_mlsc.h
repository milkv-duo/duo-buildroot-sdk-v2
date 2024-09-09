/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_mlsc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_MLSC_H_
#define _ISP_ALGO_MLSC_H_

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define LSC_GAIN_BASE				(9)

struct mlsc_param_in {
	CVI_U32 color_temperature;
	CVI_U32 color_temp_size;

	CVI_U16 color_temp_tbl[ISP_MLSC_COLOR_TEMPERATURE_SIZE];
	ISP_U16_PTR mlsc_lut_r[ISP_MLSC_COLOR_TEMPERATURE_SIZE];
	ISP_U16_PTR mlsc_lut_g[ISP_MLSC_COLOR_TEMPERATURE_SIZE];
	ISP_U16_PTR mlsc_lut_b[ISP_MLSC_COLOR_TEMPERATURE_SIZE];

	CVI_U32 grid_col;
	CVI_U32 grid_row;

	CVI_U16 strength;
	//CVI_U32 rsv[32];
};

struct mlsc_param_out {
	CVI_U32 temp_lower_idx;
	CVI_U32 temp_upper_idx;

	ISP_U16_PTR mlsc_lut_r;
	ISP_U16_PTR mlsc_lut_g;
	ISP_U16_PTR mlsc_lut_b;

	CVI_FLOAT mlsc_compensate_gain;
};

CVI_S32 isp_algo_mlsc_main(struct mlsc_param_in *mlsc_param_in, struct mlsc_param_out *mlsc_param_out);
CVI_S32 isp_algo_mlsc_init(void);
CVI_S32 isp_algo_mlsc_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_MLSC_H_
