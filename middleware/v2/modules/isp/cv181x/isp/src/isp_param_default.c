/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_param_default.c
 * Description:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_isp.h"
#include "isp_peri_ctrl.h"

#include "cvi_ae.h"

#include "isp_feature.h"


#define INIT_V(attr, param, value) {(attr).param = (value); }
#define INIT_V_ARRAY(attr, param, ...) do {\
	typeof((attr).param[0]) arr[] = { __VA_ARGS__ };\
	for (CVI_U32 x = 0; x < ARRAY_SIZE(arr); x++) {\
		(attr).param[x] = arr[x];\
	} \
} while (0)
#define INIT_A(attr, param, manual, ...) do {\
	typeof((attr).stManual.param) arr[] = { __VA_ARGS__ };\
	(attr).stManual.param = manual;\
	for (CVI_U32 x = 0; x < ISP_AUTO_ISO_STRENGTH_NUM; x++) {\
		(attr).stAuto.param[x] = arr[x];\
	} \
} while (0)
#define INIT_LV_A(attr, param, manual, ...) do {\
	typeof((attr).stManual.param) arr[] = { __VA_ARGS__ };\
	(attr).stManual.param = manual;\
	for (CVI_U32 x = 0; x < ISP_AUTO_LV_NUM; x++) {\
		(attr).stAuto.param[x] = arr[x];\
	} \
} while (0)

// static void setBlcDefault(VI_PIPE ViPipe);
//static void setRlscDefault(VI_PIPE ViPipe);
static void setGammaDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setAutoGammaDefault(VI_PIPE ViPipe);
static void setDemosaicDefault(VI_PIPE ViPipe);
static void setRgbCacDefault(VI_PIPE ViPipe);
static void setLCacDefault(VI_PIPE ViPipe);
static void setBnrDefault(VI_PIPE ViPipe);
static void setYnrDefault(VI_PIPE ViPipe);
static void setCnrDefault(VI_PIPE ViPipe);
static void setTnrDefault(VI_PIPE ViPipe);
static void setDpcDefault(VI_PIPE ViPipe);
static void setCrossTalkDefault(VI_PIPE ViPipe);
static void setMlscDefault(VI_PIPE ViPipe);
static void setFsWdrDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setDrcDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setExposureAttrDefault(VI_PIPE ViPipe);
static void setWdrExposureAttrDefault(VI_PIPE ViPipe);
static void setDciDefault(VI_PIPE ViPipe);
static void setLdciDefault(VI_PIPE ViPipe);
static void setCaDefault(VI_PIPE ViPipe);
static void setDehazeDefault(VI_PIPE ViPipe);
static void setClutDefault(VI_PIPE ViPipe);
static void setCcmDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setCcmSaturationDefault(VI_PIPE ViPipe);
static void setSaturationDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setCacDefault(VI_PIPE ViPipe);
static void setCa2Default(VI_PIPE ViPipe);
static void setPreSharpenDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setSharpenDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static void setYContrastDefault(VI_PIPE ViPipe);
static void setMonoDefault(VI_PIPE ViPipe);
static CVI_S32 setNoiseProfile(VI_PIPE ViPipe);
static CVI_S32 setDisDefault(VI_PIPE ViPipe);
static void setVCDefault(VI_PIPE ViPipe);
static void setCscDefault(VI_PIPE ViPipe);

static CVI_S32 setIspIqParamDefault(VI_PIPE ViPipe, CVI_U32 wdrEn);
static CVI_VOID setDemosaicDemoire(VI_PIPE ViPipe);
static CVI_VOID setDemosaic(VI_PIPE ViPipe);
static CVI_VOID setMeshShading(VI_PIPE ViPipe);
static CVI_VOID setMeshShadingGainLut(VI_PIPE ViPipe);
//static CVI_VOID setRadialShading(VI_PIPE ViPipe);
//static CVI_VOID setRadialShadingGainLut(VI_PIPE ViPipe);
static CVI_VOID setDpcDynamic(VI_PIPE ViPipe);
static CVI_VOID setDpcCalibration(VI_PIPE ViPipe);
static CVI_VOID setDpcStatic(VI_PIPE ViPipe);
static CVI_VOID setCnr(VI_PIPE ViPipe);
static CVI_VOID setCnrMotion(VI_PIPE ViPipe);
static CVI_VOID setTnr(VI_PIPE ViPipe);
static CVI_VOID setTnrNoiseModel(VI_PIPE ViPipe);
static CVI_VOID setTnrLumaMotion(VI_PIPE ViPipe);
static CVI_VOID setTnrGhost(VI_PIPE ViPipe);
static CVI_VOID setTnrMtPrt(VI_PIPE ViPipe);
static CVI_VOID setTnrMotionAdapt(VI_PIPE ViPipe);
static CVI_VOID setYnr(VI_PIPE ViPipe);
static CVI_VOID setYnrMotionNr(VI_PIPE ViPipe);
static CVI_VOID setYnrFilter(VI_PIPE ViPipe);
static CVI_VOID setClut(VI_PIPE ViPipe);
static CVI_VOID setClutSaturation(VI_PIPE ViPipe);
static CVI_VOID setBnr(VI_PIPE ViPipe);
static CVI_VOID setBnrFilter(VI_PIPE ViPipe);
static CVI_S32 setDisAttr(VI_PIPE ViPipe);
static CVI_S32 setDisConfig(VI_PIPE ViPipe);

CVI_S32 isp_iqParam_addr_initDefault(VI_PIPE ViPipe)
{
	ISP_CTX_S *pstIspCtx = NULL;
	CVI_U32 wdrEn = 0;

	ISP_GET_CTX(ViPipe, pstIspCtx);
	wdrEn = IS_2to1_WDR_MODE(pstIspCtx->u8SnsWDRMode);

	setIspIqParamDefault(ViPipe, wdrEn);

	return 0;
}

static CVI_S32 setIspIqParamDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
#ifdef ARCH_RTOS_CV181X
	setExposureAttrDefault(ViPipe);
#else
	// setBlcDefault(ViPipe);
	//setRlscDefault(ViPipe);
	setGammaDefault(ViPipe, wdrEn);
	setAutoGammaDefault(ViPipe);
	setDemosaicDefault(ViPipe);
	setRgbCacDefault(ViPipe);
	setLCacDefault(ViPipe);
	setBnrDefault(ViPipe);
	setYnrDefault(ViPipe);
	setCnrDefault(ViPipe);
	setTnrDefault(ViPipe);
	setDpcDefault(ViPipe);
	setCrossTalkDefault(ViPipe);
	setMlscDefault(ViPipe);
	setFsWdrDefault(ViPipe, wdrEn);
	setDrcDefault(ViPipe, wdrEn);
	setExposureAttrDefault(ViPipe);
	setWdrExposureAttrDefault(ViPipe);
	setDciDefault(ViPipe);
	setLdciDefault(ViPipe);
	setCaDefault(ViPipe);
	setDehazeDefault(ViPipe);
	setClutDefault(ViPipe);
	setCcmDefault(ViPipe, wdrEn);
	setCcmSaturationDefault(ViPipe);
	setSaturationDefault(ViPipe, wdrEn);
	setCacDefault(ViPipe);
	setCa2Default(ViPipe);
	setPreSharpenDefault(ViPipe, wdrEn);
	setSharpenDefault(ViPipe, wdrEn);
	setYContrastDefault(ViPipe);
	setMonoDefault(ViPipe);
	setVCDefault(ViPipe);
	setNoiseProfile(ViPipe);
	setDisDefault(ViPipe);
	setCscDefault(ViPipe);
#endif
	return 0;
}

#if 0
CVI_VOID setBlcDefault(VI_PIPE ViPipe)
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
#endif

//static CVI_VOID setRlscDefault(VI_PIPE ViPipe)
//{
//	ISP_LOG_DEBUG("(%d)\n", ViPipe);
//
//	setRadialShading(ViPipe);
//	setRadialShadingGainLut(ViPipe);
//}
//
//static CVI_VOID setRadialShading(VI_PIPE ViPipe)
//{
//	ISP_LOG_DEBUG("(%d)\n", ViPipe);
//
//	ISP_RADIAL_SHADING_ATTR_S attr = {0};
//
//	INIT_V(attr, Enable, 0);
//	INIT_V(attr, enOpType, 0);
//	INIT_V(attr, UpdateInterval, 1);
//	INIT_V(attr, CenterX, 960);
//	INIT_V(attr, CenterY, 540);
//	INIT_V(attr, RadiusScaleRGB, 0);
//	INIT_V(attr, RadiusScaleIR, 0);
//	INIT_A(attr, RadiusStr, 0,
//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
//	);
//	INIT_A(attr, RadiusIRStr, 0,
//	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
//	);
//
//	switch (ViPipe) {
//	case 3:
//	case 2:
//	case 1:
//	case 0:
//	break;
//	default:
//		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
//	break;
//	}
//
//	isp_rlsc_ctrl_set_rlsc_attr(ViPipe, &attr);
//}
//
//static CVI_VOID setRadialShadingGainLut(VI_PIPE ViPipe)
//{
//	ISP_LOG_DEBUG("(%d)\n", ViPipe);
//
//	ISP_RADIAL_SHADING_GAIN_LUT_ATTR_S attr = {0};
//
//	// INIT_V(attr, LscGainLut[0].RadiusGain, 512);
//	for (CVI_U32 table = 0; table < ISP_RLSC_COLOR_TEMPERATURE_SIZE; table++) {
//		for (CVI_U32 cell = 0; cell < ISP_RLSC_WINDOW_SIZE; cell++) {
//			attr.RLscGainLut[table].RGain[cell] = 512;
//			attr.RLscGainLut[table].GGain[cell] = 512;
//			attr.RLscGainLut[table].BGain[cell] = 512;
//			attr.RLscGainLut[table].IrGain[cell] = 512;
//		}
//	}
//
//	INIT_V(attr, Size, 1);
//
//	CVI_U16 au16ColorTemp[ISP_RLSC_COLOR_TEMPERATURE_SIZE] = {2300, 2850, 3000, 3900, 4200, 5000, 6500};
//
//	for (CVI_U32 idx = 0; idx < ISP_RLSC_COLOR_TEMPERATURE_SIZE; ++idx) {
//		attr.RLscGainLut[idx].ColorTemperature = au16ColorTemp[idx];
//	}
//
//	switch (ViPipe) {
//	case 3:
//	case 2:
//	case 1:
//	case 0:
//	break;
//	default:
//		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
//	break;
//	}
//
//	isp_rlsc_ctrl_set_rlsc_lut_attr(ViPipe, &attr);
//}

CVI_VOID setGammaDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_GAMMA_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, enCurveType, ISP_GAMMA_CURVE_USER_DEFINE);
	if (wdrEn) {
		INIT_V_ARRAY(attr, Table,
		   0,   36,   70,  102,  131,  159,  186,  211,  236,  261,  284,  307,  330,  353,  375,  397,
		 418,  439,  461,  481,  502,  523,  543,  563,  583,  603,  623,  642,  662,  681,  700,  719,
		 738,  757,  776,  795,  813,  832,  850,  868,  887,  905,  923,  941,  959,  977,  994, 1012,
		1030, 1047, 1065, 1082, 1100, 1117, 1134, 1152, 1169, 1186, 1203, 1220, 1237, 1254, 1271, 1288,
		1305, 1321, 1338, 1355, 1371, 1388, 1404, 1421, 1437, 1454, 1470, 1487, 1503, 1519, 1535, 1552,
		1568, 1584, 1600, 1616, 1632, 1648, 1664, 1680, 1696, 1712, 1727, 1743, 1759, 1775, 1790, 1806,
		1822, 1837, 1853, 1869, 1884, 1900, 1915, 1931, 1946, 1962, 1977, 1992, 2008, 2023, 2038, 2053,
		2069, 2084, 2099, 2114, 2129, 2145, 2160, 2175, 2190, 2205, 2220, 2235, 2250, 2265, 2280, 2295,
		2310, 2325, 2339, 2354, 2369, 2384, 2399, 2413, 2428, 2443, 2458, 2472, 2487, 2502, 2516, 2531,
		2545, 2560, 2575, 2589, 2604, 2618, 2633, 2647, 2662, 2676, 2690, 2705, 2719, 2734, 2748, 2762,
		2777, 2791, 2805, 2820, 2834, 2848, 2862, 2877, 2891, 2905, 2919, 2933, 2948, 2962, 2976, 2990,
		3004, 3018, 3032, 3046, 3060, 3074, 3088, 3102, 3116, 3130, 3144, 3158, 3172, 3186, 3200, 3214,
		3228, 3242, 3256, 3270, 3283, 3297, 3311, 3325, 3339, 3353, 3366, 3380, 3394, 3408, 3421, 3435,
		3449, 3462, 3476, 3490, 3503, 3517, 3531, 3544, 3558, 3572, 3585, 3599, 3612, 3626, 3639, 3653,
		3667, 3680, 3694, 3707, 3721, 3734, 3748, 3761, 3774, 3788, 3801, 3815, 3828, 3842, 3855, 3868,
		3882, 3895, 3908, 3922, 3935, 3948, 3962, 3975, 3988, 4002, 4015, 4028, 4042, 4055, 4068, 4081
		);
	} else {
		INIT_V_ARRAY(attr, Table,
		   0,   14,   42,   85,  146,  221,  307,  399,  494,  589,  681,  765,  840,  907,  975, 1041,
		1105, 1168, 1229, 1288, 1345, 1398, 1448, 1496, 1539, 1579, 1615, 1651, 1686, 1720, 1753, 1785,
		1817, 1847, 1877, 1906, 1934, 1962, 1989, 2015, 2041, 2067, 2091, 2116, 2140, 2163, 2187, 2210,
		2233, 2255, 2277, 2300, 2321, 2342, 2363, 2383, 2402, 2421, 2439, 2457, 2474, 2491, 2508, 2524,
		2540, 2555, 2571, 2586, 2600, 2615, 2629, 2644, 2658, 2672, 2686, 2700, 2714, 2728, 2741, 2755,
		2768, 2781, 2794, 2806, 2818, 2830, 2842, 2853, 2865, 2876, 2887, 2898, 2908, 2919, 2930, 2940,
		2950, 2961, 2971, 2982, 2992, 3002, 3013, 3023, 3033, 3043, 3053, 3063, 3073, 3083, 3092, 3102,
		3112, 3121, 3131, 3140, 3149, 3158, 3167, 3177, 3186, 3195, 3204, 3212, 3221, 3230, 3239, 3248,
		3257, 3265, 3274, 3282, 3291, 3299, 3308, 3316, 3324, 3333, 3341, 3349, 3357, 3365, 3373, 3381,
		3389, 3397, 3405, 3413, 3420, 3428, 3436, 3444, 3451, 3459, 3467, 3474, 3482, 3489, 3497, 3504,
		3511, 3519, 3526, 3533, 3540, 3548, 3555, 3562, 3569, 3576, 3583, 3590, 3597, 3604, 3610, 3617,
		3624, 3631, 3638, 3645, 3651, 3658, 3665, 3671, 3678, 3684, 3691, 3697, 3704, 3710, 3717, 3723,
		3730, 3736, 3742, 3749, 3755, 3761, 3767, 3774, 3780, 3786, 3792, 3798, 3804, 3811, 3817, 3823,
		3829, 3835, 3841, 3846, 3852, 3858, 3864, 3870, 3876, 3881, 3887, 3893, 3899, 3904, 3910, 3916,
		3921, 3927, 3933, 3938, 3944, 3949, 3955, 3961, 3966, 3972, 3978, 3983, 3989, 3995, 4001, 4007,
		4013, 4019, 4025, 4031, 4037, 4042, 4048, 4053, 4059, 4064, 4069, 4074, 4078, 4083, 4087, 4091
		);
	}

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

	isp_gamma_ctrl_set_gamma_attr(ViPipe, &attr);
}

static CVI_VOID setAutoGammaDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_AUTO_GAMMA_ATTR_S attr = {0};

	INIT_V(attr, GammaTabNum, 4);
	INIT_V(attr, GammaTab[0].Lv, 0);
	INIT_V(attr, GammaTab[1].Lv, 200);
	INIT_V(attr, GammaTab[2].Lv, 500);
	INIT_V(attr, GammaTab[3].Lv, 800);
	INIT_V_ARRAY(attr, GammaTab[0].Tbl,
	   0,   25,   52,   80,  109,  139,  170,  202,  235,  269,  303,  338,  374,  411,  447,  485,
	 522,  560,  598,  637,  675,  714,  752,  791,  829,  867,  905,  943,  980, 1017, 1053, 1089,
	1124, 1158, 1192, 1224, 1256, 1287, 1317, 1346, 1374, 1400, 1426, 1450, 1473, 1496, 1519, 1540,
	1561, 1582, 1602, 1621, 1640, 1659, 1676, 1694, 1711, 1727, 1744, 1759, 1775, 1790, 1804, 1819,
	1833, 1847, 1860, 1873, 1886, 1899, 1912, 1924, 1937, 1949, 1961, 1973, 1984, 1996, 2008, 2020,
	2031, 2043, 2055, 2067, 2079, 2091, 2102, 2114, 2125, 2136, 2147, 2157, 2167, 2177, 2186, 2196,
	2205, 2214, 2223, 2231, 2240, 2248, 2256, 2264, 2272, 2280, 2288, 2295, 2303, 2310, 2318, 2325,
	2332, 2340, 2347, 2354, 2362, 2369, 2377, 2384, 2392, 2400, 2407, 2415, 2423, 2431, 2440, 2448,
	2457, 2465, 2474, 2482, 2490, 2499, 2507, 2515, 2523, 2531, 2539, 2547, 2555, 2563, 2571, 2579,
	2587, 2595, 2603, 2611, 2619, 2627, 2636, 2644, 2652, 2661, 2669, 2678, 2686, 2695, 2704, 2713,
	2722, 2732, 2741, 2751, 2761, 2771, 2781, 2791, 2802, 2812, 2823, 2835, 2846, 2857, 2869, 2881,
	2893, 2905, 2918, 2930, 2943, 2955, 2968, 2981, 2995, 3008, 3021, 3035, 3048, 3062, 3076, 3090,
	3104, 3118, 3132, 3146, 3161, 3175, 3190, 3204, 3219, 3233, 3248, 3263, 3277, 3292, 3307, 3322,
	3336, 3351, 3366, 3381, 3396, 3411, 3426, 3440, 3456, 3471, 3487, 3503, 3519, 3535, 3552, 3569,
	3585, 3602, 3619, 3637, 3654, 3671, 3688, 3706, 3723, 3740, 3757, 3775, 3792, 3809, 3826, 3843,
	3859, 3876, 3892, 3908, 3924, 3940, 3956, 3971, 3986, 4001, 4015, 4029, 4043, 4057, 4070, 4082
	);
	INIT_V_ARRAY(attr, GammaTab[1].Tbl,
	   0,   17,   36,   55,   75,   96,  117,  139,  161,  184,  207,  231,  255,  279,  304,  329,
	 354,  380,  406,  432,  458,  484,  510,  536,  563,  589,  615,  641,  667,  692,  718,  743,
	 768,  793,  817,  841,  864,  887,  909,  931,  953,  973,  994, 1013, 1033, 1052, 1071, 1089,
	1108, 1126, 1144, 1162, 1180, 1197, 1215, 1232, 1249, 1265, 1282, 1298, 1315, 1331, 1347, 1362,
	1378, 1393, 1409, 1424, 1439, 1454, 1469, 1484, 1498, 1513, 1528, 1542, 1556, 1571, 1585, 1599,
	1613, 1627, 1641, 1655, 1669, 1683, 1696, 1710, 1723, 1736, 1749, 1761, 1774, 1786, 1798, 1810,
	1822, 1833, 1845, 1856, 1868, 1879, 1890, 1901, 1912, 1923, 1934, 1944, 1955, 1966, 1977, 1988,
	1998, 2009, 2020, 2031, 2042, 2053, 2064, 2076, 2087, 2098, 2110, 2122, 2133, 2145, 2158, 2170,
	2183, 2195, 2208, 2221, 2233, 2246, 2259, 2272, 2285, 2298, 2311, 2324, 2338, 2351, 2364, 2377,
	2391, 2404, 2418, 2431, 2445, 2458, 2472, 2486, 2499, 2513, 2527, 2541, 2555, 2569, 2583, 2597,
	2611, 2625, 2639, 2653, 2667, 2681, 2696, 2710, 2724, 2739, 2753, 2768, 2782, 2797, 2811, 2826,
	2841, 2855, 2870, 2885, 2900, 2915, 2930, 2945, 2960, 2975, 2991, 3006, 3021, 3036, 3052, 3067,
	3083, 3098, 3113, 3129, 3144, 3160, 3176, 3191, 3207, 3222, 3238, 3254, 3269, 3285, 3300, 3316,
	3332, 3347, 3363, 3379, 3394, 3410, 3426, 3441, 3457, 3473, 3490, 3506, 3523, 3539, 3556, 3573,
	3590, 3607, 3624, 3641, 3659, 3676, 3693, 3710, 3728, 3745, 3762, 3779, 3796, 3813, 3829, 3846,
	3862, 3879, 3895, 3911, 3926, 3942, 3957, 3972, 3987, 4002, 4016, 4030, 4043, 4057, 4070, 4082
	);
	INIT_V_ARRAY(attr, GammaTab[2].Tbl,
	   0,   16,   33,   51,   70,   89,  108,  129,  149,  171,  192,  214,  237,  260,  283,  306,
	 330,  354,  378,  402,  426,  450,  475,  499,  524,  548,  572,  596,  620,  644,  667,  691,
	 713,  736,  758,  780,  802,  823,  843,  863,  883,  901,  920,  937,  955,  972,  988, 1005,
	1021, 1037, 1052, 1068, 1083, 1097, 1112, 1126, 1141, 1155, 1169, 1182, 1196, 1209, 1222, 1236,
	1249, 1261, 1274, 1287, 1300, 1312, 1325, 1338, 1350, 1363, 1375, 1388, 1401, 1413, 1426, 1439,
	1452, 1465, 1478, 1491, 1504, 1518, 1531, 1544, 1558, 1571, 1584, 1597, 1610, 1623, 1636, 1649,
	1662, 1674, 1687, 1700, 1713, 1725, 1738, 1751, 1763, 1776, 1789, 1801, 1814, 1827, 1840, 1852,
	1865, 1878, 1891, 1904, 1917, 1930, 1943, 1956, 1969, 1983, 1996, 2010, 2023, 2037, 2051, 2065,
	2079, 2093, 2107, 2121, 2135, 2149, 2164, 2178, 2193, 2207, 2222, 2236, 2251, 2266, 2281, 2295,
	2310, 2325, 2340, 2355, 2370, 2385, 2400, 2415, 2431, 2446, 2461, 2476, 2492, 2507, 2522, 2538,
	2553, 2569, 2584, 2600, 2615, 2631, 2646, 2662, 2678, 2693, 2709, 2725, 2740, 2756, 2772, 2788,
	2804, 2820, 2836, 2852, 2868, 2884, 2900, 2917, 2933, 2949, 2966, 2982, 2998, 3015, 3031, 3048,
	3064, 3081, 3097, 3114, 3130, 3147, 3163, 3180, 3196, 3213, 3229, 3246, 3262, 3279, 3295, 3312,
	3328, 3344, 3361, 3377, 3393, 3409, 3426, 3442, 3458, 3475, 3491, 3508, 3525, 3542, 3559, 3576,
	3593, 3610, 3627, 3645, 3662, 3679, 3696, 3713, 3730, 3748, 3765, 3781, 3798, 3815, 3831, 3848,
	3864, 3880, 3896, 3912, 3928, 3943, 3958, 3973, 3988, 4002, 4016, 4030, 4044, 4057, 4070, 4082
	);
	INIT_V_ARRAY(attr, GammaTab[3].Tbl,
	   0,   15,   31,   47,   63,   79,   95,  111,  127,  143,  159,  175,  191,  207,  223,  239,
	 255,  271,  287,  303,  319,  335,  351,  367,  383,  399,  415,  431,  447,  463,  479,  495,
	 511,  527,  543,  559,  575,  591,  607,  623,  639,  655,  671,  687,  703,  719,  735,  751,
	 767,  783,  799,  815,  831,  847,  863,  879,  895,  911,  927,  943,  959,  975,  991, 1007,
	1023, 1039, 1055, 1071, 1087, 1103, 1119, 1135, 1151, 1167, 1183, 1199, 1215, 1231, 1247, 1263,
	1279, 1295, 1311, 1327, 1343, 1359, 1375, 1391, 1407, 1423, 1439, 1455, 1471, 1487, 1503, 1519,
	1535, 1551, 1567, 1583, 1599, 1615, 1631, 1647, 1663, 1679, 1695, 1711, 1727, 1743, 1759, 1775,
	1791, 1807, 1823, 1839, 1855, 1871, 1887, 1903, 1919, 1935, 1951, 1967, 1983, 1999, 2015, 2031,
	2047, 2063, 2079, 2095, 2111, 2127, 2143, 2159, 2175, 2191, 2207, 2223, 2239, 2255, 2271, 2287,
	2303, 2319, 2335, 2351, 2367, 2383, 2399, 2415, 2431, 2447, 2463, 2479, 2495, 2511, 2527, 2543,
	2559, 2575, 2591, 2607, 2623, 2639, 2655, 2671, 2687, 2703, 2719, 2735, 2751, 2767, 2783, 2799,
	2815, 2831, 2847, 2863, 2879, 2895, 2911, 2927, 2943, 2959, 2975, 2991, 3007, 3023, 3039, 3055,
	3071, 3087, 3103, 3119, 3135, 3151, 3167, 3183, 3199, 3215, 3231, 3247, 3263, 3279, 3295, 3311,
	3327, 3343, 3359, 3375, 3391, 3407, 3423, 3439, 3455, 3471, 3487, 3503, 3519, 3535, 3551, 3567,
	3583, 3599, 3615, 3631, 3647, 3663, 3679, 3695, 3711, 3727, 3743, 3759, 3775, 3791, 3807, 3823,
	3839, 3855, 3871, 3887, 3903, 3919, 3935, 3951, 3967, 3983, 3999, 4015, 4031, 4047, 4063, 4079
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

	isp_gamma_ctrl_set_auto_gamma_attr(ViPipe, &attr);
}

static CVI_VOID setDemosaicDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setDemosaic(ViPipe);
	setDemosaicDemoire(ViPipe);
}

static CVI_VOID setDemosaic(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_DEMOSAIC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, RbVtEnable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_A(attr, CoarseEdgeThr, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, CoarseStr, 2560, 2560, 2560, 2560, 2560, 2560, 2560, 2560, 2560,/*8*/
	2560, 2560, 2560, 2560, 2560, 2560, 2560, 2560
	);
	INIT_A(attr, FineEdgeThr, 128,
	256, 256, 256, 256, 256, 256, 256, 256, /*8*/256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, FineStr, 32,
	128, 128, 128, 128, 128, 128, 128, 128, /*8*/128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, RbSigLumaThd, 320,
	320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320, 320
	);
	INIT_A(attr, FilterMode,  0,
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

	isp_demosaic_ctrl_set_demosaic_attr(ViPipe, &attr);
}

static CVI_VOID setDemosaicDemoire(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_DEMOSAIC_DEMOIRE_ATTR_S attr = {0};

	INIT_V(attr, AntiFalseColorEnable, 0);
	INIT_V(attr, ProtectColorEnable, 1);
	INIT_V(attr, DetailDetectLumaEnable, 1);
	INIT_V(attr, DetailSmoothEnable, 1);
	INIT_V(attr, DetailMode, 1);

	INIT_A(attr, AntiFalseColorStr, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, SatGainIn[0], 200,
	200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200
	);
	INIT_A(attr, SatGainIn[1], 800,
	800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800, 800
	);
	INIT_A(attr, SatGainOut[0], 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
	);
	INIT_A(attr, SatGainOut[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, ProtectColorGainIn[0], 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 00, 20, 20, 20
	);
	INIT_A(attr, ProtectColorGainIn[1], 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
	);
	INIT_A(attr, ProtectColorGainOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, ProtectColorGainOut[1], 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
	);
	INIT_A(attr, UserDefineProtectColor1, 960,
	960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960
	);
	INIT_A(attr, UserDefineProtectColor2, 560,
	560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560, 560
	);
	INIT_A(attr, UserDefineProtectColor3, 960,
	960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960, 960
	);
	INIT_A(attr, EdgeGainIn[0], 150,
	150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150
	);
	INIT_A(attr, EdgeGainIn[1], 200,
	200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200
	);
	INIT_A(attr, EdgeGainOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, EdgeGainOut[1], 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
	);
	INIT_A(attr, DetailGainIn[0], 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
	);
	INIT_A(attr, DetailGainIn[1], 150,
	150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150
	);
	INIT_A(attr, DetailGaintOut[0], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, DetailGaintOut[1], 4095,
	4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095
	);
	INIT_A(attr, DetailDetectLumaStr, 480,
	480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480, 480
	);
	INIT_A(attr, DetailSmoothStr, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, DetailWgtThr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, DetailWgtMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, DetailWgtMax, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, DetailWgtSlope, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, EdgeWgtNp, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, EdgeWgtThr, 160,
	160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160
	);
	INIT_A(attr, EdgeWgtMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, EdgeWgtMax, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, EdgeWgtSlope, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, DetailSmoothMapTh, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, DetailSmoothMapMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, DetailSmoothMapMax, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, DetailSmoothMapSlope, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LumaWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
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

	isp_demosaic_ctrl_set_demosaic_demoire_attr(ViPipe, &attr);
}

static CVI_VOID setRgbCacDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_RGBCAC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, PurpleDetRange0, 64);
	INIT_V(attr, PurpleDetRange1, 96);
	INIT_V(attr, DePurpleStrMax0, 232);
	INIT_V(attr, DePurpleStrMin0, 0);
	INIT_V(attr, DePurpleStrMax1, 232);
	INIT_V(attr, DePurpleStrMin1, 0);
	INIT_V(attr, EdgeGlobalGain, 64);
	INIT_V_ARRAY(attr, EdgeGainIn, 1, 2, 12);
	INIT_V_ARRAY(attr, EdgeGainOut, 0, 16, 32);
	INIT_V(attr, LumaScale, 16);
	INIT_V(attr, UserDefineLuma, 896);
	INIT_V(attr, LumaBlendWgt, 0);
	INIT_V(attr, LumaBlendWgt2, 32);
	INIT_V(attr, LumaBlendWgt3, 0);
	INIT_V(attr, PurpleCb, 3712);
	INIT_V(attr, PurpleCr, 2816);
	INIT_V(attr, PurpleCb2, 3712);
	INIT_V(attr, PurpleCr2, 2816);
	INIT_V(attr, PurpleCb3, 3712);
	INIT_V(attr, PurpleCr3, 2816);
	INIT_V(attr, GreenCb, 688);
	INIT_V(attr, GreenCr, 336);
	INIT_V(attr, TuningMode, 0);

	INIT_A(attr, DePurpleStr0, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, DePurpleStr1, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, EdgeCoring, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, DePurpleCrStr0, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, DePurpleCbStr0, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, DePurpleCrStr1, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, DePurpleCbStr1, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
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

	isp_rgbcac_ctrl_set_rgbcac_attr(ViPipe, &attr);
}

static CVI_VOID setLCacDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_LCAC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, DePurpleCrStr0, 32);
	INIT_V(attr, DePurpleCbStr0, 32);
	INIT_V(attr, DePurpleCrStr1, 32);
	INIT_V(attr, DePurpleCbStr1, 32);
	INIT_V(attr, FilterTypeBase, 0);
	INIT_V(attr, EdgeGainBase0, 0);
	INIT_V(attr, EdgeGainBase1, 0);
	INIT_V(attr, EdgeStrWgtBase, 8);
	INIT_V(attr, DePurpleStrMaxBase, 128);
	INIT_V(attr, DePurpleStrMinBase, 0);
	INIT_V(attr, FilterScaleAdv, 8);
	INIT_V(attr, LumaWgt, 32);
	INIT_V(attr, FilterTypeAdv, 0);
	INIT_V(attr, EdgeGainAdv0, 0);
	INIT_V(attr, EdgeGainAdv1, 0);
	INIT_V(attr, EdgeStrWgtAdvG, 8);
	INIT_V(attr, DePurpleStrMaxAdv, 128);
	INIT_V(attr, DePurpleStrMinAdv, 0);
	INIT_V(attr, EdgeWgtBase.Wgt, 96);
	INIT_V(attr, EdgeWgtBase.Sigma, 76);
	INIT_V(attr, EdgeWgtAdv.Wgt, 80);
	INIT_V(attr, EdgeWgtAdv.Sigma, 64);

	INIT_A(attr, DePurpleCrGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, DePurpleCbGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, DePurepleCrWgt0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, DePurepleCbWgt0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, DePurepleCrWgt1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, DePurepleCbWgt1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, EdgeCoringBase, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, EdgeCoringAdv, 0,
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

	isp_lcac_ctrl_set_lcac_attr(ViPipe, &attr);
}

static CVI_VOID setBnrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setBnr(ViPipe);
	setBnrFilter(ViPipe);
}

static CVI_VOID setBnr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_NR_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, CoringParamEnable, 0);
	INIT_A(attr, WindowType, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
	);
	INIT_A(attr, DetailSmoothMode, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, NoiseSuppressStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 16, 16, 16, 16
	);
	INIT_A(attr, FilterType, 0,
	0, 1, 2, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64, 64, 64, 64
	);
	INIT_A(attr, NoiseSuppressStrMode, 0,
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

	isp_bnr_ctrl_set_bnr_attr(ViPipe, &attr);
}

static CVI_VOID setBnrFilter(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_NR_FILTER_ATTR_S attr = {0};

	INIT_V(attr, TuningMode, 8);
	INIT_A(attr, LumaStr[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[2], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[3], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[4], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[5], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[6], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaStr[7], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, VarThr, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, CoringWgtLF, 0,
	128, 115, 102, 90, 77, 64, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, CoringWgtHF, 0,
	128, 115, 102, 90, 77, 64, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, NonDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, VhDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, AaDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, NpSlopeR, 1023,
	28, 30, 38, 45, 45, 63, 91, 128, 187, 287, 423, 525, 632, 879, 1023, 1023
	);
	INIT_A(attr, NpSlopeGr, 1023,
	14, 15, 16, 19, 17, 20, 28, 44, 70, 98, 124, 188, 224, 308, 616, 1023
	);
	INIT_A(attr, NpSlopeGb, 1023,
	14, 15, 16, 19, 17, 20, 28, 44, 70, 98, 124, 188, 224, 308, 616, 1023
	);
	INIT_A(attr, NpSlopeB, 1023,
	28, 30, 38, 45, 45, 63, 91, 128, 187, 287, 423, 525, 632, 879, 1023, 1023
	);
	INIT_A(attr, NpLumaThrR, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, NpLumaThrGr, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, NpLumaThrGb, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, NpLumaThrB, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, NpLowOffsetR, 0,
	5, 12, 16, 25, 53, 76, 111, 166, 250, 348, 508, 792, 1023, 1023, 1023, 1023
	);
	INIT_A(attr, NpLowOffsetGr, 0,
	2, 6, 10, 18, 43, 71, 97, 123, 163, 243, 393, 525, 760, 1023, 1023, 1023
	);
	INIT_A(attr, NpLowOffsetGb, 0,
	2, 6, 10, 18, 43, 71, 97, 123, 163, 243, 393, 525, 760, 1023, 1023, 1023
	);
	INIT_A(attr, NpLowOffsetB, 0,
	5, 12, 16, 25, 53, 76, 111, 166, 250, 348, 508, 792, 1023, 1023, 1023, 1023
	);
	INIT_A(attr, NpHighOffsetR, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
	);
	INIT_A(attr, NpHighOffsetGr, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
	);
	INIT_A(attr, NpHighOffsetGb, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
	);
	INIT_A(attr, NpHighOffsetB, 1023,
	1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023
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

	isp_bnr_ctrl_set_bnr_filter_attr(ViPipe, &attr);
}

CVI_VOID setYnrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setYnr(ViPipe);
	setYnrMotionNr(ViPipe);
	setYnrFilter(ViPipe);
}

static CVI_VOID setYnr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_YNR_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, CoringParamEnable, 1);
	INIT_V(attr, FiltModeEnable, 0);
	INIT_V(attr, FiltMode, 128);
	INIT_V(attr, TuningMode, 8);
	INIT_A(attr, WindowType, 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
	);
	INIT_A(attr, DetailSmoothMode, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, NoiseSuppressStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, FilterType, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, NoiseCoringMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, NoiseCoringBase, 0,
	0, 6, 12, 18, 24, 30, 45, 60, 70, 80, 110, 140, 180, 180, 180, 180
	);
	INIT_A(attr, NoiseCoringAdv, 0,
	0, 2, 4, 6, 8, 10, 15, 20, 23, 25, 25, 25, 25, 25, 25, 25
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

	isp_ynr_ctrl_set_ynr_attr(ViPipe, &attr);
}

static CVI_VOID setYnrMotionNr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_YNR_MOTION_NR_ATTR_S attr = {0};

	INIT_A(attr, MotionCoringWgtMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	// MotionYnrLut setting.
	{
		int paramManual[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param6400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param12800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param25600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param51200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param102400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param204800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param409600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param819200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1638400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3276800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

		for (int idx = 0; idx < 16; idx++) {
			INIT_A(attr, MotionYnrLut[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}
	// MotionCoringWgt setting.
	{
		int paramManual[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param6400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param12800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param25600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param51200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param102400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param204800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param409600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param819200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1638400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3276800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

		for (int idx = 0; idx < 16; idx++) {
			INIT_A(attr, MotionCoringWgt[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}
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

	isp_ynr_ctrl_set_ynr_motion_attr(ViPipe, &attr);
}

static CVI_VOID setYnrFilter(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_YNR_FILTER_ATTR_S attr = {0};

	INIT_A(attr, VarThr, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, CoringWgtLF, 0,
	0, 0, 0, 0, 0, 0, 0, 0, /*8*/0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, CoringWgtHF, 0,
	0, 0, 0, 0, 0, 0, 0, 0, /*8*/0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, NonDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, VhDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, AaDirFiltStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, CoringWgtMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, FilterMode, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
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

	isp_ynr_ctrl_set_ynr_filter_attr(ViPipe, &attr);
}

CVI_VOID setCnrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setCnr(ViPipe);
	setCnrMotion(ViPipe);
}

static CVI_VOID setCnr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CNR_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_A(attr, CnrStr, 0,
	16, 26, 35, 45, 54, 64, 80, 96, 112, 128, 160, 192, 255, 255, 255, 255
	);
	INIT_A(attr, NoiseSuppressStr, 0,
	0, 0, 0, 0, 0, 0, 64, 128, 160, 192, 208, 224, 255, 255, 255, 255
	);
	INIT_A(attr, NoiseSuppressGain,  4,
	6, 5, 4, 4, 3, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, FilterType, 0,
	0, 2, 3, 5, 6, 8, 9, 10, 12, 13, 15, 15, 15, 15, 15, 15
	);
	INIT_A(attr, MotionNrStr, 0,
	32, 45, 58, 70, 83, 96, 112, 128, 144, 160, 192, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LumaWgt, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, DetailSmoothMode, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
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

	CVI_ISP_SetCNRAttr(ViPipe, &attr);
}

static CVI_VOID setCnrMotion(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CNR_MOTION_NR_ATTR_S attr = {0};

	INIT_V(attr, MotionCnrEnable, 1);

	// MotionCnrCoringLut setting.
	{
		int paramManual[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param6400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param12800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param25600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param51200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param102400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param204800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param409600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param819200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1638400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3276800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

		for (int idx = 0; idx < 16; idx++) {
			INIT_A(attr, MotionCnrCoringLut[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}

	// MotionCoringWgt setting.
	{
		int paramManual[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param100[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param6400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param12800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param25600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param51200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param102400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param204800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param409600[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param819200[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param1638400[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		int param3276800[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

		for (int idx = 0; idx < 16; idx++) {
			INIT_A(attr, MotionCnrStrLut[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}

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

	isp_cnr_ctrl_set_cnr_motion_attr(ViPipe, &attr);
}

static CVI_VOID setTnrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setTnr(ViPipe);
	setTnrNoiseModel(ViPipe);
	setTnrLumaMotion(ViPipe);
	setTnrGhost(ViPipe);
	setTnrMtPrt(ViPipe);
	setTnrMotionAdapt(ViPipe);
}

static CVI_VOID setTnr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_A(attr, TnrStrength0, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, MapThdLow0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MapThdHigh0, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, MtDetectUnit, 3,
	3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5
	);
	INIT_A(attr, BrightnessNoiseLevelLE, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, BrightnessNoiseLevelSE, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, MtFiltMode, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, MtFiltWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);

	INIT_V(attr, TnrMtMode, 0);
	INIT_V(attr, YnrCnrSharpenMtMode, 1);
	INIT_V(attr, PreSharpenMtMode, 0);
	INIT_V(attr, ChromaScalingDownMode, 0);
	INIT_V(attr, CompGainEnable, 0);

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

	isp_tnr_ctrl_set_tnr_attr(ViPipe, &attr);
}

static CVI_VOID setTnrNoiseModel(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_NOISE_MODEL_ATTR_S attr = {0};

	INIT_A(attr, RNoiseLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, GNoiseLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, BNoiseLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, RNoiseLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, GNoiseLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, BNoiseLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, RNoiseHiLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, GNoiseHiLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, BNoiseHiLevel0, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, RNoiseHiLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, GNoiseHiLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, BNoiseHiLevel1, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
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

	isp_tnr_ctrl_set_tnr_noise_model_attr(ViPipe, &attr);
}

static CVI_VOID setTnrLumaMotion(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_LUMA_MOTION_ATTR_S attr = {0};

	INIT_A(attr, L2mIn0[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, L2mIn0[1], 600,
	600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600
	);
	INIT_A(attr, L2mIn0[2], 1500,
	1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500
	);
	INIT_A(attr, L2mIn0[3], 2500,
	2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500
	);
	INIT_A(attr, L2mOut0[0], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut0[1], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut0[2], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut0[3], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mIn1[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, L2mIn1[1], 600,
	600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600, 600
	);
	INIT_A(attr, L2mIn1[2], 1500,
	1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500, 1500
	);
	INIT_A(attr, L2mIn1[3], 2500,
	2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500
	);
	INIT_A(attr, L2mOut1[0], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut1[1], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut1[2], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, L2mOut1[3], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, MtLumaMode, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
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

	isp_tnr_ctrl_set_tnr_luma_motion_attr(ViPipe, &attr);
}

static CVI_VOID setTnrGhost(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_GHOST_ATTR_S attr = {0};

	INIT_A(attr, MotionHistoryStr, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
	);
	INIT_A(attr, PrvMotion0[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, PrvMotion0[1], 45,
	45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45
	);
	INIT_A(attr, PrvMotion0[2], 90,
	90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90
	);
	INIT_A(attr, PrvMotion0[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, PrtctWgt0[0], 8,
	12, 12, 12, 12, 13, 14, 14, 14, /*8*/14, 14, 14, 14, 14, 14, 14, 14
	);
	INIT_A(attr, PrtctWgt0[1], 8,
	12, 12, 12, 12, 13, 14, 14, 14, /*8*/14, 14, 14, 14, 14, 14, 14, 14
	);
	INIT_A(attr, PrtctWgt0[2], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, PrtctWgt0[3], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
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

	isp_tnr_ctrl_set_tnr_ghost_attr(ViPipe, &attr);
}

static CVI_VOID setTnrMtPrt(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_MT_PRT_ATTR_S attr = {0};

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(attr, LowMtPrtEn, 1);
	INIT_V(attr, LowMtLowPassEnable, 0);
	INIT_V(attr, LowMtPrtAdvLumaEnable, 0);
	INIT_V(attr, LowMtPrtAdvMode, 0);
	INIT_V(attr, LowMtPrtAdvMax, 255);
	INIT_V(attr, LowMtPrtAdvDebugMode, 0);

	INIT_V(attr, LowMtPrtAdvDebugIn[0], 8);
	INIT_V(attr, LowMtPrtAdvDebugIn[1], 11);
	INIT_V(attr, LowMtPrtAdvDebugIn[2], 192);
	INIT_V(attr, LowMtPrtAdvDebugIn[3], 208);
	INIT_V(attr, LowMtPrtAdvDebugOut[0], 16);
	INIT_V(attr, LowMtPrtAdvDebugOut[1], 16);
	INIT_V(attr, LowMtPrtAdvDebugOut[2], 16);
	INIT_V(attr, LowMtPrtAdvDebugOut[3], 16);

	INIT_A(attr, LowMtPrtLevelY, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtLevelU, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtLevelV, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtInY[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtInY[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtInY[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtInY[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtOutY[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtOutY[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtOutY[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtOutY[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtInU[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtInU[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtInU[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtInU[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtOutU[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtOutU[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtOutU[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtOutU[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtInV[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtInV[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtInV[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtInV[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, LowMtPrtOutV[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LowMtPrtOutV[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LowMtPrtOutV[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LowMtPrtOutV[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);

	INIT_A(attr, LowMtPrtAdvIn[0], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, LowMtPrtAdvIn[1], 11,
	11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11
	);
	INIT_A(attr, LowMtPrtAdvIn[2], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, LowMtPrtAdvIn[3], 208,
	208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208, 208
	);
	INIT_A(attr, LowMtPrtAdvOut[0], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, LowMtPrtAdvOut[1], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, LowMtPrtAdvOut[2], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, LowMtPrtAdvOut[3], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	isp_tnr_ctrl_set_tnr_mt_prt_attr(ViPipe, &attr);
}

static CVI_VOID setTnrMotionAdapt(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_TNR_MOTION_ADAPT_ATTR_S attr = {0};

	INIT_A(attr, AdaptNrLumaStrIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, AdaptNrLumaStrIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, AdaptNrLumaStrIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, AdaptNrLumaStrIn[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, AdaptNrLumaStrOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, AdaptNrLumaStrOut[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, AdaptNrLumaStrOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, AdaptNrLumaStrOut[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, AdaptNrChromaStrIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, AdaptNrChromaStrIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, AdaptNrChromaStrIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, AdaptNrChromaStrIn[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, AdaptNrChromaStrOut[0], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, AdaptNrChromaStrOut[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, AdaptNrChromaStrOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, AdaptNrChromaStrOut[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
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

	isp_tnr_ctrl_set_tnr_motion_adapt_attr(ViPipe, &attr);
}
static CVI_VOID setDpcDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setDpcDynamic(ViPipe);
	setDpcCalibration(ViPipe);
	setDpcStatic(ViPipe);
}

static CVI_VOID setDpcDynamic(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);
#ifndef ARCH_RTOS_CV181X
	ISP_DP_DYNAMIC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, DynamicDPCEnable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_A(attr, ClusterSize, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, BrightDefectToNormalPixRatio, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, DarkDefectToNormalPixRatio, 160,
	160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160, 160
	);
	INIT_A(attr, FlatThreR, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, FlatThreG, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, FlatThreB, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, FlatThreMinG, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, FlatThreMinRB, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
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
	isp_dpc_ctrl_set_dpc_dynamic_attr(ViPipe, &attr);
#endif
}

static CVI_VOID setDpcCalibration(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);
#ifndef ARCH_RTOS_CV181X
	ISP_DP_CALIB_ATTR_S *pattr = NULL;

	pattr = ISP_CALLOC(1, sizeof(ISP_DP_CALIB_ATTR_S));
	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "calloc fail");
		return;
	}

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(*pattr, EnableDetect, 0);
	INIT_V(*pattr, StaticDPType, 0);
	INIT_V(*pattr, StartThresh, 3);
	INIT_V(*pattr, CountMax, 1024);
	INIT_V(*pattr, CountMin, 1);
	INIT_V(*pattr, TimeLimit, 1600);
	INIT_V(*pattr, saveFileEn, 0);
	for (int i = 0 ; i < STATIC_DP_COUNT_MAX ; i++)
		INIT_V(*pattr, Table[i], 0);
	INIT_V(*pattr, FinishThresh, 3);
	INIT_V(*pattr, Count, 0);
	INIT_V(*pattr, Status, 0);
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	CVI_ISP_SetDPCalibrate(ViPipe, pattr);
	free(pattr);
#endif
}

static CVI_VOID setDpcStatic(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	#if 0
	ISP_DP_STATIC_ATTR_S attr = {0};

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(attr, Enable, 0);
	INIT_V(attr, BrightCount, 0);
	INIT_V(attr, DarkCount, 0);
	INIT_V(attr, BrightTable[4095], 0);
	INIT_V(attr, DarkTable[4095], 0);
	INIT_V(attr, Show, 0);
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	isp_dpc_ctrl_set_dpc_static_attr(ViPipe, &attr);
	#endif

	UNUSED(ViPipe);
}

CVI_VOID setCrossTalkDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CROSSTALK_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V_ARRAY(attr, GrGbDiffThreSec, 128, 192, 224, 256);
	INIT_V_ARRAY(attr, FlatThre, 128, 192, 224, 256);
	INIT_A(attr, Strength, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
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

	isp_crosstalk_ctrl_set_crosstalk_attr(ViPipe, &attr);
}

static CVI_VOID setMlscDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	setMeshShading(ViPipe);
	setMeshShadingGainLut(ViPipe);
}

static CVI_VOID setMeshShading(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_MESH_SHADING_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, OverflowProtection, 0);
	INIT_A(attr, MeshStr, 4095,
	3686, 3195, 2703, 2212, 1720, 1229, 615, 0, 0, 0, 0, 0, 0, 0, 0, 0
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

	isp_mlsc_ctrl_set_mlsc_attr(ViPipe, &attr);
}

static CVI_VOID setMeshShadingGainLut(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pattr = NULL;

	pattr = ISP_CALLOC(1, sizeof(ISP_MESH_SHADING_GAIN_LUT_ATTR_S));
	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "calloc fail");
		return;
	}

	for (CVI_U32 ct = 0; ct < ISP_MLSC_COLOR_TEMPERATURE_SIZE; ct++) {
		for (CVI_U32 point = 0; point < CVI_ISP_LSC_GRID_POINTS; point++) {
			pattr->LscGainLut[ct].RGain[point] = 512;
			pattr->LscGainLut[ct].GGain[point] = 512;
			pattr->LscGainLut[ct].BGain[point] = 512;
		}
	}

	INIT_V(*pattr, Size, 3);
	CVI_U16 au16ColorTemp[ISP_MLSC_COLOR_TEMPERATURE_SIZE] = {2300, 2850, 3000, 3900, 4200, 5000, 6500};

	for (CVI_U32 idx = 0; idx < ISP_MLSC_COLOR_TEMPERATURE_SIZE; ++idx) {
		pattr->LscGainLut[idx].ColorTemperature = au16ColorTemp[idx];
	}

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

	isp_mlsc_ctrl_set_mlsc_lut_attr(ViPipe, pattr);
	free(pattr);
}

CVI_VOID setFsWdrDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_FSWDR_ATTR_S attr = {0};

	INIT_V(attr, Enable, wdrEn);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, MotionCompEnable, 0);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, WDRDCMode, 0);
	INIT_V(attr, WDRLumaMode, 0);
	INIT_LV_A(attr, WDRCombineLongThr, 3300,
	3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300,
	3300, 3300, 3300, 3300, 3300
	);
	INIT_LV_A(attr, WDRCombineShortThr, 3900,
	3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900,
	3900, 3900, 3900, 3900, 3900
	);
	INIT_LV_A(attr, WDRCombineMaxWeight, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_LV_A(attr, WDRCombineMinWeight, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_LV_A(attr, WDRMtIn[0], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_LV_A(attr, WDRMtIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_LV_A(attr, WDRMtIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_LV_A(attr, WDRMtIn[3], 240,
	240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240
	);
	INIT_LV_A(attr, WDRMtOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_LV_A(attr, WDRMtOut[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_LV_A(attr, WDRMtOut[2], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_LV_A(attr, WDRMtOut[3], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_V(attr, WDRType, 1);
	INIT_LV_A(attr, WDRLongWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_V(attr, WDRCombineSNRAwareEn, 0);
	INIT_V(attr, WDRCombineSNRAwareLowThr, 1024);
	INIT_V(attr, WDRCombineSNRAwareHighThr, 8192);
	INIT_LV_A(attr, WDRCombineSNRAwareToleranceLevel, 128,
	128, 128, 128, 128, 128, 144, 160, 176, 192, 224, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_V(attr, WDRCombineSNRAwareSmoothLevel, 30);
	INIT_V(attr, LocalToneRefinedDCMode, 0);
	INIT_V(attr, LocalToneRefinedLumaMode, 0);
	INIT_V(attr, DarkToneRefinedThrL, 2000);
	INIT_V(attr, DarkToneRefinedThrH, 3900);
	INIT_V(attr, DarkToneRefinedMaxWeight, 256);
	INIT_V(attr, DarkToneRefinedMinWeight, 128);
	INIT_V(attr, BrightToneRefinedThrL, 2000);
	INIT_V(attr, BrightToneRefinedThrH, 3900);
	INIT_V(attr, BrightToneRefinedMaxWeight, 128);
	INIT_V(attr, BrightToneRefinedMinWeight, 256);
	INIT_V(attr, WDRMotionFusionMode, 0);
	INIT_LV_A(attr, MergeModeAlpha, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_V(attr, MtMode, 0);
	INIT_LV_A(attr, WDRMotionCombineLongThr, 3300,
	3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300, 3300,
	3300, 3300, 3300, 3300, 3300
	);
	INIT_LV_A(attr, WDRMotionCombineShortThr, 3900,
	3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900, 3900,
	3900, 3900, 3900, 3900, 3900
	);
	INIT_LV_A(attr, WDRMotionCombineMaxWeight, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_LV_A(attr, WDRMotionCombineMinWeight, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
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

	isp_fswdr_ctrl_set_fswdr_attr(ViPipe, &attr);
}

CVI_VOID setDrcDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_DRC_ATTR_S *pattr = NULL;

	pattr = ISP_CALLOC(1, sizeof(ISP_DRC_ATTR_S));
	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "calloc fail");
		return;
	}

	INIT_V(*pattr, Enable, wdrEn);
	INIT_V(*pattr, enOpType, 0);
	INIT_V(*pattr, UpdateInterval, 1);
	INIT_V(*pattr, TuningMode, 0);
	INIT_V(*pattr, LocalToneEn, 0);
	INIT_V(*pattr, LocalToneRefineEn, 0);

	INIT_LV_A(*pattr, TargetYScale, 224,
	224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224
	);
	INIT_LV_A(*pattr, HdrStrength, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_LV_A(*pattr, DEAdaptPercentile, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
	);
	INIT_LV_A(*pattr, DEAdaptTargetGain, 40,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
	);
	INIT_LV_A(*pattr, DEAdaptGainUB, 96,
	96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96
	);
	INIT_LV_A(*pattr, DEAdaptGainLB, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_LV_A(*pattr, BritInflectPtLuma, 40,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
	);
	INIT_LV_A(*pattr, BritContrastLow, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
	);
	INIT_LV_A(*pattr, BritContrastHigh, 80,
	80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
	);
	INIT_V(*pattr, ToneCurveSelect, 1);

#define MAX_CURVE_USER_DEFINE		(65535)
	for (CVI_U32 u32Idx = 0, u32Value = 0; u32Idx < DRC_GLOBAL_USER_DEFINE_NUM; ++u32Idx, u32Value += 4) {
		u32Value = MIN(u32Value, MAX_CURVE_USER_DEFINE);
		INIT_V(*pattr, CurveUserDefine[u32Idx], u32Value);
	}
#undef MAX_CURVE_USER_DEFINE
	INIT_V(*pattr, ToneCurveSmooth, 300);
	INIT_V(*pattr, CoarseFltScale, 6);
	INIT_V(*pattr, SdrTargetYGainMode, 0);
	INIT_LV_A(*pattr, SdrTargetY, 56,
	56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56, 56
	);
	INIT_LV_A(*pattr, SdrTargetYGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_LV_A(*pattr, SdrGlobalToneStr, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_LV_A(*pattr, SdrDEAdaptPercentile, 10,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10
	);
	INIT_LV_A(*pattr, SdrDEAdaptTargetGain, 40,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
	);
	INIT_LV_A(*pattr, SdrDEAdaptGainUB, 96,
	96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96
	);
	INIT_LV_A(*pattr, SdrDEAdaptGainLB, 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_LV_A(*pattr, SdrBritInflectPtLuma, 40,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
	);
	INIT_LV_A(*pattr, SdrBritContrastLow, 75,
	75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75
	);
	INIT_LV_A(*pattr, SdrBritContrastHigh, 80,
	80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
	);
	INIT_V(*pattr, DetailEnhanceEn, 0);
	INIT_A(*pattr, TotalGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	for (CVI_U32 i = 0 ; i < 33 ; i++)
		INIT_V(*pattr, LumaGain[i], 64);
	INIT_V_ARRAY(*pattr, DetailEnhanceMtIn, 0, 64, 128, 192);
	INIT_V_ARRAY(*pattr, DetailEnhanceMtOut, 128, 128, 128, 128);
	INIT_V(*pattr, OverShootThd, 32);
	INIT_V(*pattr, UnderShootThd, 32);
	INIT_V(*pattr, OverShootGain, 4);
	INIT_V(*pattr, UnderShootGain, 4);
	INIT_V(*pattr, OverShootThdMax, 255);
	INIT_V(*pattr, UnderShootThdMin, 255);
	INIT_V(*pattr, SoftClampEnable, 0);
	INIT_V(*pattr, SoftClampUB, 1);
	INIT_V(*pattr, SoftClampLB, 1);
	INIT_V(*pattr, dbg_182x_sim_enable, 0);
	INIT_V(*pattr, DarkMapStr, 45);
	INIT_V(*pattr, BritMapStr, 55);
	INIT_V(*pattr, SdrDarkMapStr, 45);
	INIT_V(*pattr, SdrBritMapStr, 55);
	INIT_V_ARRAY(*pattr, DRCMu, 256, 0, 60, 1, 1, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
			/*16*/0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 18, 0, 0, 0, 0);

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

	CVI_ISP_SetDRCAttr(ViPipe, pattr);
	free(pattr);
}

CVI_VOID setExposureAttrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_EXPOSURE_ATTR_S attr = {0};

	CVI_ISP_GetExposureAttr(ViPipe, &attr);

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(attr, bByPass, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, u8AERunInterval, 1);
	INIT_V(attr, bAERouteExValid, 0);
	INIT_V(attr, enMeterMode, 0);
	INIT_V(attr, u8DebugMode, 0);
	INIT_V(attr, bAEGainSepCfg, 0);

	INIT_V(attr.stManual, enExpTimeOpType, 0);
	INIT_V(attr.stManual, enAGainOpType, 0);
	INIT_V(attr.stManual, enDGainOpType, 0);
	INIT_V(attr.stManual, enISPDGainOpType, 0);
	INIT_V(attr.stManual, enISONumOpType, 0);
	INIT_V(attr.stManual, u32ExpTime, 16384);
	INIT_V(attr.stManual, u32AGain, 1024);
	INIT_V(attr.stManual, u32DGain, 1024);
	INIT_V(attr.stManual, u32ISPDGain, 1024);
	INIT_V(attr.stManual, enGainType, 0);
	INIT_V(attr.stManual, u32ISONum, 100);

	INIT_V(attr.stAuto, stExpTimeRange.u32Min, 10);
	INIT_V(attr.stAuto, stExpTimeRange.u32Max, 100000);
	INIT_V(attr.stAuto, stAGainRange.u32Max, 204800);
	INIT_V(attr.stAuto, stDGainRange.u32Max, 204800);
	INIT_V(attr.stAuto, stISONumRange.u32Max, 128000000);
	INIT_V(attr.stAuto, stSysGainRange.u32Max, 1310720000);
	INIT_V(attr.stAuto, u32GainThreshold, 1310720000);

	INIT_V(attr.stAuto, enGainType, 0);
	INIT_V(attr.stAuto, stISONumRange.u32Min, 100);
	INIT_V(attr.stAuto, stAGainRange.u32Min, 1024);
	INIT_V(attr.stAuto, stDGainRange.u32Min, 1024);
	INIT_V(attr.stAuto, stISPDGainRange.u32Max, 32767);
	INIT_V(attr.stAuto, stISPDGainRange.u32Min, 1024);
	INIT_V(attr.stAuto, stSysGainRange.u32Min, 1024);
	INIT_V(attr.stAuto, u8Speed, 64);
	INIT_V(attr.stAuto, u16BlackSpeedBias, 144);
	INIT_V(attr.stAuto, u8Tolerance, 2);
	INIT_V(attr.stAuto, u8Compensation, 56);
	INIT_V(attr.stAuto, u16EVBias, 1024);
	INIT_V(attr.stAuto, enAEStrategyMode, 0);
	INIT_V(attr.stAuto, u16HistRatioSlope, 128);
	INIT_V(attr.stAuto, u8MaxHistOffset, 0);
	INIT_V(attr.stAuto, enAEMode, 1);
	INIT_V(attr.stAuto, stAntiflicker.bEnable, 0);
	INIT_V(attr.stAuto, stAntiflicker.enMode, 1);
	INIT_V(attr.stAuto, stAntiflicker.enFrequency, 0);
	INIT_V(attr.stAuto, stSubflicker.bEnable, 0);
	INIT_V(attr.stAuto, stSubflicker.u8LumaDiff, 50);
	INIT_V(attr.stAuto, bManualExpValue, 0);
	INIT_V(attr.stAuto, u32ExpValue, 0);
	INIT_V_ARRAY(attr.stAuto, au8AdjustTargetMin, 40, 40, 40, 40,
	40, 40,	40, 40, 40, 40, 40, 45, 50, 50, 50, 50, 50, 50, 60,
	60, 60
	);
	INIT_V_ARRAY(attr.stAuto, au8AdjustTargetMax, 50, 50, 50, 50,
	50, 50, 50, 50, 50, 50, 50, 60, 60, 60, 60, 60, 60, 60, 70,
	70, 70
	);
	INIT_V(attr.stAuto, u16LowBinThr, 2);
	INIT_V(attr.stAuto, u16HighBinThr, 256);
	INIT_V(attr.stAuto, s16IRCutOnLv, 0);
	INIT_V(attr.stAuto, s16IRCutOffLv, 700);
	INIT_V(attr.stAuto, enIRCutStatus, 0);

	INIT_V(attr.stAuto, bEnableFaceAE, 0);
	INIT_V(attr.stAuto, u8FaceTargetLuma, 46);
	INIT_V(attr.stAuto, u8FaceWeight, 80);

	INIT_V(attr.stAuto, u8GridBvWeight, 0);

	INIT_V(attr.stAuto, u8HighLightLumaThr, 224);
	INIT_V(attr.stAuto, u8HighLightBufLumaThr, 176);
	INIT_V(attr.stAuto, u8LowLightLumaThr, 16);
	INIT_V(attr.stAuto, u8LowLightBufLumaThr, 48);
	INIT_V(attr.stAuto, bHistogramAssist, 0);

	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	CVI_ISP_SetExposureAttr(ViPipe, &attr);
}

CVI_VOID setWdrExposureAttrDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_WDR_EXPOSURE_ATTR_S attr = {0};

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(attr, enExpRatioType, 0);	// TODO@Kidd check "Op" with lenny, vincent
	INIT_V_ARRAY(attr, au32ExpRatio, 64, 0, 0);
	INIT_V(attr, u32ExpRatioMax, 2048);
	INIT_V(attr, u32ExpRatioMin, 256);
	INIT_V(attr, u16Tolerance, 2);
	INIT_V(attr, u16Speed, 32);
	INIT_V(attr, u16RatioBias, 1024);
	INIT_V(attr, u8SECompensation, 56);
	INIT_V(attr, u16SEHisThr, 64);
	INIT_V(attr, u16SEHisCntRatio1, 20);
	INIT_V(attr, u16SEHisCntRatio2, 10);
	INIT_V(attr, u16SEHis255CntThr1, 20000);
	INIT_V(attr, u16SEHis255CntThr2, 0);
	INIT_V_ARRAY(attr, au8LEAdjustTargetMin,
	20, 20, 20, 25, 35, 40, 40, 40, 40, 40, 40, 45, 50, 70, 80, 90, 100, 110, 120, 135, 150
	);
	INIT_V_ARRAY(attr, au8LEAdjustTargetMax,
	20, 20, 20, 25, 35, 40, 40, 40, 40, 40, 40, 45, 50, 70, 80, 90, 100, 110, 120, 135, 150
	);
	INIT_V_ARRAY(attr, au8SEAdjustTargetMin,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 15, 15, 20, 20, 20, 20, 20, 20, 25, 30
	);
	INIT_V_ARRAY(attr, au8SEAdjustTargetMax,
	10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 15, 15, 20, 20, 20, 20, 20, 20, 25, 30
	);
	INIT_V_ARRAY(attr, au8FrameAvgLumaMin,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_V_ARRAY(attr, au8FrameAvgLumaMax,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_V(attr, u8AdjustTargetDetectFrmNum, 2);
	INIT_V(attr, u32DiffPixelNum, 100000);
	INIT_V(attr, u16LELowBinThr, 0);
	INIT_V(attr, u16LEHighBinThr, 256);
	INIT_V(attr, u16SELowBinThr, 0);
	INIT_V(attr, u16SEHighBinThr, 256);
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	CVI_ISP_SetWDRExposureAttr(ViPipe, &attr);
}

CVI_VOID setDciDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_DCI_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, Method, 0);
	INIT_V(attr, Speed, 0);
	INIT_V(attr, DciStrength, 0);
	INIT_V(attr, DciGamma, 100);
	INIT_V(attr, DciOffset, 0);
	INIT_V(attr, ToleranceY, 4);
	INIT_V(attr, Sensitivity, 255);
	INIT_A(attr, ContrastGain, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, BlcThr, 60,
	60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60
	);
	INIT_A(attr, WhtThr, 200,
	200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200, 200
	);
	INIT_A(attr, BlcCtrl, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, WhtCtrl, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, DciGainMax, 48,
	48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48, 48
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

	CVI_ISP_SetDCIAttr(ViPipe, &attr);
}

CVI_VOID setLdciDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_LDCI_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, GaussLPFSigma, 64);

	INIT_A(attr, LdciStrength, 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, LdciRange, 256,
	256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256, 256
	);
	INIT_A(attr, TprCoef, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30
	);
	INIT_A(attr, EdgeCoring, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaWgtMax, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LumaWgtMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, VarMapMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, VarMapMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, UvGainMax, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, UvGainMin, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);

	INIT_A(attr, BrightContrastHigh, 80,
	80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80
	);
	INIT_A(attr, BrightContrastLow, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30
	);
	INIT_A(attr, DarkContrastHigh, 20,
	20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20
	);
	INIT_A(attr, DarkContrastLow, 15,
	15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15
	);

	INIT_V(attr, stManual.LumaPosWgt.Wgt, 128);
	INIT_V(attr, stManual.LumaPosWgt.Sigma, 128);
	INIT_V(attr, stManual.LumaPosWgt.Mean, 1);

	for (CVI_U32 IsoIdx = 0; IsoIdx < ISP_AUTO_ISO_STRENGTH_NUM; ++IsoIdx) {
		INIT_V(attr, stAuto.LumaPosWgt[IsoIdx].Wgt, 128);
		INIT_V(attr, stAuto.LumaPosWgt[IsoIdx].Sigma, 128);
		INIT_V(attr, stAuto.LumaPosWgt[IsoIdx].Mean, 1);
	}

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

	CVI_ISP_SetLDCIAttr(ViPipe, &attr);
}

CVI_VOID setCaDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CA_ATTR_S *pattr = NULL;

	pattr = ISP_CALLOC(1, sizeof(ISP_CA_ATTR_S));

	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "calloc fail");
		return;
	}
	INIT_V(*pattr, Enable, 0);
	INIT_V(*pattr, enOpType, 0);
	INIT_V(*pattr, UpdateInterval, 1);
	INIT_V(*pattr, CaCpMode, 0);

	for (CVI_U32 idx = 0 ; idx < CA_LUT_NUM ; idx++) {
		INIT_V(*pattr, CPLutY[idx], idx);
		INIT_V(*pattr, CPLutU[idx], idx);
		INIT_V(*pattr, CPLutV[idx], idx);
	}
	INIT_A(*pattr, ISORatio, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	for (CVI_U32 idx = 0 ; idx < CA_LUT_NUM ; idx++) {
		INIT_A(*pattr, YRatioLut[idx], 128,
		128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
		);
	}

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

	isp_ca_ctrl_set_ca_attr(ViPipe, pattr);
	free(pattr);
}

CVI_VOID setDehazeDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_DEHAZE_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_A(attr, Strength, 40,
	40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40
	);
	INIT_V(attr, CumulativeThr, 1500);
	INIT_V(attr, MinTransMapValue, 819);
	INIT_V(attr, DehazeLumaEnable, 0);
	INIT_V(attr, DehazeSkinEnable, 0);
	INIT_V(attr, AirLightMixWgt, 0);
	INIT_V(attr, DehazeWgt, 0);
	INIT_V(attr, TransMapScale, 16);
	INIT_V(attr, AirlightDiffWgt, 0);
	INIT_V(attr, AirLightMax, 4013);
	INIT_V(attr, AirLightMin, 3276);
	INIT_V(attr, SkinCb, 124);
	INIT_V(attr, SkinCr, 132);
	INIT_V(attr, DehazeLumaCOEFFI, 50);
	INIT_V(attr, DehazeSkinCOEFFI, 25);
	INIT_V(attr, TransMapWgtWgt, 64);
	INIT_V(attr, TransMapWgtSigma, 96);

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

	isp_dehaze_ctrl_set_dehaze_attr(ViPipe, &attr);
}

CVI_VOID setClutDefault(VI_PIPE ViPipe)
{
	setClut(ViPipe);
	setClutSaturation(ViPipe);
}

static CVI_VOID setClut(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CLUT_ATTR_S *pattr = NULL;

	pattr = ISP_CALLOC(1, sizeof(ISP_CLUT_ATTR_S));
	if (pattr == NULL) {
		ISP_LOG_ERR("%s\n", "calloc fail");
		return;
	}
	INIT_V(*pattr, Enable, 0);
	INIT_V(*pattr, UpdateInterval, 1);
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

	isp_clut_ctrl_set_clut_attr(ViPipe, pattr);
	free(pattr);
}

static CVI_VOID setClutSaturation(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CLUT_SATURATION_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);

	INIT_A(attr, SatIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatIn[1], 409,
	409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409, 409
	);
	INIT_A(attr, SatIn[2], 819,
	819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819
	);
	INIT_A(attr, SatIn[3], 1228,
	1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228
	);

	INIT_A(attr, SatOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatOut[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatOut[2], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatOut[3], 1228,
	1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228
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

	isp_clut_ctrl_set_clut_saturation_attr(ViPipe, &attr);
}

CVI_VOID setCcmDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CCM_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, stAuto.ISOActEnable, 1);
	INIT_V(attr, stAuto.TempActEnable, 1);
	INIT_V(attr, stAuto.CCMTabNum, 3);
	typeof(attr.stAuto.CCMTab[0].ColorTemp) colorTemp[7] = {6500, 3900, 2850};

	for (CVI_U32 ct = 0; ct < attr.stAuto.CCMTabNum; ct++) {
		INIT_V(attr, stAuto.CCMTab[ct].ColorTemp, colorTemp[ct]);
	}
	typeof(attr.stAuto.CCMTab[0].CCM[0]) autoCCM[7][9] = {
		{2008, -1012, 28, -543, 2026, -459, -47, -613, 1684},
		{1687, -496, -167, -694, 2043, -325, -28, -780, 1832},
		{1755, -546, -185, -591, 1917, -302, 130, -1269, 2163}
	};

	for (CVI_U32 ct = 0; ct < attr.stAuto.CCMTabNum; ct++) {
		for (CVI_U32 i = 0; i < 9; i++) {
			INIT_V(attr, stAuto.CCMTab[ct].CCM[i], autoCCM[ct][i]);
		}
	}
	INIT_V(attr, stManual.SatEnable, 1);
	typeof(attr.stManual.CCM[0]) manualCCM[9] = {1024, 0, 0, 0, 1024, 0, 0, 0, 1024};

	for (CVI_U32 i = 0; i < 9; i++) {
		INIT_V(attr, stManual.CCM[i], manualCCM[i]);
	}

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

	UNUSED(wdrEn);

	CVI_ISP_SetCCMAttr(ViPipe, &attr);
}

CVI_VOID setCcmSaturationDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CCM_SATURATION_ATTR_S attr = {0};

	INIT_A(attr, SaturationLE, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, SaturationSE, 128,
	116, 116, 112, 107, 100, 91, 82, 68, 59, 56, 56, 56, 56, 56, 56, 56
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

	isp_ccm_ctrl_set_ccm_saturation_attr(ViPipe, &attr);
}

CVI_VOID setSaturationDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_SATURATION_ATTR_S attr = {0};

	switch (ViPipe) {
	case 3:
	case 2:
	case 1:
	case 0:
	INIT_V(attr, enOpType, 0);
	if (wdrEn) {
		INIT_A(attr, Saturation, 128,
		32, 29, 26, 22, 19, 16, 12, 8, /*8*/4, 0, 0, 0, 0, 0, 0, 0
		);
	} else {
		INIT_A(attr, Saturation, 128,
		128, 115, 102, 90, 77, 64, 48, 32, /*8*/24, 16, 8, 0, 0, 0, 0, 0
		);
	}
	break;
	default:
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
	break;
	}

	CVI_ISP_SetSaturationAttr(ViPipe, &attr);
}

CVI_VOID setCacDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CAC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, PurpleDetRange, 96);
	INIT_V(attr, PurpleCb, 232);
	INIT_V(attr, PurpleCr, 157);
	INIT_V(attr, PurpleCb2, 232);
	INIT_V(attr, PurpleCr2, 176);
	INIT_V(attr, PurpleCb3, 232);
	INIT_V(attr, PurpleCr3, 176);
	INIT_V(attr, GreenCb, 43);
	INIT_V(attr, GreenCr, 21);
	INIT_V(attr, TuningMode, 0);
	INIT_V_ARRAY(attr, EdgeGainIn, 1, 2, 7);
	INIT_V_ARRAY(attr, EdgeGainOut, 0, 4, 32);

	INIT_A(attr, DePurpleStr, 30,
	30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30
	);
	INIT_A(attr, EdgeGlobalGain, 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
	);
	INIT_A(attr, EdgeCoring, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, EdgeStrMin, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, EdgeStrMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, DePurpleCbStr, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, DePurpleCrStr, 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, DePurpleStrMaxRatio, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, DePurpleStrMinRatio, 0,
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

	CVI_ISP_SetCACAttr(ViPipe, &attr);
}

CVI_VOID setCa2Default(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CA2_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);

	INIT_A(attr, Ca2In[0], 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, Ca2In[1], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, Ca2In[2], 12,
	12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12
	);
	INIT_A(attr, Ca2In[3], 18,
	18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18
	);
	INIT_A(attr, Ca2In[4], 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, Ca2In[5], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);

	INIT_A(attr, Ca2Out[0], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, Ca2Out[1], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, Ca2Out[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, Ca2Out[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, Ca2Out[4], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, Ca2Out[5], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
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

	isp_ca2_ctrl_set_ca2_attr(ViPipe, &attr);
}

CVI_VOID setPreSharpenDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_PRESHARPEN_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, LumaAdpGainEn, 1);
	INIT_V(attr, DeltaAdpGainEn, 1);
	INIT_V(attr, NoiseSuppressEnable, 0);
	INIT_V(attr, SatShtCtrlEn, 1);
	INIT_V(attr, SoftClampEnable, 0);
	INIT_V(attr, SoftClampUB, 1);
	INIT_V(attr, SoftClampLB, 1);

	CVI_U8 array[] = {
		60, 56, 52, 48, 44, 40, 38, 36, 34, 32,
		30, 28, 26, 25, 24, 23, 22, 21, 20, 19,
		18, 17, 16, 16, 16, 16, 16, 16, 16, 16,
		16, 16, 16
	};

	for (int i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
		INIT_A(attr, LumaAdpGain[i], 16,
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
		);
		INIT_A(attr, DeltaAdpGain[i], array[i],
		array[i], array[i], array[i], array[i], array[i], array[i], array[i], array[i],
		array[i], array[i], array[i], array[i], array[i], array[i], array[i], array[i]);
	}


	INIT_A(attr, LumaCorLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaCorLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LumaCorLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LumaCorLutIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, LumaCorLutOut[0], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[1], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[2], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[3], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, MotionCorLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionCorLutIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, MotionCorLutOut[0], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[1], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[2], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[3], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorWgtLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorWgtLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorWgtLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionCorWgtLutIn[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, MotionCorWgtLutOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorWgtLutOut[1], 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, MotionCorWgtLutOut[2], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorWgtLutOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);

	INIT_A(attr, GlobalGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, OverShootGain, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, UnderShootGain, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, HFBlendWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MFBlendWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, OverShootThr, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, UnderShootThr, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, OverShootThrMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, UnderShootThrMin, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, MotionShtGainIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionShtGainIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionShtGainIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, MotionShtGainOut[0], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[1], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	{
		int paramManual[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param100[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param1600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param3200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param6400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param12800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param25600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param51200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param102400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param204800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param409600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param819200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param1638400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param3276800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};

		for (int idx = 0; idx < SHARPEN_LUT_NUM; idx++) {
			INIT_A(attr, HueShtCtrl[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}
	INIT_A(attr, SatShtGainIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainIn[1], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, SatShtGainIn[2], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, SatShtGainIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, SatShtGainOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainOut[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, SatShtGainOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
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

	isp_presharpen_ctrl_set_presharpen_attr(ViPipe, &attr);

	UNUSED(wdrEn);
}

CVI_VOID setSharpenDefault(VI_PIPE ViPipe, CVI_U32 wdrEn)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_SHARPEN_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, TuningMode, 0);
	INIT_V(attr, LumaAdpGainEn, 1);
	INIT_V(attr, DeltaAdpGainEn, 1);

	CVI_U8 array[] = {
		60, 56, 52, 48, 44, 40, 38, 36, 34, 32,
		30, 28, 26, 25, 24, 23, 22, 21, 20, 19,
		18, 17, 16, 16, 16, 16, 16, 16, 16, 16,
		16, 16, 16
	};

	INIT_V(attr, NoiseSuppressEnable, 0);
	INIT_V(attr, SatShtCtrlEn, 1);
	INIT_V(attr, SoftClampEnable, 0);
	INIT_V(attr, SoftClampUB, 1);
	INIT_V(attr, SoftClampLB, 1);

	for (int i = 0 ; i < SHARPEN_LUT_NUM ; i++) {
		INIT_A(attr, LumaAdpGain[i], 16,
		16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
		);
		INIT_A(attr, DeltaAdpGain[i], array[i],
		array[i], array[i], array[i], array[i], array[i], array[i], array[i], array[i],
		array[i], array[i], array[i], array[i], array[i], array[i], array[i], array[i]);
	}

	INIT_A(attr, LumaCorLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, LumaCorLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, LumaCorLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, LumaCorLutIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, LumaCorLutOut[0], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[1], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[2], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, LumaCorLutOut[3], 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	);
	INIT_A(attr, MotionCorLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionCorLutIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, MotionCorLutOut[0], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[1], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[2], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorLutOut[3], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, MotionCorWgtLutIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorWgtLutIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorWgtLutIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionCorWgtLutIn[3], 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, MotionCorWgtLutOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionCorWgtLutOut[1], 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, MotionCorWgtLutOut[2], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionCorWgtLutOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);

	INIT_A(attr, GlobalGain, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, OverShootGain, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, UnderShootGain, 4,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
	);
	INIT_A(attr, HFBlendWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MFBlendWgt, 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, OverShootThr, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, UnderShootThr, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
	);
	INIT_A(attr, OverShootThrMax, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, UnderShootThrMin, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
	);
	INIT_A(attr, MotionShtGainIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, MotionShtGainIn[1], 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	);
	INIT_A(attr, MotionShtGainIn[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, MotionShtGainOut[0], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[1], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, MotionShtGainOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	{
		int paramManual[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param100[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param1600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param3200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param6400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param12800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param25600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param51200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param102400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param204800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param409600[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param819200[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param1638400[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};
		int param3276800[] = {16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
			16, 16, 20, 24, 28, 30, 32, 30, 38, 24, 20, 16, 16, 16, 16, 16, 16, 16, 16};

		for (int idx = 0; idx < SHARPEN_LUT_NUM; idx++) {
			INIT_A(attr, HueShtCtrl[idx], paramManual[idx]
				, /*0*/param100[idx], param200[idx], param400[idx], param800[idx],
				param1600[idx], param3200[idx], param6400[idx], param12800[idx]
				, /*8*/param25600[idx], param51200[idx], param102400[idx], param204800[idx]
				, param409600[idx], param819200[idx], param1638400[idx], param3276800[idx]);
		}
	}
	INIT_A(attr, SatShtGainIn[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainIn[1], 8,
	8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	);
	INIT_A(attr, SatShtGainIn[2], 16,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
	);
	INIT_A(attr, SatShtGainIn[3], 192,
	192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192, 192
	);
	INIT_A(attr, SatShtGainOut[0], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainOut[1], 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	);
	INIT_A(attr, SatShtGainOut[2], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
	);
	INIT_A(attr, SatShtGainOut[3], 128,
	128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128
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

	isp_sharpen_ctrl_set_sharpen_attr(ViPipe, &attr);

	UNUSED(wdrEn);
}

CVI_VOID setYContrastDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_YCONTRAST_ATTR_S attr = {0};

	INIT_V(attr, Enable, 1);
	INIT_V(attr, enOpType, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_LV_A(attr, ContrastLow, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
	);
	INIT_LV_A(attr, ContrastHigh, 50,
	50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50
	);
	INIT_LV_A(attr, CenterLuma, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
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

	isp_ycontrast_ctrl_set_ycontrast_attr(ViPipe, &attr);
}

CVI_VOID setMonoDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_MONO_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, UpdateInterval, 1);

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

	isp_mono_ctrl_set_mono_attr(ViPipe, &attr);
}

CVI_S32 setNoiseProfile(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CMOS_DEFAULT_S *pstSnsDft = CVI_NULL;

	isp_sensor_default_get(ViPipe, &pstSnsDft);
	if (pstSnsDft == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_CMOS_NOISE_CALIBRATION_S np = {0};

	memcpy(np.CalibrationCoef, pstSnsDft->stNoiseCalibration.CalibrationCoef,
		sizeof(ISP_CMOS_NOISE_CALIBRATION_S));

	CVI_FLOAT checkSum = 0;

	// Check ISO 100 noise profile. If checksum = 0 means sensor driver has no noise profile.
	for (CVI_U32 i = 0; i < NOISE_PROFILE_CHANNEL_NUM; i++) {
		for (CVI_U32 j = 0; j < NOISE_PROFILE_LEVEL_NUM; j++) {
			checkSum += np.CalibrationCoef[0][i][j];
		}
	}

	if (checkSum == (CVI_FLOAT)0) {
		ISP_LOG_ERR("Noise profile get fail. Please check\n");
	}

	CVI_ISP_SetNoiseProfileAttr(ViPipe, &np);

	return CVI_SUCCESS;
}

CVI_S32 setDisDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("%d\n", ViPipe);

	setDisAttr(ViPipe);
	setDisConfig(ViPipe);

	return CVI_SUCCESS;
}

CVI_S32 setDisAttr(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("%d\n", ViPipe);
#ifndef ARCH_RTOS_CV181X
	ISP_DIS_ATTR_S attr = {0};

	INIT_V(attr, enable, 0);
	INIT_V(attr, movingSubjectLevel, 0);
	INIT_V(attr, horizontalLimit, 60);
	INIT_V(attr, verticalLimit, 60);
	INIT_V(attr, stillCrop, 0);

	isp_dis_ctrl_set_dis_attr(ViPipe, &attr);
#endif

	return CVI_SUCCESS;
}

CVI_S32 setDisConfig(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("%d\n", ViPipe);
#ifndef ARCH_RTOS_CV181X
	ISP_DIS_CONFIG_S attr = {0};

	INIT_V(attr, mode, 0);
	INIT_V(attr, motionLevel, 0);
	INIT_V(attr, cropRatio, 94);

	isp_dis_ctrl_set_dis_config(ViPipe, &attr);
#endif

	return CVI_SUCCESS;
}

static void setVCDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);
#ifndef ARCH_RTOS_CV181X
	ISP_VC_ATTR_S attr = {0};

	INIT_V_ARRAY(attr, MotionThreshold, 64,
	64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64);

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

	isp_motion_ctrl_set_motion_attr(ViPipe, &attr);
#endif
}

static void setCscDefault(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("(%d)\n", ViPipe);

	ISP_CSC_ATTR_S attr = {0};

	INIT_V(attr, Enable, 0);
	INIT_V(attr, UpdateInterval, 1);
	INIT_V(attr, Hue, 50);
	INIT_V(attr, Luma, 50);
	INIT_V(attr, Contrast, 50);
	INIT_V(attr, Saturation, 50);

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

	isp_csc_ctrl_set_csc_attr(ViPipe, &attr);
}
