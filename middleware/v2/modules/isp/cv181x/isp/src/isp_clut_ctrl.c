/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_clut_ctrl.c
 * Description:
 *
 */

#include "cvi_sys.h"

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_clut_ctrl.h"
#include "isp_mgr_buf.h"

#define CLUT_CHANNEL_SIZE				(17)		// HW dependent
#define CLUT_PARTIAL_UPDATE_SIZE		(119)
#define CLUT_PARTIAL_UPDATE_TIMES		(42)		// 4913/CLUT_PARTIAL_UPDATE_SIZE
#define CLUT_OFFLINE_FULL_UPDATE_SYMBOL	(0xFF)

const struct isp_module_ctrl clut_mod = {
	.init = isp_clut_ctrl_init,
	.uninit = isp_clut_ctrl_uninit,
	.suspend = isp_clut_ctrl_suspend,
	.resume = isp_clut_ctrl_resume,
	.ctrl = isp_clut_ctrl_ctrl
};

static CVI_S32 isp_clut_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_clut_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_clut_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_clut_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 isp_clut_ctrl_check_clut_attr_valid(const ISP_CLUT_ATTR_S *pstCLUTAttr);
static CVI_S32 isp_clut_ctrl_check_clut_saturation_attr_valid(const ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr);
static CVI_VOID table_update_full(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg);
static CVI_VOID lut_update_full(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg);
static CVI_S32 table_update_partial(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg);
static CVI_S32 lut_update_partial(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg);

static CVI_S32 set_clut_proc_info(VI_PIPE ViPipe);

static struct isp_clut_ctrl_runtime  *_get_clut_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_clut_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_algo_clut_init(ViPipe);

	runtime->bTableUpdateFullMode = CVI_FALSE;

	runtime->clut_partial_update.updateFailCnt[0] = 0;
	runtime->clut_partial_update.updateFailCnt[1] = 0;
	runtime->clut_partial_update.u8LastSyncIdx[0] = 0;
	runtime->clut_partial_update.u8LastSyncIdx[1] = 0;
	runtime->clut_partial_update.updateListLen = 0;

	runtime->preprocess_table_updated = CVI_TRUE;
	runtime->postprocess_table_updating = CVI_FALSE;
	runtime->postprocess_lut_updating = CVI_FALSE;
	runtime->preprocess_lut_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;
	runtime->tun_table_updated_flage = 0;
	runtime->tun_lut_updated_flage = 0;
	runtime->bSatLutUpdated = CVI_FALSE;

	return ret;
}

CVI_S32 isp_clut_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	isp_algo_clut_uninit(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_clut_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_clut_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassClut;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassClut = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_clut_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_clut_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_clut_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_clut_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_clut_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_clut_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_CLUT_ATTR_S *clut_attr;
	const ISP_CLUT_SATURATION_ATTR_S *clut_saturation_attr = NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &clut_saturation_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(clut_attr->UpdateInterval, 1);

	is_preprocess_update = (
							(runtime->preprocess_table_updated)
							|| (runtime->preprocess_lut_updated)
							|| (runtime->postprocess_table_updating)
							|| (runtime->postprocess_lut_updating)
							|| ((algoResult->u32FrameIdx % intvl) == 0)
						);

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	// runtime->preprocess_table_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!clut_attr->Enable || runtime->is_module_bypass)
		return ret;

	// ParamIn
	runtime->clut_param_in.is_table_update = runtime->preprocess_table_updated;
	runtime->clut_param_in.is_updating = (runtime->postprocess_table_updating || runtime->postprocess_lut_updating);
	runtime->clut_param_in.is_lut_update = runtime->preprocess_lut_updated;

	runtime->clut_param_in.iso = algoResult->u32PostIso;
	runtime->clut_param_in.ClutR = ISP_PTR_CAST_PTR(clut_attr->ClutR);
	runtime->clut_param_in.ClutG = ISP_PTR_CAST_PTR(clut_attr->ClutG);
	runtime->clut_param_in.ClutB = ISP_PTR_CAST_PTR(clut_attr->ClutB);
	runtime->clut_param_in.saturation_attr = ISP_PTR_CAST_PTR(clut_saturation_attr);

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_clut_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_clut_main(ViPipe,
		(struct clut_param_in *)&runtime->clut_param_in,
		(struct clut_param_out *)&runtime->clut_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_clut_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_clut_config *clut_cfg =
		(struct cvi_vip_isp_clut_config *)&(post_addr->tun_cfg[tun_idx].clut_cfg);

	struct isp_clut_partial_update *ptPartialUp = &(runtime->clut_partial_update);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;
	const ISP_CLUT_SATURATION_ATTR_S *clut_saturation_attr = NULL;
	const CVI_BOOL bIsMultiCam = IS_MULTI_CAM();

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &clut_saturation_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (bIsMultiCam));

	// Force update full mode when in multiple sensor situation.
	// TODO: Need to verify when multiple sensor environment is ready.
	if (bIsMultiCam) {
		runtime->bTableUpdateFullMode = CVI_TRUE;
	}

	CVI_BOOL enable = clut_attr->Enable && !runtime->is_module_bypass;

	if (is_postprocess_update == CVI_TRUE) {
		if (runtime->preprocess_table_updated) {
			runtime->preprocess_table_updated = CVI_FALSE;
			runtime->postprocess_table_updating = CVI_TRUE;
		} else if ((runtime->preprocess_lut_updated) ||
				(runtime->clut_param_out.updateList.length > 0)) {
			if (enable) {
				runtime->postprocess_lut_updating = CVI_TRUE;
			}
			if (runtime->preprocess_lut_updated) {
				runtime->preprocess_lut_updated = CVI_FALSE;
			} else {
				runtime->tun_lut_updated_flage = 0;
			}
			ptPartialUp->updateListLen = runtime->clut_param_out.updateList.length;
			runtime->clut_param_out.updateList.length = 0;
		}

		if (runtime->bTableUpdateFullMode) {
			if (runtime->postprocess_table_updating) {
				table_update_full(ViPipe, runtime, clut_cfg);
			} else if (runtime->postprocess_lut_updating) {
				lut_update_full(ViPipe, runtime, clut_cfg);
			}
		} else {
			if (runtime->postprocess_table_updating) {
				table_update_partial(ViPipe, runtime, clut_cfg);
			} else if (runtime->postprocess_lut_updating) {
				lut_update_partial(ViPipe, runtime, clut_cfg);
			}
		}
	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_VOID table_update_full(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg)
{
	struct isp_clut_ctrl_runtime *runtime = (struct isp_clut_ctrl_runtime *)ptRuntime;
	struct cvi_vip_isp_clut_config *clut_cfg = (struct cvi_vip_isp_clut_config *)ptCfg;
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);

	CVI_BOOL enable = clut_attr->Enable && !runtime->is_module_bypass;

	if (runtime->tun_table_updated_flage == CLUT_OFFLINE_FULL_UPDATE_SYMBOL) {
		runtime->postprocess_table_updating = CVI_FALSE;
	} else {
		clut_cfg->update = 1;
		clut_cfg->is_update_partial = 0;
		clut_cfg->enable = enable;
		clut_cfg->tbl_idx = CLUT_OFFLINE_FULL_UPDATE_SYMBOL;
		runtime->tun_table_updated_flage |= 0xf <<  (tun_idx * 4);
		if (clut_cfg->enable) {
			memcpy(clut_cfg->r_lut, clut_attr->ClutR, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
			memcpy(clut_cfg->g_lut, clut_attr->ClutG, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
			memcpy(clut_cfg->b_lut, clut_attr->ClutB, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
		}
	}
}

static CVI_VOID lut_update_full(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg)
{
	struct isp_clut_ctrl_runtime *runtime = (struct isp_clut_ctrl_runtime *)ptRuntime;
	struct cvi_vip_isp_clut_config *clut_cfg = (struct cvi_vip_isp_clut_config *)ptCfg;
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;
	const ISP_CLUT_SATURATION_ATTR_S *clut_saturation_attr = NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &clut_saturation_attr);

	CVI_BOOL enable = clut_attr->Enable && !runtime->is_module_bypass;

	if (clut_saturation_attr->Enable) {
		if (runtime->tun_lut_updated_flage == CLUT_OFFLINE_FULL_UPDATE_SYMBOL) {
			runtime->postprocess_lut_updating = CVI_FALSE;
			runtime->bSatLutUpdated = CVI_TRUE;
			runtime->clut_partial_update.updateListLen = 0;
		} else {
			clut_cfg->update = 1;
			clut_cfg->is_update_partial = 0;
			clut_cfg->enable = enable;
			clut_cfg->tbl_idx = CLUT_OFFLINE_FULL_UPDATE_SYMBOL;
			runtime->tun_lut_updated_flage |= 0xf <<  (tun_idx * 4);
			memcpy(clut_cfg->r_lut, clut_attr->ClutR, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
			memcpy(clut_cfg->g_lut, clut_attr->ClutG, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);
			memcpy(clut_cfg->b_lut, clut_attr->ClutB, sizeof(uint16_t) * ISP_CLUT_LUT_LENGTH);

			CVI_U32 len = runtime->clut_partial_update.updateListLen;

			for (CVI_U32 i = 0; i < len; i++) {
				CVI_U32 addr = runtime->clut_param_out.updateList.items[i].addr;
				CVI_U32 data = runtime->clut_param_out.updateList.items[i].data;
				CVI_U32 idx = 289 * ((addr >> 16) & 0xFF) + 17 * ((addr >> 8) & 0xFF) + (addr & 0xFF);

				clut_cfg->r_lut[idx] = (data >> 20) & 0x3FF;
				clut_cfg->g_lut[idx] = (data >> 10) & 0x3FF;
				clut_cfg->b_lut[idx] = data & 0x3FF;
			}
		}
	} else {
		runtime->postprocess_lut_updating = CVI_FALSE;
		runtime->tun_lut_updated_flage = 0;
		if (runtime->bSatLutUpdated) {
			runtime->postprocess_table_updating = CVI_TRUE;
			runtime->tun_table_updated_flage = 0;
			runtime->bSatLutUpdated = CVI_FALSE;
		}
	}
}

static CVI_S32 table_update_partial(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg)
{
	struct isp_clut_ctrl_runtime *runtime = (struct isp_clut_ctrl_runtime *)ptRuntime;
	struct cvi_vip_isp_clut_config *clut_cfg = (struct cvi_vip_isp_clut_config *)ptCfg;
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;
	const ISP_CLUT_SATURATION_ATTR_S *clut_saturation_attr = NULL;
	struct isp_clut_partial_update *ptPartialUp = &(runtime->clut_partial_update);

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &clut_saturation_attr);

	CVI_BOOL enable = clut_attr->Enable && !runtime->is_module_bypass;

	if (runtime->tun_table_updated_flage == CLUT_OFFLINE_FULL_UPDATE_SYMBOL) {
		runtime->postprocess_table_updating = CVI_FALSE;
		ptPartialUp->u8LastSyncIdx[0] = 0;
		ptPartialUp->u8LastSyncIdx[1] = 0;
		ptPartialUp->updateFailCnt[0] = 0;
		ptPartialUp->updateFailCnt[1] = 0;
	} else {
		CVI_U32 u32CurSyncIdx = ViPipe;

		G_EXT_CTRLS_VALUE(VI_IOCTL_GET_CLUT_TBL_IDX, tun_idx, &u32CurSyncIdx);
		if ((u32CurSyncIdx != ptPartialUp->u8LastSyncIdx[tun_idx])
					&& (ptPartialUp->u8LastSyncIdx[tun_idx] != 0)) {
			if (ptPartialUp->updateFailCnt[tun_idx]++ > 50) {
				ptPartialUp->updateFailCnt[tun_idx] = 0;
				ISP_LOG_ERR("ViPipe=%d,tun_idx=%d,clut table partial update fail reset index.\n",
					ViPipe, tun_idx);
			}
		} else {
			ptPartialUp->updateFailCnt[tun_idx] = 0;
			if (ptPartialUp->u8LastSyncIdx[tun_idx] == CLUT_PARTIAL_UPDATE_TIMES) {
				ptPartialUp->updateFailCnt[tun_idx] = 0;
				runtime->tun_table_updated_flage |= 0xf <<  (tun_idx * 4);
				clut_cfg->update = 0;
				clut_cfg->is_update_partial = 0;
			} else {
				if (enable) {
					CVI_U32 u32DataCount = 0;
					CVI_U32 u32LutIdx;
					CVI_U32 b_idx, g_idx, r_idx;
					CVI_U32 b_value, g_value, r_value;

					u32LutIdx = ptPartialUp->u8LastSyncIdx[tun_idx] * CLUT_PARTIAL_UPDATE_SIZE;

					for (CVI_U32 idx = 0; ((idx < CLUT_PARTIAL_UPDATE_SIZE)
						&& (u32LutIdx < ISP_CLUT_LUT_LENGTH));
						++idx, ++u32LutIdx) {

						b_idx = u32LutIdx / (CLUT_CHANNEL_SIZE * CLUT_CHANNEL_SIZE);
						g_idx = (u32LutIdx / CLUT_CHANNEL_SIZE) % CLUT_CHANNEL_SIZE;
						r_idx = u32LutIdx % CLUT_CHANNEL_SIZE;

						b_value = clut_attr->ClutB[u32LutIdx];
						g_value = clut_attr->ClutG[u32LutIdx];
						r_value = clut_attr->ClutR[u32LutIdx];

						// 0: addr, 1: value
						clut_cfg->lut[idx][0] = (b_idx << 16) + (g_idx << 8) + r_idx;
						clut_cfg->lut[idx][1] = ((r_value & 0x3FF) << 20)
							+ ((g_value & 0x3FF) << 10) + (b_value & 0x3FF);
						u32DataCount++;
					}
					ptPartialUp->u8LastSyncIdx[tun_idx]++;
					clut_cfg->tbl_idx = ptPartialUp->u8LastSyncIdx[tun_idx];
					clut_cfg->update_length = u32DataCount;
					clut_cfg->update = 1;
					clut_cfg->is_update_partial = 1;
					if ((clut_cfg->tbl_idx == CLUT_PARTIAL_UPDATE_TIMES)
						|| (runtime->tun_table_updated_flage == 0x0F)
						|| (runtime->tun_table_updated_flage == 0xF0)) {
						clut_cfg->enable = enable;
					}
				} else {
					ptPartialUp->u8LastSyncIdx[tun_idx] = CLUT_PARTIAL_UPDATE_TIMES;
					clut_cfg->tbl_idx = CLUT_PARTIAL_UPDATE_TIMES;
					clut_cfg->update_length = 0;
					clut_cfg->update = 1;
					clut_cfg->is_update_partial = 1;
					clut_cfg->enable = enable;
				}
			}
		}
	}

	return 0;
}

static CVI_S32 lut_update_partial(VI_PIPE ViPipe, void *ptRuntime, void *ptCfg)
{
	struct isp_clut_ctrl_runtime *runtime = (struct isp_clut_ctrl_runtime *)ptRuntime;
	struct cvi_vip_isp_clut_config *clut_cfg = (struct cvi_vip_isp_clut_config *)ptCfg;
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	const ISP_CLUT_ATTR_S *clut_attr = NULL;
	const ISP_CLUT_SATURATION_ATTR_S *clut_saturation_attr = NULL;
	struct isp_clut_partial_update *ptPartialUp = &(runtime->clut_partial_update);

	isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &clut_saturation_attr);

	if (clut_saturation_attr->Enable) {
		if (runtime->tun_lut_updated_flage == CLUT_OFFLINE_FULL_UPDATE_SYMBOL) {
			runtime->postprocess_lut_updating = CVI_FALSE;
			runtime->bSatLutUpdated = CVI_TRUE;
			runtime->clut_partial_update.updateListLen = 0;
			ptPartialUp->u8LastSyncIdx[0] = 0;
			ptPartialUp->u8LastSyncIdx[1] = 0;
			ptPartialUp->updateFailCnt[0] = 0;
			ptPartialUp->updateFailCnt[1] = 0;
		} else {
			CVI_U32 u32CurSyncIdx = ViPipe;

			G_EXT_CTRLS_VALUE(VI_IOCTL_GET_CLUT_TBL_IDX, tun_idx, &u32CurSyncIdx);
			if ((u32CurSyncIdx != ptPartialUp->u8LastSyncIdx[tun_idx])
					&& (ptPartialUp->u8LastSyncIdx[tun_idx] != 0)) {
				if (ptPartialUp->updateFailCnt[tun_idx]++ > 50) {
					ptPartialUp->updateFailCnt[tun_idx] = 0;
					ISP_LOG_ERR("ViPipe=%d,tun_idx=%d,clut lut partial update fail reset index.\n",
						ViPipe, tun_idx);
				}
			} else {
				ptPartialUp->updateFailCnt[tun_idx] = 0;
				CVI_U32 len = runtime->clut_partial_update.updateListLen;
				CVI_U32 idx = (len % CLUT_PARTIAL_UPDATE_SIZE) ? (len / CLUT_PARTIAL_UPDATE_SIZE + 1)
					: (len / CLUT_PARTIAL_UPDATE_SIZE);

				if (ptPartialUp->u8LastSyncIdx[tun_idx] == idx) {
					ptPartialUp->updateFailCnt[tun_idx] = 0;
					runtime->tun_lut_updated_flage |= 0xf <<  (tun_idx * 4);
					clut_cfg->update = 0;
					clut_cfg->is_update_partial = 0;
				} else {
					CVI_U32 u32DataCount = 0;
					CVI_U32 u32LutIdx;

					u32LutIdx = ptPartialUp->u8LastSyncIdx[tun_idx] * CLUT_PARTIAL_UPDATE_SIZE;
					for (CVI_U32 i = 0; ((i < CLUT_PARTIAL_UPDATE_SIZE)
						&& (u32LutIdx < len)); ++i, ++u32LutIdx) {
						// 0: addr, 1: value
						clut_cfg->lut[i][0] = runtime->clut_param_out.updateList.items[i].addr;
						clut_cfg->lut[i][1] = runtime->clut_param_out.updateList.items[i].data;
						u32DataCount++;
					}

					ptPartialUp->u8LastSyncIdx[tun_idx]++;
					clut_cfg->update = 1;
					clut_cfg->is_update_partial = 1;
					clut_cfg->tbl_idx = ptPartialUp->u8LastSyncIdx[tun_idx];
					clut_cfg->update_length = u32DataCount;
				}
			}
		}
	} else {
		runtime->postprocess_lut_updating = CVI_FALSE;
		runtime->tun_lut_updated_flage = 0;
		ptPartialUp->u8LastSyncIdx[0] = 0;
		ptPartialUp->u8LastSyncIdx[1] = 0;
		ptPartialUp->updateFailCnt[0] = 0;
		ptPartialUp->updateFailCnt[1] = 0;
		if (runtime->bSatLutUpdated) {
			runtime->postprocess_table_updating = CVI_TRUE;
			runtime->tun_table_updated_flage = 0;
			runtime->bSatLutUpdated = CVI_FALSE;
		}
	}

	return 0;
}

static CVI_S32 set_clut_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_CLUT_ATTR_S *clut_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_clut_ctrl_get_clut_attr(ViPipe, &clut_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->CLutEnable = clut_attr->Enable;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}

static struct isp_clut_ctrl_runtime  *_get_clut_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_clut_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_clut_ctrl_check_clut_attr_valid(const ISP_CLUT_ATTR_S *pstCLUTAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstCLUTAttr, Enable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstCLUTAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutR, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutG, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);
	CHECK_VALID_ARRAY_1D(pstCLUTAttr, ClutB, ISP_CLUT_LUT_LENGTH, 0x0, 0x3ff);

	return ret;
}

static CVI_S32 isp_clut_ctrl_check_clut_saturation_attr_valid(const ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstClutSaturationAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_AUTO_ISO_2D(pstClutSaturationAttr, SatIn, 4, 0x0, 0x2000);
	CHECK_VALID_AUTO_ISO_2D(pstClutSaturationAttr, SatOut, 4, 0x0, 0x2000);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_clut_ctrl_get_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S **pstCLUTAttr)
{
	if (pstCLUTAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buffer);
	*pstCLUTAttr = &shared_buffer->stCLUTAttr;

	return ret;
}

CVI_S32 isp_clut_ctrl_set_clut_attr(VI_PIPE ViPipe, const ISP_CLUT_ATTR_S *pstCLUTAttr)
{
	if (pstCLUTAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_clut_ctrl_check_clut_attr_valid(pstCLUTAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CLUT_ATTR_S *p = CVI_NULL;

	isp_clut_ctrl_get_clut_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstCLUTAttr, sizeof(*pstCLUTAttr));

	runtime->preprocess_table_updated = CVI_TRUE;
	runtime->tun_table_updated_flage = 0;

	return CVI_SUCCESS;
}

CVI_S32 isp_clut_ctrl_get_clut_saturation_attr(VI_PIPE ViPipe,
	const ISP_CLUT_SATURATION_ATTR_S **pstClutSaturationAttr)
{
	if (pstClutSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_CLUT, (CVI_VOID *) &shared_buffer);
	*pstClutSaturationAttr = &shared_buffer->stClutSaturationAttr;

	return ret;
}

CVI_S32 isp_clut_ctrl_set_clut_saturation_attr(VI_PIPE ViPipe,
	const ISP_CLUT_SATURATION_ATTR_S *pstClutSaturationAttr)
{
	if (pstClutSaturationAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_clut_ctrl_runtime *runtime = _get_clut_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_clut_ctrl_check_clut_saturation_attr_valid(pstClutSaturationAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_CLUT_SATURATION_ATTR_S *p = CVI_NULL;

	isp_clut_ctrl_get_clut_saturation_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstClutSaturationAttr, sizeof(*pstClutSaturationAttr));

	runtime->preprocess_lut_updated = CVI_TRUE;
	runtime->tun_lut_updated_flage = 0;

	return CVI_SUCCESS;
}

