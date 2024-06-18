/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_blc_ctrl.c
 * Description:
 *
 */

#include <math.h>
#include "cvi_comm_isp.h"

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "isp_interpolate.h"
#include "isp_blc_ctrl.h"
 #include "isp_mgr_buf.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"

// SOC dependent
#define BLC_DATA_RANGE					(4095)		// 12bit
#define BLC_DATA_RANGE_WITH_GAIN_BASE	(4193280)	// 0xFFF * 1024 (1x)
#define BLC_GAIN_MIN					(1024)
#define BLC_GAIN_MAX					(65535)

// #define RANGE_UNSIGNED_HIGHT_LIMIT(var, max) ((var) = ((var) > (max)) ? (max) : (var))
// # apply isp gain and blc gain to the same register, maximum is 64X (1024=1X)
// isp digital gain: 32bit (1024=1X)
// blc gain: 16 bit (1024=1X)
#define ISP_DGAIN_UNIT	1024
#define APPLY_ISP_DGAIN(gain, dgain) ({\
	CVI_U64 blcgain = (gain);\
	CVI_U64 total_gain = blcgain * (dgain) / ISP_DGAIN_UNIT;\
	/*ISP_LOG_DEBUG("isp dgain=%u, blcgain=%lu, total_gain=%lu\n", (dgain), blcgain, total_gain);*/\
	total_gain = MIN(total_gain, 32767);\
})

const struct isp_module_ctrl blc_mod = {
	.init = isp_blc_ctrl_init,
	.uninit = isp_blc_ctrl_uninit,
	.suspend = isp_blc_ctrl_suspend,
	.resume = isp_blc_ctrl_resume,
	.ctrl = isp_blc_ctrl_ctrl
};

// TODO@ST 1822dev- structure refine

enum BLC_ID {
	BLC_ID_PRE0_FE_LE,
	BLC_ID_PRE0_FE_SE,
	BLC_ID_PRE1_FE_LE,
	BLC_ID_PRE1_FE_SE,
	BLC_ID_PRE2_FE_LE,
	BLC_ID_PRE_BE_LE,
	BLC_ID_PRE_BE_SE,
	BLC_ID_MAX
};


static CVI_S32 isp_blc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_blc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_blc_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_blc_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_VOID isp_blc_ctrl_set_blc_attr_default(VI_PIPE ViPipe);
static CVI_S32 isp_blc_ctrl_check_param_valid(const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr);
static CVI_S32 set_blc_proc_info(VI_PIPE ViPipe);
static struct isp_blc_ctrl_runtime  *_get_blc_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_blc_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	isp_blc_ctrl_set_blc_attr_default(ViPipe);

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_blc_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_blc_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_blc_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_blc_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_blc_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassBlc;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassBlc = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_blc_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_blc_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_blc_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_blc_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_blc_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_blc_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_BLACK_LEVEL_ATTR_S *blc_attr = NULL;

	isp_blc_ctrl_get_blc_attr(ViPipe, &blc_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(blc_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!blc_attr->Enable || runtime->is_module_bypass)
		return ret;

	CVI_U32 au32IspPreDgain[ISP_BLC_EXP_MAX];

	au32IspPreDgain[ISP_BLC_EXP_LE] = algoResult->u32IspPreDgain;
	au32IspPreDgain[ISP_BLC_EXP_SE] = algoResult->u32IspPreDgainSE;

	if (blc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_prefix, _attr, _param) \
		runtime->_prefix##_##_attr[ISP_BLC_EXP_LE]._param \
		= runtime->_prefix##_##_attr[ISP_BLC_EXP_SE]._param \
		= _attr->stManual._param

		MANUAL(pre_fe, blc_attr, OffsetR);
		MANUAL(pre_fe, blc_attr, OffsetGr);
		MANUAL(pre_fe, blc_attr, OffsetGb);
		MANUAL(pre_fe, blc_attr, OffsetB);
		MANUAL(pre_fe, blc_attr, OffsetR2);
		MANUAL(pre_fe, blc_attr, OffsetGr2);
		MANUAL(pre_fe, blc_attr, OffsetGb2);
		MANUAL(pre_fe, blc_attr, OffsetB2);

		#undef MANUAL
	} else {
		#define AUTO(_prefix, _attr, _param, type) \
		runtime->_prefix##_##_attr[ISP_BLC_EXP_LE]._param \
		= runtime->_prefix##_##_attr[ISP_BLC_EXP_SE]._param \
		= INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(pre_fe, blc_attr, OffsetR, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetGr, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetGb, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetB, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetR2, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetGr2, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetGb2, INTPLT_PRE_BLCISO);
		AUTO(pre_fe, blc_attr, OffsetB2, INTPLT_PRE_BLCISO);

		#undef AUTO
	}

	for (CVI_U32 u32ExpIdx = 0; u32ExpIdx < ISP_BLC_EXP_MAX; ++u32ExpIdx) {
		CVI_U32 u32IspPreDGain = au32IspPreDgain[u32ExpIdx];
		CVI_U32 u32TempGain;

		if (runtime->pre_fe_blc_attr[u32ExpIdx].OffsetR < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_fe_blc_attr[u32ExpIdx].OffsetR));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_fe_blc_attr[u32ExpIdx].GainR = APPLY_ISP_DGAIN(u32TempGain, u32IspPreDGain);

		if (runtime->pre_fe_blc_attr[u32ExpIdx].OffsetGr < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_fe_blc_attr[u32ExpIdx].OffsetGr));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_fe_blc_attr[u32ExpIdx].GainGr = APPLY_ISP_DGAIN(u32TempGain, u32IspPreDGain);

		if (runtime->pre_fe_blc_attr[u32ExpIdx].OffsetGb < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_fe_blc_attr[u32ExpIdx].OffsetGb));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_fe_blc_attr[u32ExpIdx].GainGb = APPLY_ISP_DGAIN(u32TempGain, u32IspPreDGain);

		if (runtime->pre_fe_blc_attr[u32ExpIdx].OffsetB < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_fe_blc_attr[u32ExpIdx].OffsetB));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_fe_blc_attr[u32ExpIdx].GainB = APPLY_ISP_DGAIN(u32TempGain, u32IspPreDGain);
	}

	if (blc_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_prefix, _attr, _param) \
		runtime->_prefix##_##_attr[ISP_BLC_EXP_LE]._param \
		= runtime->_prefix##_##_attr[ISP_BLC_EXP_SE]._param \
		= _attr->stManual._param

		MANUAL(pre_be, blc_attr, OffsetR);
		MANUAL(pre_be, blc_attr, OffsetGr);
		MANUAL(pre_be, blc_attr, OffsetGb);
		MANUAL(pre_be, blc_attr, OffsetB);
		MANUAL(pre_be, blc_attr, OffsetR2);
		MANUAL(pre_be, blc_attr, OffsetGr2);
		MANUAL(pre_be, blc_attr, OffsetGb2);
		MANUAL(pre_be, blc_attr, OffsetB2);

		#undef MANUAL
	} else {
		#define AUTO(_prefix, _attr, _param, type) \
		runtime->_prefix##_##_attr[ISP_BLC_EXP_LE]._param \
		= runtime->_prefix##_##_attr[ISP_BLC_EXP_SE]._param \
		= INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(pre_be, blc_attr, OffsetR, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetGr, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetGb, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetB, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetR2, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetGr2, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetGb2, INTPLT_POST_BLCISO);
		AUTO(pre_be, blc_attr, OffsetB2, INTPLT_POST_BLCISO);

		#undef AUTO
	}

	// single sensor be post sbm on, dual sensor be post online,
	// so be post param effect together
	CVI_U32 au32IspPostDgain[ISP_BLC_EXP_MAX];

	au32IspPostDgain[ISP_BLC_EXP_LE] = algoResult->u32IspPostDgain;
	au32IspPostDgain[ISP_BLC_EXP_SE] = algoResult->u32IspPostDgainSE;

	for (CVI_U32 u32ExpIdx = 0; u32ExpIdx < ISP_BLC_EXP_MAX; ++u32ExpIdx) {
		CVI_U32 u32IspDGain = au32IspPostDgain[u32ExpIdx];
		CVI_U32 u32TempGain;

		if (runtime->pre_be_blc_attr[u32ExpIdx].OffsetR < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_be_blc_attr[u32ExpIdx].OffsetR));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_be_blc_attr[u32ExpIdx].GainR = APPLY_ISP_DGAIN(u32TempGain, u32IspDGain);

		if (runtime->pre_be_blc_attr[u32ExpIdx].OffsetGr < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_be_blc_attr[u32ExpIdx].OffsetGr));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_be_blc_attr[u32ExpIdx].GainGr = APPLY_ISP_DGAIN(u32TempGain, u32IspDGain);

		if (runtime->pre_be_blc_attr[u32ExpIdx].OffsetGb < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_be_blc_attr[u32ExpIdx].OffsetGb));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_be_blc_attr[u32ExpIdx].GainGb = APPLY_ISP_DGAIN(u32TempGain, u32IspDGain);

		if (runtime->pre_be_blc_attr[u32ExpIdx].OffsetB < BLC_DATA_RANGE) {
			u32TempGain = round((float)BLC_DATA_RANGE_WITH_GAIN_BASE
				/ (float)(BLC_DATA_RANGE - runtime->pre_be_blc_attr[u32ExpIdx].OffsetB));
			u32TempGain = MINMAX(u32TempGain, BLC_GAIN_MIN, BLC_GAIN_MAX);
		} else {
			u32TempGain = BLC_GAIN_MAX;
		}
		runtime->pre_be_blc_attr[u32ExpIdx].GainB = APPLY_ISP_DGAIN(u32TempGain, u32IspDGain);
	}
	runtime->process_updated = CVI_TRUE;

	return ret;
}

#if 0
static CVI_S32 isp_blc_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_blc_main(
		(struct blc_param_in *)&runtime->blc_param_in,
		(struct blc_param_out *)&runtime->blc_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_S32 isp_blc_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_BLACK_LEVEL_ATTR_S *blc_attr = NULL;

	// Set to driver
	struct cvi_vip_isp_fe_cfg *pre_fe_addr = get_pre_fe_tuning_buf_addr(ViPipe);
	struct cvi_vip_isp_be_cfg *pre_be_addr = get_pre_be_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_blc_config *pre_fe_blc_cfg[2] = {
		(struct cvi_vip_isp_blc_config *)&(pre_fe_addr->tun_cfg[tun_idx].blc_cfg[0]),
		(struct cvi_vip_isp_blc_config *)&(pre_fe_addr->tun_cfg[tun_idx].blc_cfg[1])
	};

	struct cvi_vip_isp_blc_config *pre_be_blc_cfg[2] = {
		(struct cvi_vip_isp_blc_config *)&(pre_be_addr->tun_cfg[tun_idx].blc_cfg[0]),
		(struct cvi_vip_isp_blc_config *)&(pre_be_addr->tun_cfg[tun_idx].blc_cfg[1])
	};

	CVI_U8 inst_base = 0;
	CVI_U8 pre_fe_num = 0;
	CVI_U8 pre_be_num = 0;

	switch (ViPipe) {
	case 0:
		inst_base = BLC_ID_PRE0_FE_LE;
		pre_fe_num = 2;
		pre_be_num = 2;
	break;
	case 1:
		inst_base = BLC_ID_PRE1_FE_LE;
		pre_fe_num = 2;
		pre_be_num = 2;
	break;
	case 2:
		inst_base = BLC_ID_PRE2_FE_LE;
		pre_fe_num = 1;
		pre_be_num = 2;
	break;
	case 3:// mipi switch
		inst_base = BLC_ID_PRE0_FE_LE;
		pre_fe_num = 2;
		pre_be_num = 2;
	break;
	}

	// CVI_U8 inst_base = (ViPipe == 0) ? BLC_ID_PRE0_FE_LE : BLC_ID_PRE1_FE_LE;

	isp_blc_ctrl_get_blc_attr(ViPipe, &blc_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	struct isp_blc_attr				*ptFEBlcAttr = runtime->pre_fe_blc_attr;
	struct isp_blc_attr				*ptBEBlcAttr = runtime->pre_be_blc_attr;
	struct cvi_vip_isp_blc_config	*ptFEBlcCfg = runtime->pre_fe_blc_cfg;
	struct cvi_vip_isp_blc_config	*ptBEBlcCfg = runtime->pre_be_blc_cfg;

	if (is_postprocess_update == CVI_FALSE) {
		for (CVI_U8 i = 0; i < pre_fe_num ; i++) {
			pre_fe_blc_cfg[i]->inst = i + inst_base;
			pre_fe_blc_cfg[i]->update = 0;
		}

		inst_base = BLC_ID_PRE_BE_LE;
		for (CVI_U8 i = 0; i < pre_be_num ; i++) {
			pre_be_blc_cfg[i]->inst = i + inst_base;
			pre_be_blc_cfg[i]->update = 0;
		}
	} else {
		for (CVI_U32 u32ExpIdx = 0; u32ExpIdx < ISP_BLC_EXP_MAX ; ++u32ExpIdx) {
			ptFEBlcCfg->update = 1;
			ptFEBlcCfg->bypass = 0;
			ptFEBlcCfg->roffset = ptFEBlcAttr->OffsetR;
			ptFEBlcCfg->groffset = ptFEBlcAttr->OffsetGr;
			ptFEBlcCfg->gboffset = ptFEBlcAttr->OffsetGb;
			ptFEBlcCfg->boffset = ptFEBlcAttr->OffsetB;
			ptFEBlcCfg->roffset_2nd = ptFEBlcAttr->OffsetR2;
			ptFEBlcCfg->groffset_2nd = ptFEBlcAttr->OffsetGr2;
			ptFEBlcCfg->gboffset_2nd = ptFEBlcAttr->OffsetGb2;
			ptFEBlcCfg->boffset_2nd = ptFEBlcAttr->OffsetB2;
			ptFEBlcCfg->rgain = ptFEBlcAttr->GainR;
			ptFEBlcCfg->grgain = ptFEBlcAttr->GainGr;
			ptFEBlcCfg->gbgain = ptFEBlcAttr->GainGb;
			ptFEBlcCfg->bgain = ptFEBlcAttr->GainB;

			ptBEBlcCfg->update = 1;
			ptBEBlcCfg->bypass = 0;

			ptBEBlcCfg->roffset = ptBEBlcAttr->OffsetR;
			ptBEBlcCfg->groffset = ptBEBlcAttr->OffsetGr;
			ptBEBlcCfg->gboffset = ptBEBlcAttr->OffsetGb;
			ptBEBlcCfg->boffset = ptBEBlcAttr->OffsetB;
			ptBEBlcCfg->roffset_2nd = ptBEBlcAttr->OffsetR2;
			ptBEBlcCfg->groffset_2nd = ptBEBlcAttr->OffsetGr2;
			ptBEBlcCfg->gboffset_2nd = ptBEBlcAttr->OffsetGb2;
			ptBEBlcCfg->boffset_2nd = ptBEBlcAttr->OffsetB2;
			ptBEBlcCfg->rgain = ptBEBlcAttr->GainR;
			ptBEBlcCfg->grgain = ptBEBlcAttr->GainGr;
			ptBEBlcCfg->gbgain = ptBEBlcAttr->GainGb;
			ptBEBlcCfg->bgain = ptBEBlcAttr->GainB;

			ptFEBlcAttr++;
			ptBEBlcAttr++;
			ptFEBlcCfg++;
			ptBEBlcCfg++;
		}

		ISP_CTX_S *pstIspCtx = NULL;
		CVI_BOOL *map_pipe_to_enable;

		CVI_BOOL map_pipe_to_enable_sdr[ISP_BLC_EXP_MAX] = {1, 0};
		CVI_BOOL map_pipe_to_enable_wdr[ISP_BLC_EXP_MAX] = {1, 1};

		ISP_GET_CTX(ViPipe, pstIspCtx);
		if (IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode))
			map_pipe_to_enable = map_pipe_to_enable_wdr;
		else
			map_pipe_to_enable = map_pipe_to_enable_sdr;

		for (CVI_U8 i = 0; i < pre_fe_num ; i++) {
			runtime->pre_fe_blc_cfg[i].inst = i + inst_base;
			runtime->pre_fe_blc_cfg[i].enable =
				blc_attr->Enable && map_pipe_to_enable[i] && !runtime->is_module_bypass;
			memcpy(pre_fe_blc_cfg[i], &(runtime->pre_fe_blc_cfg[i]),
				sizeof(struct cvi_vip_isp_blc_config));
		}

		inst_base = BLC_ID_PRE_BE_LE;
		for (CVI_U8 i = 0; i < pre_be_num ; i++) {
			runtime->pre_be_blc_cfg[i].inst = i + inst_base;
			runtime->pre_be_blc_cfg[i].enable =
				blc_attr->Enable && map_pipe_to_enable[i] && !runtime->is_module_bypass;
			memcpy(pre_be_blc_cfg[i], &(runtime->pre_be_blc_cfg[i]),
				sizeof(struct cvi_vip_isp_blc_config));
		}
	}
	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_VOID isp_blc_ctrl_set_blc_attr_default(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_BLACK_LEVEL_ATTR_S attr = {0};
	ISP_BLACK_LEVEL_ATTR_S *snsBlcAddr;

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_A(attr, OffsetR, 240,
	240, 240, 240, 240, 240, 240, 239, 238, 238, 233, 255, 336, 524, 917, 1014, 1010
	);
	INIT_A(attr, OffsetGr, 240,
	240, 240, 240, 240, 240, 240, 239, 238, 238, 234, 255, 338, 527, 922, 1023, 1014
	);
	INIT_A(attr, OffsetGb, 240,
	240, 240, 240, 240, 240, 240, 239, 238, 238, 232, 256, 334, 526, 919, 1013, 1015
	);
	INIT_A(attr, OffsetB, 240,
	240, 240, 240, 240, 240, 240, 239, 238, 238, 233, 256, 335, 528, 927, 1016, 1021
	);
	INIT_A(attr, OffsetR2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, OffsetGr2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, OffsetGb2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, OffsetB2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	CVI_S32 s32Ret = isp_sensor_updateBlc(ViPipe, &snsBlcAddr);

	if (s32Ret != CVI_SUCCESS) {
		ISP_LOG_WARNING("Sensor get blc fail. Use API blc value\n");
	} else {
		memcpy(&attr, snsBlcAddr, sizeof(ISP_BLACK_LEVEL_ATTR_S));
		INIT_V(attr, UpdateInterval, 1);
	}

	isp_blc_ctrl_set_blc_attr(ViPipe, &attr);
}

static CVI_S32 set_blc_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_BLACK_LEVEL_ATTR_S *blc_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_blc_ctrl_get_blc_attr(ViPipe, &blc_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->BlackLevelEnable = blc_attr->Enable;
		pProcST->BlackLevelisManualMode = blc_attr->enOpType;
	}
#else
	UNUSED(ViPipe);
#endif

	return ret;
}

static struct isp_blc_ctrl_runtime  *_get_blc_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_blc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_BLC, (CVI_VOID *) &shared_buffer);

	return &shared_buffer->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_blc_ctrl_check_param_valid(const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstBlackLevelAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstBlackLevelAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstBlackLevelAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetR, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetGr, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetGb, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetB, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetR2, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetGr2, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetGb2, 0, 0xfff);
	CHECK_VALID_AUTO_ISO_1D(pstBlackLevelAttr, OffsetB2, 0, 0xfff);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_blc_ctrl_set_blc_attr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S *pstBlackLevelAttr)
{
	if (pstBlackLevelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_blc_ctrl_check_param_valid(pstBlackLevelAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_BLACK_LEVEL_ATTR_S *p = CVI_NULL;

	isp_blc_ctrl_get_blc_attr(ViPipe, &p);
	memcpy((void *)p, pstBlackLevelAttr, sizeof(*pstBlackLevelAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_blc_ctrl_get_blc_attr(VI_PIPE ViPipe, const ISP_BLACK_LEVEL_ATTR_S **pstBlackLevelAttr)
{
	if (pstBlackLevelAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_blc_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_BLC, (CVI_VOID *) &shared_buffer);

	*pstBlackLevelAttr = &shared_buffer->stBlackLevelAttr;

	return ret;
}

CVI_S32 isp_blc_ctrl_get_blc_info(VI_PIPE ViPipe, struct blc_info *info)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_blc_ctrl_runtime *runtime = _get_blc_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	info->OffsetR = runtime->pre_fe_blc_attr[0].OffsetR;
	info->OffsetGr = runtime->pre_fe_blc_attr[0].OffsetGr;
	info->OffsetGb = runtime->pre_fe_blc_attr[0].OffsetGb;
	info->OffsetB = runtime->pre_fe_blc_attr[0].OffsetB;
	info->OffsetR2 = runtime->pre_fe_blc_attr[0].OffsetR2;
	info->OffsetGr2 = runtime->pre_fe_blc_attr[0].OffsetGr2;
	info->OffsetGb2 = runtime->pre_fe_blc_attr[0].OffsetGb2;
	info->OffsetB2 = runtime->pre_fe_blc_attr[0].OffsetB2;
	info->GainR = runtime->pre_fe_blc_attr[0].GainR;
	info->GainGr = runtime->pre_fe_blc_attr[0].GainGr;
	info->GainGb = runtime->pre_fe_blc_attr[0].GainGb;
	info->GainB = runtime->pre_fe_blc_attr[0].GainB;

	return ret;
}

