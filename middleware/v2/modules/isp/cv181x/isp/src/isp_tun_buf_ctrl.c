/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_tun_buf_ctrl.c
 * Description:
 *
 */

#include "stdio.h"
#include "stdlib.h"
#include <errno.h>

#include "isp_main_local.h"
#include "isp_mw_compat.h"
#include "isp_ioctl.h"
#include "isp_defines.h"
#include "isp_mgr_buf.h"
#include "isp_tun_buf_ctrl.h"

#define MAX_TUNING_BUF_NUM 2

static CVI_VOID *fe_vir[VI_MAX_PIPE_NUM];
static CVI_VOID *be_vir[VI_MAX_PIPE_NUM];
static CVI_VOID *post_vir[VI_MAX_PIPE_NUM];

static struct isp_tun_buf_ctrl_runtime  *_get_tun_buf_ctrl_runtime(VI_PIPE ViPipe);

static CVI_S32 isp_tuningBuf_map(VI_PIPE ViPipe);
static CVI_S32 isp_tuningBuf_unmap(VI_PIPE ViPipe);
static CVI_S32 add_tun_buf_idx(VI_PIPE ViPipe);

//TODO@ST Set as private
struct cvi_vip_isp_fe_cfg *get_pre_fe_tuning_buf_addr(VI_PIPE ViPipe)
{
	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	return (struct cvi_vip_isp_fe_cfg *)(fe_vir[idx]);
}

struct cvi_vip_isp_be_cfg *get_pre_be_tuning_buf_addr(VI_PIPE ViPipe)
{
	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	return (struct cvi_vip_isp_be_cfg *)(be_vir[idx]);
}

struct cvi_vip_isp_post_cfg *get_post_tuning_buf_addr(VI_PIPE ViPipe)
{
	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	return (struct cvi_vip_isp_post_cfg *)(post_vir[idx]);
}

CVI_U32 get_tuning_buf_idx(VI_PIPE ViPipe)
{
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	// printf("[%s] idx(%d)\n", __FUNCTION__, pre_addr->tun_idx);
	return runtime->widx;
}

CVI_S32 isp_tun_buf_ctrl_init(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	if (!runtime->is_init) {
		runtime->widx = 1;
		runtime->ridx = 0;

		runtime->fe_addr = 0;
		runtime->be_addr = 0;
		runtime->post_addr = 0;
	}

#ifndef ARCH_RTOS_CV181X
	struct isp_tuning_cfg tun_buf_info;

	G_EXT_CTRLS_PTR(VI_IOCTL_GET_TUN_ADDR, &tun_buf_info);

	runtime->fe_addr = tun_buf_info.fe_addr[ViPipe];
	runtime->be_addr = tun_buf_info.be_addr[ViPipe];
	runtime->post_addr = tun_buf_info.post_addr[ViPipe];
#else
	if (!runtime->is_init) {
		// TODO: mason.zou
	}
#endif

	ret = isp_tuningBuf_map(ViPipe);

	runtime->is_init = CVI_TRUE;

	return ret;
}

CVI_S32 isp_tun_buf_ctrl_uninit(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	runtime->widx = 1;
	runtime->ridx = 0;

	ret = isp_tuningBuf_unmap(ViPipe);

	runtime->is_init = CVI_FALSE;

	return ret;
}

CVI_S32 isp_tun_buf_invalid(VI_PIPE ViPipe)
{
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	MW_COMPAT_InvalidateCache(runtime->fe_addr, fe_vir[idx], sizeof(struct cvi_vip_isp_fe_cfg));
	MW_COMPAT_InvalidateCache(runtime->be_addr, be_vir[idx], sizeof(struct cvi_vip_isp_be_cfg));
	MW_COMPAT_InvalidateCache(runtime->post_addr, post_vir[idx], sizeof(struct cvi_vip_isp_post_cfg));

	return CVI_SUCCESS;
}

CVI_S32 isp_tun_buf_flush(VI_PIPE ViPipe)
{
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	MW_COMPAT_FlushCache(runtime->fe_addr, fe_vir[idx], sizeof(struct cvi_vip_isp_fe_cfg));
	MW_COMPAT_FlushCache(runtime->be_addr, be_vir[idx], sizeof(struct cvi_vip_isp_be_cfg));
	MW_COMPAT_FlushCache(runtime->post_addr, post_vir[idx], sizeof(struct cvi_vip_isp_post_cfg));

	return CVI_SUCCESS;
}

CVI_S32 isp_tun_buf_ctrl_frame_done(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct cvi_vip_isp_fe_cfg *pre_fe_addr = get_pre_fe_tuning_buf_addr(ViPipe);
	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	pre_fe_addr->tun_update[tun_idx] = 1;
	pre_be_addr->tun_update[tun_idx] = 1;
	post_addr->tun_update[tun_idx] = 1;

	add_tun_buf_idx(ViPipe);

	isp_tun_buf_flush(ViPipe);

	return ret;
}

static CVI_S32 isp_tuningBuf_map(VI_PIPE ViPipe)
{
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	fe_vir[idx] = MMAP_CACHE_COMPAT(runtime->fe_addr, sizeof(struct cvi_vip_isp_fe_cfg));
	be_vir[idx] = MMAP_CACHE_COMPAT(runtime->be_addr, sizeof(struct cvi_vip_isp_be_cfg));
	post_vir[idx] = MMAP_CACHE_COMPAT(runtime->post_addr, sizeof(struct cvi_vip_isp_post_cfg));

	return CVI_SUCCESS;
}

static CVI_S32 isp_tuningBuf_unmap(VI_PIPE ViPipe)
{
#ifndef ARCH_RTOS_CV181X
	CVI_U8 idx = VIPIPE_TO_IDX(ViPipe);

	MUNMAP_COMPAT(fe_vir[idx], sizeof(struct cvi_vip_isp_fe_cfg));
	MUNMAP_COMPAT(be_vir[idx], sizeof(struct cvi_vip_isp_be_cfg));
	MUNMAP_COMPAT(post_vir[idx], sizeof(struct cvi_vip_isp_post_cfg));
#else
	UNUSED(ViPipe);
#endif
	return CVI_SUCCESS;
}

static CVI_S32 add_tun_buf_idx(VI_PIPE ViPipe)
{
	struct isp_tun_buf_ctrl_runtime *runtime = _get_tun_buf_ctrl_runtime(ViPipe);

	runtime->ridx = runtime->widx;
	runtime->widx = RING_ADD(runtime->widx, 1, MAX_TUNING_BUF_NUM);

	struct cvi_vip_isp_fe_cfg *pre_fe_tun_buf_addr;
	struct cvi_vip_isp_be_cfg *pre_be_tun_buf_addr;
	struct cvi_vip_isp_post_cfg *post_tun_buf_addr;

	pre_fe_tun_buf_addr = get_pre_fe_tuning_buf_addr(ViPipe);
	pre_be_tun_buf_addr = get_pre_be_tuning_buf_addr(ViPipe);
	post_tun_buf_addr = get_post_tuning_buf_addr(ViPipe);

	pre_fe_tun_buf_addr->tun_idx = runtime->ridx;
	pre_be_tun_buf_addr->tun_idx = runtime->ridx;
	post_tun_buf_addr->tun_idx = runtime->ridx;

	//printf("tun_idx %d\n", pre_cfg[ViPipe]->tun_idx);

	return CVI_SUCCESS;
}

static struct isp_tun_buf_ctrl_runtime  *_get_tun_buf_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_tun_buf_ctrl_runtime *runtime = CVI_NULL;

	isp_mgr_buf_get_tun_buf_ctrl_runtime_addr(ViPipe, (CVI_VOID *) &runtime);

	return runtime;
}
