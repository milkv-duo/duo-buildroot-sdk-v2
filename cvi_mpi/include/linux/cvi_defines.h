/*
 * Copyright (C) Cvitek Co., Ltd. 2024-2025. All rights reserved.
 *
 * File Name: cvi_defines.h
 * Description:
 *   The common definitions per chip capability.
 */
 /******************************************************************************         */

#ifndef __U_CVI_DEFINES_H__
#define __U_CVI_DEFINES_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define IS_CHIP_CV181X(x) (((x) == E_CHIPID_CV1820A) || ((x) == E_CHIPID_CV1821A) \
                        || ((x) == E_CHIPID_CV1822A) || ((x) == E_CHIPID_CV1823A) \
                        || ((x) == E_CHIPID_CV1825A) || ((x) == E_CHIPID_CV1826A) \
                        || ((x) == E_CHIPID_CV1810C) || ((x) == E_CHIPID_CV1811C) \
                        || ((x) == E_CHIPID_CV1812C) || ((x) == E_CHIPID_CV1811H) \
                        || ((x) == E_CHIPID_CV1812H) || ((x) == E_CHIPID_CV1813H))

#define IS_CHIP_CV180X(x) (((x) == E_CHIPID_CV1800B) || ((x) == E_CHIPID_CV1801B) \
                            || ((x) == E_CHIPID_CV1800C) || ((x) == E_CHIPID_CV1801C))

#define IS_CHIP_PKG_TYPE_QFN(x) (((x) == E_CHIPID_CV1820A) || ((x) == E_CHIPID_CV1821A) \
                        || ((x) == E_CHIPID_CV1822A) || ((x) == E_CHIPID_CV1810C) \
                        || ((x) == E_CHIPID_CV1811C) || ((x) == E_CHIPID_CV1812C))

#if defined(__CV181X__)
#include "cvi_cv181x_defines.h"
#elif defined(__CV180X__)
#include "cvi_cv180x_defines.h"
#else
#error "Unknown Chip Architecture!"
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __U_CVI_DEFINES_H__ */

