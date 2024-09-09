/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_debug.c
 * Description:
 *
 */

#include <unistd.h>
#include "isp_debug.h"

// #include "isp_main_local.h"
// #include "isp_ioctl.h"
// #include "isp_defines.h"

// #include "cvi_sys.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"
// #include "devmem.h"

// #include "isp_tun_buf_ctrl.h"
#include "isp_gamma_ctrl.h"

void isp_dbg_init(void)
{
	clog_init();

	if (access("/mnt/sd/enable_isplog.txt", F_OK) == 0) {
		clog_set_level(CLOG_LVL_DEBUG);
		clog_file_enable();
	}
}

void isp_dbg_deinit(void)
{
	clog_deinit();
}

CVI_U32 isp_dbg_get_time_diff_us(const struct timeval *pre, const struct timeval *cur)
{
	return ((cur->tv_sec - pre->tv_sec) * 1000000 + (cur->tv_usec - pre->tv_usec));
}

void CVI_DEBUG_SetDebugLevel(int level)
{
	clog_set_level(level);
	ISP_LOG_INFO("set log level = %d\n", level);
}

CVI_S32 isp_dbg_dumpFrameRawInfoToFile(VI_PIPE ViPipe, FILE *fp)
{
	ISP_EXP_INFO_S stExpInfo = {};
	ISP_WB_INFO_S stWBInfo = {};
	ISP_INNER_STATE_INFO_S stInnerStateInfo = {};
	ISP_MESH_SHADING_GAIN_LUT_ATTR_S stMLSC = {};
	ISP_DRC_ATTR_S stDrc = {};
	CVI_U16 *pu16GammaLut = NULL;

	if (CVI_ISP_QueryExposureInfo(ViPipe, &stExpInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	if (CVI_ISP_QueryWBInfo(ViPipe, &stWBInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	if (CVI_ISP_QueryInnerStateInfo(ViPipe, &stInnerStateInfo) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	if (isp_gamma_ctrl_get_real_gamma_lut(ViPipe, &pu16GammaLut) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetMeshShadingGainLutAttr(ViPipe, &stMLSC) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	if (CVI_ISP_GetDRCAttr(ViPipe, &stDrc) != CVI_SUCCESS) {
		return CVI_FAILURE;
	}

	fprintf(fp, "ISO = %u\n", stExpInfo.u32ISO);
	fprintf(fp, "Light Value = %.2f\n", stExpInfo.fLightValue);
	fprintf(fp, "Color Temp. = %u\n", stWBInfo.u16ColorTemp);
	fprintf(fp, "ISP DGain = %u\n", stExpInfo.u32ISPDGain);
	fprintf(fp, "Exposure Time = %u\n", stExpInfo.u32ExpTime);
	fprintf(fp, "Long Exposure = %u\n", stExpInfo.u32LongExpTime);
	fprintf(fp, "Short Exposure = %u\n", stExpInfo.u32ShortExpTime);
	fprintf(fp, "Exposure Ratio = %u\n", stExpInfo.u32WDRExpRatio);
	fprintf(fp, "Exposure AGain = %u\n", stExpInfo.u32AGain);
	fprintf(fp, "Exposure DGain = %u\n", stExpInfo.u32DGain);
	fprintf(fp, "Exposure AGainSF = %u\n", stExpInfo.u32AGainSF);
	fprintf(fp, "Exposure DGainSF = %u\n", stExpInfo.u32DGainSF);
	fprintf(fp, "Exposure ISPDGainSF = %u\n", stExpInfo.u32ISPDGainSF);
	fprintf(fp, "Exposure ISOSF = %u\n", stExpInfo.u32ISOSF);
	fprintf(fp, "reg_wbg_rgain = %u\n", stWBInfo.u16Rgain);
	fprintf(fp, "reg_wbg_bgain = %u\n", stWBInfo.u16Bgain);
	fprintf(fp, "reg_wbg_grgain = %u\n", stWBInfo.u16Grgain);
	fprintf(fp, "reg_wbg_gbgain = %u\n", stWBInfo.u16Gbgain);

	fprintf(fp, "reg_ccm_00 = %d\n", (CVI_S16)stInnerStateInfo.ccm[0]);
	fprintf(fp, "reg_ccm_01 = %d\n", (CVI_S16)stInnerStateInfo.ccm[1]);
	fprintf(fp, "reg_ccm_02 = %d\n", (CVI_S16)stInnerStateInfo.ccm[2]);
	fprintf(fp, "reg_ccm_10 = %d\n", (CVI_S16)stInnerStateInfo.ccm[3]);
	fprintf(fp, "reg_ccm_11 = %d\n", (CVI_S16)stInnerStateInfo.ccm[4]);
	fprintf(fp, "reg_ccm_12 = %d\n", (CVI_S16)stInnerStateInfo.ccm[5]);
	fprintf(fp, "reg_ccm_20 = %d\n", (CVI_S16)stInnerStateInfo.ccm[6]);
	fprintf(fp, "reg_ccm_21 = %d\n", (CVI_S16)stInnerStateInfo.ccm[7]);
	fprintf(fp, "reg_ccm_22 = %d\n", (CVI_S16)stInnerStateInfo.ccm[8]);

	fprintf(fp, "reg_blc_offset_r = %u\n", stInnerStateInfo.blcOffsetR);
	fprintf(fp, "reg_blc_offset_gr = %u\n", stInnerStateInfo.blcOffsetGr);
	fprintf(fp, "reg_blc_offset_gb = %u\n", stInnerStateInfo.blcOffsetGb);
	fprintf(fp, "reg_blc_offset_b = %u\n", stInnerStateInfo.blcOffsetB);
	fprintf(fp, "reg_blc_gain_r = %u\n", stInnerStateInfo.blcGainR);
	fprintf(fp, "reg_blc_gain_gr = %u\n", stInnerStateInfo.blcGainGr);
	fprintf(fp, "reg_blc_gain_gb = %u\n", stInnerStateInfo.blcGainGb);
	fprintf(fp, "reg_blc_gain_b = %u\n", stInnerStateInfo.blcGainB);

	// DRC group 1
	fprintf(fp, "DRC Enable = %u\n", stDrc.Enable);
	fprintf(fp, "DRC LocalToneEn = %u\n", stDrc.LocalToneEn);
	fprintf(fp, "DRC ToneCurveSelect = %u\n", stDrc.ToneCurveSelect);

	// DRC group 2
	fprintf(fp, "Manual.TargetYScale = %u\n", stDrc.stManual.TargetYScale);
	fprintf(fp, "Auto.TargetYScale\n");
	for (CVI_U32 u32Idx = 0; u32Idx < (ISP_AUTO_LV_NUM - 1); ++u32Idx) {
		fprintf(fp, "%d, ", stDrc.stAuto.TargetYScale[u32Idx]);
	}
	fprintf(fp, "%d\n", stDrc.stAuto.TargetYScale[ISP_AUTO_LV_NUM - 1]);

	// DRC group 3
	fprintf(fp, "Manual.HdrStrength = %u\n", stDrc.stManual.HdrStrength);
	fprintf(fp, "Auto.HdrStrength\n");
	for (CVI_U32 u32Idx = 0; u32Idx < (ISP_AUTO_LV_NUM - 1); ++u32Idx) {
		fprintf(fp, "%d, ", stDrc.stAuto.HdrStrength[u32Idx]);
	}
	fprintf(fp, "%d\n", stDrc.stAuto.HdrStrength[ISP_AUTO_LV_NUM - 1]);

	// DRC group 4 (Linear DRC)
	// DRC Curve User Define (DRC_GLOBAL_USER_DEFINE_NUM)
	fprintf(fp, "DRC CurveUserDefine =");
	for (CVI_U32 u32PntIdx = 0; u32PntIdx < DRC_GLOBAL_USER_DEFINE_NUM; ++u32PntIdx) {
		if ((u32PntIdx % 32) == 0) {
			fprintf(fp, "\n");
		}
		fprintf(fp, "%d, ", stDrc.CurveUserDefine[u32PntIdx]);
	}
	fprintf(fp, "\n");

	// DRC Dark User Define (DRC_DARK_USER_DEFINE_NUM)
	fprintf(fp, "DRC DarkUserDefine =");
	for (CVI_U32 u32PntIdx = 0; u32PntIdx < DRC_DARK_USER_DEFINE_NUM; ++u32PntIdx) {
		if ((u32PntIdx % 32) == 0) {
			fprintf(fp, "\n");
		}
		fprintf(fp, "%d, ", stDrc.DarkUserDefine[u32PntIdx]);
	}
	fprintf(fp, "\n");

	// DRC Bright User Define (DRC_BRIGHT_USER_DEFINE_NUM)
	fprintf(fp, "DRC BrightUserDefine =");
	for (CVI_U32 u32PntIdx = 0; u32PntIdx < DRC_BRIGHT_USER_DEFINE_NUM; ++u32PntIdx) {
		if ((u32PntIdx % 32) == 0) {
			fprintf(fp, "\n");
		}
		fprintf(fp, "%d, ", stDrc.BrightUserDefine[u32PntIdx]);
	}
	fprintf(fp, "\n");

	// Gamma Log
	// In DRC7-4 the gamma lut is not fix to SRGB.
	// In DRC7-6 we can fix the table to SRGB lut.
	if (pu16GammaLut) {
		fprintf(fp, "gamma =");
		for (CVI_U32 u32PntIdx = 0; u32PntIdx < GAMMA_NODE_NUM; ++u32PntIdx) {
			if ((u32PntIdx % 32) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%d, ", pu16GammaLut[u32PntIdx]);
		}
		fprintf(fp, "\n");
	}

	if (stMLSC.Size > 0) {
		CVI_U32 u32LutIndex = stMLSC.Size - 1;

		// LSC RGain Log
		fprintf(fp, "lsc_r_gain =");
		for (CVI_U32 u32PntIdx = 0; u32PntIdx < CVI_ISP_LSC_GRID_POINTS; ++u32PntIdx) {
			if ((u32PntIdx % 32) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%d, ", stMLSC.LscGainLut[u32LutIndex].RGain[u32PntIdx]);
		}
		fprintf(fp, "\n");

		// LSC GGain Log
		fprintf(fp, "lsc_g_gain =");
		for (CVI_U32 u32PntIdx = 0; u32PntIdx < CVI_ISP_LSC_GRID_POINTS; ++u32PntIdx) {
			if ((u32PntIdx % 32) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%d, ", stMLSC.LscGainLut[u32LutIndex].GGain[u32PntIdx]);
		}
		fprintf(fp, "\n");

		// LSC BGain Log
		fprintf(fp, "lsc_b_gain =");
		for (CVI_U32 u32PntIdx = 0; u32PntIdx < CVI_ISP_LSC_GRID_POINTS; ++u32PntIdx) {
			if ((u32PntIdx % 32) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%d, ", stMLSC.LscGainLut[u32LutIndex].BGain[u32PntIdx]);
		}
		fprintf(fp, "\n");
	}

	return CVI_SUCCESS;
}
