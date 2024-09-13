#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <syslog.h>
#include <errno.h>
#include <math.h>
#ifdef ARCH_CV182X
#include "cvi_type.h"
#include "cvi_comm_video.h"
#include <linux/cvi_vip_snsr.h>
#else
#include <linux/cvi_type.h>
#include <linux/cvi_comm_video.h>
#endif
#include "cvi_debug.h"
#include "cvi_comm_sns.h"
#include "cvi_sns_ctrl.h"
#include "cvi_ae_comm.h"
#include "cvi_awb_comm.h"
#include "cvi_ae.h"
#include "cvi_awb.h"
#include "cvi_isp.h"

#include "os02n10_1l_cmos_ex.h"
#include "os02n10_1l_cmos_param.h"

#define DIV_0_TO_1(a)   ((0 == (a)) ? 1 : (a))
#define DIV_0_TO_1_FLOAT(a) ((((a) < 1E-10) && ((a) > -1E-10)) ? 1 : (a))
#define OS02N10_1L_ID 0x2309
#define OS02N10_1L_I2C_ADDR_1 0x3C
#define OS02N10_1L_I2C_ADDR_2 0x3C
#define OS02N10_1L_I2C_ADDR_IS_VALID(addr)      ((addr) == OS02N10_1L_I2C_ADDR_1 || (addr) == OS02N10_1L_I2C_ADDR_2)
#define OS02N10_1L_PCLK (84)
/****************************************************************************
 * global variables                                                            *
 ****************************************************************************/

ISP_SNS_STATE_S *g_pastOs02n10_1l[VI_MAX_PIPE_NUM] = {CVI_NULL};

#define OS02N10_1L_SENSOR_GET_CTX(dev, pstCtx)   (pstCtx = g_pastOs02n10_1l[dev])
#define OS02N10_1L_SENSOR_SET_CTX(dev, pstCtx)   (g_pastOs02n10_1l[dev] = pstCtx)
#define OS02N10_1L_SENSOR_RESET_CTX(dev)         (g_pastOs02n10_1l[dev] = CVI_NULL)

ISP_SNS_COMMBUS_U g_aunOs02n10_1l_BusInfo[VI_MAX_PIPE_NUM] = {
	[0] = { .s8I2cDev = 0},
	[1 ... VI_MAX_PIPE_NUM - 1] = { .s8I2cDev = -1}
};

CVI_U16 g_au16Os02n10_1l_GainMode[VI_MAX_PIPE_NUM] = {0};
CVI_U16 g_au16Os02n10_1l_UseHwSync[VI_MAX_PIPE_NUM] = {0};

OS02N10_1L_STATE_S g_astOs02n10_1l_State[VI_MAX_PIPE_NUM] = {{0} };
ISP_SNS_MIRRORFLIP_TYPE_E g_aeOs02n10_1l_MirrorFip[VI_MAX_PIPE_NUM] = {0};
static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg);
/****************************************************************************
 * local variables and functions                                                           *
 ****************************************************************************/

static CVI_U32 g_au32InitExposure[VI_MAX_PIPE_NUM]  = {0};
static CVI_U32 g_au32LinesPer500ms[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16InitWBGain[VI_MAX_PIPE_NUM][3] = {{0} };
static CVI_U16 g_au16SampleRgain[VI_MAX_PIPE_NUM] = {0};
static CVI_U16 g_au16SampleBgain[VI_MAX_PIPE_NUM] = {0};
/*****Os02n10_1l Lines Range*****/
#define OS02N10_1L_FULL_LINES_MAX  (0xFFFF)

/*****Os02n10 Register Address*****/
#define OS02N10_1L_PAGE_SWITCH_ADDR		0xFD
#define OS02N10_1L_TRIGGER_ADDR		        0xFE
#define OS02N10_1L_EXP0_ADDR		        0x0E
#define OS02N10_1L_AGAIN_ADDR		        0x24
#define OS02N10_1L_DGAIN0_ADDR		        0x1F
#define OS02N10_1L_VB0_ADDR			0x14
#define OS02N10_1L_FLIP_MIRROR_ADDR		0x12
#define OS02N10_1L_FLIP_PAGE_SWITCH_ADDR	0xFD
#define OS02N10_1L_FLIP_MIRROR0_ADDR		0x46
#define OS02N10_1L_FLIP_MIRROR1_ADDR		0x47
#define OS02N10_1L_FLIP_MIRROR2_ADDR		0x45
#define OS02N10_1L_FLIP_col_offset_ADDR		0x48
#define OS02N10_1L_FLIP_row_offset0_ADDR	0x49
#define OS02N10_1L_FLIP_row_offset1_ADDR	0x4A
#define OS02N10_1L_TABLE_END		        0xffff

#define OS02N10_1L_RES_IS_1080P(w, h)      ((w) <= 1920 && (h) <= 1080)

static CVI_S32 cmos_get_ae_default(VI_PIPE ViPipe, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
	const OS02N10_1L_MODE_S *pstMode;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	CMOS_CHECK_POINTER(pstAeSnsDft);
	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstMode = &g_astOs02n10_1l_mode[pstSnsState->u8ImgMode];
#if 0
	memset(&pstAeSnsDft->stAERouteAttr, 0, sizeof(ISP_AE_ROUTE_S));
#endif
	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FlickerFreq = 50 * 256;
	pstAeSnsDft->u32FullLinesMax = OS02N10_1L_FULL_LINES_MAX;
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
	pstAeSnsDft->u32SnsStableFrame = 0;
	/* OV sensor cannot update new setting before the old setting takes effect */
	pstAeSnsDft->u8AERunInterval = 4;
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

		pstAeSnsDft->u32MaxAgain = pstMode->stAgain[0].u32Max;
		pstAeSnsDft->u32MinAgain = pstMode->stAgain[0].u32Min;
		pstAeSnsDft->u32MaxAgainTarget = pstAeSnsDft->u32MaxAgain;
		pstAeSnsDft->u32MinAgainTarget = pstAeSnsDft->u32MinAgain;

		pstAeSnsDft->u32MaxDgain = pstMode->stDgain[0].u32Max;
		pstAeSnsDft->u32MinDgain = pstMode->stDgain[0].u32Min;
		pstAeSnsDft->u32MaxDgainTarget = pstAeSnsDft->u32MaxDgain;
		pstAeSnsDft->u32MinDgainTarget = pstAeSnsDft->u32MinDgain;

		pstAeSnsDft->u8AeCompensation = 40;
		pstAeSnsDft->u32InitAESpeed = 64;
		pstAeSnsDft->u32InitAETolerance = 5;
		pstAeSnsDft->u32AEResponseFrame = 2;
		pstAeSnsDft->enAeExpMode = AE_EXP_HIGHLIGHT_PRIOR;
		pstAeSnsDft->u32InitExposure = g_au32InitExposure[ViPipe] ? g_au32InitExposure[ViPipe] : 76151;

		pstAeSnsDft->u32MaxIntTime = pstMode->stExp[0].u16Max;
		pstAeSnsDft->u32MinIntTime = pstMode->stExp[0].u16Min;
		pstAeSnsDft->u32MaxIntTimeTarget = 65535;
		pstAeSnsDft->u32MinIntTimeTarget = 1;
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
	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u32Vts = g_astOs02n10_1l_mode->u32VtsDef;
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;
	f32MaxFps = g_astOs02n10_1l_mode->f32MaxFps;
	f32MinFps = g_astOs02n10_1l_mode->f32MinFps;

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		if ((f32Fps <= f32MaxFps) && (f32Fps >= f32MinFps)) {
			u32VMAX = u32Vts * f32MaxFps / DIV_0_TO_1_FLOAT(f32Fps);
		} else {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport Fps: %f\n", f32Fps);
			return CVI_FAILURE;
		}

		u32VMAX = (u32VMAX > OS02N10_1L_FULL_LINES_MAX) ? OS02N10_1L_FULL_LINES_MAX : u32VMAX;
		u32VB = 1194 + (u32VMAX - u32Vts);
		pstSnsRegsInfo->astI2cData[LINEAR_VB_0].u32Data = (u32VB >> 8 & 0xFF);
		pstSnsRegsInfo->astI2cData[LINEAR_VB_1].u32Data = (u32VB & 0xFF);
	}

	pstSnsState->u32FLStd = u32VMAX;

	pstAeSnsDft->f32Fps = f32Fps;
	pstAeSnsDft->u32LinesPer500ms = pstSnsState->u32FLStd * f32Fps /2;
	pstAeSnsDft->u32FullLinesStd = pstSnsState->u32FLStd;
	pstAeSnsDft->u32MaxIntTime = pstSnsState->u32FLStd - 9;
	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstAeSnsDft->u32FullLines = pstSnsState->au32FL[0];
	pstAeSnsDft->u32HmaxTimes = (1000000) / (pstSnsState->u32FLStd * DIV_0_TO_1_FLOAT(f32Fps));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_inttime_update(VI_PIPE ViPipe, CVI_U32 *u32IntTime)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(u32IntTime);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		/* linear exposure reg range:
		 * min : 2
		 * max : vts - 9
		 * step : 1
		 */
		CVI_U32 u32TmpIntTime = u32IntTime[0];
		CVI_U32 mimExp = 2;
		CVI_U32 maxExp = pstSnsState->au32FL[0] - 9;
		u32TmpIntTime = (u32TmpIntTime > maxExp) ? maxExp : u32TmpIntTime;
		u32TmpIntTime = (u32TmpIntTime < mimExp) ? mimExp : u32TmpIntTime;
		u32IntTime[0] = u32TmpIntTime;

		pstSnsRegsInfo->astI2cData[LINEAR_EXP_0].u32Data = (u32TmpIntTime >> 8 & 0xFF );
		pstSnsRegsInfo->astI2cData[LINEAR_EXP_1].u32Data = (u32TmpIntTime & 0xFF);

	} else {
		CVI_TRACE_SNS(CVI_DBG_ERR, "Not support WDR: %d\n", pstSnsState->enWDRMode);
		return CVI_FAILURE;
	}
	return CVI_SUCCESS;
}

typedef struct gain_tbl_info_s {
	CVI_U32	gainMax;
	CVI_U16	idxBase;
	CVI_U8	regGain;
	CVI_U8	regGainFineBase;
	CVI_U8	regGainFineStep;
} gain_tbl_info_s;

static struct gain_tbl_info_s AgainInfo[4] = {
	{
		.gainMax = 1984,
		.idxBase = 0,
		.regGain = 0x00,
		.regGainFineBase = 0x10,
		.regGainFineStep = 0x1,
	},
	{
		.gainMax = 3968,
		.idxBase = 16,
		.regGain = 0x00,
		.regGainFineBase = 0x20,
		.regGainFineStep = 0x2,
	},
	{
		.gainMax = 7936,
		.idxBase = 32,
		.regGain = 0x00,
		.regGainFineBase = 0x40,
		.regGainFineStep = 0x4,
	},
	{
		.gainMax = 15872,
		.idxBase = 48,
		.regGain = 0x00,
		.regGainFineBase = 0x80,
		.regGainFineStep = 0x8,
	},
};

/*
 * regGainFineBase {P1:0x24}
 * AGain
 * 0x10~0xF8			1x-15.5x
 */
static CVI_U32 Again_table[] = {
	1024, 1088, 1152, 1216, 1280, 1344, 1408, 1472, 1536, 1600, 1664, 1728, 1792, 1856, 1920,
	1984, 2048, 2176, 2304, 2432, 2560, 2688, 2816, 2944, 3072, 3200, 3328, 3456, 3584, 3712,
	3840, 3968, 4096, 4352, 4608, 4864, 5120, 5376, 5632, 5888, 6144, 6400, 6656, 6912, 7168,
	7424, 7680, 7936, 8192, 8704, 9216, 9728, 10240, 10752, 11264, 11776, 12288, 12800, 13312,
	13824, 14336, 14848, 15360, 15872
};

static struct gain_tbl_info_s DgainInfo[9] = {
	{
		.gainMax = 2048,
		.idxBase = 0,
		.regGain = 0x00,
		.regGainFineBase = 0x40,
		.regGainFineStep = 1,
	},
	{
		.gainMax = 4064,
		.idxBase = 65,
		.regGain = 0x00,
		.regGainFineBase = 0x82,
		.regGainFineStep = 0x02,
	},
	{
		.gainMax = 6144,
		.idxBase = 128,
		.regGain = 0x01,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 10240,
		.idxBase = 130,
		.regGain = 0x02,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 14336,
		.idxBase = 132,
		.regGain = 0x03,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 18432,
		.idxBase = 134,
		.regGain = 0x04,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 22528,
		.idxBase = 136,
		.regGain = 0x05,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 26624,
		.idxBase = 138,
		.regGain = 0x06,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
	{
		.gainMax = 32768,
		.idxBase = 140,
		.regGain = 0x07,
		.regGainFineBase = 0x00,
		.regGainFineStep = 0x80,
	},
};

/*
 * GainReg {P1:0x1F P1:0x20}
 * DGain
 * 1x: 0x1F=0x00;0x20=0x40
 * 32x:0x1F=0x07;0x20=0xFF
 * minmum step is 1/64
 */
static CVI_U32 Dgain_table[] = {
	1024, 1040, 1056, 1072, 1088, 1104, 1120, 1136, 1152, 1168, 1184, 1200, 1216, 1232, 1248,
	1264, 1280, 1296, 1312, 1328, 1344, 1360, 1376, 1392, 1408, 1424, 1440, 1456, 1472, 1488,
	1504, 1520, 1536, 1552, 1568, 1584, 1600, 1616, 1632, 1648, 1664, 1680, 1696, 1712, 1728,
	1744, 1760, 1776, 1792, 1808, 1824, 1840, 1856, 1872, 1888, 1904, 1920, 1936, 1952, 1968,
	1984, 2000, 2016, 2032, 2048, 2080, 2112, 2144, 2176, 2208, 2240, 2272, 2304, 2336, 2368,
	2400, 2432, 2464, 2496, 2528, 2560, 2592, 2624, 2656, 2688, 2720, 2752, 2784, 2816, 2848,
	2880, 2912, 2944, 2976, 3008, 3040, 3072, 3104, 3136, 3168, 3200, 3232, 3264, 3296, 3328,
	3360, 3392, 3424, 3456, 3488, 3520, 3552, 3584, 3616, 3648, 3680, 3712, 3744, 3776, 3808,
	3840, 3872, 3904, 3936, 3968, 4000, 4032, 4064, 4096, 6144, 8192, 10240, 12288, 14336, 16384,
	18432, 20480, 22528, 24576, 26624, 28672, 30720, 32768,
};

static const CVI_U32 again_table_size = ARRAY_SIZE(Again_table);
static const CVI_U32 dgain_table_size = ARRAY_SIZE(Dgain_table);

static CVI_S32 cmos_again_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32AgainLin, CVI_U32 *pu32AgainDb)
{
	CVI_U32 i;

	(void) ViPipe;

	CMOS_CHECK_POINTER(pu32AgainLin);
	CMOS_CHECK_POINTER(pu32AgainDb);

	if (*pu32AgainLin >= Again_table[again_table_size - 1]) {
		*pu32AgainLin = Again_table[again_table_size - 1];
		*pu32AgainDb = again_table_size - 1;
		return CVI_SUCCESS;
	}

	for (i = 1; i < again_table_size; i++) {
		if (*pu32AgainLin < Again_table[i]) {
			*pu32AgainLin = Again_table[i - 1];
			*pu32AgainDb = i - 1;
			break;
		}
	}
	return CVI_SUCCESS;
}

static CVI_S32 cmos_dgain_calc_table(VI_PIPE ViPipe, CVI_U32 *pu32DgainLin, CVI_U32 *pu32DgainDb)
{
	CVI_U32 i;

	(void) ViPipe;

	CMOS_CHECK_POINTER(pu32DgainLin);
	CMOS_CHECK_POINTER(pu32DgainDb);

	if (*pu32DgainLin >= Dgain_table[dgain_table_size - 1]) {
		*pu32DgainLin = Dgain_table[dgain_table_size - 1];
		*pu32DgainDb = dgain_table_size - 1;
		return CVI_SUCCESS;
	}

	for (i = 1; i < dgain_table_size; i++) {
		if (*pu32DgainLin < Dgain_table[i]) {
			*pu32DgainLin = Dgain_table[i - 1];
			*pu32DgainDb = i - 1;
			break;
		}
	}

	return CVI_SUCCESS;
}

static CVI_S32 cmos_gains_update(VI_PIPE ViPipe, CVI_U32 *pu32Again, CVI_U32 *pu32Dgain)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	CVI_U32 u32Again;
	CVI_U32 u32Dgain;
	struct gain_tbl_info_s *info;
	int i, tbl_num;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pu32Again);
	CMOS_CHECK_POINTER(pu32Dgain);
	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;
	u32Again = pu32Again[0];
	u32Dgain = pu32Dgain[0];

	if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
		/* linear mode */

		/* find Again register setting. */
		tbl_num = sizeof(AgainInfo)/sizeof(struct gain_tbl_info_s);
		for (i = tbl_num - 1; i >= 0; i--) {
			info = &AgainInfo[i];

			if (u32Again >= info->idxBase)
				break;
		}

		u32Again = info->regGainFineBase + (u32Again - info->idxBase) * info->regGainFineStep;
		pstSnsRegsInfo->astI2cData[LINEAR_AGAIN].u32Data = u32Again & 0xFF;

		/* find Dgain register setting. */
		tbl_num = sizeof(DgainInfo)/sizeof(struct gain_tbl_info_s);
		for (i = tbl_num - 1; i >= 0; i--) {
			info = &DgainInfo[i];

			if (u32Dgain >= info->idxBase)
				break;
		}

		pstSnsRegsInfo->astI2cData[LINEAR_DGAIN_0].u32Data = (info->regGain & 0xFF);
		u32Dgain = info->regGainFineBase + (u32Dgain - info->idxBase) * info->regGainFineStep;
		u32Dgain = (u32Dgain > 0xff) ? 0xff : u32Dgain;
		pstSnsRegsInfo->astI2cData[LINEAR_DGAIN_1].u32Data = u32Dgain & 0xFF;
	}
	return CVI_SUCCESS;
}


static CVI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
	CMOS_CHECK_POINTER(pstExpFuncs);

	memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

	pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
	pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
	//pstExpFuncs->pfn_cmos_slow_framerate_set = cmos_slow_framerate_set;
	pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
	pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;
	pstExpFuncs->pfn_cmos_again_calc_table  = cmos_again_calc_table;
	pstExpFuncs->pfn_cmos_dgain_calc_table  = cmos_dgain_calc_table;
	// pstExpFuncs->pfn_cmos_get_inttime_max   = cmos_get_inttime_max;
	// pstExpFuncs->pfn_cmos_ae_fswdr_attr_set = cmos_ae_fswdr_attr_set;

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_awb_default(VI_PIPE ViPipe, AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
	(void) ViPipe;

	CMOS_CHECK_POINTER(pstAwbSnsDft);

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
	(void) ViPipe;

	memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_blc_default(VI_PIPE ViPipe, ISP_CMOS_BLACK_LEVEL_S *pstBlc)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pstBlc);

	memset(pstBlc, 0, sizeof(ISP_CMOS_BLACK_LEVEL_S));

	memcpy(pstBlc, &g_stIspBlcCalibratio, sizeof(ISP_CMOS_BLACK_LEVEL_S));

	return CVI_SUCCESS;
}

static CVI_S32 cmos_get_wdr_size(VI_PIPE ViPipe, ISP_SNS_ISP_INFO_S *pstIspCfg)
{
	const OS02N10_1L_MODE_S *pstMode = CVI_NULL;
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstMode = &g_astOs02n10_1l_mode[pstSnsState->u8ImgMode];

	if (pstSnsState->enWDRMode != WDR_MODE_NONE) {
		pstIspCfg->frm_num = 2;
		memcpy(&pstIspCfg->img_size[0], &pstMode->astImg[0], sizeof(ISP_WDR_SIZE_S));
		memcpy(&pstIspCfg->img_size[1], &pstMode->astImg[1], sizeof(ISP_WDR_SIZE_S));
	} else {
		pstIspCfg->frm_num = 1;
		memcpy(&pstIspCfg->img_size[0], &pstMode->astImg[0], sizeof(ISP_WDR_SIZE_S));
	}

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

static CVI_S32 cmos_set_wdr_mode(VI_PIPE ViPipe, CVI_U8 u8Mode)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstSnsState->bSyncInit = CVI_FALSE;

	switch (u8Mode) {
	case WDR_MODE_NONE:
		pstSnsState->u8ImgMode = OS02N10_1L_MODE_1080P15;
		pstSnsState->enWDRMode = WDR_MODE_NONE;
		pstSnsState->u32FLStd = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].u32VtsDef;
		syslog(LOG_INFO, "WDR_MODE_NONE\n");
		break;

	default:
		CVI_TRACE_SNS(CVI_DBG_ERR, "Unsupport sensor mode!\n");
		return CVI_FAILURE;
	}

	pstSnsState->au32FL[0] = pstSnsState->u32FLStd;
	pstSnsState->au32FL[1] = pstSnsState->au32FL[0];
	memset(pstSnsState->au32WDRIntTime, 0, sizeof(pstSnsState->au32WDRIntTime));

	return CVI_SUCCESS;
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
	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	pstSnsRegsInfo = &pstSnsSyncInfo->snsCfg;
	pstCfg0 = &pstSnsState->astSyncInfo[0];
	pstCfg1 = &pstSnsState->astSyncInfo[1];
	pstI2c_data = pstCfg0->snsCfg.astI2cData;

	if ((pstSnsState->bSyncInit == CVI_FALSE) || (pstSnsRegsInfo->bConfig == CVI_FALSE)) {
		pstCfg0->snsCfg.enSnsType = SNS_I2C_TYPE;
		pstCfg0->snsCfg.unComBus.s8I2cDev = g_aunOs02n10_1l_BusInfo[ViPipe].s8I2cDev;
		pstCfg0->snsCfg.u8Cfg2ValidDelayMax = 0;
		pstCfg0->snsCfg.use_snsr_sram = CVI_TRUE;
		pstCfg0->snsCfg.u32RegNum = LINEAR_REGS_NUM;

		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			pstI2c_data[i].bUpdate = CVI_TRUE;
			pstI2c_data[i].u8DevAddr = os02n10_1l_i2c_addr;
			pstI2c_data[i].u32AddrByteNum = os02n10_1l_addr_byte;
			pstI2c_data[i].u32DataByteNum = os02n10_1l_data_byte;
		}

		switch (pstSnsState->enWDRMode) {
		case WDR_MODE_NONE:
			// switch page 1
			pstI2c_data[LINEAR_PAGE_SWITCH].u32RegAddr = OS02N10_1L_PAGE_SWITCH_ADDR;
			pstI2c_data[LINEAR_PAGE_SWITCH].u32Data = 0x01;
			pstI2c_data[LINEAR_EXP_0].u32RegAddr = OS02N10_1L_EXP0_ADDR;
			pstI2c_data[LINEAR_EXP_1].u32RegAddr = OS02N10_1L_EXP0_ADDR + 1;
			pstI2c_data[LINEAR_EXP_TRIGGER].u32RegAddr = OS02N10_1L_TRIGGER_ADDR;
			pstI2c_data[LINEAR_EXP_TRIGGER].u32Data = 0x02;

			pstI2c_data[LINEAR_AGAIN].u32RegAddr = OS02N10_1L_AGAIN_ADDR;
			pstI2c_data[LINEAR_DGAIN_0].u32RegAddr = OS02N10_1L_DGAIN0_ADDR;
			pstI2c_data[LINEAR_DGAIN_1].u32RegAddr = OS02N10_1L_DGAIN0_ADDR + 1;
			pstI2c_data[LINEAR_GAIN_TRIGGER].u32RegAddr = OS02N10_1L_TRIGGER_ADDR;
			pstI2c_data[LINEAR_GAIN_TRIGGER].u32Data = 0x02;

			pstI2c_data[LINEAR_VB_0].u32RegAddr = OS02N10_1L_VB0_ADDR;
			pstI2c_data[LINEAR_VB_1].u32RegAddr = OS02N10_1L_VB0_ADDR + 1;
			pstI2c_data[LINEAR_VB_1].u32Data = 0x90;
			pstI2c_data[LINEAR_VB_TRIGGER].u32RegAddr = OS02N10_1L_TRIGGER_ADDR;
			pstI2c_data[LINEAR_VB_TRIGGER].u32Data = 0x02;

			pstI2c_data[LINEAR_FLIP_MIRROR].u32RegAddr   = OS02N10_1L_FLIP_MIRROR_ADDR;
			pstI2c_data[LINEAR_FLIP_PAGE_SWITCH].u32RegAddr = OS02N10_1L_FLIP_PAGE_SWITCH_ADDR;
			pstI2c_data[LINEAR_FLIP_PAGE_SWITCH].u32Data = 0x03;
			pstI2c_data[LINEAR_FLIP_MIRROR_0].u32RegAddr = OS02N10_1L_FLIP_MIRROR0_ADDR;
			pstI2c_data[LINEAR_FLIP_MIRROR_1].u32RegAddr = OS02N10_1L_FLIP_MIRROR1_ADDR;
			pstI2c_data[LINEAR_FLIP_MIRROR_2].u32RegAddr = OS02N10_1L_FLIP_MIRROR2_ADDR;
			pstI2c_data[LINEAR_COL_OFFSET].u32RegAddr    = OS02N10_1L_FLIP_col_offset_ADDR;
			pstI2c_data[LINEAR_ROW_OFFSET_0].u32RegAddr  = OS02N10_1L_FLIP_row_offset0_ADDR;
			pstI2c_data[LINEAR_ROW_OFFSET_1].u32RegAddr  = OS02N10_1L_FLIP_row_offset1_ADDR;

			break;
		default:
			break;
		}
		pstSnsState->bSyncInit = CVI_TRUE;
		pstCfg0->snsCfg.need_update = CVI_TRUE;
		cmos_get_wdr_size(ViPipe, &pstCfg0->ispCfg);
		pstCfg0->ispCfg.need_update = CVI_TRUE;

	} else {

		CVI_U32 gainsUpdate = 0, vtsUpdate = 0, shutterUpdate = 0, flipmirrorUpdate = 0;

		pstCfg0->snsCfg.need_update = CVI_FALSE;
		for (i = 0; i < pstCfg0->snsCfg.u32RegNum; i++) {
			if (pstCfg0->snsCfg.astI2cData[i].u32Data == pstCfg1->snsCfg.astI2cData[i].u32Data) {
				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_FALSE;
			} else {
				if ((i >= LINEAR_AGAIN) && (i <= LINEAR_GAIN_TRIGGER))
					gainsUpdate = 1;
				if ((i >= LINEAR_VB_0) && (i <= LINEAR_VB_TRIGGER))
					vtsUpdate = 1;
				if ((i >= LINEAR_EXP_0 ) && (i <= LINEAR_EXP_TRIGGER))
					shutterUpdate = 1;
				if ((i >= LINEAR_FLIP_MIRROR ) && (i <= LINEAR_ROW_OFFSET_1))
					flipmirrorUpdate = 1;

				pstCfg0->snsCfg.astI2cData[i].bUpdate = CVI_TRUE;
				pstCfg0->snsCfg.need_update = CVI_TRUE;
			}
		}
		if (gainsUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_PAGE_SWITCH].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_AGAIN].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_DGAIN_0].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_DGAIN_1].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_GAIN_TRIGGER].bUpdate = CVI_TRUE;
		}
		if (shutterUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_PAGE_SWITCH].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_0].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_1].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_EXP_TRIGGER].bUpdate = CVI_TRUE;
		}
		if (vtsUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_PAGE_SWITCH].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_0].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_1].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_VB_TRIGGER].bUpdate = CVI_TRUE;
		}

		if (flipmirrorUpdate) {
			pstCfg0->snsCfg.astI2cData[LINEAR_PAGE_SWITCH].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_MIRROR].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_PAGE_SWITCH].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_MIRROR_0].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_MIRROR_1].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_FLIP_MIRROR_2].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_COL_OFFSET].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_ROW_OFFSET_0].bUpdate = CVI_TRUE;
			pstCfg0->snsCfg.astI2cData[LINEAR_ROW_OFFSET_1].bUpdate = CVI_TRUE;
		}

		/* check update isp crop or not */
		pstCfg0->ispCfg.need_update = (sensor_cmp_wdr_size(&pstCfg0->ispCfg, &pstCfg1->ispCfg) ?
				CVI_TRUE : CVI_FALSE);
		pstCfg0->ispCfg.u8DelayFrmNum = 3;
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
	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);

	u8SensorImageMode = pstSnsState->u8ImgMode;
	pstSnsState->bSyncInit = CVI_FALSE;
	if (pstSensorImageMode->f32Fps <= 30) {
		if (pstSnsState->enWDRMode == WDR_MODE_NONE) {
			if (OS02N10_1L_RES_IS_1080P(pstSensorImageMode->u16Width, pstSensorImageMode->u16Height)) {
				u8SensorImageMode = OS02N10_1L_MODE_1080P15;
			} else {
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
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;
	ISP_SNS_REGS_INFO_S *pstSnsRegsInfo = CVI_NULL;
	ISP_SNS_ISP_INFO_S *pstIspCfg0 = CVI_NULL;

	CVI_U8 value, value0, value1, value2, col_offset, row_offset_0, row_offset_1, start_x, start_y;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsRegsInfo = &pstSnsState->astSyncInfo[0].snsCfg;
	pstIspCfg0 = &pstSnsState->astSyncInfo[0].ispCfg;

	if (pstSnsState->bInit == CVI_TRUE && g_aeOs02n10_1l_MirrorFip[ViPipe] != eSnsMirrorFlip) {

		switch (eSnsMirrorFlip) {
		case ISP_SNS_NORMAL:
			value  = 0x00;
			value0 = 0x10;
			value1 = 0xe2;
			value2 = 0x50;
			col_offset = 0x00;
			row_offset_0 = 0x00;
			row_offset_1 = 0x00;
			start_x = 4;
			start_y = 4;
			break;
		case ISP_SNS_MIRROR:
			value = 0x02;
			value0 = 0x10;
			value1 = 0xe2;
			value2 = 0x5c;
			col_offset = 0xc3;
			row_offset_0 = 0x00;
			row_offset_1 = 0x00;
			start_x = 5;
			start_y = 4;
			break;
		case ISP_SNS_FLIP:
			value = 0x01;
			value0 = 0x10;
			value1 = 0xe2;
			value2 = 0x50;
			col_offset = 0x00;
			row_offset_0 = 0x3f;
			row_offset_1 = 0x04;
			start_x = 5;
			start_y = 5;
			break;
		case ISP_SNS_MIRROR_FLIP:
			value = 0x03;
			value0 = 0x10;
			value1 = 0xe2;
			value2 = 0x5c;
			col_offset = 0x00;
			row_offset_0 = 0x3f;
			row_offset_1 = 0x04;
			start_x = 4;
			start_y = 5;
			break;
		default:
			return;
		}

		pstSnsRegsInfo->astI2cData[LINEAR_PAGE_SWITCH].u32Data = 0x01;
		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_MIRROR].u32Data =value;
		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_PAGE_SWITCH].u32Data = 0x03;
		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_MIRROR_0].u32Data =value0;
		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_MIRROR_1].u32Data =value1;
		pstSnsRegsInfo->astI2cData[LINEAR_FLIP_MIRROR_2].u32Data =value2;
		pstSnsRegsInfo->astI2cData[LINEAR_COL_OFFSET].u32Data =col_offset;
		pstSnsRegsInfo->astI2cData[LINEAR_ROW_OFFSET_0].u32Data =row_offset_0;
		pstSnsRegsInfo->astI2cData[LINEAR_ROW_OFFSET_1].u32Data =row_offset_1;
		pstSnsRegsInfo->astI2cData[LINEAR_ROW_OFFSET_1].bDropFrm = 1;
		pstSnsRegsInfo->astI2cData[LINEAR_ROW_OFFSET_1].u8DropFrmNum = 1;

		g_aeOs02n10_1l_MirrorFip[ViPipe] = eSnsMirrorFlip;
		pstIspCfg0->img_size[0].stWndRect.s32X = start_x;
		pstIspCfg0->img_size[0].stWndRect.s32Y = start_y;
	}
}

static CVI_VOID sensor_global_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER_VOID(pstSnsState);

	pstSnsState->bInit = CVI_FALSE;
	pstSnsState->bSyncInit = CVI_FALSE;
	pstSnsState->u8ImgMode = OS02N10_1L_MODE_1080P15;
	pstSnsState->enWDRMode = WDR_MODE_NONE;
	pstSnsState->u32FLStd  = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].u32VtsDef;
	pstSnsState->au32FL[0] = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].u32VtsDef;
	pstSnsState->au32FL[1] = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].u32VtsDef;

	memset(&pstSnsState->astSyncInfo[0], 0, sizeof(ISP_SNS_SYNC_INFO_S));
	memset(&pstSnsState->astSyncInfo[1], 0, sizeof(ISP_SNS_SYNC_INFO_S));
}

static CVI_S32 sensor_rx_attr(VI_PIPE ViPipe, SNS_COMBO_DEV_ATTR_S *pstRxAttr)
{
	ISP_SNS_STATE_S *pstSnsState = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pstSnsState);
	CMOS_CHECK_POINTER(pstSnsState);
	CMOS_CHECK_POINTER(pstRxAttr);

	memcpy(pstRxAttr, &os02n10_1l_rx_attr, sizeof(*pstRxAttr));

	pstRxAttr->img_size.width = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Width;
	pstRxAttr->img_size.height = g_astOs02n10_1l_mode[pstSnsState->u8ImgMode].astImg[0].stSnsSize.u32Height;
	if (pstSnsState->enWDRMode == WDR_MODE_NONE)
		pstRxAttr->mipi_attr.wdr_mode = CVI_MIPI_WDR_MODE_NONE;
	else
		pstRxAttr->mac_clk = RX_MAC_CLK_400M;
	return CVI_SUCCESS;
}

static CVI_S32 sensor_patch_rx_attr(RX_INIT_ATTR_S *pstRxInitAttr)
{
	SNS_COMBO_DEV_ATTR_S *pstRxAttr = &os02n10_1l_rx_attr;
	int i;

	CMOS_CHECK_POINTER(pstRxInitAttr);

	if (pstRxInitAttr->stMclkAttr.bMclkEn)
		pstRxAttr->mclk.cam = pstRxInitAttr->stMclkAttr.u8Mclk;

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

	pstSensorExpFunc->pfn_cmos_sensor_init = os02n10_1l_init;
	pstSensorExpFunc->pfn_cmos_sensor_exit = os02n10_1l_exit;
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
static CVI_VOID sensor_patch_i2c_addr(CVI_S32 s32I2cAddr)
{
	if (OS02N10_1L_I2C_ADDR_IS_VALID(s32I2cAddr))
		os02n10_1l_i2c_addr = s32I2cAddr;
}

static CVI_S32 os02n10_1l_set_bus_info(VI_PIPE ViPipe, ISP_SNS_COMMBUS_U unSNSBusInfo)
{
	g_aunOs02n10_1l_BusInfo[ViPipe].s8I2cDev = unSNSBusInfo.s8I2cDev;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_ctx_init(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);

	if (pastSnsStateCtx == CVI_NULL) {
		pastSnsStateCtx = (ISP_SNS_STATE_S *)malloc(sizeof(ISP_SNS_STATE_S));
		if (pastSnsStateCtx == CVI_NULL) {
			CVI_TRACE_SNS(CVI_DBG_ERR, "Isp[%d] SnsCtx malloc memory failed!\n", ViPipe);
			return -ENOMEM;
		}
	}

	memset(pastSnsStateCtx, 0, sizeof(ISP_SNS_STATE_S));

	OS02N10_1L_SENSOR_SET_CTX(ViPipe, pastSnsStateCtx);

	return CVI_SUCCESS;
}

static CVI_VOID sensor_ctx_exit(VI_PIPE ViPipe)
{
	ISP_SNS_STATE_S *pastSnsStateCtx = CVI_NULL;

	OS02N10_1L_SENSOR_GET_CTX(ViPipe, pastSnsStateCtx);
	SENSOR_FREE(pastSnsStateCtx);
	OS02N10_1L_SENSOR_RESET_CTX(ViPipe);
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

	stSnsAttrInfo.eSensorId = OS02N10_1L_ID;

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

	s32Ret = CVI_ISP_SensorUnRegCallBack(ViPipe, OS02N10_1L_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AE_SensorUnRegCallBack(ViPipe, pstAeLib, OS02N10_1L_ID);
	if (s32Ret != CVI_SUCCESS) {
		CVI_TRACE_SNS(CVI_DBG_ERR, "sensor unregister callback function to ae lib failed!\n");
		return s32Ret;
	}

	s32Ret = CVI_AWB_SensorUnRegCallBack(ViPipe, pstAwbLib, OS02N10_1L_ID);
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
	g_au16Os02n10_1l_GainMode[ViPipe] = pstInitAttr->enGainMode;
	g_au16Os02n10_1l_UseHwSync[ViPipe] = pstInitAttr->u16UseHwSync;

	return CVI_SUCCESS;
}

static CVI_S32 sensor_probe(VI_PIPE ViPipe)
{
	return os02n10_1l_probe(ViPipe);
}

ISP_SNS_OBJ_S stSnsOs02n10_1l_Obj = {
	.pfnRegisterCallback    = sensor_register_callback,
	.pfnUnRegisterCallback  = sensor_unregister_callback,
	.pfnStandby             = os02n10_1l_standby,
	.pfnRestart             = os02n10_1l_restart,
	.pfnMirrorFlip          = sensor_mirror_flip,
	.pfnWriteReg            = os02n10_1l_write_register,
	.pfnReadReg             = os02n10_1l_read_register,
	.pfnSetBusInfo          = os02n10_1l_set_bus_info,
	.pfnSetInit             = sensor_set_init,
	.pfnPatchRxAttr		= sensor_patch_rx_attr,
	.pfnPatchI2cAddr	= sensor_patch_i2c_addr,
	.pfnGetRxAttr		= sensor_rx_attr,
	.pfnExpSensorCb		= cmos_init_sensor_exp_function,
	.pfnExpAeCb		= cmos_init_ae_exp_function,
	.pfnSnsProbe		= sensor_probe,
};

