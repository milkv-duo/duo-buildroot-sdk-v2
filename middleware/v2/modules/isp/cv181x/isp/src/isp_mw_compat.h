/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mw_compat.h
 * Description:
 *
 */

#ifndef _ISP_MW_COMPAT_H_
#define _ISP_MW_COMPAT_H_

#include "cvi_comm_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef ARCH_RTOS_CV181X
#define MMAP_COMPAT MW_COMPAT_Mmap
#define MMAP_CACHE_COMPAT MW_COMPAT_MmapCache
#define MUNMAP_COMPAT
#define INVALID_CACHE_COMPAT MW_COMPAT_InvalidateCache
#define FLUSH_CACHE_COMPAT MW_COMPAT_FlushCache
#else
#include "cvi_sys.h"
#define MMAP_COMPAT CVI_SYS_Mmap
#define MMAP_CACHE_COMPAT CVI_SYS_MmapCache
#define MUNMAP_COMPAT CVI_SYS_Munmap
#define INVALID_CACHE_COMPAT CVI_SYS_IonInvalidateCache
#define FLUSH_CACHE_COMPAT CVI_SYS_IonFlushCache
#endif

#define USLEEP_COMPAT MW_COMPAT_usleep

CVI_VOID *MW_COMPAT_Mmap(CVI_U64 u64PhyAddr, CVI_U32 u32Size);
CVI_VOID *MW_COMPAT_MmapCache(CVI_U64 u64PhyAddr, CVI_U32 u32Size);
CVI_S32 MW_COMPAT_Munmap(void *pVirAddr, CVI_U32 u32Size);
CVI_S32 MW_COMPAT_usleep(CVI_U32 us);
CVI_S32 MW_COMPAT_InvalidateCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len);
CVI_S32 MW_COMPAT_FlushCache(CVI_U64 u64PhyAddr, CVI_VOID *pVirAddr, CVI_U32 u32Len);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_MW_COMPAT_H_
