/*
 * Copyright (c) 2015-2016, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_DEF_H__
#define __PLATFORM_DEF_H__

#if defined(CONFIG_TARGET_CVITEK_CV181X)
#include <../../../board/cvitek/cv181x/cv181x_reg.h>
#elif defined(CONFIG_TARGET_CVITEK_CV180X)
#include <../../../board/cvitek/cv180x/cv180x_reg.h>
#else
#error "use cvi_utask at wrong platform"
#endif

#endif /* __PLATFORM_DEF_H__ */
