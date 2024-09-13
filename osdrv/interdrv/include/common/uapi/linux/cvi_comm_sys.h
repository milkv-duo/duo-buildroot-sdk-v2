/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/cvi_comm_sys.h
 * Description:
 *   The common sys type defination.
 */

#ifndef __CVI_COMM_SYS_H__
#define __CVI_COMM_SYS_H__

#include <linux/cvi_comm_video.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define BIND_DEST_MAXNUM 32
#define BIND_NODE_MAXNUM 64

/**
 * total_size: total size for ion
 * free_size: total free size for ion
 * max_avail_size: max available size for ion
 */
typedef struct _ION_MM_STATICS_S {
	CVI_U64 total_size;
	CVI_U64 free_size;
	CVI_U64 max_avail_size;
} ION_MM_STATICS_S;

typedef struct _MMF_BIND_DEST_S {
	CVI_U32   u32Num;	/*	mmf bind dest cnt	*/
	MMF_CHN_S astMmfChn[BIND_DEST_MAXNUM];	/*	mmf bind dest chns	*/
} MMF_BIND_DEST_S;

typedef struct _BIND_NODE_S {
	CVI_BOOL bUsed;	/*	is bind node used	*/
	MMF_CHN_S src;	/*	bind node src chn	*/
	MMF_BIND_DEST_S dsts;	/*	bind node dest chns	*/
} BIND_NODE_S;

typedef enum _VI_VPSS_MODE_E {
	VI_OFFLINE_VPSS_OFFLINE = 0,	/*	vi offline vpss offline	*/
	VI_OFFLINE_VPSS_ONLINE,	/*	vi offline vpss online	*/
	VI_ONLINE_VPSS_OFFLINE,	/*	vi online vpss offline	*/
	VI_ONLINE_VPSS_ONLINE,	/*	vi online vpss online	*/
	VI_BE_OFL_POST_OL_VPSS_OFL,	/*	vi be offline post online vpss offline	*/
	VI_BE_OFL_POST_OFL_VPSS_OFL,	/*	vi be offline post offline vpss offline	*/
	VI_BE_OL_POST_OFL_VPSS_OFL,	/*	vi be online post offline vpss offline	*/
	VI_BE_OL_POST_OL_VPSS_OFL,	/*	vi be online post online vpss offline	*/
	VI_VPSS_MODE_BUTT
} VI_VPSS_MODE_E;


typedef struct _VI_VPSS_MODE_S {
	VI_VPSS_MODE_E aenMode[VI_MAX_PIPE_NUM];	/*	vi_vpss modes	*/
} VI_VPSS_MODE_S;

typedef enum _VPSS_MODE_E {
	VPSS_MODE_SINGLE = 0,	/*	vpss work as 1 device	*/
	VPSS_MODE_DUAL,	/*	vpss work as 2 device	*/
	VPSS_MODE_RGNEX,	/*	not supported now	*/
	VPSS_MODE_BUTT
} VPSS_MODE_E;

typedef enum _VPSS_INPUT_E {
	VPSS_INPUT_MEM = 0,	/*	vpss input from memory	*/
	VPSS_INPUT_ISP,	/*	vpss input from isp*/
	VPSS_INPUT_BUTT
} VPSS_INPUT_E;

typedef struct _VPSS_MODE_S {
	VPSS_MODE_E enMode;	/*	decide vpss work as 1/2 device	*/
	VPSS_INPUT_E aenInput[VPSS_IP_NUM];	/*	decide the input of each vpss device	*/
	VI_PIPE ViPipe[VPSS_IP_NUM];	/*	only meaningful if enInput is ISP	*/
} VPSS_MODE_S;

typedef struct _CVI_TDMA_2D_S {
	uint64_t paddr_src;	/*	physical address of src mem	*/
	uint64_t paddr_dst;	/*	physical address of dst mem	*/
	uint32_t w_bytes;	/*	width of mem	*/
	uint32_t h;	/*	height of mem	*/
	uint32_t stride_bytes_src;	/*	row span of src mem	*/
	uint32_t stride_bytes_dst;	/*	row span of dst mem	*/
} CVI_TDMA_2D_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif  /* __CVI_COMM_SYS_H__ */

