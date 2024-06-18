/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_sts_ctrl.c
 * Description:
 *
 */

#include "cvi_isp.h"
#include "cvi_awb.h"

#include "isp_main_local.h"
#include "isp_ioctl.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_dis_ctrl.h"
#include "isp_motion_ctrl.h"
#include "isp_sts_ctrl.h"
#include "isp_mw_compat.h"
#include "isp_mgr_buf.h"

#define AE_UNIT_SIZE 4

// typedef int ISP_HIST_EDGE_STATISTIC_S;

#define STATISTIC_MAX_SUM ((1 << 10) - 1)
#define AE_STS_DIV_MGC 4194304
#define AE_STS_DIV_WIDTH (13)
#define AE_STS_DIV_MAX ((1 << AE_STS_DIV_WIDTH) - 1)

struct isp_sts_ctrl_runtime *sts_ctrl_runtime[VI_MAX_PIPE_NUM];

//-----------------------------------------------------------------------------
//  statistics configuration
//-----------------------------------------------------------------------------
static CVI_S32 isp_sts_ctrl_set_ae_cfg(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_set_af_cfg(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_set_gms_cfg(VI_PIPE ViPipe);
//-----------------------------------------------------------------------------
//  statistics unpack
//-----------------------------------------------------------------------------
static CVI_S32 isp_sts_ctrl_set_ae_sts(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_unpack_ae_sts(VI_PIPE ViPipe, CVI_VOID *sts_in,
	CVI_VOID *ae_sts_out, CVI_VOID *awb_sts_out, ISP_CHANNEL_LIST_E chn, CVI_U32 ae_max);
static CVI_S32 isp_sts_ctrl_unpack_ae_hist_sts(VI_PIPE ViPipe, CVI_VOID *sts_ign, CVI_VOID *sts_out,
	ISP_CHANNEL_LIST_E chn, CVI_U32 ae_max);

static CVI_S32 isp_sts_ctrl_set_gms_sts(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_unpack_gms_sts(ISP_DIS_STATS_INFO *disStatsInfo, CVI_VOID *disBuf);

static CVI_S32 isp_sts_ctrl_set_af_sts(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_unpack_af_sts(VI_PIPE ViPipe, CVI_VOID *sts_in, CVI_VOID *sts_out);

static CVI_S32 isp_sts_ctrl_set_dci_sts(VI_PIPE ViPipe);

static CVI_S32 isp_sts_ctrl_initial_motion_sts(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_release_motion_sts(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_set_motion_sts(VI_PIPE ViPipe);

#ifndef ARCH_RTOS_CV181X
static CVI_S32 _isp_sts_ctrl_get_idx(VI_PIPE ViPipe, CVI_U32 id, CVI_U32 *idx);
static CVI_S32 _isp_sts_ctrl_put_idx(VI_PIPE ViPipe, CVI_U32 id);
#endif
static CVI_S32 _isp_sts_ctrl_mmap(VI_PIPE ViPipe);
static CVI_S32 _isp_sts_ctrl_mmap_get_paddr(VI_PIPE ViPipe);
static CVI_S32 _isp_sts_ctrl_mmap_get_vaddr(VI_PIPE ViPipe);
static CVI_S32 _isp_sts_ctrl_munmap(VI_PIPE ViPipe);
static CVI_S32 isp_sts_ctrl_ready(VI_PIPE ViPipe);


static struct isp_sts_ctrl_runtime **_get_sts_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_sts_ctrl_init(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = CVI_NULL;

	isp_mgr_buf_get_sts_ctrl_runtime_addr(ViPipe, (CVI_VOID *) &runtime);
	*runtime_ptr = runtime;

	if (!runtime->initialized) {

		memset(runtime, 0, sizeof(struct isp_sts_ctrl_runtime));

		isp_sts_ctrl_set_ae_cfg(ViPipe);
		// isp_sts_ctrl_set_af_cfg(ViPipe);
		// isp_sts_ctrl_set_gms_cfg(ViPipe);
	}

	runtime->initialized = CVI_TRUE;

	return ret;
}

CVI_S32 isp_sts_ctrl_exit(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	isp_sts_ctrl_release_motion_sts(ViPipe);
	ret = _isp_sts_ctrl_munmap(ViPipe);

	runtime->initialized = CVI_FALSE;
	runtime->mapped = CVI_FALSE;

	return ret;
}

CVI_S32 isp_sts_ctrl_set_pre_idx(VI_PIPE ViPipe, CVI_U8 stt_idx)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->pre_sts_idx = stt_idx;

	return ret;
}

CVI_S32 isp_sts_ctrl_set_post_idx(VI_PIPE ViPipe, CVI_U8 stt_idx)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->post_sts_idx = stt_idx;

	return ret;
}

CVI_S32 isp_sts_ctrl_pre_fe_eof(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	// if can get buffer only after streaming on
	if (runtime->mapped == CVI_FALSE) {
		_isp_sts_ctrl_mmap(ViPipe);
		isp_sts_ctrl_initial_motion_sts(ViPipe);
		runtime->mapped = CVI_TRUE;
	}

	return ret;
}

CVI_S32 isp_sts_ctrl_pre_be_eof(VI_PIPE ViPipe, CVI_U32 frame_cnt)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	// ISP_CTX_S *pstIspCtx = NULL;

	// ISP_GET_CTX(ViPipe, pstIspCtx);

	runtime->pre_frame_cnt = frame_cnt;

	if (isp_sts_ctrl_ready(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

#ifndef ARCH_RTOS_CV181X
	_isp_sts_ctrl_get_idx(ViPipe, VI_IOCTL_STS_GET, &(runtime->pre_sts_idx));
#else
	// TODO: mason.zou
#endif

//  TODO: mason.zou
//	if (pstIspCtx->ispProcCtrl.u32AFStatIntvl) {
//		if ((runtime->pre_frame_cnt % pstIspCtx->ispProcCtrl.u32AFStatIntvl) == 0) {
//			isp_sts_ctrl_set_af_sts(ViPipe);
//		}
//	}

#ifndef ARCH_RTOS_CV181X
	_isp_sts_ctrl_put_idx(ViPipe, VI_IOCTL_STS_PUT);
#else
	// TODO: mason.zou
#endif

	return ret;
}

CVI_S32 isp_sts_ctrl_post_eof(VI_PIPE ViPipe, CVI_U32 frame_cnt)
{
	ISP_LOG_DEBUG("%d(%d)+\n", ViPipe, frame_cnt);

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);

	runtime->post_frame_cnt = frame_cnt;

	if (isp_sts_ctrl_ready(ViPipe) != CVI_SUCCESS)
		return CVI_FAILURE;

#ifndef ARCH_RTOS_CV181X
	_isp_sts_ctrl_get_idx(ViPipe, VI_IOCTL_POST_STS_GET, &(runtime->post_sts_idx));
#else
	// TODO: mason.zou
#endif

	if (pstIspCtx->ispProcCtrl.u32AFStatIntvl) {
		if ((runtime->pre_frame_cnt % pstIspCtx->ispProcCtrl.u32AFStatIntvl) == 0) {
			isp_sts_ctrl_set_af_sts(ViPipe);
		}
	}

	if (pstIspCtx->ispProcCtrl.u32AEStatIntvl) {
		if ((runtime->pre_frame_cnt % pstIspCtx->ispProcCtrl.u32AEStatIntvl) == 0) {
			isp_sts_ctrl_set_ae_sts(ViPipe);
		}
	}

	isp_sts_ctrl_set_gms_sts(ViPipe);
	isp_sts_ctrl_set_dci_sts(ViPipe);
	isp_sts_ctrl_set_motion_sts(ViPipe);

#ifndef ARCH_RTOS_CV181X
	_isp_sts_ctrl_put_idx(ViPipe, VI_IOCTL_POST_STS_PUT);
#else
	// TODO: mason.zou
#endif

	isp_sts_ctrl_set_ae_cfg(ViPipe);
	isp_sts_ctrl_set_af_cfg(ViPipe);
	isp_sts_ctrl_set_gms_cfg(ViPipe);

	return ret;
}

// 1822 AE footprint
//  AE     : 6 * AE_ZONE_ROW * (256 / 8) =  2880 = 0x0B40 byte (Assume AE_ZONE_ROW = 15)
//  AE_HIST:                             =  8192 = 0x2000 byte
//  FACE_AE:                             =   128 = 0x0080 byte
//  TOTAL  : (AE + AE_HIST + FACE_AE) * 2= 22670 = 0x588E byte
CVI_S32 isp_sts_ctrl_get_ae_sts(VI_PIPE ViPipe, ISP_AE_STATISTICS_COMPAT_S **ae_sts)
{
	if (ae_sts == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*ae_sts = &(runtime->ae_sts);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_awb_sts(VI_PIPE ViPipe, ISP_CHANNEL_LIST_E chn, ISP_WB_STATISTICS_S **awb_sts)
{
	if (awb_sts == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*awb_sts = &(runtime->awb_sts[chn]);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_af_sts(VI_PIPE ViPipe, ISP_AF_STATISTICS_S **af_sts)
{
	if (af_sts == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*af_sts = &(runtime->af_sts);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_gms_sts(VI_PIPE ViPipe, ISP_DIS_STATS_INFO **disStatsInfo)
{
	if (disStatsInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*disStatsInfo = &(runtime->gms_sts);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_dci_sts(VI_PIPE ViPipe, ISP_DCI_STATISTICS_S **dciStatsInfo)
{
	if (dciStatsInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*dciStatsInfo = &(runtime->dci_sts);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_motion_sts(VI_PIPE ViPipe, ISP_MOTION_STATS_INFO **motionStatsInfo)
{
	if (motionStatsInfo == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*motionStatsInfo = &(runtime->motion_sts);

	return ret;
}

CVI_S32 isp_sts_ctrl_get_pre_be_frm_idx(VI_PIPE ViPipe, CVI_U32 *frame_cnt)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*frame_cnt = runtime->pre_frame_cnt;

	return ret;
}
//-----------------------------------------------------------------------------
//  statistics unpack
//-----------------------------------------------------------------------------
static CVI_S32 isp_sts_ctrl_set_ae_sts(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_isp_sts_mem *sts_mem = &runtime->sts_mem[runtime->post_sts_idx];
	struct cvi_vip_memblock *ae_sts[2] = {
		&sts_mem->ae_le,
		&sts_mem->ae_se
	};

	ISP_AE_STATISTICS_COMPAT_S *ae_sts_out = &(runtime->ae_sts);
	ISP_WB_STATISTICS_S *awb_sts = (ISP_WB_STATISTICS_S *)&(runtime->awb_sts);

	// 6  = CEIL(AE_ZONE_MAX_COL / 3);
	// 32 = bytes per window
	// 2  = ae & ir_ae interleaving
	CVI_U32 hist_ofs = 36 * 30 * 8;

	for (CVI_U32 chn = 0; chn < ISP_CHANNEL_MAX_NUM; chn++) {
		if (ae_sts[chn]->vir_addr && ae_sts_out) {
			INVALID_CACHE_COMPAT(ae_sts[chn]->phy_addr, ae_sts[chn]->vir_addr, ae_sts[chn]->size);
			isp_sts_ctrl_unpack_ae_sts(ViPipe, ae_sts[chn]->vir_addr, ae_sts_out, &(awb_sts[chn]), chn, 0);
			// isp_sts_ctrl_unpack_ae_face_sts(ViPipe);
			isp_sts_ctrl_unpack_ae_hist_sts(ViPipe, (ae_sts[chn]->vir_addr + hist_ofs), ae_sts_out, chn, 0);
		}
	}

	return ret;
}

static CVI_S32 isp_sts_ctrl_unpack_ae_sts(VI_PIPE ViPipe, CVI_VOID *sts_in,
	CVI_VOID *ae_sts_out, CVI_VOID *awb_sts_out, ISP_CHANNEL_LIST_E chn, CVI_U32 ae_max)
{
	// ae_grp:
	//  7  6  5  4  3  2  1  0
	// g2 b2 b2 b2 b2 b2 b2 b2
	// r2 r2 g2 g2 g2 g2 g2 g2
	// b1 b1 b1 r2 r2 r2 r2 r2
	// g1 g1 g1 g1 b1 b1 b1 b1
	// r1 r1 r1 r1 r1 g1 g1 g1
	// b0 b0 b0 b0 b0 b0 r1 r1
	// g0 g0 g0 g0 g0 g0 g0 b0
	// xx r0 r0 r0 r0 r0 r0 r0

	// stride = 17 (AE_MAX_ZONE_COLUMN) / 3 (grp_num) * 2 (ae + ir_ae)
	// ae_win0 ir_win0 ae_win1 ir_win1 ... ae_win(17/3) ir_win(17/3)
	// | ae_g00 | ir_g00 | ae_g01 | ir_g01 | ae_g02 | ir_g02 | ae_g03 | ir_g03 | ae_g04 | ir_g04 | ae_g05 | ir_g05 |
	//  -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- -------- --------
	// | ae_g10 | ir_g10 | ae_g11 | ir_g11 | ae_g12 | ir_g12 | ae_g13 | ir_g13 | ae_g14 | ir_g14 | ae_g15 | ir_g15 |
	// ...

	typedef union ae {
		CVI_U8 raw[8];
		struct {
			CVI_U8 winIdx;
			CVI_U8 winOver;
			CVI_U16 winbData;
			CVI_U16 wingData;
			CVI_U16 winrData;
		} bytes;
	} aeDataUnit;

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	// window information
	CVI_U32 win_num_col = AE_ZONE_COLUMN;
	CVI_U32 win_num_row = AE_ZONE_ROW;
	CVI_U32 win_grp_remain  = win_num_col % AE_UNIT_SIZE;
	CVI_U32 win_grp_num_col = win_num_col / AE_UNIT_SIZE;
	CVI_U32 win_idx_in  = 0;
	CVI_U32 ae_win_idx_out = 0;
	CVI_U32 awb_win_idx_out = 0;

	// Pointer casting
	ISP_AE_STATISTICS_COMPAT_S *ae_sts = (ISP_AE_STATISTICS_COMPAT_S *)ae_sts_out;
	aeDataUnit *pAeData     = (aeDataUnit *)sts_in;
	aeDataUnit *ae_data_raw = NULL;
	CVI_U16 *ae_data       = NULL;
	CVI_U16 *aa_stt_info   = NULL;

	ISP_WB_STATISTICS_S *awb_sts = (ISP_WB_STATISTICS_S *)awb_sts_out;

	CVI_U32 sum_r, sum_g, sum_b;

	win_idx_in = 0;
	sum_r = sum_g = sum_b = 0;
	for (CVI_U8 ridx = 0 ; ridx < win_num_row ; ridx++) {
		for (CVI_U8 cidx = 0 ; cidx < win_grp_num_col ; cidx++) {
			ae_win_idx_out = (cidx+1) * AE_UNIT_SIZE;
			awb_win_idx_out = (ridx * AE_ZONE_COLUMN) + (cidx+1) * AE_UNIT_SIZE;
			for (CVI_U8 gcidx = AE_UNIT_SIZE ; gcidx > 0 ; gcidx--) {
				ae_data_raw = (aeDataUnit *)&pAeData[win_idx_in];
				ae_data     = (CVI_U16 *)ae_data_raw->raw;

				// CVI_U16 over = (ae_data[0] & 0xff00) >> 8;
				// CVI_U16 idx  = (ae_data[0] & 0xff);
				CVI_U16 b = ae_data[1];
				CVI_U16 g = ae_data[2];
				CVI_U16 r = ae_data[3];

				if (runtime->out_of_ae_sts_div_bit) {
					b <<= runtime->out_of_ae_sts_div_bit;
					g <<= runtime->out_of_ae_sts_div_bit;
					r <<= runtime->out_of_ae_sts_div_bit;
				}

				// printf("[%d][%d] %d %d %d %d\n", win_idx_in, idx, over, r, g, b);

				aa_stt_info =
					(CVI_U16 *)&(ae_sts->aeStat3[ae_max].au16ZoneAvg[chn][ridx][--ae_win_idx_out]);
				aa_stt_info[3] = r;
				aa_stt_info[2] = g;
				aa_stt_info[1] = g;
				aa_stt_info[0] = b;

				awb_win_idx_out--;
				awb_sts->au16ZoneAvgR[awb_win_idx_out] = r;
				awb_sts->au16ZoneAvgG[awb_win_idx_out] = g;
				awb_sts->au16ZoneAvgB[awb_win_idx_out] = b;

				sum_r += r;
				sum_g += g;
				sum_b += b;

				win_idx_in++;
			}
		}
		win_idx_in += AE_UNIT_SIZE - win_grp_remain;
		ae_win_idx_out = win_grp_num_col * AE_UNIT_SIZE + win_grp_remain;
		awb_win_idx_out = (ridx * AE_ZONE_COLUMN) + win_grp_num_col * AE_UNIT_SIZE + win_grp_remain;
		for (CVI_U8 gcidx = win_grp_remain ; gcidx > 0 ; gcidx--) {
			ae_data_raw = (aeDataUnit *)&pAeData[win_idx_in];
			ae_data     = (CVI_U16 *)ae_data_raw->raw;

			// CVI_U16 over = (ae_data[0] & 0xff00) >> 8;
			// CVI_U16 idx  = (ae_data[0] & 0xff);
			CVI_U16 b = ae_data[1];
			CVI_U16 g = ae_data[2];
			CVI_U16 r = ae_data[3];

			if (runtime->out_of_ae_sts_div_bit) {
				b <<= runtime->out_of_ae_sts_div_bit;
				g <<= runtime->out_of_ae_sts_div_bit;
				r <<= runtime->out_of_ae_sts_div_bit;
			}

			// printf("[%d][%d] %d %d %d %d\n", win_idx_in, idx, over, r, g, b);

			aa_stt_info = (CVI_U16 *)&(ae_sts->aeStat3[ae_max].au16ZoneAvg[chn][ridx][--ae_win_idx_out]);
			aa_stt_info[3] = r;
			aa_stt_info[2] = g;
			aa_stt_info[1] = g;
			aa_stt_info[0] = b;

			awb_win_idx_out--;
			awb_sts->au16ZoneAvgR[awb_win_idx_out] = r;
			awb_sts->au16ZoneAvgG[awb_win_idx_out] = g;
			awb_sts->au16ZoneAvgB[awb_win_idx_out] = b;

			sum_r += r;
			sum_g += g;
			sum_b += b;

			win_idx_in++;
		}
	}

	ae_sts->aeStat1[ae_max].u32PixelCount[chn] = 1920 * 1080;
	// TODO@CF. Don't know how to set weight.
	ae_sts->aeStat1[ae_max].u32PixelWeight[chn] = 1920 * 1080;

	ae_sts->aeStat2[ae_max].u16GlobalAvgB[chn] = sum_r / AE_ZONE_NUM;
	ae_sts->aeStat2[ae_max].u16GlobalAvgGr[chn] =
	ae_sts->aeStat2[ae_max].u16GlobalAvgGb[chn] = sum_g / AE_ZONE_NUM;
	ae_sts->aeStat2[ae_max].u16GlobalAvgR[chn] = sum_b / AE_ZONE_NUM;

	awb_sts->u16GlobalR = sum_r / AWB_ZONE_NUM;
	awb_sts->u16GlobalG = sum_g / AWB_ZONE_NUM;
	awb_sts->u16GlobalB = sum_b / AWB_ZONE_NUM;

	return CVI_SUCCESS;
}

static CVI_S32 isp_sts_ctrl_unpack_ae_hist_sts(VI_PIPE ViPipe, CVI_VOID *sts_in, CVI_VOID *sts_out,
	ISP_CHANNEL_LIST_E chn, CVI_U32 ae_max)
{
	CVI_U32 i;

	typedef struct hist {
		CVI_U32 hist0Data[4];
		CVI_U32 rsv[4];
	} histDataUnit;

	if (sts_in == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (sts_out == CVI_NULL) {
		return CVI_FAILURE;
	}

	histDataUnit *pHistData = (histDataUnit *)sts_in;
	ISP_AE_STATISTICS_COMPAT_S *ae_sts = (ISP_AE_STATISTICS_COMPAT_S *)sts_out;
	CVI_U32 *hist_g = (CVI_U32 *)ae_sts->aeStat1[ae_max].au32HistogramMemArray[chn];

	for (i = 0; i < MAX_HIST_BINS; i++) {
		// Shift 4 bits to sync 183x sytle for IQ debug
		hist_g[i] = (((pHistData + i)->hist0Data[1]) << 4);
	}

	UNUSED(ViPipe);

	return CVI_SUCCESS;
}

static CVI_S32 isp_sts_ctrl_set_af_sts(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_isp_sts_mem *sts_mem = &runtime->sts_mem[runtime->post_sts_idx];
	CVI_U8 *af_sts_in = sts_mem->af.vir_addr;
	ISP_AF_STATISTICS_S *af_sts_out = &(runtime->af_sts);

	if (af_sts_in && af_sts_out) {
		INVALID_CACHE_COMPAT(sts_mem->af.phy_addr, sts_mem->af.vir_addr, sts_mem->af.size);
		isp_sts_ctrl_unpack_af_sts(ViPipe, af_sts_in, af_sts_out);
	}

	return ret;
}

static CVI_S32 isp_sts_ctrl_unpack_af_sts(VI_PIPE ViPipe, CVI_VOID *sts_in, CVI_VOID *sts_out)
{
#define AF_DATA_UNIT_SIZE 32
	if (sts_in == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (sts_out == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U32 win_num_col = AF_ZONE_COLUMN;
	CVI_U32 win_num_row = AF_ZONE_ROW;

	CVI_U16 *pAfData = (CVI_U16 *)sts_in;
	ISP_AF_STATISTICS_S *af_sts = (ISP_AF_STATISTICS_S *)sts_out;

	for (CVI_U32 row = 0 ; row < win_num_row ; row++) {
		for (CVI_U32 col = 0 ; col < win_num_col ; col++) {
			af_sts->stFEAFStat.stZoneMetrics[row][col].u16HlCnt = *pAfData;
			af_sts->stFEAFStat.stZoneMetrics[row][col].u32v0 =
				(*(pAfData + 2) << 16) + *(pAfData + 1);
			af_sts->stFEAFStat.stZoneMetrics[row][col].u64h1 =
				(*(pAfData + 4) << 16)
				+ *(pAfData + 3);
			af_sts->stFEAFStat.stZoneMetrics[row][col].u64h0 =
				(*(pAfData + 6) << 12)
				+ ((*(pAfData + 5) & 0xFFF0) >> 4);
			pAfData += (AF_DATA_UNIT_SIZE / sizeof(CVI_U16));
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 isp_sts_ctrl_set_dci_sts(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_U8 *dci_sts_in = runtime->sts_mem[runtime->post_sts_idx].dci.vir_addr;
	ISP_DCI_STATISTICS_S *dci_sts_out = &(runtime->dci_sts);

	if (dci_sts_in && dci_sts_out) {
		memcpy(dci_sts_out, dci_sts_in, sizeof(CVI_U16) * DCI_BINS_NUM);
	}

	return ret;
}

static CVI_S32 isp_sts_ctrl_set_gms_sts(VI_PIPE ViPipe)
{
	if (ViPipe != 0)
		return CVI_FAILURE;

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_isp_sts_mem *sts_mem = &runtime->sts_mem[runtime->pre_sts_idx];
	CVI_U8 *gms_sts_in = sts_mem->gms.vir_addr;
	ISP_DIS_STATS_INFO *gms_sts_out = &(runtime->gms_sts);

	gms_sts_out->frameCnt = runtime->pre_frame_cnt;

	if (gms_sts_in && gms_sts_out) {
		INVALID_CACHE_COMPAT(sts_mem->gms.phy_addr, sts_mem->gms.vir_addr, sts_mem->gms.size);
		isp_sts_ctrl_unpack_gms_sts(gms_sts_out, gms_sts_in);
	}

	return ret;
}

static CVI_S32 isp_sts_ctrl_set_motion_sts(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_isp_sts_mem *sts_mem = &runtime->sts_mem[runtime->post_sts_idx];
	CVI_U8 *motion_sts_in = sts_mem->mmap.vir_addr;
	ISP_MOTION_STATS_INFO *motion_sts_out = &(runtime->motion_sts);

	if (motion_sts_in && motion_sts_out) {
		INVALID_CACHE_COMPAT(sts_mem->mmap.phy_addr, sts_mem->mmap.vir_addr, sts_mem->mmap.size);
		memcpy(ISP_PTR_CAST_VOID(motion_sts_out->motionStsData),
			motion_sts_in, motion_sts_out->motionStsBufSize);
	}

	struct cvi_isp_mmap_grid_size m_gd_sz;

	m_gd_sz.raw_num = ViPipe;
	S_EXT_CTRLS_PTR(VI_IOCTL_MMAP_GRID_SIZE, &m_gd_sz);

	motion_sts_out->gridWidth = m_gd_sz.grid_size;
	motion_sts_out->gridHeight = m_gd_sz.grid_size;
	motion_sts_out->frameCnt = runtime->post_frame_cnt;
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static CVI_S32 isp_sts_ctrl_unpack_gms_sts(ISP_DIS_STATS_INFO *disStatsInfo, CVI_VOID *disBuf)
{
	#if 0
	typedef struct dis {
		CVI_U8  yBlkIdx;
		CVI_U8  yColIdx;
		CVI_U16 yRsv;
		CVI_U16 histY[6];
		CVI_U8  xBlkIdx;
		CVI_U8  xColIdx;
		CVI_U16 xRsv;
		CVI_U16 histX[6];
	} disDataUnit;

	disDataUnit *disData = (disDataUnit *)disBuf;
	CVI_U32 *u32ptr = (CVI_U32 *)disData;
	#endif

	CVI_U32 *u32ptr = (CVI_U32 *)disBuf;

	CVI_U32 val0, val1, val2;

	CVI_U32 section_1_end = 0;
	CVI_U32 section_2_end = 0;
	CVI_U32 section_2_start = 0;

	if (XHIST_LENGTH >= YHIST_LENGTH) {
		section_1_end = (YHIST_LENGTH>>1);
		section_2_start = section_1_end;
		section_2_end = (XHIST_LENGTH>>1);
	} else {
		section_1_end = (XHIST_LENGTH>>1);
		section_2_start = section_1_end;
		section_2_end = (YHIST_LENGTH>>1);
	}

	for (CVI_U32 row = 0 ; row < DIS_MAX_WINDOW_X_NUM ; row++) {

		CVI_U16 *histX0prt = (disStatsInfo->histX[row][0]);
		CVI_U16 *histX1prt = (disStatsInfo->histX[row][1]);
		CVI_U16 *histX2prt = (disStatsInfo->histX[row][2]);
		CVI_U16 *histY0prt = (disStatsInfo->histY[row][0]);
		CVI_U16 *histY1prt = (disStatsInfo->histY[row][1]);
		CVI_U16 *histY2prt = (disStatsInfo->histY[row][2]);

		for (CVI_U32 col = 0 ; col < section_1_end ; col++) {
			u32ptr++;

			val0 = *(u32ptr++);
			val1 = *(u32ptr++);
			val2 = *(u32ptr++);
			// printf("%d %d %x %x %x\n", row, col, val0, val1, val2);

			*(histY2prt++) = (val1 & 0xffff0000) >> 16;
			*(histY2prt++) = (val0 & 0x0000ffff) >>  0;

			*(histY1prt++) = (val2 & 0x0000ffff) >>  0;
			*(histY1prt++) = (val0 & 0xffff0000) >> 16;

			*(histY0prt++) = (val2 & 0xffff0000) >> 16;
			*(histY0prt++) = (val1 & 0x0000ffff) >>  0;

			u32ptr++;

			val0 = *(u32ptr++);
			val1 = *(u32ptr++);
			val2 = *(u32ptr++);
			// printf("%x %x %x\n", val0, val1, val2);

			*(histX2prt++) = (val1 & 0xffff0000) >> 16;
			*(histX2prt++) = (val0 & 0x0000ffff) >>  0;
			*(histX1prt++) = (val2 & 0x0000ffff) >>  0;
			*(histX1prt++) = (val0 & 0xffff0000) >> 16;
			*(histX0prt++) = (val2 & 0xffff0000) >> 16;
			*(histX0prt++) = (val1 & 0x0000ffff) >>  0;
		}

		for (CVI_U32 col = section_2_start ; col < section_2_end ; col++) {
			u32ptr++;

			val0 = *(u32ptr++);
			val1 = *(u32ptr++);
			val2 = *(u32ptr++);
			// printf("%d %d %x %x %x\n", row, col, val0, val1, val2);

			// *(histY2prt++) = (val1 & 0xffff0000) >> 16;
			// *(histY2prt++) = (val0 & 0x0000ffff) >>  0;

			// *(histY1prt++) = (val2 & 0x0000ffff) >>  0;
			// *(histY1prt++) = (val0 & 0xffff0000) >> 16;

			// *(histY0prt++) = (val2 & 0xffff0000) >> 16;
			// *(histY0prt++) = (val1 & 0x0000ffff) >>  0;

			u32ptr++;

			val0 = *(u32ptr++);
			val1 = *(u32ptr++);
			val2 = *(u32ptr++);
			// printf("%x %x %x\n", val0, val1, val2);

			*(histX2prt++) = (val1 & 0xffff0000) >> 16;
			*(histX2prt++) = (val0 & 0x0000ffff) >>  0;
			*(histX1prt++) = (val2 & 0x0000ffff) >>  0;
			*(histX1prt++) = (val0 & 0xffff0000) >> 16;
			*(histX0prt++) = (val2 & 0xffff0000) >> 16;
			*(histX0prt++) = (val1 & 0x0000ffff) >>  0;
		}
	}

	return CVI_SUCCESS;
}

//-----------------------------------------------------------------------------
//  statistics configuration
//-----------------------------------------------------------------------------
static CVI_S32 isp_sts_ctrl_set_ae_cfg(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;
	ISP_AE_STATISTICS_CFG_S *ae_attr;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	ae_attr = &(pstIspCtx->stsCfgInfo.stAECfg);

	struct cvi_vip_isp_ae_config *ae_cfg = &(runtime->ae_cfg);
	CVI_U16 u16W, u16H;
	CVI_U32 sts_div_temp;

	ae_cfg->update = 1;
	ae_cfg->ae_enable = 1;

	ae_cfg->ae_numx = AE_ZONE_COLUMN;
	ae_cfg->ae_numy = AE_ZONE_ROW;
	if (ae_attr->stCrop[0].bEnable) {
		u16W = pstIspCtx->stSysRect.u32Width - ae_attr->stCrop[0].u16X;
		u16H = pstIspCtx->stSysRect.u32Height - ae_attr->stCrop[0].u16Y;
		u16W = MIN(ae_attr->stCrop[0].u16W, u16W);
		u16H = MIN(ae_attr->stCrop[0].u16H, u16H);
		ae_cfg->ae_offsetx = ((ae_attr->stCrop[0].u16X >> 1) << 1);
		ae_cfg->ae_offsety = ((ae_attr->stCrop[0].u16Y >> 1) << 1);
		ae_cfg->ae_width = (((u16W / ae_cfg->ae_numx) >> 1) << 1);
		ae_cfg->ae_height = (((u16H / ae_cfg->ae_numy) >> 1) << 1);
	} else {
		ae_cfg->ae_offsetx = 0;
		ae_cfg->ae_offsety = 0;
		ae_cfg->ae_width = (((pstIspCtx->stSysRect.u32Width / ae_cfg->ae_numx) >> 1) << 1);
		ae_cfg->ae_height = (((pstIspCtx->stSysRect.u32Height / ae_cfg->ae_numy) >> 1) << 1);
	}

	runtime->out_of_ae_sts_div_bit = 0;
	sts_div_temp = AE_STS_DIV_MGC / (ae_cfg->ae_width * ae_cfg->ae_height);

	if (sts_div_temp > AE_STS_DIV_MAX) {

		CVI_U32 temp = sts_div_temp >> AE_STS_DIV_WIDTH;

		while (temp) {
			runtime->out_of_ae_sts_div_bit++;
			temp >>= 1;
		}

		sts_div_temp = sts_div_temp >> runtime->out_of_ae_sts_div_bit;
	}

	ae_cfg->ae_sts_div = sts_div_temp;

	for (CVI_U8 i = 0 ; i < FACE_WIN_NUM ; i++) {
		ae_cfg->ae_face_offset_x[i] = ae_attr->stFaceCrop[i].u16X;
		ae_cfg->ae_face_offset_y[i] = ae_attr->stFaceCrop[i].u16Y;
		ae_cfg->ae_face_size_minus1_x[i] = ae_attr->stFaceCrop[i].u16W;
		ae_cfg->ae_face_size_minus1_y[i] = ae_attr->stFaceCrop[i].u16H;
	}

	struct cvi_isp_ae_tun_cfg *ae_0_cfg = &(ae_cfg->ae_cfg);

	CVI_U32 face_sts_div[4] = {1024, 1024, 1024, 1024};

	for (CVI_U8 i = 0 ; i < FACE_WIN_NUM ; i++) {
		CVI_U32 pxl = ae_attr->stFaceCrop[i].u16W * ae_attr->stFaceCrop[i].u16H;

		if ((pxl != 0)) {
			// TODO: mason.zou, face div also has out of bit width issue, but not use yet.
			face_sts_div[i] = AE_STS_DIV_MGC / pxl;
		}
	}

	ae_0_cfg->AE_FACE0_ENABLE.bits.AE_FACE0_ENABLE = ae_attr->stFaceCrop[0].bEnable;
	ae_0_cfg->AE_FACE0_ENABLE.bits.AE_FACE1_ENABLE = ae_attr->stFaceCrop[1].bEnable;
	ae_0_cfg->AE_FACE0_ENABLE.bits.AE_FACE2_ENABLE = ae_attr->stFaceCrop[2].bEnable;
	ae_0_cfg->AE_FACE0_ENABLE.bits.AE_FACE3_ENABLE = ae_attr->stFaceCrop[3].bEnable;
	ae_0_cfg->AE_FACE0_STS_DIV.bits.AE_FACE0_STS_DIV = face_sts_div[0];
	ae_0_cfg->AE_FACE1_STS_DIV.bits.AE_FACE1_STS_DIV = face_sts_div[1];
	ae_0_cfg->AE_FACE2_STS_DIV.bits.AE_FACE2_STS_DIV = face_sts_div[2];
	ae_0_cfg->AE_FACE3_STS_DIV.bits.AE_FACE3_STS_DIV = face_sts_div[3];
	ae_0_cfg->STS_ENABLE.bits.STS_AWB_ENABLE = 1;
	ae_0_cfg->AE_ALGO_ENABLE.bits.AE_ALGO_ENABLE = ae_attr->fast2A_ena;
	ae_0_cfg->AE_HIST_LOW.bits.AE_HIST_LOW = ae_attr->fast2A_ae_low;
	ae_0_cfg->AE_HIST_HIGH.bits.AE_HIST_HIGH = ae_attr->fast2A_ae_high;
	ae_0_cfg->AE_TOP.bits.AE_AWB_TOP = ae_attr->fast2A_awb_top;
	ae_0_cfg->AE_BOT.bits.AE_AWB_BOT = ae_attr->fast2A_awb_bot;
	ae_0_cfg->AE_OVEREXP_THR.bits.AE_OVEREXP_THR = ae_attr->over_exp_thr;
	ae_0_cfg->AE_NUM_GAPLINE.bits.AE_NUM_GAPLINE = 0;

	struct cvi_isp_ae_2_tun_cfg *ae_2_cfg = &(ae_cfg->ae_2_cfg);

	CVI_U32 val[32] = {0};
	CVI_U8 idx = 0;

	for (CVI_U32 row = 0 ; row < AE_WEIGHT_ZONE_ROW ; row++) {
		for (CVI_U32 col = 0 ; col < AE_WEIGHT_ZONE_COLUMN ; col++) {
			CVI_U32 pos = idx >> 3;
			CVI_U32 shft = ((idx & 0x7) << 2);

			val[pos] += (ae_attr->au8Weight[row][col] << shft);

			idx++;
		}
	}

	ae_2_cfg->AE_WGT_00.bits.AE_WGT_00 = val[0];
	ae_2_cfg->AE_WGT_01.bits.AE_WGT_01 = val[1];
	ae_2_cfg->AE_WGT_02.bits.AE_WGT_02 = val[2];
	ae_2_cfg->AE_WGT_03.bits.AE_WGT_03 = val[3];
	ae_2_cfg->AE_WGT_04.bits.AE_WGT_04 = val[4];
	ae_2_cfg->AE_WGT_05.bits.AE_WGT_05 = val[5];
	ae_2_cfg->AE_WGT_06.bits.AE_WGT_06 = val[6];
	ae_2_cfg->AE_WGT_07.bits.AE_WGT_07 = val[7];
	ae_2_cfg->AE_WGT_08.bits.AE_WGT_08 = val[8];
	ae_2_cfg->AE_WGT_09.bits.AE_WGT_09 = val[9];
	ae_2_cfg->AE_WGT_10.bits.AE_WGT_10 = val[10];
	ae_2_cfg->AE_WGT_11.bits.AE_WGT_11 = val[11];
	ae_2_cfg->AE_WGT_12.bits.AE_WGT_12 = val[12];
	ae_2_cfg->AE_WGT_13.bits.AE_WGT_13 = val[13];
	ae_2_cfg->AE_WGT_14.bits.AE_WGT_14 = val[14];
	ae_2_cfg->AE_WGT_15.bits.AE_WGT_15 = val[15];
	ae_2_cfg->AE_WGT_16.bits.AE_WGT_16 = val[16];
	ae_2_cfg->AE_WGT_17.bits.AE_WGT_17 = val[17];
	ae_2_cfg->AE_WGT_18.bits.AE_WGT_18 = val[18];
	ae_2_cfg->AE_WGT_19.bits.AE_WGT_19 = val[19];
	ae_2_cfg->AE_WGT_20.bits.AE_WGT_20 = val[20];
	ae_2_cfg->AE_WGT_21.bits.AE_WGT_21 = val[21];
	ae_2_cfg->AE_WGT_22.bits.AE_WGT_22 = val[22];
	ae_2_cfg->AE_WGT_23.bits.AE_WGT_23 = val[23];
	ae_2_cfg->AE_WGT_24.bits.AE_WGT_24 = val[24];
	ae_2_cfg->AE_WGT_25.bits.AE_WGT_25 = val[25];
	ae_2_cfg->AE_WGT_26.bits.AE_WGT_26 = val[26];
	ae_2_cfg->AE_WGT_27.bits.AE_WGT_27 = val[27];
	ae_2_cfg->AE_WGT_28.bits.AE_WGT_28 = val[28];
	ae_2_cfg->AE_WGT_29.bits.AE_WGT_29 = val[29];
	ae_2_cfg->AE_WGT_30.bits.AE_WGT_30 = val[30];
	ae_2_cfg->AE_WGT_31.bits.AE_WGT_31 = val[31];

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_ae_config *post_ae_cfg[2] = {
		(struct cvi_vip_isp_ae_config *)&(post_addr->tun_cfg[tun_idx].ae_cfg[0]),
		(struct cvi_vip_isp_ae_config *)&(post_addr->tun_cfg[tun_idx].ae_cfg[1])
	};
#if !defined(__CV180X__)
	for (CVI_U8 i = 0; i < 2 ; i++) {
#else
	for (CVI_U8 i = 0; i < 1 ; i++) {
#endif
		ae_cfg->inst = i;
		CVI_BOOL map_pipe_to_enable = 1;//ae_cfg->inst == 0) ? 1 : IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode);

		ae_cfg->ae_enable = ae_cfg->ae_enable && map_pipe_to_enable;
		memcpy(post_ae_cfg[i], ae_cfg, sizeof(struct cvi_vip_isp_ae_config));
	}

	return ret;
}

static CVI_S32 isp_sts_ctrl_set_af_cfg(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifndef ARCH_RTOS_CV181X
	CVI_S8 lowpassSignFactor = 1, horizon0SignFactor = 1, horizon1SignFactor = 1, verticalSignFactor = 1;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CTX_S *pstIspCtx = NULL;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	struct cvi_vip_isp_af_config *af_cfg = &(runtime->af_cfg);

	af_cfg->update = 1;
	af_cfg->inst = ViPipe;
	af_cfg->enable = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.bEnable;
	af_cfg->dpc_enable = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stPreFltCfg.PreFltEn;
	af_cfg->hlc_enable = 1;
	af_cfg->square_enable = 1;
	af_cfg->outshift = 0;
	af_cfg->block_numx = AF_ZONE_COLUMN;
	af_cfg->block_numy = AF_ZONE_ROW;
	// Check AF window size.
	if (pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.bEnable) {
		af_cfg->offsetx = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16X;
		af_cfg->offsety = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16Y;

		if (pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16W >
			(pstIspCtx->stSysRect.u32Width - (af_cfg->offsetx << 1))) {
			ISP_LOG_NOTICE("CropSize_W:%d is fail, set CropSize_W:%d\n",
				pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16W,
				pstIspCtx->stSysRect.u32Width - (af_cfg->offsetx << 1));
			pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16W =
				pstIspCtx->stSysRect.u32Width - (af_cfg->offsetx << 1);
		}
		if (pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16H >
			(pstIspCtx->stSysRect.u32Height - (af_cfg->offsety << 1))) {
			ISP_LOG_NOTICE("CropSize_H:%d is fail, set CropSize_H:%d\n",
				pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16H,
				pstIspCtx->stSysRect.u32Height - (af_cfg->offsety << 1));
			pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16H =
				pstIspCtx->stSysRect.u32Height - (af_cfg->offsety << 1);
		}

		af_cfg->block_width = ((pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16W)
			/ af_cfg->block_numx) >> 1 << 1;
		af_cfg->block_height = ((pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.stCrop.u16H)
			/ af_cfg->block_numy) >> 1 << 1;
	} else {
		af_cfg->offsetx = AF_XOFFSET_MIN;
		af_cfg->offsety = AF_YOFFSET_MIN;
		af_cfg->block_width = ((pstIspCtx->stSysRect.u32Width - (AF_XOFFSET_MIN << 1))
			/ af_cfg->block_numx) >> 1 << 1;
		af_cfg->block_height = ((pstIspCtx->stSysRect.u32Height - (AF_YOFFSET_MIN << 1))
			/ af_cfg->block_numy) >> 1 << 1;
	}

	af_cfg->h_low_pass_value_shift = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8HFltShift;
	af_cfg->h_corning_offset_0 = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.H0FltCoring;
	af_cfg->h_corning_offset_1 = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.H1FltCoring;
	af_cfg->v_corning_offset = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.V0FltCoring;
	af_cfg->high_luma_threshold = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u16HighLumaTh;

	// Workaround. When filter's first coefficient is negative. All coefficient need sign change.
	#define SIGN_FACTOR_CHECK(firstFilterCoeff, signFactor)\
		(signFactor = (firstFilterCoeff < 0) ? -1 : 1)

	SIGN_FACTOR_CHECK(pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.s8HVFltLpCoeff[0], lowpassSignFactor);
	SIGN_FACTOR_CHECK(pstIspCtx->stsCfgInfo.stFocusCfg.stHParam_FIR0.s8HFltHpCoeff[0], horizon0SignFactor);
	SIGN_FACTOR_CHECK(pstIspCtx->stsCfgInfo.stFocusCfg.stHParam_FIR1.s8HFltHpCoeff[0], horizon1SignFactor);
	SIGN_FACTOR_CHECK(pstIspCtx->stsCfgInfo.stFocusCfg.stVParam_FIR.s8VFltHpCoeff[0], verticalSignFactor);
	for (CVI_U32 i = 0; i < FIR_H_GAIN_NUM; i++) {
		af_cfg->h_low_pass_coef[i] = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.s8HVFltLpCoeff[i]
			* lowpassSignFactor;
		af_cfg->h_high_pass_coef_0[i] = pstIspCtx->stsCfgInfo.stFocusCfg.stHParam_FIR0.s8HFltHpCoeff[i]
			* horizon0SignFactor;
		af_cfg->h_high_pass_coef_1[i] = pstIspCtx->stsCfgInfo.stFocusCfg.stHParam_FIR1.s8HFltHpCoeff[i]
			* horizon1SignFactor;
	}
	for (CVI_U32 i = 0; i < FIR_V_GAIN_NUM; i++) {
		af_cfg->v_high_pass_coef[i] = pstIspCtx->stsCfgInfo.stFocusCfg.stVParam_FIR.s8VFltHpCoeff[i]
			* verticalSignFactor;
	}
	// fix coring setting.

	//cv182x new param
	af_cfg->th_low =  pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8ThLow;
	af_cfg->th_high = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8ThHigh;
	af_cfg->gain_low = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8GainLow;
	af_cfg->gain_high = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8GainHigh;
	af_cfg->slop_low = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8SlopLow;
	af_cfg->slop_high = pstIspCtx->stsCfgInfo.stFocusCfg.stConfig.u8SlopHigh;

	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_af_config *pre_be_af_cfg =
		(struct cvi_vip_isp_af_config *)&(pre_be_addr->tun_cfg[tun_idx].af_cfg);

	memcpy(pre_be_af_cfg, af_cfg, sizeof(struct cvi_vip_isp_af_config));
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static CVI_S32 isp_sts_ctrl_set_gms_cfg(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}
#ifndef ARCH_RTOS_CV181X
	if (ViPipe != 0)
		return CVI_SUCCESS;

	struct cvi_vip_isp_gms_config *gms_cfg = &(runtime->gms_cfg);

	isp_dis_ctrl_get_gms_attr(ViPipe, gms_cfg);

	gms_cfg->update = 1;
	gms_cfg->inst = ViPipe;
	gms_cfg->enable = 1;

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_gms_config *post_gms_cfg =
		(struct cvi_vip_isp_gms_config *)&(post_addr->tun_cfg[tun_idx].gms_cfg);

	memcpy(post_gms_cfg, gms_cfg, sizeof(struct cvi_vip_isp_gms_config));
#endif
	return ret;
}

//-----------------------------------------------------------------------------
//  private function
//-----------------------------------------------------------------------------
#ifndef ARCH_RTOS_CV181X
static CVI_S32 _isp_sts_ctrl_get_idx(VI_PIPE ViPipe, CVI_U32 id, CVI_U32 *idx)
{
	CVI_S32 ret = CVI_SUCCESS;

	G_EXT_CTRLS_VALUE(id, ViPipe, idx);

	return ret;
}

static CVI_S32 _isp_sts_ctrl_put_idx(VI_PIPE ViPipe, CVI_U32 id)
{
	CVI_S32 ret = CVI_SUCCESS;

	S_EXT_CTRLS_VALUE(id, ViPipe, CVI_NULL);

	return ret;
}
#endif

static CVI_S32 isp_sts_ctrl_initial_motion_sts(VI_PIPE ViPipe)
{
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_isp_sts_mem *sts_mem = &(runtime->sts_mem[0]);

	runtime->motion_sts.motionStsBufSize = sts_mem->mmap.size;
	if (runtime->motion_sts.motionStsData == CVI_NULL) {
		runtime->motion_sts.motionStsData = ISP_PTR_CAST_PTR(calloc(1, runtime->motion_sts.motionStsBufSize));
		if (runtime->motion_sts.motionStsData == CVI_NULL) {
			ISP_LOG_ERR("motionStsData alloca fail. size =  %d\n",
				runtime->motion_sts.motionStsBufSize);
			return CVI_FAILURE;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 isp_sts_ctrl_release_motion_sts(VI_PIPE ViPipe)
{
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->motion_sts.motionStsBufSize = 0;
	if (runtime->motion_sts.motionStsData) {
		free(ISP_PTR_CAST_VOID(runtime->motion_sts.motionStsData));
		runtime->motion_sts.motionStsData = CVI_NULL;
	}

	return CVI_SUCCESS;
}

static CVI_S32 _isp_sts_ctrl_mmap(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret |= _isp_sts_ctrl_mmap_get_paddr(ViPipe);
	ret |= _isp_sts_ctrl_mmap_get_vaddr(ViPipe);

	return ret;
}

static CVI_S32 _isp_sts_ctrl_mmap_get_paddr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

#ifdef ARCH_RTOS_CV181X
	// TODO: mason.zou
#else
	for (CVI_U32 i = 0; i < ISP_MAX_STS_BUF_NUM; i++) {
		struct cvi_isp_sts_mem *sts_mem = &(runtime->sts_mem[i]);

		memset(sts_mem, 0, sizeof(struct cvi_isp_sts_mem));
		sts_mem->raw_num = ViPipe;
	}

	G_EXT_CTRLS_PTR(VI_IOCTL_STS_MEM, &(runtime->sts_mem[0]));
#endif

	return ret;
}

static CVI_S32 _isp_sts_ctrl_mmap_get_vaddr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	#define _mmap_cache(va, pa, size) do {\
		if (size)\
			va = MMAP_CACHE_COMPAT(pa, size); \
	} while (0)

	for (CVI_U32 i = 0; i < ISP_MAX_STS_BUF_NUM; i++) {
		struct cvi_isp_sts_mem *sts_mem = &(runtime->sts_mem[i]);

		_mmap_cache(sts_mem->af.vir_addr, sts_mem->af.phy_addr, sts_mem->af.size);
		_mmap_cache(sts_mem->gms.vir_addr, sts_mem->gms.phy_addr, sts_mem->gms.size);
		_mmap_cache(sts_mem->ae_le.vir_addr, sts_mem->ae_le.phy_addr, sts_mem->ae_le.size);
		_mmap_cache(sts_mem->ae_se.vir_addr, sts_mem->ae_se.phy_addr, sts_mem->ae_se.size);
		_mmap_cache(sts_mem->dci.vir_addr, sts_mem->dci.phy_addr, sts_mem->dci.size);
		_mmap_cache(sts_mem->hist_edge_v.vir_addr, sts_mem->hist_edge_v.phy_addr, sts_mem->hist_edge_v.size);
		_mmap_cache(sts_mem->mmap.vir_addr, sts_mem->mmap.phy_addr, sts_mem->mmap.size);
	}

	#undef _mmap_cache

	return ret;
}

static CVI_S32 _isp_sts_ctrl_munmap(VI_PIPE ViPipe)
{
#ifndef ARCH_RTOS_CV181X
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	#define _munmap(va, size) do {\
		if (size)\
			MUNMAP_COMPAT(va, size); \
	} while (0)
	for (CVI_U32 i = 0; i < ISP_MAX_STS_BUF_NUM; i++) {
		struct cvi_isp_sts_mem *sts_mem = &(runtime->sts_mem[i]);

		_munmap(sts_mem->af.vir_addr, sts_mem->af.size);
		_munmap(sts_mem->gms.vir_addr, sts_mem->gms.size);
		_munmap(sts_mem->ae_le.vir_addr, sts_mem->ae_le.size);
		_munmap(sts_mem->ae_se.vir_addr, sts_mem->ae_se.size);
		_munmap(sts_mem->dci.vir_addr, sts_mem->dci.size);
		_munmap(sts_mem->hist_edge_v.vir_addr, sts_mem->hist_edge_v.size);
		_munmap(sts_mem->mmap.vir_addr, sts_mem->mmap.size);
	}

	#undef _munmap
#else
	UNUSED(ViPipe);
#endif

	return CVI_SUCCESS;
}

static struct isp_sts_ctrl_runtime  **_get_sts_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &sts_ctrl_runtime[ViPipe];
}

static CVI_S32 isp_sts_ctrl_ready(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_sts_ctrl_runtime **runtime_ptr = _get_sts_ctrl_runtime(ViPipe);
	struct isp_sts_ctrl_runtime *runtime = *runtime_ptr;

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (!runtime->initialized || !runtime->mapped)
		ret = CVI_FAILURE;

	return ret;
}
