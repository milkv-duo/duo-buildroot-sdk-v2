/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_dehaze_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"
#include "isp_ioctl.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"
#include "isp_sts_ctrl.h"

#include "isp_dehaze_ctrl.h"
#include "isp_mgr_buf.h"

#ifndef ENABLE_ISP_C906L
const struct isp_module_ctrl dehaze_mod = {
	.init = isp_dehaze_ctrl_init,
	.uninit = isp_dehaze_ctrl_uninit,
	.suspend = isp_dehaze_ctrl_suspend,
	.resume = isp_dehaze_ctrl_resume,
	.ctrl = isp_dehaze_ctrl_ctrl
};

static CVI_S32 isp_dehaze_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dehaze_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_dehaze_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_dehaze_ctrl_postprocess(VI_PIPE ViPipe);

static CVI_S32 set_dehaze_proc_info(VI_PIPE ViPipe);
static CVI_S32 isp_dehaze_ctrl_check_dehaze_attr_valid(const ISP_DEHAZE_ATTR_S *pstdehazeAttr);

static struct isp_dehaze_ctrl_runtime  *_get_dehaze_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_dehaze_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_dehaze_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dehaze_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dehaze_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_dehaze_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_dehaze_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassDehaze;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassDehaze = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dehaze_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_dehaze_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dehaze_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	ret = isp_dehaze_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_dehaze_proc_info(ViPipe);

	return ret;
}

static CVI_S32 isp_dehaze_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_DEHAZE_ATTR_S *dehaze_attr = NULL;

	isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &dehaze_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(dehaze_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!dehaze_attr->Enable || runtime->is_module_bypass)
		return ret;

	if (dehaze_attr->enOpType == OP_TYPE_MANUAL) {
		#define MANUAL(_attr, _param) \
		runtime->_attr._param = _attr->stManual._param

		MANUAL(dehaze_attr, Strength);

		#undef MANUAL
	} else {
		#define AUTO(_attr, _param, type) \
		runtime->_attr._param = INTERPOLATE_LINEAR(ViPipe, type, _attr->stAuto._param)

		AUTO(dehaze_attr, Strength, INTPLT_POST_ISO);

		#undef AUTO
	}

	runtime->dehaze_param_in.DehazeLumaCOEFFI = dehaze_attr->DehazeLumaCOEFFI;
	runtime->dehaze_param_in.DehazeSkinCOEFFI = dehaze_attr->DehazeSkinCOEFFI;
	runtime->dehaze_param_in.TransMapWgtWgt = dehaze_attr->TransMapWgtWgt;
	runtime->dehaze_param_in.TransMapWgtSigma = dehaze_attr->TransMapWgtSigma;

	runtime->process_updated = CVI_TRUE;

	return ret;
}

static CVI_S32 isp_dehaze_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_dehaze_main(
		(struct dehaze_param_in *)&runtime->dehaze_param_in,
		(struct dehaze_param_out *)&runtime->dehaze_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 isp_dehaze_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;

	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_dhz_config *dhz_cfg =
		(struct cvi_vip_isp_dhz_config *)&(post_addr->tun_cfg[tun_idx].dhz_cfg);

	const ISP_DEHAZE_ATTR_S *dehaze_attr = NULL;

	isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &dehaze_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		dhz_cfg->update = 0;
	} else {
		dhz_cfg->update = 1;
		dhz_cfg->enable = dehaze_attr->Enable && !runtime->is_module_bypass;
		dhz_cfg->strength = runtime->dehaze_attr.Strength;
		dhz_cfg->cum_th = dehaze_attr->CumulativeThr;
		dhz_cfg->hist_th = 1024;
		dhz_cfg->tmap_min = dehaze_attr->MinTransMapValue;
		dhz_cfg->tmap_max = 8191;
		dhz_cfg->th_smooth = 200;
		dhz_cfg->luma_lut_enable = dehaze_attr->DehazeLumaEnable;
		dhz_cfg->skin_lut_enable = dehaze_attr->DehazeSkinEnable;
		dhz_cfg->a_luma_wgt = dehaze_attr->AirLightMixWgt;
		dhz_cfg->blend_wgt = dehaze_attr->DehazeWgt;
		dhz_cfg->tmap_scale = dehaze_attr->TransMapScale;
		dhz_cfg->d_wgt = dehaze_attr->AirlightDiffWgt;
		dhz_cfg->sw_dc_th = 1000;
		dhz_cfg->sw_aglobal_r = 3840;
		dhz_cfg->sw_aglobal_g = 3840;
		dhz_cfg->sw_aglobal_b = 3840;
		dhz_cfg->aglobal_max = dehaze_attr->AirLightMax;
		dhz_cfg->aglobal_min = dehaze_attr->AirLightMin;
		dhz_cfg->skin_cb = dehaze_attr->SkinCb;
		dhz_cfg->skin_cr = dehaze_attr->SkinCr;

		// This solution (fix aglobal) is only for CV182x/CV183x. (SOC doesn't support HW and SW IIR)
		// In next generation, we should implement IIR to AGlobal coefficient.
		// dhz_cfg->sw_dc_aglobal_trig = 1;

		struct cvi_vip_isp_dhz_luma_tun_cfg *luma_cfg =  &dhz_cfg->luma_cfg;

		luma_cfg->LUMA_00.bits.DEHAZE_LUMA_LUT00 = runtime->dehaze_param_out.DehazeLumaLut[0];
		luma_cfg->LUMA_00.bits.DEHAZE_LUMA_LUT01 = runtime->dehaze_param_out.DehazeLumaLut[1];
		luma_cfg->LUMA_00.bits.DEHAZE_LUMA_LUT02 = runtime->dehaze_param_out.DehazeLumaLut[2];
		luma_cfg->LUMA_00.bits.DEHAZE_LUMA_LUT03 = runtime->dehaze_param_out.DehazeLumaLut[3];
		luma_cfg->LUMA_04.bits.DEHAZE_LUMA_LUT04 = runtime->dehaze_param_out.DehazeLumaLut[4];
		luma_cfg->LUMA_04.bits.DEHAZE_LUMA_LUT05 = runtime->dehaze_param_out.DehazeLumaLut[5];
		luma_cfg->LUMA_04.bits.DEHAZE_LUMA_LUT06 = runtime->dehaze_param_out.DehazeLumaLut[6];
		luma_cfg->LUMA_04.bits.DEHAZE_LUMA_LUT07 = runtime->dehaze_param_out.DehazeLumaLut[7];
		luma_cfg->LUMA_08.bits.DEHAZE_LUMA_LUT08 = runtime->dehaze_param_out.DehazeLumaLut[8];
		luma_cfg->LUMA_08.bits.DEHAZE_LUMA_LUT09 = runtime->dehaze_param_out.DehazeLumaLut[9];
		luma_cfg->LUMA_08.bits.DEHAZE_LUMA_LUT10 = runtime->dehaze_param_out.DehazeLumaLut[10];
		luma_cfg->LUMA_08.bits.DEHAZE_LUMA_LUT11 = runtime->dehaze_param_out.DehazeLumaLut[11];
		luma_cfg->LUMA_12.bits.DEHAZE_LUMA_LUT12 = runtime->dehaze_param_out.DehazeLumaLut[12];
		luma_cfg->LUMA_12.bits.DEHAZE_LUMA_LUT13 = runtime->dehaze_param_out.DehazeLumaLut[13];
		luma_cfg->LUMA_12.bits.DEHAZE_LUMA_LUT14 = runtime->dehaze_param_out.DehazeLumaLut[14];
		luma_cfg->LUMA_12.bits.DEHAZE_LUMA_LUT15 = runtime->dehaze_param_out.DehazeLumaLut[15];

		struct cvi_vip_isp_dhz_skin_tun_cfg *skin_cfg =  &dhz_cfg->skin_cfg;

		skin_cfg->SKIN_00.bits.DEHAZE_SKIN_LUT00 = runtime->dehaze_param_out.DehazeSkinLut[0];
		skin_cfg->SKIN_00.bits.DEHAZE_SKIN_LUT01 = runtime->dehaze_param_out.DehazeSkinLut[1];
		skin_cfg->SKIN_00.bits.DEHAZE_SKIN_LUT02 = runtime->dehaze_param_out.DehazeSkinLut[2];
		skin_cfg->SKIN_00.bits.DEHAZE_SKIN_LUT03 = runtime->dehaze_param_out.DehazeSkinLut[3];
		skin_cfg->SKIN_04.bits.DEHAZE_SKIN_LUT04 = runtime->dehaze_param_out.DehazeSkinLut[4];
		skin_cfg->SKIN_04.bits.DEHAZE_SKIN_LUT05 = runtime->dehaze_param_out.DehazeSkinLut[5];
		skin_cfg->SKIN_04.bits.DEHAZE_SKIN_LUT06 = runtime->dehaze_param_out.DehazeSkinLut[6];
		skin_cfg->SKIN_04.bits.DEHAZE_SKIN_LUT07 = runtime->dehaze_param_out.DehazeSkinLut[7];
		skin_cfg->SKIN_08.bits.DEHAZE_SKIN_LUT08 = runtime->dehaze_param_out.DehazeSkinLut[8];
		skin_cfg->SKIN_08.bits.DEHAZE_SKIN_LUT09 = runtime->dehaze_param_out.DehazeSkinLut[9];
		skin_cfg->SKIN_08.bits.DEHAZE_SKIN_LUT10 = runtime->dehaze_param_out.DehazeSkinLut[10];
		skin_cfg->SKIN_08.bits.DEHAZE_SKIN_LUT11 = runtime->dehaze_param_out.DehazeSkinLut[11];
		skin_cfg->SKIN_12.bits.DEHAZE_SKIN_LUT12 = runtime->dehaze_param_out.DehazeSkinLut[12];
		skin_cfg->SKIN_12.bits.DEHAZE_SKIN_LUT13 = runtime->dehaze_param_out.DehazeSkinLut[13];
		skin_cfg->SKIN_12.bits.DEHAZE_SKIN_LUT14 = runtime->dehaze_param_out.DehazeSkinLut[14];
		skin_cfg->SKIN_12.bits.DEHAZE_SKIN_LUT15 = runtime->dehaze_param_out.DehazeSkinLut[15];

		struct cvi_vip_isp_dhz_tmap_tun_cfg *tmap_cfg =  &dhz_cfg->tmap_cfg;

		tmap_cfg->TMAP_00.bits.DEHAZE_TMAP_GAIN_LUT000 = runtime->dehaze_param_out.DehazeTmapGainLut[0];
		tmap_cfg->TMAP_00.bits.DEHAZE_TMAP_GAIN_LUT001 = runtime->dehaze_param_out.DehazeTmapGainLut[1];
		tmap_cfg->TMAP_00.bits.DEHAZE_TMAP_GAIN_LUT002 = runtime->dehaze_param_out.DehazeTmapGainLut[2];
		tmap_cfg->TMAP_00.bits.DEHAZE_TMAP_GAIN_LUT003 = runtime->dehaze_param_out.DehazeTmapGainLut[3];
		tmap_cfg->TMAP_01.bits.DEHAZE_TMAP_GAIN_LUT004 = runtime->dehaze_param_out.DehazeTmapGainLut[4];
		tmap_cfg->TMAP_01.bits.DEHAZE_TMAP_GAIN_LUT005 = runtime->dehaze_param_out.DehazeTmapGainLut[5];
		tmap_cfg->TMAP_01.bits.DEHAZE_TMAP_GAIN_LUT006 = runtime->dehaze_param_out.DehazeTmapGainLut[6];
		tmap_cfg->TMAP_01.bits.DEHAZE_TMAP_GAIN_LUT007 = runtime->dehaze_param_out.DehazeTmapGainLut[7];
		tmap_cfg->TMAP_02.bits.DEHAZE_TMAP_GAIN_LUT008 = runtime->dehaze_param_out.DehazeTmapGainLut[8];
		tmap_cfg->TMAP_02.bits.DEHAZE_TMAP_GAIN_LUT009 = runtime->dehaze_param_out.DehazeTmapGainLut[9];
		tmap_cfg->TMAP_02.bits.DEHAZE_TMAP_GAIN_LUT010 = runtime->dehaze_param_out.DehazeTmapGainLut[10];
		tmap_cfg->TMAP_02.bits.DEHAZE_TMAP_GAIN_LUT011 = runtime->dehaze_param_out.DehazeTmapGainLut[11];
		tmap_cfg->TMAP_03.bits.DEHAZE_TMAP_GAIN_LUT012 = runtime->dehaze_param_out.DehazeTmapGainLut[12];
		tmap_cfg->TMAP_03.bits.DEHAZE_TMAP_GAIN_LUT013 = runtime->dehaze_param_out.DehazeTmapGainLut[13];
		tmap_cfg->TMAP_03.bits.DEHAZE_TMAP_GAIN_LUT014 = runtime->dehaze_param_out.DehazeTmapGainLut[14];
		tmap_cfg->TMAP_03.bits.DEHAZE_TMAP_GAIN_LUT015 = runtime->dehaze_param_out.DehazeTmapGainLut[15];
		tmap_cfg->TMAP_04.bits.DEHAZE_TMAP_GAIN_LUT016 = runtime->dehaze_param_out.DehazeTmapGainLut[16];
		tmap_cfg->TMAP_04.bits.DEHAZE_TMAP_GAIN_LUT017 = runtime->dehaze_param_out.DehazeTmapGainLut[17];
		tmap_cfg->TMAP_04.bits.DEHAZE_TMAP_GAIN_LUT018 = runtime->dehaze_param_out.DehazeTmapGainLut[18];
		tmap_cfg->TMAP_04.bits.DEHAZE_TMAP_GAIN_LUT019 = runtime->dehaze_param_out.DehazeTmapGainLut[19];
		tmap_cfg->TMAP_05.bits.DEHAZE_TMAP_GAIN_LUT020 = runtime->dehaze_param_out.DehazeTmapGainLut[20];
		tmap_cfg->TMAP_05.bits.DEHAZE_TMAP_GAIN_LUT021 = runtime->dehaze_param_out.DehazeTmapGainLut[21];
		tmap_cfg->TMAP_05.bits.DEHAZE_TMAP_GAIN_LUT022 = runtime->dehaze_param_out.DehazeTmapGainLut[22];
		tmap_cfg->TMAP_05.bits.DEHAZE_TMAP_GAIN_LUT023 = runtime->dehaze_param_out.DehazeTmapGainLut[23];
		tmap_cfg->TMAP_06.bits.DEHAZE_TMAP_GAIN_LUT024 = runtime->dehaze_param_out.DehazeTmapGainLut[24];
		tmap_cfg->TMAP_06.bits.DEHAZE_TMAP_GAIN_LUT025 = runtime->dehaze_param_out.DehazeTmapGainLut[25];
		tmap_cfg->TMAP_06.bits.DEHAZE_TMAP_GAIN_LUT026 = runtime->dehaze_param_out.DehazeTmapGainLut[26];
		tmap_cfg->TMAP_06.bits.DEHAZE_TMAP_GAIN_LUT027 = runtime->dehaze_param_out.DehazeTmapGainLut[27];
		tmap_cfg->TMAP_07.bits.DEHAZE_TMAP_GAIN_LUT028 = runtime->dehaze_param_out.DehazeTmapGainLut[28];
		tmap_cfg->TMAP_07.bits.DEHAZE_TMAP_GAIN_LUT029 = runtime->dehaze_param_out.DehazeTmapGainLut[29];
		tmap_cfg->TMAP_07.bits.DEHAZE_TMAP_GAIN_LUT030 = runtime->dehaze_param_out.DehazeTmapGainLut[30];
		tmap_cfg->TMAP_07.bits.DEHAZE_TMAP_GAIN_LUT031 = runtime->dehaze_param_out.DehazeTmapGainLut[31];
		tmap_cfg->TMAP_08.bits.DEHAZE_TMAP_GAIN_LUT032 = runtime->dehaze_param_out.DehazeTmapGainLut[32];
		tmap_cfg->TMAP_08.bits.DEHAZE_TMAP_GAIN_LUT033 = runtime->dehaze_param_out.DehazeTmapGainLut[33];
		tmap_cfg->TMAP_08.bits.DEHAZE_TMAP_GAIN_LUT034 = runtime->dehaze_param_out.DehazeTmapGainLut[34];
		tmap_cfg->TMAP_08.bits.DEHAZE_TMAP_GAIN_LUT035 = runtime->dehaze_param_out.DehazeTmapGainLut[35];
		tmap_cfg->TMAP_09.bits.DEHAZE_TMAP_GAIN_LUT036 = runtime->dehaze_param_out.DehazeTmapGainLut[36];
		tmap_cfg->TMAP_09.bits.DEHAZE_TMAP_GAIN_LUT037 = runtime->dehaze_param_out.DehazeTmapGainLut[37];
		tmap_cfg->TMAP_09.bits.DEHAZE_TMAP_GAIN_LUT038 = runtime->dehaze_param_out.DehazeTmapGainLut[38];
		tmap_cfg->TMAP_09.bits.DEHAZE_TMAP_GAIN_LUT039 = runtime->dehaze_param_out.DehazeTmapGainLut[39];
		tmap_cfg->TMAP_10.bits.DEHAZE_TMAP_GAIN_LUT040 = runtime->dehaze_param_out.DehazeTmapGainLut[40];
		tmap_cfg->TMAP_10.bits.DEHAZE_TMAP_GAIN_LUT041 = runtime->dehaze_param_out.DehazeTmapGainLut[41];
		tmap_cfg->TMAP_10.bits.DEHAZE_TMAP_GAIN_LUT042 = runtime->dehaze_param_out.DehazeTmapGainLut[42];
		tmap_cfg->TMAP_10.bits.DEHAZE_TMAP_GAIN_LUT043 = runtime->dehaze_param_out.DehazeTmapGainLut[43];
		tmap_cfg->TMAP_11.bits.DEHAZE_TMAP_GAIN_LUT044 = runtime->dehaze_param_out.DehazeTmapGainLut[44];
		tmap_cfg->TMAP_11.bits.DEHAZE_TMAP_GAIN_LUT045 = runtime->dehaze_param_out.DehazeTmapGainLut[45];
		tmap_cfg->TMAP_11.bits.DEHAZE_TMAP_GAIN_LUT046 = runtime->dehaze_param_out.DehazeTmapGainLut[46];
		tmap_cfg->TMAP_11.bits.DEHAZE_TMAP_GAIN_LUT047 = runtime->dehaze_param_out.DehazeTmapGainLut[47];
		tmap_cfg->TMAP_12.bits.DEHAZE_TMAP_GAIN_LUT048 = runtime->dehaze_param_out.DehazeTmapGainLut[48];
		tmap_cfg->TMAP_12.bits.DEHAZE_TMAP_GAIN_LUT049 = runtime->dehaze_param_out.DehazeTmapGainLut[49];
		tmap_cfg->TMAP_12.bits.DEHAZE_TMAP_GAIN_LUT050 = runtime->dehaze_param_out.DehazeTmapGainLut[50];
		tmap_cfg->TMAP_12.bits.DEHAZE_TMAP_GAIN_LUT051 = runtime->dehaze_param_out.DehazeTmapGainLut[51];
		tmap_cfg->TMAP_13.bits.DEHAZE_TMAP_GAIN_LUT052 = runtime->dehaze_param_out.DehazeTmapGainLut[52];
		tmap_cfg->TMAP_13.bits.DEHAZE_TMAP_GAIN_LUT053 = runtime->dehaze_param_out.DehazeTmapGainLut[53];
		tmap_cfg->TMAP_13.bits.DEHAZE_TMAP_GAIN_LUT054 = runtime->dehaze_param_out.DehazeTmapGainLut[54];
		tmap_cfg->TMAP_13.bits.DEHAZE_TMAP_GAIN_LUT055 = runtime->dehaze_param_out.DehazeTmapGainLut[55];
		tmap_cfg->TMAP_14.bits.DEHAZE_TMAP_GAIN_LUT056 = runtime->dehaze_param_out.DehazeTmapGainLut[56];
		tmap_cfg->TMAP_14.bits.DEHAZE_TMAP_GAIN_LUT057 = runtime->dehaze_param_out.DehazeTmapGainLut[57];
		tmap_cfg->TMAP_14.bits.DEHAZE_TMAP_GAIN_LUT058 = runtime->dehaze_param_out.DehazeTmapGainLut[58];
		tmap_cfg->TMAP_14.bits.DEHAZE_TMAP_GAIN_LUT059 = runtime->dehaze_param_out.DehazeTmapGainLut[59];
		tmap_cfg->TMAP_15.bits.DEHAZE_TMAP_GAIN_LUT060 = runtime->dehaze_param_out.DehazeTmapGainLut[60];
		tmap_cfg->TMAP_15.bits.DEHAZE_TMAP_GAIN_LUT061 = runtime->dehaze_param_out.DehazeTmapGainLut[61];
		tmap_cfg->TMAP_15.bits.DEHAZE_TMAP_GAIN_LUT062 = runtime->dehaze_param_out.DehazeTmapGainLut[62];
		tmap_cfg->TMAP_15.bits.DEHAZE_TMAP_GAIN_LUT063 = runtime->dehaze_param_out.DehazeTmapGainLut[63];
		tmap_cfg->TMAP_16.bits.DEHAZE_TMAP_GAIN_LUT064 = runtime->dehaze_param_out.DehazeTmapGainLut[64];
		tmap_cfg->TMAP_16.bits.DEHAZE_TMAP_GAIN_LUT065 = runtime->dehaze_param_out.DehazeTmapGainLut[65];
		tmap_cfg->TMAP_16.bits.DEHAZE_TMAP_GAIN_LUT066 = runtime->dehaze_param_out.DehazeTmapGainLut[66];
		tmap_cfg->TMAP_16.bits.DEHAZE_TMAP_GAIN_LUT067 = runtime->dehaze_param_out.DehazeTmapGainLut[67];
		tmap_cfg->TMAP_17.bits.DEHAZE_TMAP_GAIN_LUT068 = runtime->dehaze_param_out.DehazeTmapGainLut[68];
		tmap_cfg->TMAP_17.bits.DEHAZE_TMAP_GAIN_LUT069 = runtime->dehaze_param_out.DehazeTmapGainLut[69];
		tmap_cfg->TMAP_17.bits.DEHAZE_TMAP_GAIN_LUT070 = runtime->dehaze_param_out.DehazeTmapGainLut[70];
		tmap_cfg->TMAP_17.bits.DEHAZE_TMAP_GAIN_LUT071 = runtime->dehaze_param_out.DehazeTmapGainLut[71];
		tmap_cfg->TMAP_18.bits.DEHAZE_TMAP_GAIN_LUT072 = runtime->dehaze_param_out.DehazeTmapGainLut[72];
		tmap_cfg->TMAP_18.bits.DEHAZE_TMAP_GAIN_LUT073 = runtime->dehaze_param_out.DehazeTmapGainLut[73];
		tmap_cfg->TMAP_18.bits.DEHAZE_TMAP_GAIN_LUT074 = runtime->dehaze_param_out.DehazeTmapGainLut[74];
		tmap_cfg->TMAP_18.bits.DEHAZE_TMAP_GAIN_LUT075 = runtime->dehaze_param_out.DehazeTmapGainLut[75];
		tmap_cfg->TMAP_19.bits.DEHAZE_TMAP_GAIN_LUT076 = runtime->dehaze_param_out.DehazeTmapGainLut[76];
		tmap_cfg->TMAP_19.bits.DEHAZE_TMAP_GAIN_LUT077 = runtime->dehaze_param_out.DehazeTmapGainLut[77];
		tmap_cfg->TMAP_19.bits.DEHAZE_TMAP_GAIN_LUT078 = runtime->dehaze_param_out.DehazeTmapGainLut[78];
		tmap_cfg->TMAP_19.bits.DEHAZE_TMAP_GAIN_LUT079 = runtime->dehaze_param_out.DehazeTmapGainLut[79];
		tmap_cfg->TMAP_20.bits.DEHAZE_TMAP_GAIN_LUT080 = runtime->dehaze_param_out.DehazeTmapGainLut[80];
		tmap_cfg->TMAP_20.bits.DEHAZE_TMAP_GAIN_LUT081 = runtime->dehaze_param_out.DehazeTmapGainLut[81];
		tmap_cfg->TMAP_20.bits.DEHAZE_TMAP_GAIN_LUT082 = runtime->dehaze_param_out.DehazeTmapGainLut[82];
		tmap_cfg->TMAP_20.bits.DEHAZE_TMAP_GAIN_LUT083 = runtime->dehaze_param_out.DehazeTmapGainLut[83];
		tmap_cfg->TMAP_21.bits.DEHAZE_TMAP_GAIN_LUT084 = runtime->dehaze_param_out.DehazeTmapGainLut[84];
		tmap_cfg->TMAP_21.bits.DEHAZE_TMAP_GAIN_LUT085 = runtime->dehaze_param_out.DehazeTmapGainLut[85];
		tmap_cfg->TMAP_21.bits.DEHAZE_TMAP_GAIN_LUT086 = runtime->dehaze_param_out.DehazeTmapGainLut[86];
		tmap_cfg->TMAP_21.bits.DEHAZE_TMAP_GAIN_LUT087 = runtime->dehaze_param_out.DehazeTmapGainLut[87];
		tmap_cfg->TMAP_22.bits.DEHAZE_TMAP_GAIN_LUT088 = runtime->dehaze_param_out.DehazeTmapGainLut[88];
		tmap_cfg->TMAP_22.bits.DEHAZE_TMAP_GAIN_LUT089 = runtime->dehaze_param_out.DehazeTmapGainLut[89];
		tmap_cfg->TMAP_22.bits.DEHAZE_TMAP_GAIN_LUT090 = runtime->dehaze_param_out.DehazeTmapGainLut[90];
		tmap_cfg->TMAP_22.bits.DEHAZE_TMAP_GAIN_LUT091 = runtime->dehaze_param_out.DehazeTmapGainLut[91];
		tmap_cfg->TMAP_23.bits.DEHAZE_TMAP_GAIN_LUT092 = runtime->dehaze_param_out.DehazeTmapGainLut[92];
		tmap_cfg->TMAP_23.bits.DEHAZE_TMAP_GAIN_LUT093 = runtime->dehaze_param_out.DehazeTmapGainLut[93];
		tmap_cfg->TMAP_23.bits.DEHAZE_TMAP_GAIN_LUT094 = runtime->dehaze_param_out.DehazeTmapGainLut[94];
		tmap_cfg->TMAP_23.bits.DEHAZE_TMAP_GAIN_LUT095 = runtime->dehaze_param_out.DehazeTmapGainLut[95];
		tmap_cfg->TMAP_24.bits.DEHAZE_TMAP_GAIN_LUT096 = runtime->dehaze_param_out.DehazeTmapGainLut[96];
		tmap_cfg->TMAP_24.bits.DEHAZE_TMAP_GAIN_LUT097 = runtime->dehaze_param_out.DehazeTmapGainLut[97];
		tmap_cfg->TMAP_24.bits.DEHAZE_TMAP_GAIN_LUT098 = runtime->dehaze_param_out.DehazeTmapGainLut[98];
		tmap_cfg->TMAP_24.bits.DEHAZE_TMAP_GAIN_LUT099 = runtime->dehaze_param_out.DehazeTmapGainLut[99];
		tmap_cfg->TMAP_25.bits.DEHAZE_TMAP_GAIN_LUT100 = runtime->dehaze_param_out.DehazeTmapGainLut[100];
		tmap_cfg->TMAP_25.bits.DEHAZE_TMAP_GAIN_LUT101 = runtime->dehaze_param_out.DehazeTmapGainLut[101];
		tmap_cfg->TMAP_25.bits.DEHAZE_TMAP_GAIN_LUT102 = runtime->dehaze_param_out.DehazeTmapGainLut[102];
		tmap_cfg->TMAP_25.bits.DEHAZE_TMAP_GAIN_LUT103 = runtime->dehaze_param_out.DehazeTmapGainLut[103];
		tmap_cfg->TMAP_26.bits.DEHAZE_TMAP_GAIN_LUT104 = runtime->dehaze_param_out.DehazeTmapGainLut[104];
		tmap_cfg->TMAP_26.bits.DEHAZE_TMAP_GAIN_LUT105 = runtime->dehaze_param_out.DehazeTmapGainLut[105];
		tmap_cfg->TMAP_26.bits.DEHAZE_TMAP_GAIN_LUT106 = runtime->dehaze_param_out.DehazeTmapGainLut[106];
		tmap_cfg->TMAP_26.bits.DEHAZE_TMAP_GAIN_LUT107 = runtime->dehaze_param_out.DehazeTmapGainLut[107];
		tmap_cfg->TMAP_27.bits.DEHAZE_TMAP_GAIN_LUT108 = runtime->dehaze_param_out.DehazeTmapGainLut[108];
		tmap_cfg->TMAP_27.bits.DEHAZE_TMAP_GAIN_LUT109 = runtime->dehaze_param_out.DehazeTmapGainLut[109];
		tmap_cfg->TMAP_27.bits.DEHAZE_TMAP_GAIN_LUT110 = runtime->dehaze_param_out.DehazeTmapGainLut[110];
		tmap_cfg->TMAP_27.bits.DEHAZE_TMAP_GAIN_LUT111 = runtime->dehaze_param_out.DehazeTmapGainLut[111];
		tmap_cfg->TMAP_28.bits.DEHAZE_TMAP_GAIN_LUT112 = runtime->dehaze_param_out.DehazeTmapGainLut[112];
		tmap_cfg->TMAP_28.bits.DEHAZE_TMAP_GAIN_LUT113 = runtime->dehaze_param_out.DehazeTmapGainLut[113];
		tmap_cfg->TMAP_28.bits.DEHAZE_TMAP_GAIN_LUT114 = runtime->dehaze_param_out.DehazeTmapGainLut[114];
		tmap_cfg->TMAP_28.bits.DEHAZE_TMAP_GAIN_LUT115 = runtime->dehaze_param_out.DehazeTmapGainLut[115];
		tmap_cfg->TMAP_29.bits.DEHAZE_TMAP_GAIN_LUT116 = runtime->dehaze_param_out.DehazeTmapGainLut[116];
		tmap_cfg->TMAP_29.bits.DEHAZE_TMAP_GAIN_LUT117 = runtime->dehaze_param_out.DehazeTmapGainLut[117];
		tmap_cfg->TMAP_29.bits.DEHAZE_TMAP_GAIN_LUT118 = runtime->dehaze_param_out.DehazeTmapGainLut[118];
		tmap_cfg->TMAP_29.bits.DEHAZE_TMAP_GAIN_LUT119 = runtime->dehaze_param_out.DehazeTmapGainLut[119];
		tmap_cfg->TMAP_30.bits.DEHAZE_TMAP_GAIN_LUT120 = runtime->dehaze_param_out.DehazeTmapGainLut[120];
		tmap_cfg->TMAP_30.bits.DEHAZE_TMAP_GAIN_LUT121 = runtime->dehaze_param_out.DehazeTmapGainLut[121];
		tmap_cfg->TMAP_30.bits.DEHAZE_TMAP_GAIN_LUT122 = runtime->dehaze_param_out.DehazeTmapGainLut[122];
		tmap_cfg->TMAP_30.bits.DEHAZE_TMAP_GAIN_LUT123 = runtime->dehaze_param_out.DehazeTmapGainLut[123];
		tmap_cfg->TMAP_31.bits.DEHAZE_TMAP_GAIN_LUT124 = runtime->dehaze_param_out.DehazeTmapGainLut[124];
		tmap_cfg->TMAP_31.bits.DEHAZE_TMAP_GAIN_LUT125 = runtime->dehaze_param_out.DehazeTmapGainLut[125];
		tmap_cfg->TMAP_31.bits.DEHAZE_TMAP_GAIN_LUT126 = runtime->dehaze_param_out.DehazeTmapGainLut[126];
		tmap_cfg->TMAP_31.bits.DEHAZE_TMAP_GAIN_LUT127 = runtime->dehaze_param_out.DehazeTmapGainLut[127];
		tmap_cfg->TMAP_32.bits.DEHAZE_TMAP_GAIN_LUT128 = runtime->dehaze_param_out.DehazeTmapGainLut[128];

	}

	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_dehaze_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_DEHAZE_ATTR_S *dehaze_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &dehaze_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->DehazeEnable = dehaze_attr->Enable;
		pProcST->DehazeisManualMode = dehaze_attr->enOpType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_dehaze_ctrl_runtime  *_get_dehaze_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_dehaze_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DEHAZE, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_dehaze_ctrl_check_dehaze_attr_valid(const ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstDehazeAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDehazeAttr, enOpType, OP_TYPE_AUTO, OP_TYPE_MANUAL);
	// CHECK_VALID_CONST(pstDehazeAttr, UpdateInterval, 0, 0xff);
	CHECK_VALID_CONST(pstDehazeAttr, CumulativeThr, 0x0, 0x3fff);
	CHECK_VALID_CONST(pstDehazeAttr, MinTransMapValue, 0x0, 0x1fff);
	// CHECK_VALID_CONST(pstDehazeAttr, DehazeLumaEnable, CVI_FALSE, CVI_TRUE);
	// CHECK_VALID_CONST(pstDehazeAttr, DehazeSkinEnable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_CONST(pstDehazeAttr, AirLightMixWgt, 0x0, 0x20);
	CHECK_VALID_CONST(pstDehazeAttr, DehazeWgt, 0x0, 0x20);
	// CHECK_VALID_CONST(pstDehazeAttr, TransMapScale, 0x0, 0xff);
	CHECK_VALID_CONST(pstDehazeAttr, AirlightDiffWgt, 0x0, 0x10);
	CHECK_VALID_CONST(pstDehazeAttr, AirLightMax, 0x0, 0xfff);
	CHECK_VALID_CONST(pstDehazeAttr, AirLightMin, 0x0, 0xfff);
	// CHECK_VALID_CONST(pstDehazeAttr, SkinCb, 0x0, 0xff);
	// CHECK_VALID_CONST(pstDehazeAttr, SkinCr, 0x0, 0xff);
	CHECK_VALID_CONST(pstDehazeAttr, DehazeLumaCOEFFI, 0x0, 0x7d0);
	CHECK_VALID_CONST(pstDehazeAttr, DehazeSkinCOEFFI, 0x0, 0x7d0);
	CHECK_VALID_CONST(pstDehazeAttr, TransMapWgtWgt, 0x0, 0x80);
	// CHECK_VALID_CONST(pstDehazeAttr, TransMapWgtSigma, 0x0, 0xff);

	CHECK_VALID_AUTO_ISO_1D(pstDehazeAttr, Strength, 0, 0x64);

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_dehaze_ctrl_get_dehaze_attr(VI_PIPE ViPipe, const ISP_DEHAZE_ATTR_S **pstDehazeAttr)
{
	if (pstDehazeAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_DEHAZE, (CVI_VOID *) &shared_buffer);
	*pstDehazeAttr = &shared_buffer->stDehazeAttr;

	return ret;
}

CVI_S32 isp_dehaze_ctrl_set_dehaze_attr(VI_PIPE ViPipe, const ISP_DEHAZE_ATTR_S *pstDehazeAttr)
{
	if (pstDehazeAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_dehaze_ctrl_runtime *runtime = _get_dehaze_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = isp_dehaze_ctrl_check_dehaze_attr_valid(pstDehazeAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_DEHAZE_ATTR_S *p = CVI_NULL;

	isp_dehaze_ctrl_get_dehaze_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstDehazeAttr, sizeof(*pstDehazeAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

