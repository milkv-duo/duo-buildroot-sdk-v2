/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*
* File Name: cviruntime_extra.h
* Description:
*/

#ifndef _CVIRUNTIME_EXTRA_H_
#define _CVIRUNTIME_EXTRA_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "cvitpu_debug.h"
#include "cviruntime.h"
#include "cviruntime_context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* CVI_KFUNC_HANDLE;

/*
 * Create tpu kernel function by given parameters.
 */
CVI_KFUNC_HANDLE CVI_NN_PrepareMatrixMulKernelFunc(
    CVI_RT_HANDLE ctx, CVI_FMT fmt, uint32_t m, uint32_t k, uint32_t n);
/*
 */
CVI_KFUNC_HANDLE CVI_NN_PrepareGrayImageLightKernelFunc(
    CVI_RT_HANDLE ctx, uint32_t ih, uint32_t iw, uint32_t kernel_sz);
/*
 * Run tpu kernel function
 */
CVI_RC CVI_NN_RunKernelFunc(CVI_KFUNC_HANDLE kfun, int32_t mem_num, ...);
/*
 * Destroy tpu kernel function
 */
CVI_RC CVI_NN_DestroyKernelFunc(CVI_KFUNC_HANDLE kfun);

#ifdef __cplusplus
}
#endif

#endif // _CVIRUNTIME_H_
