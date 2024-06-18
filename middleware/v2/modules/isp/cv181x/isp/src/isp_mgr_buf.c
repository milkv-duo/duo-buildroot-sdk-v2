/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_mgr_buf.c
 * Description:
 *
 */
#include "stdio.h"
#include "stdlib.h"
#include "stdint.h"
#include <inttypes.h>

#ifndef ARCH_RTOS_CV181X
#include "cvi_sys.h"
#else
#include "arch_helpers.h"
#endif

#include "isp_mgr_buf.h"
#include "isp_feature.h"
#include "isp_defines.h"
#include "rtos_isp_cmd.h"
#include "isp_sts_ctrl.h"

extern ISP_CTX_S *g_astIspCtx[VI_MAX_PIPE_NUM];
CVI_U8 g_ispCtxBufCnt;

struct isp_shared_buffer {
	struct rtos_isp_cmd_event eventCmd;

	struct isp_tun_buf_ctrl_runtime tun_buf;
	struct isp_sts_ctrl_runtime sts_ctrl;
	ISP_CMOS_NOISE_CALIBRATION_S np;
	struct isp_3a_shared_buffer isp_3a;

	/* ---------------------------------------------- */
	struct isp_mlsc_shared_buffer mlsc;
	struct isp_dpc_shared_buffer dpc;
	struct isp_dci_shared_buffer dci;
	struct isp_demosaic_shared_buffer demosaic;
	struct isp_fswdr_shared_buffer fswdr;
	struct isp_drc_shared_buffer drc;
	struct isp_ycur_shared_buffer ycur;
	struct isp_gamma_shared_buffer gamma;
	struct isp_ldci_shared_buffer ldci;
	struct isp_dehaze_shared_buffer dehaze;
	struct isp_lcac_shared_buffer lcac;
	struct isp_rgbcac_shared_buffer rgbcac;
	struct isp_tnr_shared_buffer tnr;
	struct isp_bnr_shared_buffer bnr;
	struct isp_presharpen_shared_buffer presharpen;
	struct isp_sharpen_shared_buffer sharpen;
	struct isp_blc_shared_buffer blc;
	struct isp_cnr_shared_buffer cnr;
	struct isp_ca_shared_buffer ca;
	struct isp_cac_shared_buffer cac;
	struct isp_motion_shared_buffer motion;
	struct isp_csc_shared_buffer csc;
	struct isp_crosstalk_shared_buffer crosstalk;
	struct isp_ca2_shared_buffer ca2;
	struct isp_clut_shared_buffer clut;
	struct isp_dis_shared_buffer dis;
	struct isp_wb_shared_buffer wb;
	struct isp_ccm_shared_buffer ccm;
	struct isp_ynr_shared_buffer ynr;
	struct isp_mono_shared_buffer mono;

	ISP_CTX_S ispCtx;
} ISP_ALIGNED(0x8);

struct isp_mgr_buf_runtime {
	CVI_U64 paddr;
	CVI_VOID *vaddr;
	CVI_U32 len;
};

#define ISP_SHARED_BUFFER_NAME "ISP_SHARED_BUFFER"

static struct isp_mgr_buf_runtime *mgr_buf_runtime[VI_MAX_PIPE_NUM];

static struct isp_mgr_buf_runtime  *_get_mgr_buf_runtime(VI_PIPE ViPipe);

#ifdef ARCH_RTOS_CV181X
extern CVI_VOID *g_pIspShardBuffer;
#endif

CVI_S32 isp_mgr_buf_init(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct rtos_isp_cmd_event *ev;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->paddr != CVI_NULL) {
		return CVI_SUCCESS;
	}

	g_ispCtxBufCnt++;

	runtime->len = sizeof(struct isp_shared_buffer);

	//printf("size,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
	//	sizeof(struct rtos_isp_cmd_event),
	//	sizeof(struct isp_tun_buf_ctrl_runtime),
	//	sizeof(struct isp_sts_ctrl_runtime),
	//	sizeof(struct isp_mlsc_shared_buffer),
	//	sizeof(struct isp_dpc_shared_buffer),
	//	sizeof(struct isp_dci_shared_buffer),
	//	sizeof(struct isp_demosaic_shared_buffer),
	//	sizeof(struct isp_ldci_shared_buffer),
	//	sizeof(struct isp_dis_shared_buffer),
	//	offsetof(struct isp_shared_buffer, ispCtx),
	//	sizeof(ISP_CTX_S),
	//	sizeof(struct isp_shared_buffer));

#ifndef ARCH_RTOS_CV181X
	CVI_CHAR ion_name[32];

	memset(ion_name, 0, sizeof(ion_name));
	snprintf(ion_name, sizeof(ion_name), "%s_%d", ISP_SHARED_BUFFER_NAME, ViPipe);

	if (CVI_SYS_IonAlloc_Cached(&(runtime->paddr), &(runtime->vaddr),
		ion_name, runtime->len) < 0) {
		printf("ISP Vipipe(%d) Allocate %s failed\n", ViPipe, ion_name);
		ret = CVI_FAILURE;
	} else {
		printf("ISP Vipipe(%d) Allocate pa(%#"PRIx64") va(0x%p) size(%d)\n",
			ViPipe, runtime->paddr, runtime->vaddr, runtime->len);
	}

	memset(runtime->vaddr, 0, runtime->len);

	ev = (struct rtos_isp_cmd_event *) ((CVI_U8 *)runtime->vaddr +
		offsetof(struct isp_shared_buffer, eventCmd));

	ev->frame_idx = offsetof(struct isp_shared_buffer, ispCtx);

	isp_mgr_buf_flush_cache(ViPipe);
	g_astIspCtx[ViPipe] = (ISP_CTX_S *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ispCtx));
#else
	runtime->paddr = (CVI_U64)g_pIspShardBuffer;
	runtime->vaddr = CVI_NULL;

	ev = (struct rtos_isp_cmd_event *) (runtime->paddr +
		offsetof(struct isp_shared_buffer, eventCmd));

	ev->frame_idx = offsetof(struct isp_shared_buffer, ispCtx);

	isp_mgr_buf_flush_cache(ViPipe);
	g_astIspCtx[ViPipe] = (ISP_CTX_S *) (runtime->paddr + offsetof(struct isp_shared_buffer, ispCtx));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_uninit(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

#ifndef ARCH_RTOS_CV181X
	printf("ISP Vipipe(%d) Free pa(%#"PRIx64") va(0x%p)\n", ViPipe,
		runtime->paddr, runtime->vaddr);
	if (CVI_SYS_IonFree(runtime->paddr, runtime->vaddr) < 0) {
		ISP_LOG_ERR("Vipipe(%d) destroy shared buffer failed\n", ViPipe);
		ret = CVI_FAILURE;
	}
#endif

	runtime->paddr = 0x0;
	runtime->vaddr = CVI_NULL;

	g_astIspCtx[ViPipe] = CVI_NULL;

	ISP_RELEASE_MEMORY(runtime);
	mgr_buf_runtime[ViPipe] = CVI_NULL;
	g_ispCtxBufCnt--;

	return ret;
}

CVI_S32 isp_mgr_buf_get_event_addr(VI_PIPE ViPipe, CVI_U64 *paddr, CVI_VOID **vaddr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

	*paddr = runtime->paddr + offsetof(struct isp_shared_buffer, eventCmd);
	*vaddr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, eventCmd));

	return ret;
}

CVI_S32 isp_mgr_buf_get_addr(VI_PIPE ViPipe, ISP_IQ_BLOCK_LIST_E block, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	switch (block) {
	case ISP_IQ_BLOCK_MLSC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, mlsc));
		break;
	case ISP_IQ_BLOCK_DPC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, dpc));
		break;
	case ISP_IQ_BLOCK_DCI:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, dci));
		break;
	case ISP_IQ_BLOCK_DEMOSAIC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, demosaic));
		break;
	case ISP_IQ_BLOCK_FUSION:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, fswdr));
		break;
	case ISP_IQ_BLOCK_DRC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, drc));
		break;
	case ISP_IQ_BLOCK_YCONTRAST:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ycur));
		break;
	case ISP_IQ_BLOCK_GAMMA:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, gamma));
		break;
	case ISP_IQ_BLOCK_LDCI:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ldci));
		break;
	case ISP_IQ_BLOCK_DEHAZE:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, dehaze));
		break;
	case ISP_IQ_BLOCK_LCAC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, lcac));
		break;
	case ISP_IQ_BLOCK_RGBCAC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, rgbcac));
		break;
	case ISP_IQ_BLOCK_3DNR:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, tnr));
		break;
	case ISP_IQ_BLOCK_BNR:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, bnr));
		break;
	case ISP_IQ_BLOCK_PREYEE:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, presharpen));
		break;
	case ISP_IQ_BLOCK_YEE:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, sharpen));
		break;
	case ISP_IQ_BLOCK_BLC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, blc));
		break;
	case ISP_IQ_BLOCK_CNR:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, cnr));
		break;
	case ISP_IQ_BLOCK_CA:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ca));
		break;
	case ISP_IQ_BLOCK_CAC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, cac));
		break;
	case ISP_IQ_BLOCK_MOTION:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, motion));
		break;
	case ISP_IQ_BLOCK_CSC:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, csc));
		break;
	case ISP_IQ_BLOCK_CROSSTALK:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, crosstalk));
		break;
	case ISP_IQ_BLOCK_CA2:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ca2));
		break;
	case ISP_IQ_BLOCK_CLUT:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, clut));
		break;
	case ISP_IQ_BLOCK_GMS:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, dis));
		break;
	case ISP_IQ_BLOCK_WBGAIN:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, wb));
		break;
	case ISP_IQ_BLOCK_MONO:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, mono));
		break;
	case ISP_IQ_BLOCK_CCM:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ccm));
		break;
	case ISP_IQ_BLOCK_YNR:
		*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ynr));
		break;
	default:
		ret = CVI_FAILURE;
		ISP_LOG_ERR("Vipipe(%d) unknown iq block(%d)\n", ViPipe, block);
		break;
	}
#else
	switch (block) {
	case ISP_IQ_BLOCK_MLSC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, mlsc));
		break;
	case ISP_IQ_BLOCK_DPC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, dpc));
		break;
	case ISP_IQ_BLOCK_DCI:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, dci));
		break;
	case ISP_IQ_BLOCK_DEMOSAIC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, demosaic));
		break;
	case ISP_IQ_BLOCK_FUSION:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, fswdr));
		break;
	case ISP_IQ_BLOCK_DRC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, drc));
		break;
	case ISP_IQ_BLOCK_YCONTRAST:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ycur));
		break;
	case ISP_IQ_BLOCK_GAMMA:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, gamma));
		break;
	case ISP_IQ_BLOCK_LDCI:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ldci));
		break;
	case ISP_IQ_BLOCK_DEHAZE:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, dehaze));
		break;
	case ISP_IQ_BLOCK_LCAC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, lcac));
		break;
	case ISP_IQ_BLOCK_RGBCAC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, rgbcac));
		break;
	case ISP_IQ_BLOCK_3DNR:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, tnr));
		break;
	case ISP_IQ_BLOCK_BNR:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, bnr));
		break;
	case ISP_IQ_BLOCK_PREYEE:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, presharpen));
		break;
	case ISP_IQ_BLOCK_YEE:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, sharpen));
		break;
	case ISP_IQ_BLOCK_BLC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, blc));
		break;
	case ISP_IQ_BLOCK_CNR:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, cnr));
		break;
	case ISP_IQ_BLOCK_CA:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ca));
		break;
	case ISP_IQ_BLOCK_CAC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, cac));
		break;
	case ISP_IQ_BLOCK_MOTION:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, motion));
		break;
	case ISP_IQ_BLOCK_CSC:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, csc));
		break;
	case ISP_IQ_BLOCK_CROSSTALK:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, crosstalk));
		break;
	case ISP_IQ_BLOCK_CA2:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ca2));
		break;
	case ISP_IQ_BLOCK_CLUT:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, clut));
		break;
	case ISP_IQ_BLOCK_GMS:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, dis));
		break;
	case ISP_IQ_BLOCK_WBGAIN:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, wb));
		break;
	case ISP_IQ_BLOCK_MONO:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, mono));
		break;
	case ISP_IQ_BLOCK_CCM:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ccm));
		break;
	case ISP_IQ_BLOCK_YNR:
		*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ynr));
		break;
	default:
		ret = CVI_FAILURE;
		ISP_LOG_ERR("Vipipe(%d) unknown iq block(%d)\n", ViPipe, block);
		break;
	}
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_get_ctx_addr(VI_PIPE ViPipe, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, ispCtx));
#else
	*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, ispCtx));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_get_tun_buf_ctrl_runtime_addr(VI_PIPE ViPipe, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, tun_buf));
#else
	*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, tun_buf));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_get_sts_ctrl_runtime_addr(VI_PIPE ViPipe, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, sts_ctrl));
#else
	*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, sts_ctrl));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_get_np_addr(VI_PIPE ViPipe, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, np));
#else
	*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, np));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_get_3a_addr(VI_PIPE ViPipe, CVI_VOID **addr)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifdef ARCH_RTOS_CV181X
	*addr = (CVI_VOID *) (runtime->paddr + offsetof(struct isp_shared_buffer, isp_3a));
#else
	*addr = (CVI_VOID *) ((CVI_U8 *)runtime->vaddr + offsetof(struct isp_shared_buffer, isp_3a));
#endif

	return ret;
}

CVI_S32 isp_mgr_buf_invalid_cache(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifndef ARCH_RTOS_CV181X
	ret = CVI_SYS_IonInvalidateCache(runtime->paddr, runtime->vaddr, runtime->len);
#else
	inv_dcache_range(runtime->paddr, runtime->len);
#endif
	return ret;
}

CVI_S32 isp_mgr_buf_flush_cache(VI_PIPE ViPipe) // Conflict with C906L !!!
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_mgr_buf_runtime *runtime = _get_mgr_buf_runtime(ViPipe);

#ifndef ARCH_RTOS_CV181X
	ret = CVI_SYS_IonFlushCache(runtime->paddr, runtime->vaddr, runtime->len);
#else
	flush_dcache_range(runtime->paddr, runtime->len);
#endif
	return ret;
}

static struct isp_mgr_buf_runtime  *_get_mgr_buf_runtime(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	UNUSED(ret);

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	if (mgr_buf_runtime[ViPipe] == CVI_NULL) {
		ISP_CREATE_RUNTIME(mgr_buf_runtime[ViPipe], struct isp_mgr_buf_runtime);
	}

	return mgr_buf_runtime[ViPipe];
}

