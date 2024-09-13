/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_algo_drc.h
 * Description:
 *
 */

#ifndef _ISP_ALGO_DRC_H_
#define _ISP_ALGO_DRC_H_

#include "cvi_comm_isp.h"
#include "cvi_comm_sns.h"
#include "isp_defines.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define LTM_G_CURVE_0_NODE_NUM			(257)
#define LTM_G_CURVE_1_NODE_NUM			(512)
#define LTM_G_CURVE_TOTAL_NODE_NUM		(LTM_G_CURVE_0_NODE_NUM + LTM_G_CURVE_1_NODE_NUM)

#define LTM_DIFF_WEIGHT_LUT_NUM			(30)
#define LTM_DIST_WEIGHT_LUT_NUM			(11)
#define LTM_LUMA_WEIGHT_LUT_NUM			(30)
#define LTM_G_CURVE_0_QUAN_BIT			(4)

typedef enum _DRC_SDR_TONE_CURVE_MODE {
	DRC_SDR_TONE_CURVE_BYPASS = 0,
	DRC_SDR_TONE_CURVE_GAMMA,
	DRC_SDR_TONE_CURVE_DCI,
	DRC_SDR_TONE_CURVE_SDR_DRC_ADAPT,
	DRC_SDR_TONE_CURVE_CONTRAST_ENHANCE
} DRC_SDR_TONE_CURVE_MODE_E;

struct drc_brit_tone_param {
	CVI_FLOAT fBritMapStr;			// WDR
	CVI_U8 u8BritInflectPtLuma;		// WDR
	CVI_U8 u8BritContrastLow;		// WDR
	CVI_U8 u8BritContrastHigh;		// WDR

	CVI_FLOAT fSdrBritMapStr;		// SDR
	CVI_U8 u8SdrTargetYGainMode;	// SDR
	CVI_U8 u8SdrTargetY;			// SDR
	CVI_U8 u8SdrTargetYGain;		// SDR
	CVI_U16 u16SdrGlobalToneStr;	// SDR
	CVI_U8 u8SdrBritInflectPtLuma;	// SDR
	CVI_U8 u8SdrBritContrastLow;	// SDR
	CVI_U8 u8SdrBritContrastHigh;	// SDR
};

struct drc_dark_tone_param {
	CVI_FLOAT fDarkMapStr;			// WDR

	CVI_FLOAT fSdrDarkMapStr;		// SDR
};

struct drc_param_in {
	CVI_BOOL fswdr_en;
	CVI_FLOAT exposure_ratio;

	int ToneCurveSelect;		// 1
	CVI_U16 au16CurveUserDefine[DRC_GLOBAL_USER_DEFINE_NUM];
	CVI_U16 au16DarkUserDefine[DRC_DARK_USER_DEFINE_NUM];
	CVI_U16 au16BrightUserDefine[DRC_BRIGHT_USER_DEFINE_NUM];

	int imgWidth;
	int imgHeight;
	int WDRCombineShortThr;
	int WDRCombineLongThr;
	int targetYScale;
	int average_frame;
	int preWdrBinNum;
	int HdrStrength;
	int dbgLevel;
	int frameCnt;
	int ToneParserMaxRatio;
	int ViPipe;
	uint32_t SEHistogram[MAX_HIST_BINS];
	uint32_t LEHistogram[MAX_HIST_BINS];

	CVI_U16 u16WDRCombineMinWeight;

	CVI_U8 u8DEAdaptTargetGain;
	CVI_U8 u8DEAdaptGainLB;
	CVI_U8 u8DEAdaptGainUB;
	CVI_U8 u8DEAdaptPercentile;
	CVI_U8 u8SdrDEAdaptTargetGain;
	CVI_U8 u8SdrDEAdaptGainLB;
	CVI_U8 u8SdrDEAdaptGainUB;
	CVI_U8 u8SdrDEAdaptPercentile;

	ISP_U32_PTR pu32PreGlobalLut;		// IIR : LTM_G_CURVE_TOTAL_NODE_NUM
	ISP_U32_PTR pu32PreDarkLut;		// IIR : LTM_DARK_CURVE_NODE_NUM
	ISP_U32_PTR pu32PreBritLut;		// IIR : LTM_BRIGHT_CURVE_NODE_NUM

	struct drc_brit_tone_param stBritToneParam;
	struct drc_dark_tone_param stDarkToneParam;

	CVI_U8 DetailEnhanceMtIn[4];
	CVI_U16 DetailEnhanceMtOut[4];

	CVI_BOOL dbg_182x_sim_enable;
	CVI_U8 DarkMapStr;
	CVI_U8 BritMapStr;
	CVI_U8 SdrDarkMapStr;
	CVI_U8 SdrBritMapStr;

	//CVI_U32 rsv[32];
};

struct drc_param_out {
	int reg_ltm_g_curve_1_quan_bit;
	int reg_ltm_b_curve_quan_bit;
	uint32_t reg_ltm_deflt_lut[LTM_G_CURVE_TOTAL_NODE_NUM];			// global tone
	uint32_t reg_ltm_dark_lut[LTM_DARK_CURVE_NODE_NUM];				// dark tone
	uint32_t reg_ltm_brit_lut[LTM_BRIGHT_CURVE_NODE_NUM];			// bright tone

	int reg_ltm_de_dist_wgt[11];
	int reg_ltm_be_dist_wgt[11];
	CVI_U8 reg_ltm_de_lmap_lut_in[4];
	CVI_U8 reg_ltm_de_lmap_lut_out[4];
	CVI_S16 reg_ltm_de_lmap_lut_slope[3];
	CVI_U8 reg_ltm_be_lmap_lut_in[4];
	CVI_U8 reg_ltm_be_lmap_lut_out[4];
	CVI_S16 reg_ltm_be_lmap_lut_slope[3];
	int reg_ltm_de_strth_dshft;
	int reg_ltm_de_strth_gain;
	int reg_ltm_be_strth_dshft;
	int reg_ltm_be_strth_gain;

	int reg_ltm_wdr_bin_num;
	uint32_t ltm_global_tone_bin_num;
	uint32_t ltm_global_tone_bin_se_step;

	CVI_S16 DetailEnhanceMtSlope[3];
};

CVI_S32 isp_algo_drc_main(
	struct drc_param_in *drc_param_in, struct drc_param_out *drc_param_out);
CVI_S32 isp_algo_drc_init(void);
CVI_S32 isp_algo_drc_uninit(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // _ISP_ALGO_DRC_H_
