/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_gamma_ctrl.c
 * Description:
 *
 */

#include "isp_main_local.h"
#include "isp_debug.h"
#include "isp_defines.h"
#include "cvi_comm_isp.h"

#include "isp_proc_local.h"
#include "isp_tun_buf_ctrl.h"
#include "isp_interpolate.h"

#include "isp_gamma_ctrl.h"
#include "isp_algo_gamma.h"
#include "isp_mgr_buf.h"
#include "isp_iir_api.h"

#define GAMMA_CUSTOMIZE_BASE		(10000)

enum GAMMMA_ID {
	GAMMA_ID_MIN = 0,
	GAMMA_ID_DEFAULT_SDR = GAMMA_ID_MIN,
	GAMMA_ID_DEFAULT_WDR,
	GAMMA_ID_SRGB,
	// GAMMA_ID_USER_DEFINE,
	GAMMA_ID_BYPASS,
	GAMMA_ID_MAX
};

#define MAX_GAMMA_LUT_VALUE			(4095)
#define Y_GAMMA_IIR_SPEED 25
const CVI_U16 gamma_table_set[GAMMA_ID_MAX][GAMMA_NODE_NUM] = {
// const CVI_U16 gamma_table_set[ISP_GAMMA_CURVE_MAX][GAMMA_NODE_NUM] = {
	{ // GAMMA_ID_DEFAULT_SDR
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
	},
	{ // GAMMA_ID_DEFAULT_WDR
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
	},
	{ // GAMMA_ID_SRGB
		   0,  203,  347,  452,  538,  613,  679,  739,  794,  845,  894,  939,  982, 1023, 1062, 1099,
		1136, 1170, 1204, 1237, 1268, 1299, 1329, 1358, 1386, 1414, 1441, 1467, 1493, 1518, 1543, 1567,
		1591, 1615, 1638, 1660, 1683, 1704, 1726, 1747, 1768, 1789, 1809, 1829, 1849, 1868, 1888, 1907,
		1926, 1944, 1962, 1981, 1998, 2016, 2034, 2051, 2068, 2085, 2102, 2119, 2135, 2151, 2168, 2184,
		2199, 2215, 2231, 2246, 2261, 2277, 2292, 2307, 2321, 2336, 2351, 2365, 2379, 2394, 2408, 2422,
		2436, 2449, 2463, 2477, 2490, 2504, 2517, 2530, 2543, 2557, 2570, 2582, 2595, 2608, 2621, 2633,
		2646, 2658, 2670, 2683, 2695, 2707, 2719, 2731, 2743, 2755, 2767, 2778, 2790, 2802, 2813, 2825,
		2836, 2847, 2859, 2870, 2881, 2892, 2903, 2914, 2925, 2936, 2947, 2958, 2969, 2979, 2990, 3001,
		3011, 3022, 3032, 3043, 3053, 3063, 3074, 3084, 3094, 3104, 3114, 3124, 3134, 3144, 3154, 3164,
		3174, 3184, 3194, 3203, 3213, 3223, 3232, 3242, 3252, 3261, 3271, 3280, 3289, 3299, 3308, 3317,
		3327, 3336, 3345, 3354, 3363, 3372, 3382, 3391, 3400, 3409, 3418, 3426, 3435, 3444, 3453, 3462,
		3471, 3479, 3488, 3497, 3505, 3514, 3523, 3531, 3540, 3548, 3557, 3565, 3574, 3582, 3590, 3599,
		3607, 3615, 3624, 3632, 3640, 3648, 3656, 3665, 3673, 3681, 3689, 3697, 3705, 3713, 3721, 3729,
		3737, 3745, 3753, 3761, 3769, 3776, 3784, 3792, 3800, 3807, 3815, 3823, 3831, 3838, 3846, 3854,
		3861, 3869, 3876, 3884, 3891, 3899, 3906, 3914, 3921, 3929, 3936, 3944, 3951, 3958, 3966, 3973,
		3980, 3988, 3995, 4002, 4009, 4017, 4024, 4031, 4038, 4045, 4053, 4060, 4067, 4074, 4081, 4088
	},
	{ // GAMMA_ID_BYPASS
		   0,   16,   32,   48,   64,   80,   96,  112,  128,  145,  161,  177,  193,  209,  225,  241,
		 257,  273,  289,  305,  321,  337,  353,  369,  385,  401,  418,  434,  450,  466,  482,  498,
		 514,  530,  546,  562,  578,  594,  610,  626,  642,  658,  674,  691,  707,  723,  739,  755,
		 771,  787,  803,  819,  835,  851,  867,  883,  899,  915,  931,  947,  964,  980,  996, 1012,
		1028, 1044, 1060, 1076, 1092, 1108, 1124, 1140, 1156, 1172, 1188, 1204, 1220, 1237, 1253, 1269,
		1285, 1301, 1317, 1333, 1349, 1365, 1381, 1397, 1413, 1429, 1445, 1461, 1477, 1493, 1510, 1526,
		1542, 1558, 1574, 1590, 1606, 1622, 1638, 1654, 1670, 1686, 1702, 1718, 1734, 1750, 1766, 1783,
		1799, 1815, 1831, 1847, 1863, 1879, 1895, 1911, 1927, 1943, 1959, 1975, 1991, 2007, 2023, 2039,
		2056, 2072, 2088, 2104, 2120, 2136, 2152, 2168, 2184, 2200, 2216, 2232, 2248, 2264, 2280, 2296,
		2312, 2329, 2345, 2361, 2377, 2393, 2409, 2425, 2441, 2457, 2473, 2489, 2505, 2521, 2537, 2553,
		2569, 2585, 2602, 2618, 2634, 2650, 2666, 2682, 2698, 2714, 2730, 2746, 2762, 2778, 2794, 2810,
		2826, 2842, 2858, 2875, 2891, 2907, 2923, 2939, 2955, 2971, 2987, 3003, 3019, 3035, 3051, 3067,
		3083, 3099, 3115, 3131, 3148, 3164, 3180, 3196, 3212, 3228, 3244, 3260, 3276, 3292, 3308, 3324,
		3340, 3356, 3372, 3388, 3404, 3421, 3437, 3453, 3469, 3485, 3501, 3517, 3533, 3549, 3565, 3581,
		3597, 3613, 3629, 3645, 3661, 3677, 3694, 3710, 3726, 3742, 3758, 3774, 3790, 3806, 3822, 3838,
		3854, 3870, 3886, 3902, 3918, 3934, 3950, 3967, 3983, 3999, 4015, 4031, 4047, 4063, 4079, 4095,
	}
};

#ifndef ENABLE_ISP_C906L

const struct isp_module_ctrl gamma_mod = {
	.init = isp_gamma_ctrl_init,
	.uninit = isp_gamma_ctrl_uninit,
	.suspend = isp_gamma_ctrl_suspend,
	.resume = isp_gamma_ctrl_resume,
	.ctrl = isp_gamma_ctrl_ctrl
};

static CVI_S32 isp_gamma_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
static CVI_S32 isp_gamma_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult);
// static CVI_S32 isp_gamma_ctrl_process(VI_PIPE ViPipe);
static CVI_S32 isp_gamma_ctrl_postprocess(VI_PIPE ViPipe);
static CVI_S32 check_gamma_attr_valid(const ISP_GAMMA_ATTR_S *pstGammaAttr);
static CVI_S32 check_auto_gamma_attr_valid(const ISP_AUTO_GAMMA_ATTR_S *pstAutoGammaAttr);

static CVI_S32 set_gamma_proc_info(VI_PIPE ViPipe);

static CVI_VOID get_gamma_curve(VI_PIPE ViPipe, const ISP_GAMMA_CURVE_TYPE_E curveType,
	CVI_U16 *curve);

static CVI_VOID update_rgbgamma_lut(CVI_U16 *pu16DestLut, CVI_U16 u16CurveType);

static struct isp_gamma_ctrl_runtime *_get_gamma_ctrl_runtime(VI_PIPE ViPipe);

CVI_S32 isp_gamma_ctrl_init(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	runtime->rgbgammacurve_index = GAMMA_ID_SRGB;
	runtime->bResetYGammaIIR = CVI_TRUE;
	runtime->preprocess_updated = CVI_TRUE;
	runtime->process_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_FALSE;
	runtime->is_module_bypass = CVI_FALSE;

	return ret;
}

CVI_S32 isp_gamma_ctrl_uninit(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_gamma_ctrl_suspend(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_gamma_ctrl_resume(VI_PIPE ViPipe)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;

	UNUSED(ViPipe);

	return ret;
}

CVI_S32 isp_gamma_ctrl_ctrl(VI_PIPE ViPipe, enum isp_module_cmd cmd, CVI_VOID *input)
{
	ISP_LOG_DEBUG("+\n");
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	switch (cmd) {
	case MOD_CMD_POST_EOF:
		isp_gamma_ctrl_post_eof(ViPipe, (ISP_ALGO_RESULT_S *)input);
		break;
	case MOD_CMD_SET_MODCTRL:
		runtime->is_module_bypass = ((ISP_MODULE_CTRL_U *)input)->bitBypassGamma;
		break;
	case MOD_CMD_GET_MODCTRL:
		((ISP_MODULE_CTRL_U *)input)->bitBypassGamma = runtime->is_module_bypass;
		break;
	default:
		break;
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  Static functions
//-----------------------------------------------------------------------------
static CVI_S32 isp_gamma_ctrl_post_eof(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;

	ret = isp_gamma_ctrl_preprocess(ViPipe, algoResult);
	if (ret != CVI_SUCCESS)
		return ret;

	#if 0
	ret = isp_gamma_ctrl_process(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;
	#endif

	ret = isp_gamma_ctrl_postprocess(ViPipe);
	if (ret != CVI_SUCCESS)
		return ret;

	set_gamma_proc_info(ViPipe);

	return ret;
}

static CVI_S32 _LinearInter(CVI_S16 v, CVI_S32 x0, CVI_S32 y0, CVI_S32 x1, CVI_S32 y1)
{
	CVI_S32 res;

	if (v <= x0)
		return y0;
	if (v >= x1)
		return y1;
	if ((x1 - x0) == 0)
		return y0;

	res = (CVI_S64)(y1 - y0) * (CVI_S64)(v - x0) / (x1 - x0) + y0;
	return res;
}

static CVI_S32 _isp_gamma_param_update(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_GAMMA_ATTR_S *gamma_attr = NULL;
	const ISP_AUTO_GAMMA_ATTR_S *auto_gamma_attr = NULL;

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &gamma_attr);

	if (!gamma_attr->Enable || runtime->is_module_bypass) {
		// Disable gamma, we need to make a bypass gamma for DRC
		for (CVI_U32 i = 0; i < GAMMA_NODE_NUM; i++) {
			runtime->au16RealGamma[i] = gamma_table_set[GAMMA_ID_BYPASS][i];
		}

		return CVI_SUCCESS;
	}

	runtime->au16RealGamma[GAMMA_NODE_NUM] = MAX_GAMMA_LUT_VALUE;

	if (gamma_attr->enCurveType != ISP_GAMMA_CURVE_AUTO) {
		get_gamma_curve(ViPipe, gamma_attr->enCurveType, runtime->au16RealGamma);
	} else {
		CVI_U16 i;
		CVI_S16 currentLV = algoResult->currentLV;

		isp_gamma_ctrl_get_auto_gamma_attr(ViPipe, &auto_gamma_attr);

		// When Current lv smaller than auto gamma min lv. Use Min Lv gamma.
		if ((currentLV <= auto_gamma_attr->GammaTab[0].Lv) || (auto_gamma_attr->GammaTabNum <= 1)) {
			for (i = 0; i < GAMMA_NODE_NUM; i++) {
				runtime->au16RealGamma[i] = auto_gamma_attr->GammaTab[0].Tbl[i];
			}
		// When Current lv bigger than auto gamma max lv. Use Max Lv gamma.
		} else if (currentLV >= auto_gamma_attr->GammaTab[auto_gamma_attr->GammaTabNum - 1].Lv) {
			for (i = 0; i < GAMMA_NODE_NUM; i++) {
				runtime->au16RealGamma[i] =
					auto_gamma_attr->GammaTab[auto_gamma_attr->GammaTabNum - 1].Tbl[i];
			}
		} else {
			// When Current lv between auto gamma min & max lv. interpolation.
			// Check gamma Lv interpolation used index
			CVI_U8 gammaIdx = 0;

			for (i = 1; i < auto_gamma_attr->GammaTabNum; i++) {
				if (currentLV <= auto_gamma_attr->GammaTab[i].Lv) {
					gammaIdx = i;
					break;
				}
			}
			// interpolation gamma.
			for (i = 0; i < GAMMA_NODE_NUM; i++) {
				runtime->au16RealGamma[i] = _LinearInter(currentLV,
					auto_gamma_attr->GammaTab[gammaIdx-1].Lv,
					auto_gamma_attr->GammaTab[gammaIdx-1].Tbl[i],
					auto_gamma_attr->GammaTab[gammaIdx].Lv,
					auto_gamma_attr->GammaTab[gammaIdx].Tbl[i]);
			}
		}
	}

	return ret;
}

static CVI_S32 isp_gamma_ctrl_preprocess(VI_PIPE ViPipe, ISP_ALGO_RESULT_S *algoResult)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	const ISP_GAMMA_ATTR_S *gamma_attr = NULL;

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &gamma_attr);

	CVI_BOOL is_preprocess_update = CVI_FALSE;
	CVI_U8 intvl = MAX(gamma_attr->UpdateInterval, 1);

	is_preprocess_update = ((runtime->preprocess_updated) || ((algoResult->u32FrameIdx % intvl) == 0));

	// No need to update status
	if (is_preprocess_update == CVI_FALSE)
		return ret;

	runtime->preprocess_updated = CVI_FALSE;
	runtime->postprocess_updated = CVI_TRUE;

	// No need to update parameters if disable. Because its meaningless
	if (!gamma_attr->Enable || runtime->is_module_bypass) {
		runtime->bResetYGammaIIR = CVI_TRUE;
		//return ret;
	}
	_isp_gamma_param_update(ViPipe, algoResult);

	runtime->process_updated = CVI_TRUE;

	return ret;
}

#if 0
static CVI_S32 isp_gamma_ctrl_process(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	if (runtime->process_updated == CVI_FALSE)
		return ret;

	ret = isp_algo_gamma_main(
		(struct gamma_param_in *)&runtime->gamma_param_in,
		(struct gamma_param_out *)&runtime->gamma_param_out
	);

	runtime->process_updated = CVI_FALSE;

	return ret;
}
#endif

static CVI_VOID update_rgbgamma_lut(CVI_U16 *pu16DestLut, CVI_U16 u16CurveType)
{
	if (u16CurveType < GAMMA_ID_MAX) {
		memcpy(pu16DestLut, gamma_table_set[u16CurveType], sizeof(CVI_U16) * GAMMA_NODE_NUM);
		return;
	}

	if (u16CurveType == (CURVE_DRC_TUNING_CURVE + GAMMA_CUSTOMIZE_BASE)) {
		for (CVI_U32 idx = 0; idx < GAMMA_NODE_NUM; ++idx) {
			pu16DestLut[idx] = MIN((idx << 5), 4095);
		}
		return;
	}

	ISP_LOG_ERR("Un-support RGBGamma curve : %d\n", u16CurveType);
}

static CVI_S32 isp_gamma_ctrl_postprocess(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	struct cvi_vip_isp_post_cfg *post_addr = get_post_tuning_buf_addr(ViPipe);
	CVI_U8 tun_idx = get_tuning_buf_idx(ViPipe);

	struct cvi_vip_isp_gamma_config *gamma_cfg =
		(struct cvi_vip_isp_gamma_config *)&(post_addr->tun_cfg[tun_idx].gamma_cfg);
	struct cvi_vip_isp_ygamma_config *ygamma_cfg =
		(struct cvi_vip_isp_ygamma_config *)&(post_addr->tun_cfg[tun_idx].ygamma_cfg);

	const ISP_GAMMA_ATTR_S *gamma_attr = NULL;

	TIIR_U16_Ctrl tIIRCtrl;

	tIIRCtrl.pu16LutIn = runtime->au16RealGamma;
	tIIRCtrl.pu32LutHistory = runtime->au32HistoryGamma;
	tIIRCtrl.pu16LutOut = runtime->au16RealGammaIIR;
	tIIRCtrl.u16LutSize = GAMMA_NODE_NUM;
	tIIRCtrl.u16IIRWeight = Y_GAMMA_IIR_SPEED;

	if (runtime->bResetYGammaIIR) {
		tIIRCtrl.u16IIRWeight = 0;
		runtime->bResetYGammaIIR = CVI_FALSE;
	}

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &gamma_attr);

	CVI_BOOL is_postprocess_update = ((runtime->postprocess_updated == CVI_TRUE) || (IS_MULTI_CAM()));

	if (is_postprocess_update == CVI_FALSE) {
		ygamma_cfg->update = 0;
		gamma_cfg->update = 0;
	} else {
		ygamma_cfg->enable = gamma_attr->Enable && !runtime->is_module_bypass;
		ygamma_cfg->update = 1;
		ygamma_cfg->max = 65536;
		IIR_U16_Once(&tIIRCtrl);
		// memcpy(ygamma_cfg->lut, runtime->au16RealGamma, sizeof(CVI_U16) * GAMMA_NODE_NUM);
		for (CVI_U32 i = 0 ; i < GAMMA_NODE_NUM - 1; i++)
			ygamma_cfg->lut[i] = runtime->au16RealGammaIIR[i] << 4;
		ygamma_cfg->lut[GAMMA_NODE_NUM - 1] = 65535;

		gamma_cfg->enable = 1;
		gamma_cfg->update = 1;
		gamma_cfg->max = 4096;

		update_rgbgamma_lut(gamma_cfg->lut, runtime->rgbgammacurve_index);
	}
	runtime->postprocess_updated = CVI_FALSE;

	return ret;
}

static CVI_S32 set_gamma_proc_info(VI_PIPE ViPipe)
{
	CVI_S32 ret = CVI_SUCCESS;
#ifdef ENABLE_ISP_PROC_DEBUG
	if (ISP_GET_PROC_ACTION(ViPipe, PROC_LEVEL_1)) {
		const ISP_GAMMA_ATTR_S *gamma_attr = NULL;
		ISP_DEBUGINFO_PROC_S *pProcST = NULL;

		isp_gamma_ctrl_get_gamma_attr(ViPipe, &gamma_attr);
		ISP_GET_PROC_INFO(ViPipe, pProcST);

		//common
		pProcST->GammaEnable = gamma_attr->Enable;
		pProcST->GammaenCurveType = gamma_attr->enCurveType;
	}
#else
	UNUSED(ViPipe);
#endif
	return ret;
}
#endif // ENABLE_ISP_C906L

static struct isp_gamma_ctrl_runtime *_get_gamma_ctrl_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	struct isp_gamma_shared_buffer *shared_buf = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GAMMA, (CVI_VOID *) &shared_buf);

	return &shared_buf->runtime;
}

//-----------------------------------------------------------------------------
//  private functions
//-----------------------------------------------------------------------------
static CVI_VOID get_gamma_curve(VI_PIPE ViPipe, const ISP_GAMMA_CURVE_TYPE_E curveType,
	CVI_U16 *curve)
{
	const CVI_U16 *gammaCurve;
	const ISP_GAMMA_ATTR_S *pstGammaAttr = CVI_NULL;

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &pstGammaAttr);

	switch (curveType) {
	case ISP_GAMMA_CURVE_DEFAULT:
		#if 0
		if (sdr)
			gammaCurve = gamma_table_set[GAMMA_ID_DEFAULT_SDR];
		else
		#endif
			gammaCurve = gamma_table_set[GAMMA_ID_DEFAULT_WDR];
		break;
	case ISP_GAMMA_CURVE_SRGB:
		gammaCurve = gamma_table_set[GAMMA_ID_SRGB];
		break;
	case ISP_GAMMA_CURVE_USER_DEFINE:
		gammaCurve = pstGammaAttr->Table;
		break;
	default:
		gammaCurve = pstGammaAttr->Table;
		break;
	}
	memcpy(curve, gammaCurve, sizeof(CVI_U16) * GAMMA_NODE_NUM);
}

static CVI_S32 check_gamma_attr_valid(const ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	// CHECK_VALID_CONST(pstGammaAttr, Enable, CVI_FALSE, CVI_TRUE);
	CHECK_VALID_ARRAY_1D(pstGammaAttr, Table, GAMMA_NODE_NUM, 0x0, 0xfff);
	CHECK_VALID_CONST(pstGammaAttr, enCurveType, 0x0, ISP_GAMMA_CURVE_MAX - 1);
	// CHECK_VALID_CONST(pstGammaAttr, UpdateInterval, 0, 0xff);

	return ret;
}

static CVI_S32 check_auto_gamma_attr_valid(const ISP_AUTO_GAMMA_ATTR_S *pstAutoGammaAttr)
{
	CVI_S32 ret = CVI_SUCCESS;

	CHECK_VALID_CONST(pstAutoGammaAttr, GammaTabNum, 1, GAMMA_MAX_INTERPOLATION_NUM);

	for (CVI_U32 u32LVIdx = 0; u32LVIdx < pstAutoGammaAttr->GammaTabNum; ++u32LVIdx) {
		const ISP_GAMMA_CURVE_ATTR_S *ptGammaCurveAttr = &(pstAutoGammaAttr->GammaTab[u32LVIdx]);

		CHECK_VALID_CONST(ptGammaCurveAttr, Lv, -500, 1500);
		CHECK_VALID_ARRAY_1D(ptGammaCurveAttr, Tbl, GAMMA_NODE_NUM, 0x0, 0xfff);
	}

	return ret;
}

//-----------------------------------------------------------------------------
//  public functions, set or get param
//-----------------------------------------------------------------------------
CVI_S32 isp_gamma_ctrl_get_gamma_curve_by_type(VI_PIPE ViPipe, ISP_GAMMA_ATTR_S *pstGammaAttr,
	const ISP_GAMMA_CURVE_TYPE_E curveType)
{
	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	const ISP_GAMMA_ATTR_S *p = CVI_NULL;

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &p);

	pstGammaAttr->Enable = p->Enable;
	pstGammaAttr->enCurveType = curveType;

	get_gamma_curve(ViPipe, curveType, pstGammaAttr->Table);

	return ret;
}

CVI_S32 isp_gamma_ctrl_get_gamma_attr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S **pstGammaAttr)
{
	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;

	struct isp_gamma_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GAMMA, (CVI_VOID *) &shared_buffer);
	*pstGammaAttr = &shared_buffer->stGammaAttr;

	return ret;
}

CVI_S32 isp_gamma_ctrl_set_gamma_attr(VI_PIPE ViPipe, const ISP_GAMMA_ATTR_S *pstGammaAttr)
{
	if (pstGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = check_gamma_attr_valid(pstGammaAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_GAMMA_ATTR_S *p = CVI_NULL;

	isp_gamma_ctrl_get_gamma_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstGammaAttr, sizeof(*pstGammaAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return CVI_SUCCESS;
}

CVI_S32 isp_gamma_ctrl_get_auto_gamma_attr(VI_PIPE ViPipe, const ISP_AUTO_GAMMA_ATTR_S **pstAutoGammaAttr)
{
	if (pstAutoGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_shared_buffer *shared_buffer = CVI_NULL;

	isp_mgr_buf_get_addr(ViPipe, ISP_IQ_BLOCK_GAMMA, (CVI_VOID *) &shared_buffer);
	*pstAutoGammaAttr = &shared_buffer->stAutoGammaAttr;

	return ret;
}

CVI_S32 isp_gamma_ctrl_set_auto_gamma_attr(VI_PIPE ViPipe, const ISP_AUTO_GAMMA_ATTR_S *pstAutoGammaAttr)
{
	if (pstAutoGammaAttr == CVI_NULL) {
		return CVI_FAILURE;
	}

	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ret = check_auto_gamma_attr_valid(pstAutoGammaAttr);
	if (ret != CVI_SUCCESS)
		return ret;

	const ISP_AUTO_GAMMA_ATTR_S *p = CVI_NULL;

	isp_gamma_ctrl_get_auto_gamma_attr(ViPipe, &p);
	memcpy((CVI_VOID *) p, pstAutoGammaAttr, sizeof(*pstAutoGammaAttr));

	runtime->preprocess_updated = CVI_TRUE;

	return ret;
}

CVI_S32 isp_gamma_ctrl_get_real_gamma_lut(VI_PIPE ViPipe, CVI_U16 **pu16GammaLut)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	*pu16GammaLut = runtime->au16RealGammaIIR;

	return ret;
}

CVI_S32 isp_gamma_ctrl_set_rgbgamma_curve(VI_PIPE ViPipe, enum RGBGAMMA_CURVE_TYPE eCurveType)
{
	CVI_S32 ret = CVI_SUCCESS;
	struct isp_gamma_ctrl_runtime *runtime = _get_gamma_ctrl_runtime(ViPipe);

	if (runtime == CVI_NULL) {
		return CVI_FAILURE;
	}

	ISP_LOG_DEBUG("RGB Gamma curve change to %d (Vipipe : %d)\n", eCurveType, ViPipe);

	switch (eCurveType) {
	case CURVE_DRC_TUNING_CURVE:
		runtime->rgbgammacurve_index = CURVE_DRC_TUNING_CURVE + GAMMA_CUSTOMIZE_BASE;
		break;
	case CURVE_BYPASS:
		runtime->rgbgammacurve_index = GAMMA_ID_BYPASS;
		break;
	case CURVE_DEFAULT:
	default:
		runtime->rgbgammacurve_index = GAMMA_ID_SRGB;
		break;
	}

	return ret;
}


