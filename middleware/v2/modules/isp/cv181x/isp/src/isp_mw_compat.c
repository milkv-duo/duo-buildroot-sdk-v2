/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mw_compat.c
 * Description:
 *
 */

#include <unistd.h>

#include "isp_mw_compat.h"
#include "isp_defines.h"

#ifdef ARCH_RTOS_CV181X
#include "FreeRTOS.h"
#include "task.h"
#include "arch_helpers.h"
#endif

CVI_VOID *MW_COMPAT_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	UNUSED(u32Size);
	return ISP_PTR_CAST_VOID(u64PhyAddr);
}

CVI_VOID *MW_COMPAT_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size)
{
	UNUSED(u32Size);
	return ISP_PTR_CAST_VOID(u64PhyAddr);
}

CVI_S32 MW_COMPAT_usleep(CVI_U32 us)
{
	#ifndef ARCH_RTOS_CV181X
		usleep(us);
	#else
		vTaskDelay(pdMS_TO_TICKS(us / 1000));
	#endif

	return CVI_SUCCESS;
}

CVI_S32 MW_COMPAT_InvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
#ifndef ARCH_RTOS_CV181X
	return CVI_SYS_IonInvalidateCache(u64PhyAddr, pVirAddr, u32Len);
#else
	UNUSED(pVirAddr);
	inv_dcache_range(u64PhyAddr, u32Len);
	return CVI_SUCCESS;
#endif
}

CVI_S32 MW_COMPAT_FlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len)
{
#ifndef ARCH_RTOS_CV181X
	return CVI_SYS_IonFlushCache(u64PhyAddr, pVirAddr, u32Len);
#else
	UNUSED(pVirAddr);
	flush_dcache_range(u64PhyAddr, u32Len);
	return CVI_SUCCESS;
#endif
}

