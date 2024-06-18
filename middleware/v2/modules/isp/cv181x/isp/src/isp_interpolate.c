/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_interpolate.c
 * Description:
 *
 */

#include "cvi_comm_isp.h"
#include "isp_defines.h"

#include "isp_interpolate.h"

static CVI_U32 iso_lut[ISP_AUTO_ISO_STRENGTH_NUM] = {
	  100,   200,    400,    800,   1600,   3200,    6400,   12800,
	25600, 51200, 102400, 204800, 409600, 819200, 1638400, 3276800
};
static CVI_U32 expRatioLut[ISP_AUTO_EXP_RATIO_NUM] = {
	1,   2,   4,   8,    16,    32,   64,   128,    256,
	512, 1024, 2048, 4096, 8192, 16384, 32768};
static CVI_S16 lvLut[ISP_AUTO_LV_NUM] = {
	-500, -400, -300, -200, -100,    0,  100,  200,  300,  400,
	 500,  600,  700,  800,  900, 1000, 1100, 1200, 1300, 1400,
	1500
};
#if 0

CVI_U32 colorTempLut[ISP_AUTO_COLORTEMP_NUM] = { 2300, 2850, 4000, 4100, 5000, 6500, 10000};

#endif

struct interplt_info {
	CVI_U32 val;
	CVI_U32 lower_scale;
	CVI_U32 lower_idx;
	CVI_U32 upper_idx;
};

#define MAX_INTRPLT_INFO_NUM 6
struct isp_interpolate_runtime {
	CVI_U32 exp_ratio;
	CVI_U32 exp_ratio_lower_scale;
	CVI_U8 exp_ratio_lower_idx;
	CVI_U8 exp_ratio_upper_idx;
	CVI_U32 pre_iso;
	CVI_U32 pre_iso_lower_scale;
	CVI_U8 pre_iso_lower_idx;
	CVI_U8 pre_iso_upper_idx;
	CVI_U32 post_iso;
	CVI_U32 post_iso_lower_scale;
	CVI_U8 post_iso_lower_idx;
	CVI_U8 post_iso_upper_idx;
	CVI_U32 pre_blc_iso;
	CVI_U32 pre_blc_iso_lower_scale;
	CVI_U8 pre_blc_iso_lower_idx;
	CVI_U8 pre_blc_iso_upper_idx;
	CVI_U32 post_blc_iso;
	CVI_U32 post_blc_iso_lower_scale;
	CVI_U8 post_blc_iso_lower_idx;
	CVI_U8 post_blc_iso_upper_idx;
	CVI_S16 lv;
	CVI_U8 lv_lower_idx;
	CVI_U8 lv_upper_idx;
};
struct isp_interpolate_runtime interpolate_runtime[VI_MAX_PIPE_NUM];

static struct isp_interpolate_runtime  *_get_interpolate_runtime(VI_PIPE ViPipe);

static inline CVI_S32 interpolate_iso_imp(CVI_S32 v, CVI_S32 x0, CVI_S32 y0, CVI_S32 x1, CVI_S32 y1);
static inline CVI_VOID get_interpolate_info(VI_PIPE ViPipe,
	enum INTPLT_TYPE type, CVI_S32 *val, CVI_S32 *val1, CVI_S32 *val2,
	CVI_S32 *idx1, CVI_S32 *idx2);

CVI_S32 isp_interpolate_init(VI_PIPE ViPipe)
{
	struct isp_interpolate_runtime *runtime = _get_interpolate_runtime(ViPipe);

	for (CVI_U32 idx = 0 ; idx < ISP_AUTO_ISO_STRENGTH_NUM ; idx++) {
		iso_lut[idx] = (1 << idx) * 100;
		// expRatioLut[idx] = (1 << idx);
	}

	memset(runtime, 0, sizeof(struct isp_interpolate_runtime));

	return CVI_SUCCESS;
}

CVI_U8 intrplt_u8(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U8 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_U8)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

CVI_U16 intrplt_u16(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U16 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_U16)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

CVI_U32 intrplt_u32(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_U32 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_U32)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

CVI_S8 intrplt_s8(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S8 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_S8)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

CVI_S16 intrplt_s16(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S16 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_S16)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

CVI_S32 intrplt_s32(VI_PIPE ViPipe, enum INTPLT_TYPE type, const CVI_S32 *arr)
{
	CVI_S32 val, val1, val2, idx1, idx2;

	get_interpolate_info(ViPipe, type, &val, &val1, &val2, &idx1, &idx2);

	return (CVI_S32)interpolate_iso_imp(val, val1, (CVI_S32)arr[idx1],
		val2, (CVI_S32)arr[idx2]);
}

static inline CVI_VOID get_interpolate_info(VI_PIPE ViPipe,
	enum INTPLT_TYPE type, CVI_S32 *val, CVI_S32 *val1, CVI_S32 *val2,
	CVI_S32 *idx1, CVI_S32 *idx2)
{
	struct isp_interpolate_runtime *runtime = _get_interpolate_runtime(ViPipe);

	#define INTERPOLATE_GET_INDEX(_type, _lut) \
	{\
		*val = runtime->_type;\
		*val1 = _lut[runtime->_type##_lower_idx];\
		*val2 = _lut[runtime->_type##_upper_idx];\
		*idx1 = runtime->_type##_lower_idx;\
		*idx2 = runtime->_type##_upper_idx;\
	}

	switch (type) {
	case INTPLT_PRE_ISO:
	INTERPOLATE_GET_INDEX(pre_iso, iso_lut);
	break;
	case INTPLT_POST_ISO:
	INTERPOLATE_GET_INDEX(post_iso, iso_lut);
	break;
	case INTPLT_PRE_BLCISO:
	INTERPOLATE_GET_INDEX(pre_blc_iso, iso_lut);
	break;
	case INTPLT_POST_BLCISO:
	INTERPOLATE_GET_INDEX(post_blc_iso, iso_lut);
	break;
	case INTPLT_EXP_RATIO:
	INTERPOLATE_GET_INDEX(exp_ratio, expRatioLut);
	break;
	case INTPLT_LV:
	INTERPOLATE_GET_INDEX(lv, lvLut);
	break;
	default:
	INTERPOLATE_GET_INDEX(post_iso, iso_lut);
	break;
	}
}

static inline CVI_S32 interpolate_iso_imp(CVI_S32 v, CVI_S32 x0, CVI_S32 y0, CVI_S32 x1, CVI_S32 y1)
{
	if (v <= x0)
		return y0;
	if (v >= x1)
		return y1;
	if ((x1 - x0) == 0)
		return y0;
	return (CVI_S64)(y1 - y0) * (CVI_S64)(v - x0) / (x1 - x0) + y0;
}

CVI_S32 isp_interpolate_update(VI_PIPE ViPipe, CVI_U32 pre_iso, CVI_U32 post_iso,
	CVI_U32 pre_blc_iso, CVI_U32 post_blc_iso, CVI_U32 exp_ratio, CVI_S16 lv)
{
	struct isp_interpolate_runtime *runtime = _get_interpolate_runtime(ViPipe);

	CVI_U8 upper_idx, lower_idx;

	#define INTERPOLATE_INDEX_UPDATE(_type, _lut, _lut_num) \
	{\
		if (runtime->_type != _type) {\
			runtime->_type = _type;\
			for (upper_idx = 0 ; upper_idx < _lut_num - 1 ; upper_idx++) {\
				if (_type <= _lut[upper_idx])\
					break;\
			} \
			lower_idx = MAX(upper_idx-1, 0);\
			runtime->_type##_upper_idx = upper_idx;\
			runtime->_type##_lower_idx = lower_idx;\
		} \
	}

	INTERPOLATE_INDEX_UPDATE(pre_iso, iso_lut, ISP_AUTO_ISO_STRENGTH_NUM);
	INTERPOLATE_INDEX_UPDATE(post_iso, iso_lut, ISP_AUTO_ISO_STRENGTH_NUM);
	INTERPOLATE_INDEX_UPDATE(pre_blc_iso, iso_lut, ISP_AUTO_ISO_STRENGTH_NUM);
	INTERPOLATE_INDEX_UPDATE(post_blc_iso, iso_lut, ISP_AUTO_ISO_STRENGTH_NUM);
	INTERPOLATE_INDEX_UPDATE(exp_ratio, expRatioLut, ISP_AUTO_EXP_RATIO_NUM);
	INTERPOLATE_INDEX_UPDATE(lv, lvLut, ISP_AUTO_LV_NUM);

	return CVI_SUCCESS;
}

static struct isp_interpolate_runtime  *_get_interpolate_runtime(VI_PIPE ViPipe)
{
	CVI_BOOL isVipipeValid = ((ViPipe >= 0) && (ViPipe < VI_MAX_PIPE_NUM));

	if (!isVipipeValid) {
		ISP_LOG_WARNING("Wrong ViPipe(%d)\n", ViPipe);
		return NULL;
	}

	return &interpolate_runtime[ViPipe];
}
