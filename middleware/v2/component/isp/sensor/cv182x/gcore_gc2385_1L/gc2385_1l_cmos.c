#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#ifdef ARCH_CV182X
#include "cvi_type.h"
#include "cvi_comm_video.h"
#include <linux/cvi_vip_snsr.h>
#else
#include <linux/cvi_type.h>
#include <linux/cvi_comm_video.h>
#include <linux/vi_snsr.h>
#endif
#include "cvi_debug.h"
#include "cvi_comm_sns.h"
#include "cvi_sns_ctrl.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"

#include "gc2385_1l_cmos_ex.h"
#include "gc2385_1l_cmos_param.h"

#define DIV_0_TO_1(a)   ((0 == (a)) ? 1 : (a))
#define DIV_0_TO_1_FLOAT(a) ((((a) < 1E-10) && ((a) > -1E-10)) ? 1 : (a))
#define GC2385_1L_ID 2385

/****************************************************************************
 * global variables                                                            *
 ****************************************************************************/

ISP_SNS_STATE_S *g_pastGc2385_1L[VI_MAX_PIPE_NUM] = {CVI_NULL};

#define GC2385_1L_SENSOR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastGc2385_1L[dev])
#define GC2385_1L_SENSOR_SET_CTX(dev, pstCtx)   (g_pastGc2385_1L[dev] = pstCtx)
#define GC2385_1L_SENSOR_RESET_CTX(dev)         (g_pastGc2385_1L[dev] = CVI_NULL)

ISP_SNS_COMMBUS_U g_aunGc2385_1L_BusInfo[VI_MAX_PIPE_NUM] = {
	[0] = { .s8I2cDev = 0},
	[1 ... VI_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

GC2385_1L_STATE_S g_astGc2385_1L_State[VI_MAX_PIPE_NUM] = { {0} };
ISP_SNS_MIRRORFLIP_TYPE_E g_aeGc2385_1L_MirrorFip[VI_MAX_PIPE_NUM] = {0};

CVI_U16 g_au16Gc2385_1L_GainMode[VI_MAX_PIPE_NUM] = {0};
CVI_U16 g_au16Gc2385_1L_L2SMode[VI_MAX_PIPE_NUM] = {0};

/****************************************************************************
 * local variables and functions                                                           *
 ****************************************************************************/
static ISP_FSWDR_MODE_E genFSWDRMode[VI_MAX_PIPE_NUM] = {
	[0 ... VI_MAX_PIPE_NUM - 1] = ISP_FSWDR_NORMAL_MODE
};

static CVI_U32 gu32MaxTimeGetCnt[VI_MAX_PIPE_NUM] = {0};
static CVI_U32 g_au32InitExposure[VI_MAX_PIPE_NUM]  = {0};
static CVI_U32 g_au32LinesPer500ms[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16InitWBGain[VI_MAX_PIPE_NUM][3] = {{0} };
static CVI_U16 g_au16SampleRgain[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16SampleBgain[VI_MAX_PIPE_NUM] = {0};
static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg);
/*****Gc2385_1L Lines Range*****/
#define GC2385_1L_FULL_LINES_MAX  (0x3fff)

/*****Gc2385_1L Register Address*****/
#define GC2385_1L_EXP_PAGE_ADDR			0xfe
#define GC2385_1L_EXP_H_ADDR			0x03
#define GC2385_1L_EXP_L_ADDR			0x04
#define GC2385_1L_VB_PAGE_ADDR			0xfe
#define GC2385_1L_VB_H_ADDR			0x07
#define GC2385_1L_VB_L_ADDR			0x08
#define GC2385_1L_FLIP_MIRROR_PAGE_ADDR		0xfe
#define GC2385_1L_FLIP_MIRROR_ADDR		0x17
#define GC2385_1L_CROP_START_X_ADDR		0x94
#define GC2385_1L_CROP_START_Y_ADDR		0x92
#define GC2385_1L_BLK_SELECT1_H_ADDR		0x4e
#define GC2385_1L_BLK_SELECT1_L_ADDR		0x4f
#define GC2385_1L_AGAIN_PAGE_ADDR		0xfe
#define GC2385_1L_T_INIT_SET_ADDR		0x20
#define GC2385_1L_T_CLK_HS_ADDR			0x22
#define GC2385_1L_COL_CODE_ADDR			0xb6
#define GC2385_1L_AUTO_PREGAIN_SYNC_ADDR	0xb1
#define GC2385_1L_AUTO_PREGAIN_ADDR		0xb2

#define GC2385_1L_RES_IS_1200P(w, h)      ((w) <= 1600 && (h) <= 1200)

static CVI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	const GC2385_1L_MODE_S *pstMode;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstAeSnsDft);
	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstMode = &g_stGc2385_1L_mode;

	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FlickerFreq = 50 * 256;
	pstAeSnsDft->u32FullLinesMax = GC2385_1L_FULL_LINES_MAX;
	pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * 30);

	pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
	pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
	pstAeSnsDft->stIntTimeAccu.f32Offset = 0;
	pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_TABLE;
	pstAeSnsDft->stAgainAccu.f32Accuracy = 1;
	pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_TABLE;
	pstAeSnsDft->stDgainAccu.f32Accuracy = 1;

	pstAeSnsDft->u32ISPDgainShift = 8;
	pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;
	pstAeSnsDft->u32MaxISPDgainTarget = 2 << pstAeSnsDft->u32ISPDgainShift;

	if (g_au32LinesPer500ms[ViPipe] == 0)
		pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * 30 / 2;
	else
		pstAeSnsDft->u32LinesPer500ms = g_au32LinesPer500ms[ViPipe];
#if 0
	pstAeSnsDft->enMaxIrisFNO = ISP_IRIS_F_NO_1_0;
	pstAeSnsDft->enMinIrisFNO = ISP_IRIS_F_NO_32_0;

	pstAeSnsDft->bAERouteExValid = CVI_FALSE;
	pstAeSnsDft->stAERouteAttr.u32TotalNum = 0;
	pstAeSnsDft->stAERouteAttrEx.u32TotalNum = 0;
#endif
	switch (pstSnsState->enWDRMode) {
	default:
	case WDR_MODE_NONE:   /*linear mode*/
		pstAeSnsDft->f32Fps = pstMode->f32MaxFps;
		pstAeSnsDft->f32MinFps = pstMode->f32MinFps;
		pstAeSnsDft->au8HistThresh[0] = 0xd;
		pstAeSnsDft->au8HistThresh[1] = 0x28;
		pstAeSnsDft->au8HistThresh[2] = 0x60;
		pstAeSnsDft->au8HistThresh[3] = 0x80;

		pstAeSnsDft->u32MaxAgain = pstMode->stAgain.u32Max;
		pstAeSnsDft->u32MinAgain = pstMode->stAgain.u32Min;
		pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
		pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

		pstAeSnsDft->u32MaxDgain = pstMode->stDgain.u32Max;
		pstAeSnsDft->u32MinDgain = pstMode->stDgain.u32Min;
		pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
		pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

		pstAeSnsDft->u8AeCompensation = 40;
		pstAeSnsDft->u32InitAESpeed = 64;
		pstAeSnsDft->u32InitAETolerance = 5;
		pstAeSnsDft->u32AEResponseFrame = 4;
		pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;
		pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] :
			pstMode->stExp.u16Def;

		pstAeSnsDft->u32MaxIntTime = pstMode->stExp.u16Max;
		pstAeSnsDft->u32MinIntTime = pstMode->stExp.u16Min;
		pstAeSnsDft->u32MaxIntTimeTarget = 65535;
		pstAeSnsDft->u32MinIntTimeTarget = 1;
		break;

	case WDR_MODE_2To1_LINE:
		break;
	}

	return CVI_SUCCESS;
}

/* the function of sensor set fps */
static CVI_S32 cmos_fps_set(VI_PIPE ViPipe, CVI_FLOAT f32Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	CVI_U32 u32VMAX = 0;
	CVI_U32 u32VB = 0;
	CVI_FLOAT f32MaxFps = 0;
	CVI_FLOAT f32MinFps = 0;
	CVI_U32 u32Vts = 0;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;

	CMOS_CHECK_POINTER(pstAeSnsDft);
	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u32Vts = g_stGc2385_1L_mode.u32VtsDef;
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;
	f32MaxFps = g_stGc2385_1L_mode.f32MaxFps;
	f32MinFps = g_stGc2385_1L_mode.f32MinFps;

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		if ((f32Fps <= f32MaxFps) && (f32Fps >= f32MinFps)) {
			u32VMAX = u32Vts * f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport Fps: %f\n", f32Fps);
			return CVI_FAILURE;
		}
		u32VMAX = (u32VMAX > GC2385_1L_FULL_LINES_MAX) ? GC2385_1L_FULL_LINES_MAX : u32VMAX;

		u32VB = u32VMAX - 1200 - 32;
		pstSnsRegsInfo->astI2cData[LINEAR_VB_H].u32Data = ((u32VB & 0xFF00) >> 8);
		pstSnsRegsInfo->astI2cData[LINEAR_VB_L].u32Data = (u32VB & 0xFF);
	}

	pstSnsState->u32FLStd = u32VMAX;

	pstAeSnsDft->f32Fps = f32Fps;
	pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps / 2;
	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 1;
	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
	pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * DIV_0_TO_1_FLOAT(f32Fps));

	return CVI_SUCCESS;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static CVI_S32 cmos_inttime_update(VI_PIPE ViPipe, CVI_U32 *u32IntTime)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(u32IntTime);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	pstSnsRegsInfo->astI2cData[LINEAR_EXP_H].u32Data = ((u32IntTime[0] >> 8 )& 0x3F);
	pstSnsRegsInfo->astI2cData[LINEAR_EXP_L].u32Data = (u32IntTime[0] & 0xFF);

	return CVI_SUCCESS;
}

static CVI_U32 regValTable[9][3] = {
	/* [reg]  0x20,       0x22,                 0xb6
	 * [name] T_init_set, T_CLK_HS_PREPARE_set, Buf_col_code
	 */
	{0x73, 0xa2, 0x00},
	{0x73, 0xa2, 0x01},
	{0x73, 0xa2, 0x02},
	{0x73, 0xa2, 0x03},
	{0x73, 0xa3, 0x04},
	{0x74, 0xa3, 0x05},
	{0x74, 0xa3, 0x06},
	{0x74, 0xa3, 0x07},
	{0x75, 0xa4, 0x08},
};

static CVI_U32 gain_table[10] = {
	16*64,
	16*92,
	16*127,
	16*183,
	16*257,
	16*369,
	16*531,
	16*750,
	16*1092,
	16*2048,
};

static CVI_S32 cmos_again_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32AgainLin, CVI_U32 *pu32AgainDb)
{
	int i, total;
	CVI_U32 pregain;

	UNUSED(ViPipe);
	CMOS_CHECK_POINTER(pu32AgainLin);
	CMOS_CHECK_POINTER(pu32AgainDb);

	total = sizeof(gain_table) / sizeof(CVI_U32);

	if (*pu32AgainLin >= gain_table[total- 1]) {
		*pu32AgainLin =  *pu32AgainDb = gain_table[total - 1];
		return CVI_SUCCESS;
	}

	for (i = 1; i < total; i++) {
		if (*pu32AgainLin < gain_table[i]) {
			break;
		}
	}

	i--;
	pregain = *pu32AgainLin * 256 / gain_table[i];

	*pu32AgainDb = *pu32AgainLin;
	// set the Lin as the closest sensor gain for AE algo reference
	if (*pu32AgainLin < 16*92) {
		*pu32AgainDb = *pu32AgainLin = 1024;
	} else {
		*pu32AgainLin = pregain * gain_table[i] / 256;
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_dgain_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32DgainLin, CVI_U32 *pu32DgainDb)
{
	UNUSED(ViPipe);
	CMOS_CHECK_POINTER(pu32DgainLin);
	CMOS_CHECK_POINTER(pu32DgainDb);

	*pu32DgainLin = 1024;
	*pu32DgainDb = 0;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_gains_update(VI_PIPE ViPipe, CVI_U32 *pu32Again, CVI_U32 *pu32Dgain)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	CVI_U32 u32Again;
	int i, total;

	total = sizeof(gain_table) / sizeof(CVI_U32);

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pu32Again);
	CMOS_CHECK_POINTER(pu32Dgain);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	if (pu32Again[0] < gain_table[total - 1]) {
		for (i = 1; i < total; i++) {
			if (*pu32Again < gain_table[i])
				break;
		}

		i--;
		// find the pregain
		u32Again = pu32Again[0] * 256 / gain_table[i];

		pstSnsRegsInfo->astI2cData[LINEAR_T_INIT_SET].u32Data = regValTable[i][0];
		pstSnsRegsInfo->astI2cData[LINEAR_T_CLK_HS].u32Data = regValTable[i][1];
		pstSnsRegsInfo->astI2cData[LINEAR_COL_CODE].u32Data = regValTable[i][2];
		pstSnsRegsInfo->astI2cData[LINEAR_AUTO_PREGAIN_SYNC].u32Data = u32Again >> 8;
		pstSnsRegsInfo->astI2cData[LINEAR_AUTO_PREGAIN].u32Data = u32Again & 0xff;
	} else {
		u32Again = pu32Again[0] * 256 / gain_table[total - 1];
		i = total - 1;
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_inttime_max(VI_PIPE ViPipe, CVI_U16 u16ManRatioEnable, CVI_U32 *au32Ratio,
				    CVI_U32 *au32IntTimeMax, CVI_U32 *au32IntTimeMin, CVI_U32 *pu32LFMaxIntTime)
{
	UNUSED(ViPipe);
	UNUSED(u16ManRatioEnable);
	UNUSED(au32Ratio);
	UNUSED(au32IntTimeMax);
	UNUSED(au32IntTimeMin);
	UNUSED(pu32LFMaxIntTime);

	return CVI_SUCCESS;
}

/* Only used in LINE_WDR mode */
static CVI_S32 cmos_ae_fswdr_attr_set(VI_PIPE ViPipe, AE_FSWDR_ATTR_S *pstAeFSWDRAttr)
{
	CMOS_CHECK_POINTER(pstAeFSWDRAttr);

	genFSWDRMode[ViPipe] = pstAeFSWDRAttr->enFSWDRMode;
	gu32MaxTimeGetCnt[ViPipe] = 0;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
	CMOS_CHECK_POINTER(pstExpFuncs);

	memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));
	pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
	pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
	pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
	pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
	pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
	pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;
	pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
	pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
	CMOS_CHECK_POINTER(pstAwbSnsDft);
	UNUSED(ViPipe);

	memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));

	pstAwbSnsDft->u16InitGgain = 1024;
	pstAwbSnsDft->u8AWBRunInterval = 1;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
	CMOS_CHECK_POINTER(pstExpFuncs);

	memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

	pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_isp_default(VI_PIPE ViPipe, ISP_CMOS_DEFAULT_S *pstDef)
{
	UNUSED(ViPipe);
	memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_blc_default(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlc)
{
	CMOS_CHECK_POINTER(pstBlc);
	UNUSED(ViPipe);

	memset(pstBlc, 0, sizeof(ISP_CMOS_BLACK_LEVEL_S));

	memcpy(pstBlc,
		&g_stIspBlcCalibratio, sizeof(ISP_CMOS_BLACK_LEVEL_S));
	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg)
{
	const GC2385_1L_MODE_S *pstMode = CVI_NULL;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstMode = &g_stGc2385_1L_mode;
	pstIspCfg->frm_num = 1;
	memcpy(&pstIspCfg->img_size[0], &pstMode->stImg, sizeof(ISP_WDR_SIZE_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, CVI_U8 u8Mode)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstSnsState->bSyncInit = CVI_FALSE;

	switch (u8Mode) {
	case WDR_MODE_NONE:
		pstSnsState->u8ImgMode = GC2385_1L_MODE_1600X1200P30;
		pstSnsState->enWDRMode = WDR_MODE_NONE;
		pstSnsState->u32FLStd = g_stGc2385_1L_mode.u32VtsDef;
		syslog(LOG_INFO, "WDR_MODE_NONE\n");
		break;

	case WDR_MODE_2To1_LINE:
	default:
		CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport sensor mode!\n");
		return CVI_FAILURE;
	}

	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstSnsState->au32FL[1] = pstSnsState->au32FL[0];
	memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));

	return CVI_SUCCESS;
}

static CVI_U32 sensor_cmp_wdr_size(ISP_SNS_ISP_INFO_S *pstWdr1, ISP_SNS_ISP_INFO_S *pstWdr2)
{
	CVI_U32 i;

	if (pstWdr1->frm_num != pstWdr2->frm_num)
		goto _mismatch;
	for (i = 0; i < 2; i++) {
		if (pstWdr1->img_size[i].stSnsSize.u32Width != pstWdr2->img_size[i].stSnsSize.u32Width)
			goto _mismatch;
		if (pstWdr1->img_size[i].stSnsSize.u32Height != pstWdr2->img_size[i].stSnsSize.u32Height)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.s32X != pstWdr2->img_size[i].stWndRect.s32X)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.s32Y != pstWdr2->img_size[i].stWndRect.s32Y)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.u32Width != pstWdr2->img_size[i].stWndRect.u32Width)
			goto _mismatch;
		if (pstWdr1->img_size[i].stWndRect.u32Height != pstWdr2->img_size[i].stWndRect.u32Height)
			goto _mismatch;
	}

	return 0;
_mismatch:
	return 1;
}

static CVI_S32 cmos_get_sns_regs_info(VI_PIPE ViPipe, ISP_SNS_SYNC_INFO_S *pstSnsSyncInfo)
{
	CVI_U32 i;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *pstCfg0 = CVI_NULL;
	ISP_SNS_SYNC_INFO_S *pstCfg1 = CVI_NULL;
	ISP_I2C_DATA_S *pstI2c_data = CVI_NULL;

	CMOS_CHECK_POINTER(pstSnsSyncInfo);
	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	pstSnsRegsInfo = &pstSnsSyncInfo->snsCfg;
	pstCfg0 = &pstSnsState->astSyncInfo[0];
	pstCfg1 = &pstSnsState->astSyncInfo[1];
	pstI2c_data = pstCfg0->snsCfg.astI2cData;

	if ((pstSnsState->bSyncInit == CVI_FALSE) || (pstSnsRegsInfo->bConfig == CVI_FALSE)) {
		pstCfg0->snsCfg.enSnsType = SNS_I2C_TYPE;
		pstCfg0->snsCfg.unComBus.s8I2cDev = g_aunGc2385_1L_BusInfo[ViPipe].s8I2cDev;
		pstCfg0->snsCfg.u8Cfg2ValidDelayMax = 0;
		pstCfg0->snsCfg.use_snsr_sram = CVI_TRUE;
		pstCfg0->snsCfg.u32RegNum = LINEAR_REGS_NUM;

		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			pstI2c_data[i].bUpdate = CVI_TRUE;
			pstI2c_data[i].u8DevAddr = gc2385_1l_i2c_addr;
			pstI2c_data[i].u32AddrByteNum = gc2385_1l_addr_byte;
			pstI2c_data[i].u32DataByteNum = gc2385_1l_data_byte;
		}

		pstI2c_data[LINEAR_EXP_PAGE].u32RegAddr = GC2385_1L_EXP_PAGE_ADDR;
		pstI2c_data[LINEAR_EXP_PAGE].u32Data = 0;
		pstI2c_data[LINEAR_EXP_H].u32RegAddr = GC2385_1L_EXP_H_ADDR;
		pstI2c_data[LINEAR_EXP_L].u32RegAddr = GC2385_1L_EXP_L_ADDR;
		pstI2c_data[LINEAR_VB_PAGE].u32RegAddr = GC2385_1L_VB_PAGE_ADDR;
		pstI2c_data[LINEAR_VB_PAGE].u32Data = 0;
		pstI2c_data[LINEAR_VB_H].u32RegAddr = GC2385_1L_VB_H_ADDR;
		pstI2c_data[LINEAR_VB_L].u32RegAddr = GC2385_1L_VB_L_ADDR;
		pstI2c_data[LINEAR_FLIP_MIRROR_PAGE].u32RegAddr = GC2385_1L_FLIP_MIRROR_PAGE_ADDR;
		pstI2c_data[LINEAR_FLIP_MIRROR_PAGE].u32Data = 0;
		pstI2c_data[LINEAR_FLIP_MIRROR].u32RegAddr = GC2385_1L_FLIP_MIRROR_ADDR;
		pstI2c_data[LINEAR_FLIP_MIRROR].u32Data = 0xd4;
		pstI2c_data[LINEAR_CROP_START_X].u32RegAddr = GC2385_1L_CROP_START_X_ADDR;
		pstI2c_data[LINEAR_CROP_START_X].u32Data = 0x05;
		pstI2c_data[LINEAR_CROP_START_Y].u32RegAddr = GC2385_1L_CROP_START_Y_ADDR;
		pstI2c_data[LINEAR_CROP_START_Y].u32Data = 0x03;
		pstI2c_data[LINEAR_BLK_Select1_H].u32RegAddr = GC2385_1L_BLK_SELECT1_H_ADDR;
		pstI2c_data[LINEAR_BLK_Select1_H].u32Data = 0x3c;
		pstI2c_data[LINEAR_BLK_Select1_L].u32RegAddr = GC2385_1L_BLK_SELECT1_L_ADDR;
		pstI2c_data[LINEAR_BLK_Select1_L].u32Data = 0x00;
		pstI2c_data[LINEAR_GAIN_PAGE].u32RegAddr = GC2385_1L_AGAIN_PAGE_ADDR;
		pstI2c_data[LINEAR_GAIN_PAGE].u32Data = 0;
		pstI2c_data[LINEAR_T_INIT_SET].u32RegAddr = GC2385_1L_T_INIT_SET_ADDR;
		pstI2c_data[LINEAR_T_INIT_SET].u32Data = 0x73;
		pstI2c_data[LINEAR_T_CLK_HS].u32RegAddr = GC2385_1L_T_CLK_HS_ADDR;
		pstI2c_data[LINEAR_T_CLK_HS].u32Data = 0xa2;
		pstI2c_data[LINEAR_COL_CODE].u32RegAddr = GC2385_1L_COL_CODE_ADDR;
		pstI2c_data[LINEAR_COL_CODE].u32Data = 0x00;
		pstI2c_data[LINEAR_AUTO_PREGAIN_SYNC].u32RegAddr = GC2385_1L_AUTO_PREGAIN_SYNC_ADDR;
		pstI2c_data[LINEAR_AUTO_PREGAIN_SYNC].u32Data = 0x01;
		pstI2c_data[LINEAR_AUTO_PREGAIN].u32RegAddr = GC2385_1L_AUTO_PREGAIN_ADDR;
		pstI2c_data[LINEAR_AUTO_PREGAIN].u32Data = 0x00;

		pstSnsState->bSyncInit = CVI_TRUE;
		pstCfg0->snsCfg.need_update = CVI_TRUE;
		/* recalcualte WDR size */
		cmos_get_wdr_size(ViPipe, &pstCfg0->ispCfg);
		pstCfg0->ispCfg.need_update = CVI_TRUE;
	} else {

		CVI_U32 gainsUpdate = 0, expUpdate = 0, vbUpdate = 0;

		pstCfg0->snsCfg.need_update = CVI_FALSE;
		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			if (pstCfg0->snsCfg.astI2cData[i].u32Data == pstCfg1->snsCfg.astI2cData[i].u32Data) {
				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_FALSE;
			} else {

				if ((i >= LINEAR_T_INIT_SET) && (i <= LINEAR_AUTO_PREGAIN))
					gainsUpdate = 1;

				if ((i >= LINEAR_EXP_H) && (i <= LINEAR_EXP_L))
					expUpdate = 1;

				if ((i >= LINEAR_VB_H) && (i <= LINEAR_VB_L))
					vbUpdate = 1;

				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_TRUE;
				pstCfg0->snsCfg.need_update = CVI_TRUE;
			}
		}

		if (gainsUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_GAIN_PAGE].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_T_INIT_SET].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_T_CLK_HS].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_COL_CODE].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_AUTO_PREGAIN_SYNC].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_AUTO_PREGAIN].bUpdate = CVI_TRUE;

		}
		if (expUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_PAGE].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_H].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_L].bUpdate = CVI_TRUE;
		}
		if (vbUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_PAGE].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_H].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_L].bUpdate = CVI_TRUE;
		}
		if (pstI2c_data[LINEAR_FLIP_MIRROR].bUpdate == CVI_TRUE) {
			pstI2c_data[LINEAR_FLIP_MIRROR_PAGE].bUpdate = CVI_TRUE;
		}

		pstCfg0->ispCfg.need_update = (sensor_cmp_wdr_size(&pstCfg0->ispCfg, &pstCfg1->ispCfg) ?
				CVI_TRUE : CVI_FALSE);
		pstCfg0->ispCfg.u8DelayFrmNum = 1;
	}

	pstSnsRegsInfo->bConfig = CVI_FALSE;
	memcpy(pstSnsSyncInfo, &pstSnsState->astSyncInfo[0], sizeof(ISP_SNS_SYNC_INFO_S));
	memcpy(&pstSnsState->astSyncInfo[1], &pstSnsState->astSyncInfo[0], sizeof(ISP_SNS_SYNC_INFO_S));
	pstSnsState->au32FL[1] = pstSnsState->au32FL[0];

	pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_MIRROR].bDropFrm = CVI_FALSE;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_set_image_mode(VI_PIPE ViPipe, ISP_CMOS_SENSOR_IMAGE_MODE_S *pstSensorImageMode)
{
	CVI_U8 u8SensorImageMode = 0;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstSensorImageMode);
	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u8SensorImageMode = pstSnsState->u8ImgMode;
	pstSnsState->bSyncInit = CVI_FALSE;

	if (pstSensorImageMode->f32Fps <= 30) {
		if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
			if (GC2385_1L_RES_IS_1200P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height))
				u8SensorImageMode = GC2385_1L_MODE_1600X1200P30;
			else {
				CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
					      pstSensorImageMode->u16Width,
					      pstSensorImageMode->u16Height,
					      pstSensorImageMode->f32Fps,
					      pstSnsState->enWDRMode);
				return CVI_FAILURE;
			}
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Not support! Width:%d, Height:%d, Fps:%f, WDRMode:%d\n",
				      pstSensorImageMode->u16Width,
				      pstSensorImageMode->u16Height,
				      pstSensorImageMode->f32Fps,
				      pstSnsState->enWDRMode);
			return CVI_FAILURE;
		}
	} else {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Not support this Fps:%f\n", pstSensorImageMode->f32Fps);
		return CVI_FAILURE;
	}

	if ((pstSnsState->bInit == CVI_TRUE) && (u8SensorImageMode == pstSnsState->u8ImgMode)) {
		/* Don't need to switch SensorImageMode */
		return CVI_FAILURE;
	}

	pstSnsState->u8ImgMode = u8SensorImageMode;

	return CVI_SUCCESS;
}

static CVI_VOID sensor_mirror_flip(VI_PIPE ViPipe, ISP_SNS_MIRRORFLIP_TYPE_E eSnsMirrorFlip)
{
	CVI_U8 value, start_x, start_y, blk_h, blk_l;

	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	/* Apply the setting on the fly  */
	if (pstSnsState->bInit == CVI_TRUE && g_aeGc2385_1L_MirrorFip[ViPipe] != eSnsMirrorFlip) {

		switch (eSnsMirrorFlip) {
		case ISP_SNS_NORMAL:
			value   = 0xd4;
			start_x = 0x05;
			start_y = 0x03;
			blk_h   = 0x3c;
			blk_l   = 0x00;
			break;
		case ISP_SNS_MIRROR:
			value   = 0xd5;
			start_x = 0x06;
			start_y = 0x03;
			blk_h   = 0x3c;
			blk_l   = 0x00;
			break;
		case ISP_SNS_FLIP:
			value   = 0xd6;
			start_x = 0x05;
			start_y = 0x02;
			blk_h   = 0x00;
			blk_l   = 0x3c;
			break;
		case ISP_SNS_MIRROR_FLIP:
			value   = 0xd7;
			start_x = 0x06;
			start_y = 0x02;
			blk_h   = 0x00;
			blk_l   = 0x3c;
			break;
		default:
			return;
		}

		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_MIRROR].u32Data = value;
		pstSnsRegsInfo->astI2cData[LINEAR_CROP_START_X].u32Data = start_x;
		pstSnsRegsInfo->astI2cData[LINEAR_CROP_START_Y].u32Data = start_y;
		pstSnsRegsInfo->astI2cData[LINEAR_BLK_Select1_H].u32Data = blk_h;
		pstSnsRegsInfo->astI2cData[LINEAR_BLK_Select1_L].u32Data = blk_l;

		g_aeGc2385_1L_MirrorFip[ViPipe] = eSnsMirrorFlip;
	}
}

static CVI_VOID sensor_global_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsState->bInit = CVI_FALSE;
	pstSnsState->bSyncInit = CVI_FALSE;
	pstSnsState->u8ImgMode = GC2385_1L_MODE_1600X1200P30;
	pstSnsState->enWDRMode = WDR_MODE_NONE;
	pstSnsState->u32FLStd  = g_stGc2385_1L_mode.u32VtsDef;
	pstSnsState->au32FL[0] = g_stGc2385_1L_mode.u32VtsDef;
	pstSnsState->au32FL[1] = g_stGc2385_1L_mode.u32VtsDef;

	memset(&pstSnsState->astSyncInfo[0], 0, sizeof(ISP_SNS_SYNC_INFO_S));
	memset(&pstSnsState->astSyncInfo[1], 0, sizeof(ISP_SNS_SYNC_INFO_S));
}

static CVI_S32 sensor_rx_attr(VI_PIPE ViPipe, SNS_COMBO_DEV_ATTR_S *pstRxAttr)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pstRxAttr);

	memcpy(pstRxAttr, &gc2385_1l_rx_attr, sizeof(*pstRxAttr));

	pstRxAttr->img_size.width = g_stGc2385_1L_mode.stImg.stSnsSize.u32Width;
	pstRxAttr->img_size.height = g_stGc2385_1L_mode.stImg.stSnsSize.u32Height;
	if (pstSnsState->enWDRMode == WDR_MODE_NONE)
		pstRxAttr->mipi_attr.wdr_mode = CVI_MIPI_WDR_MODE_NONE;

	return CVI_SUCCESS;

}

static CVI_S32 sensor_patch_rx_attr(RX_INIT_ATTR_S *pstRxInitAttr)
{
	SNS_COMBO_DEV_ATTR_S *pstRxAttr = &gc2385_1l_rx_attr;
	int i;

	CMOS_CHECK_POINTER(pstRxInitAttr);

	if (pstRxInitAttr->MipiDev >= VI_MAX_DEV_NUM)
		return CVI_SUCCESS;

	pstRxAttr->devno = pstRxInitAttr->MipiDev;

	if (pstRxAttr->input_mode == INPUT_MODE_MIPI) {
		struct mipi_dev_attr_s *attr = &pstRxAttr->mipi_attr;

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			attr->lane_id[i] = pstRxInitAttr->as16LaneId[i];
			attr->pn_swap[i] = pstRxInitAttr->as8PNSwap[i];
		}
	} else {
		struct lvds_dev_attr_s *attr = &pstRxAttr->lvds_attr;

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			attr->lane_id[i] = pstRxInitAttr->as16LaneId[i];
			attr->pn_swap[i] = pstRxInitAttr->as8PNSwap[i];
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
	CMOS_CHECK_POINTER(pstSensorExpFunc);

	memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

	pstSensorExpFunc->pfn_cmos_sensor_init = gc2385_1l_init;
	pstSensorExpFunc->pfn_cmos_sensor_exit = gc2385_1l_exit;
	pstSensorExpFunc->pfn_cmos_sensor_global_init = sensor_global_init;
	pstSensorExpFunc->pfn_cmos_set_image_mode = cmos_set_image_mode;
	pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;
	pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
	pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_blc_default;
	pstSensorExpFunc->pfn_cmos_get_sns_reg_info = cmos_get_sns_regs_info;

	return CVI_SUCCESS;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

static CVI_S32 gc2385_1l_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
	g_aunGc2385_1L_BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

	if (pastSnsStateCtx == CVI_NULL) {
		pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
		if (pastSnsStateCtx == CVI_NULL) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
			return -ENOMEM;
		}
	}

	memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

	GC2385_1L_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

	return CVI_SUCCESS;
}

static CVI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	GC2385_1L_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
	SENSOR_FREE(pastSnsStateCtx);
	GC2385_1L_SENSOR_RESET_CTX(ViPipe);
}

static CVI_S32 sensor_register_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	CVI_S32 s32Ret;
	ISP_SENSOR_REGISTER_S stIspRegister;
	AE_SENSOR_REGISTER_S  stAeRegister;
	AWB_SENSOR_REGISTER_S stAwbRegister;
	ISP_SNS_ATTR_INFO_S   stSnsAttrInfo;

	CMOS_CHECK_POINTER(pstAeLib);
	CMOS_CHECK_POINTER(pstAwbLib);

	s32Ret = sensor_ctx_init(ViPipe);

	if (s32Ret != CVI_SUCCESS)
		return CVI_FAILURE;

	stSnsAttrInfo.eSensorId = GC2385_1L_ID;

	s32Ret  = cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
	s32Ret |= CVI_ISP_SensorRegCallBack(ViPipe, &stSnsAttrInfo, &stIspRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function failed!\n");
		return s32Ret;
	}

	s32Ret  = cmos_init_ae_exp_function(&stAeRegister.stAeExp);
	s32Ret |= CVI_AE_SensorRegCallBack(ViPipe, pstAeLib, &stSnsAttrInfo, &stAeRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function to ae lib failed!\n");
		return s32Ret;
	}

	s32Ret  = cmos_init_awb_exp_function(&stAwbRegister.stAwbExp);
	s32Ret |= CVI_AWB_SensorRegCallBack(ViPipe, pstAwbLib, &stSnsAttrInfo, &stAwbRegister);

	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor register callback function to awb lib failed!\n");
		return s32Ret;
	}

	return CVI_SUCCESS;
}

static CVI_S32 sensor_unregister_callback(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ALG_LIB_S *pstAwbLib)
{
	CVI_S32 s32Ret;

	CMOS_CHECK_POINTER(pstAeLib);
	CMOS_CHECK_POINTER(pstAwbLib);

	s32Ret = CVI_ISP_SensorUnRegCallBack(ViPipe, GC2385_1L_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, GC2385_1L_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function to ae lib failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, GC2385_1L_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function to awb lib failed!\n");
		return s32Ret;
	}

	sensor_ctx_exit(ViPipe);

	return CVI_SUCCESS;
}

static CVI_S32 sensor_set_init(VI_PIPE ViPipe, ISP_INIT_ATTR_S *pstInitAttr)
{
	CMOS_CHECK_POINTER(pstInitAttr);

	g_au32InitExposure[ViPipe] = pstInitAttr->u32Exposure;
	g_au32LinesPer500ms[ViPipe] = pstInitAttr->u32LinesPer500ms;
	g_au16InitWBGain[ViPipe][0] = pstInitAttr->u16WBRgain;
	g_au16InitWBGain[ViPipe][1] = pstInitAttr->u16WBGgain;
	g_au16InitWBGain[ViPipe][2] = pstInitAttr->u16WBBgain;
	g_au16SampleRgain[ViPipe] = pstInitAttr->u16SampleRgain;
	g_au16SampleBgain[ViPipe] = pstInitAttr->u16SampleBgain;
	g_au16Gc2385_1L_GainMode[ViPipe] = pstInitAttr->enGainMode;
	g_au16Gc2385_1L_L2SMode[ViPipe] = pstInitAttr->enL2SMode;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_probe(VI_PIPE ViPipe)
{
	return gc2385_1l_probe(ViPipe);
}

ISP_SNS_OBJ_S stSnsGc2385_1L_Obj = {
	.pfnRegisterCallback    = sensor_register_callback,
	.pfnUnRegisterCallback  = sensor_unregister_callback,
	.pfnStandby             = gc2385_1l_standby,
	.pfnRestart             = gc2385_1l_restart,
	.pfnWriteReg            = gc2385_1l_write_register,
	.pfnReadReg             = gc2385_1l_read_register,
	.pfnSetBusInfo          = gc2385_1l_set_bus_info,
	.pfnSetInit             = sensor_set_init,
	.pfnMirrorFlip          = sensor_mirror_flip,
	.pfnPatchRxAttr		= sensor_patch_rx_attr,
	.pfnPatchI2cAddr	= CVI_NULL,
	.pfnGetRxAttr		= sensor_rx_attr,
	.pfnExpSensorCb		= cmos_init_sensor_exp_function,
	.pfnExpAeCb		= cmos_init_ae_exp_function,
	.pfnSnsProbe		= sensor_probe,
};

