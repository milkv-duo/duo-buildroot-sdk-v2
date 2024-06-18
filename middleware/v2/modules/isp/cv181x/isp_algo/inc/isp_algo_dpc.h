/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_dpc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DPC_H_
#define _ISP_ALGO_DPC_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct {
	uint16_t *image;
	uint16_t width;
	uint16_t height;
	uint16_t stride; //unit is pixel, not byte
	uint16_t abs_thresh; //if you want to align hisi's behavior, you should set abs_thresh = (startThresh << 2)
	uint8_t dpType; //0: detect bright pixel, 1:detect dark pixel
	uint32_t table_max_size;
	uint32_t verbose; //0: no verbose, 1: print verbose, such as median, mean, stddev, abs_thresh, dev_thresh ...
	uint32_t debug; //if debug is not 0, will print detail information of pixel at
					//row=debug[bit 31:16] and column=debug[bit 0:15].
					//the column should in range [2, height-3]
} DPC_Input;

typedef struct {
	uint32_t *table; //for each element, bit[32:16] is row, bit[15:0] is column
	uint32_t table_size;
} DPC_Output;

struct dpc_param_in {
	//CVI_U32 rsv[32];
};

struct dpc_param_out {

};

CVI_S32 isp_algo_dpc_main(struct dpc_param_in *dpc_param_in, struct dpc_param_out *dpc_param_out);
CVI_S32 isp_algo_dpc_init(void);
CVI_S32 isp_algo_dpc_uninit(void);

// #define DPC_DEBUG
int32_t Bayer_12bit_2_16bit(uint8_t *bayerBuffer, uint16_t *outBuffer,
	uint16_t width, uint16_t height, uint16_t stride);
uint16_t DPC_Calib(const DPC_Input *input, DPC_Output *output);
#ifdef DPC_DEBUG
void DPC_PrintInput(const DPC_Input *input);
void DPC_PrintOutput(const DPC_Input *input, const DPC_Output *output);
#endif // DPC_DEBUG

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DPC_H_
