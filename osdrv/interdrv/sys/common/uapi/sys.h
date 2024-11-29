#ifndef __SYS_H__
#define __SYS_H__

#include <linux/types.h>
#include <linux/cdev.h>

#include <linux/cvi_comm_sys.h>
#include "base.h"
#include "ion/ion.h"
#include "ion/cvitek/cvitek_ion_alloc.h"

CVI_S32 sys_exit(void);
CVI_S32 sys_init(void);

CVI_S32 sys_ion_dump(void);
CVI_S32 sys_ion_alloc(CVI_U64 *p_paddr, void **pp_vaddr, CVI_U8 *buf_name, CVI_U32 buf_len, bool is_cached);
CVI_S32 sys_ion_alloc_nofd(CVI_U64 *p_paddr, void **pp_vaddr, CVI_U8 *buf_name, CVI_U32 buf_len, bool is_cached);
CVI_S32 sys_ion_free(CVI_U64 u64PhyAddr);
CVI_S32 sys_ion_free_nofd(CVI_U64 u64PhyAddr);
CVI_S32 sys_ion_get_memory_state(CVI_U64 *total_size, CVI_U64 *free_size, CVI_U64 *max_avail_size);

CVI_S32 sys_cache_invalidate(CVI_U64 addr_p, void *addr_v, CVI_U32 u32Len);
CVI_S32 sys_cache_flush(CVI_U64 addr_p, void *addr_v, CVI_U32 u32Len);

CVI_U32 sys_get_chipid(void);
CVI_U8 *sys_get_version(void);
CVI_S32 sys_get_bindbysrc(MMF_CHN_S *pstSrcChn, MMF_BIND_DEST_S *pstBindDest);
CVI_S32 sys_get_bindbydst(MMF_CHN_S *pstDestChn, MMF_CHN_S *pstSrcChn);

CVI_S32 sys_bind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn);
CVI_S32 sys_unbind(MMF_CHN_S *pstSrcChn, MMF_CHN_S *pstDestChn);

const CVI_U8 *sys_get_modname(MOD_ID_E id);
VPSS_MODE_E sys_get_vpssmode(void);

#if 0
#define SYS_IOCTL_BASE	'y'
#define SYS_ION_ALLOC		_IOWR(SYS_IOCTL_BASE, 0x01, unsigned long long)
#define SYS_ION_FREE		_IOW(SYS_IOCTL_BASE, 0x02, unsigned long long)
#define SYS_CACHE_INVLD	_IOW(SYS_IOCTL_BASE, 0x03, unsigned long long)
#define SYS_CACHE_FLUSH	_IOW(SYS_IOCTL_BASE, 0x04, unsigned long long)

#define SYS_INIT_USER		_IOW(SYS_IOCTL_BASE, 0x05, unsigned long long)
#define SYS_EXIT_USER		_IOW(SYS_IOCTL_BASE, 0x06, unsigned long long)
#define SYS_GET_SYSINFO	_IOR(SYS_IOCTL_BASE, 0x07, unsigned long long)

#define SYS_SET_MODECFG	_IOW(SYS_IOCTL_BASE, 0x08, unsigned long long)
#define SYS_GET_MODECFG	_IOR(SYS_IOCTL_BASE, 0x08, unsigned long long)
#define SYS_SET_BINDCFG	_IOW(SYS_IOCTL_BASE, 0x09, unsigned long long)
#endif

#endif  /* __SYS_H__ */

