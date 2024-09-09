/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_json_struct.c
 * Description:
 *
 */

#include "isp_json_struct.h"

// -----------------------------------------------------------------------------
// Common enumeration
// -----------------------------------------------------------------------------
static void ISP_OP_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_OP_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, OP_TYPE_AUTO, OP_TYPE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
// AE enumeration
// -----------------------------------------------------------------------------
static void ISP_AE_METER_MODE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_METER_MODE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_METER_MULTI, AE_METER_FISHEYE);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AE_STRATEGY_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_STRATEGY_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_EXP_HIGHLIGHT_PRIOR, AE_STRATEGY_MODE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AE_MODE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_MODE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_MODE_SLOW_SHUTTER, AE_MODE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AE_ANTIFLICKER_FREQUENCE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ANTIFLICKER_FREQUENCE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_FREQUENCE_60HZ, AE_FREQUENCE_50HZ);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_ANTIFLICKER_MODE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_ANTIFLICKER_MODE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_ANTIFLICKER_NORMAL_MODE, ISP_ANTIFLICKER_MODE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AE_IR_CUT_FORCE_STATUS_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_IR_CUT_FORCE_STATUS *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_IR_CUT_FORCE_AUTO, AE_IR_CUT_FORCE_OFF);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_IRIS_F_NO_E_JSON(int r_w_flag, JSON *j, char *key, ISP_IRIS_F_NO_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_IRIS_F_NO_32_0, ISP_IRIS_F_NO_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

static void ISP_AE_GAIN_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_GAIN_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AE_TYPE_GAIN, AE_TYPE_ISO);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_IRIS_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_IRIS_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_IRIS_DC_TYPE, ISP_IRIS_TYPE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_IRIS_STATUS_E_JSON(int r_w_flag, JSON *j, char *key, ISP_IRIS_STATUS_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_IRIS_KEEP, ISP_IRIS_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
// AWB enumeration
// -----------------------------------------------------------------------------
static void ISP_AWB_ALG_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_ALG_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AWB_ALG_LOWCOST, AWB_ALG_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AWB_ALG_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_ALG_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ALG_AWB, ALG_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AWB_MULTI_LS_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_MULTI_LS_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AWB_MULTI_LS_SAT, AWB_MULTI_LS_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AWB_INDOOR_OUTDOOR_STATUS_E_JSON(int r_w_flag, JSON *j, char *key,
	ISP_AWB_INDOOR_OUTDOOR_STATUS_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, AWB_INDOOR_MODE, AWB_INDOOR_OUTDOOR_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_AWB_SWITCH_E_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_SWITCH_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_AWB_AFTER_DG, ISP_AWB_SWITCH_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
// ISP enumeration
// -----------------------------------------------------------------------------
static void DIS_MODE_E_JSON(int r_w_flag, JSON *j, char *key, DIS_MODE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, DIS_MODE_2_DOF_GME, DIS_MODE_DOF_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void DIS_MOTION_LEVEL_E_JSON(int r_w_flag, JSON *j, char *key, DIS_MOTION_LEVEL_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, DIS_MOTION_LEVEL_NORMAL, DIS_MOTION_LEVEL_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_STATUS_E_JSON(int r_w_flag, JSON *j, char *key, ISP_STATUS_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_STATUS_INIT, ISP_STATUS_SIZE);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void CVI_STATIC_DP_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, CVI_STATIC_DP_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_STATIC_DP_BRIGHT, ISP_STATIC_DP_DARK);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_GAMMA_CURVE_TYPE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_GAMMA_CURVE_TYPE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_GAMMA_CURVE_DEFAULT, ISP_GAMMA_CURVE_MAX);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_FSWDR_MODE_E_JSON(int r_w_flag, JSON *j, char *key, ISP_FSWDR_MODE_E *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_FSWDR_NORMAL_MODE, ISP_FSWDR_MODE_BUTT);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

static void ISP_CSC_COLORGAMUT_JSON(int r_w_flag, JSON *j, char *key, ISP_CSC_COLORGAMUT *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, ISP_CSC_COLORGAMUT_BT601, ISP_CSC_COLORGAMUT_NUM);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

// -----------------------------------------------------------------------------
void ISP_PUB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_PUB_ATTR_S *data)
{
	JSON_START(r_w_flag);

	/*only read and write f32FrameRate, not to add other members of ISP_PUB_ATTR_S*/

	//JSON(r_w_flag, RECT_S, stWndRect);
	//JSON(r_w_flag, SIZE_S, stSnsSize);
	JSON(r_w_flag, CVI_FLOAT, f32FrameRate);
	//JSON(r_w_flag, ISP_BAYER_FORMAT_E, enBayer);
	//JSON(r_w_flag, WDR_MODE_E, enWDRMode);
	//JSON(r_w_flag, CVI_U8, u8SnsMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP Pre-RAW
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ISP structure - BLC
// -----------------------------------------------------------------------------
static void ISP_BLACK_LEVEL_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_BLACK_LEVEL_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, OffsetR);
	JSON(r_w_flag, CVI_U16, OffsetGr);
	JSON(r_w_flag, CVI_U16, OffsetGb);
	JSON(r_w_flag, CVI_U16, OffsetB);
	JSON(r_w_flag, CVI_U16, OffsetR2);
	JSON(r_w_flag, CVI_U16, OffsetGr2);
	JSON(r_w_flag, CVI_U16, OffsetGb2);
	JSON(r_w_flag, CVI_U16, OffsetB2);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_BLACK_LEVEL_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_BLACK_LEVEL_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, OffsetR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetGr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetGb, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetB, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetR2, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetGr2, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetGb2, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, OffsetB2, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_BLACK_LEVEL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_BLACK_LEVEL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_BLACK_LEVEL_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_BLACK_LEVEL_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - DP
// -----------------------------------------------------------------------------
static void ISP_DP_DYNAMIC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_DYNAMIC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, ClusterSize);
	JSON(r_w_flag, CVI_U8, BrightDefectToNormalPixRatio);
	JSON(r_w_flag, CVI_U8, DarkDefectToNormalPixRatio);
	JSON(r_w_flag, CVI_U8, FlatThreR);
	JSON(r_w_flag, CVI_U8, FlatThreG);
	JSON(r_w_flag, CVI_U8, FlatThreB);
	JSON(r_w_flag, CVI_U8, FlatThreMinG);
	JSON(r_w_flag, CVI_U8, FlatThreMinRB);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DP_DYNAMIC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_DYNAMIC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, ClusterSize, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BrightDefectToNormalPixRatio, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DarkDefectToNormalPixRatio, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FlatThreR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FlatThreG, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FlatThreB, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FlatThreMinG, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FlatThreMinRB, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DP_DYNAMIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_DYNAMIC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_U32, DynamicDPCEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_DP_DYNAMIC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DP_DYNAMIC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DP_STATIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_STATIC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_U16, BrightCount);
	JSON(r_w_flag, CVI_U16, DarkCount);
	JSON_A(r_w_flag, CVI_U32, BrightTable, STATIC_DP_COUNT_MAX);
	JSON_A(r_w_flag, CVI_U32, DarkTable, STATIC_DP_COUNT_MAX);
	JSON(r_w_flag, CVI_BOOL, Show);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DP_CALIB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DP_CALIB_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, EnableDetect);
	JSON(r_w_flag, CVI_STATIC_DP_TYPE_E, StaticDPType);
	JSON(r_w_flag, CVI_U8, StartThresh);
	JSON(r_w_flag, CVI_U16, CountMax);
	JSON(r_w_flag, CVI_U16, CountMin);
	JSON(r_w_flag, CVI_U16, TimeLimit);
	JSON(r_w_flag, CVI_BOOL, saveFileEn);
	JSON_A(r_w_flag, CVI_U32, Table, STATIC_DP_COUNT_MAX);
	JSON(r_w_flag, CVI_U8, FinishThresh);
	JSON(r_w_flag, CVI_U16, Count);
	JSON(r_w_flag, ISP_STATUS_E, Status);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Crosstalk
// -----------------------------------------------------------------------------
static void ISP_CROSSTALK_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CROSSTALK_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, Strength);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CROSSTALK_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CROSSTALK_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, Strength, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CROSSTALK_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CROSSTALK_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON_A(r_w_flag, CVI_U16, GrGbDiffThreSec, 4);
	JSON_A(r_w_flag, CVI_U16, FlatThre, 4);
	JSON(r_w_flag, ISP_CROSSTALK_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CROSSTALK_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP RAW-Top
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ISP structure - NR
// -----------------------------------------------------------------------------
static void ISP_NR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, WindowType);
	JSON(r_w_flag, CVI_U8, DetailSmoothMode);
	JSON(r_w_flag, CVI_U8, NoiseSuppressStr);
	JSON(r_w_flag, CVI_U8, FilterType);
	JSON(r_w_flag, CVI_U8, NoiseSuppressStrMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_NR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, WindowType, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailSmoothMode, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseSuppressStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FilterType, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseSuppressStrMode, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, CoringParamEnable);
	JSON(r_w_flag, ISP_NR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_NR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_NR_FILTER_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_FILTER_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaStr, 8);
	JSON(r_w_flag, CVI_U8, VarThr);
	JSON(r_w_flag, CVI_U16, CoringWgtLF);
	JSON(r_w_flag, CVI_U16, CoringWgtHF);
	JSON(r_w_flag, CVI_U8, NonDirFiltStr);
	JSON(r_w_flag, CVI_U8, VhDirFiltStr);
	JSON(r_w_flag, CVI_U8, AaDirFiltStr);
	JSON(r_w_flag, CVI_U16, NpSlopeR);
	JSON(r_w_flag, CVI_U16, NpSlopeGr);
	JSON(r_w_flag, CVI_U16, NpSlopeGb);
	JSON(r_w_flag, CVI_U16, NpSlopeB);
	JSON(r_w_flag, CVI_U16, NpLumaThrR);
	JSON(r_w_flag, CVI_U16, NpLumaThrGr);
	JSON(r_w_flag, CVI_U16, NpLumaThrGb);
	JSON(r_w_flag, CVI_U16, NpLumaThrB);
	JSON(r_w_flag, CVI_U16, NpLowOffsetR);
	JSON(r_w_flag, CVI_U16, NpLowOffsetGr);
	JSON(r_w_flag, CVI_U16, NpLowOffsetGb);
	JSON(r_w_flag, CVI_U16, NpLowOffsetB);
	JSON(r_w_flag, CVI_U16, NpHighOffsetR);
	JSON(r_w_flag, CVI_U16, NpHighOffsetGr);
	JSON(r_w_flag, CVI_U16, NpHighOffsetGb);
	JSON(r_w_flag, CVI_U16, NpHighOffsetB);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_NR_FILTER_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_FILTER_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaStr, 8 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, VarThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, CoringWgtLF, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, CoringWgtHF, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NonDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, VhDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, AaDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpSlopeR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpSlopeGr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpSlopeGb, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpSlopeB, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLumaThrR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLumaThrGr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLumaThrGb, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLumaThrB, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLowOffsetR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLowOffsetGr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLowOffsetGb, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpLowOffsetB, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpHighOffsetR, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpHighOffsetGr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpHighOffsetGb, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, NpHighOffsetB, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_NR_FILTER_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_NR_FILTER_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, ISP_NR_FILTER_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_NR_FILTER_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Demosaic
// -----------------------------------------------------------------------------
static void ISP_DEMOSAIC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, CoarseEdgeThr);
	JSON(r_w_flag, CVI_U16, CoarseStr);
	JSON(r_w_flag, CVI_U16, FineEdgeThr);
	JSON(r_w_flag, CVI_U16, FineStr);
	JSON(r_w_flag, CVI_U16, RbSigLumaThd);
	JSON(r_w_flag, CVI_U8, FilterMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DEMOSAIC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, CoarseEdgeThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, CoarseStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, FineEdgeThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, FineStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, RbSigLumaThd, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FilterMode, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DEMOSAIC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_BOOL, TuningMode);
	JSON(r_w_flag, CVI_BOOL, RbVtEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_DEMOSAIC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DEMOSAIC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DEMOSAIC_DEMOIRE_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_DEMOSAIC_DEMOIRE_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, AntiFalseColorStr);
	JSON_A(r_w_flag, CVI_U16, SatGainIn, 2);
	JSON_A(r_w_flag, CVI_U16, SatGainOut, 2);
	JSON_A(r_w_flag, CVI_U16, ProtectColorGainIn, 2);
	JSON_A(r_w_flag, CVI_U16, ProtectColorGainOut, 2);
	JSON(r_w_flag, CVI_U16, UserDefineProtectColor1);
	JSON(r_w_flag, CVI_U16, UserDefineProtectColor2);
	JSON(r_w_flag, CVI_U16, UserDefineProtectColor3);
	JSON_A(r_w_flag, CVI_U16, EdgeGainIn, 2);
	JSON_A(r_w_flag, CVI_U16, EdgeGainOut, 2);
	JSON_A(r_w_flag, CVI_U16, DetailGainIn, 2);
	JSON_A(r_w_flag, CVI_U16, DetailGaintOut, 2);
	JSON(r_w_flag, CVI_U16, DetailDetectLumaStr);
	JSON(r_w_flag, CVI_U8, DetailSmoothStr);
	JSON(r_w_flag, CVI_U8, DetailWgtThr);
	JSON(r_w_flag, CVI_U16, DetailWgtMin);
	JSON(r_w_flag, CVI_U16, DetailWgtMax);
	JSON(r_w_flag, CVI_U16, DetailWgtSlope);
	JSON(r_w_flag, CVI_U8, EdgeWgtNp);
	JSON(r_w_flag, CVI_U8, EdgeWgtThr);
	JSON(r_w_flag, CVI_U16, EdgeWgtMin);
	JSON(r_w_flag, CVI_U16, EdgeWgtMax);
	JSON(r_w_flag, CVI_U16, EdgeWgtSlope);
	JSON(r_w_flag, CVI_U8, DetailSmoothMapTh);
	JSON(r_w_flag, CVI_U16, DetailSmoothMapMin);
	JSON(r_w_flag, CVI_U16, DetailSmoothMapMax);
	JSON(r_w_flag, CVI_U16, DetailSmoothMapSlope);
	JSON(r_w_flag, CVI_U8, LumaWgt);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DEMOSAIC_DEMOIRE_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_DEMOSAIC_DEMOIRE_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, AntiFalseColorStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, SatGainIn, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, SatGainOut, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, ProtectColorGainIn, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, ProtectColorGainOut, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, UserDefineProtectColor1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, UserDefineProtectColor2, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, UserDefineProtectColor3, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeGainIn, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeGainOut, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailGainIn, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailGaintOut, 2 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailDetectLumaStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailSmoothStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailWgtThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailWgtMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailWgtMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailWgtSlope, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeWgtNp, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeWgtThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeWgtMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeWgtMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeWgtSlope, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailSmoothMapTh, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailSmoothMapMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailSmoothMapMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DetailSmoothMapSlope, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaWgt, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DEMOSAIC_DEMOIRE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEMOSAIC_DEMOIRE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, AntiFalseColorEnable);
	JSON(r_w_flag, CVI_BOOL, ProtectColorEnable);
	JSON(r_w_flag, CVI_BOOL, DetailDetectLumaEnable);
	JSON(r_w_flag, CVI_BOOL, DetailSmoothEnable);
	JSON(r_w_flag, CVI_BOOL, DetailMode);
	JSON(r_w_flag, ISP_DEMOSAIC_DEMOIRE_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DEMOSAIC_DEMOIRE_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - RGBCAC
// -----------------------------------------------------------------------------
static void ISP_RGBCAC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_RGBCAC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, DePurpleStr0);
	JSON(r_w_flag, CVI_U8, DePurpleStr1);
	JSON(r_w_flag, CVI_U16, EdgeCoring);
	JSON(r_w_flag, CVI_U8, DePurpleCrStr0);
	JSON(r_w_flag, CVI_U8, DePurpleCbStr0);
	JSON(r_w_flag, CVI_U8, DePurpleCrStr1);
	JSON(r_w_flag, CVI_U8, DePurpleCbStr1);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_RGBCAC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_RGBCAC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, DePurpleStr0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleStr1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, EdgeCoring, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCrStr0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCbStr0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCrStr1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCbStr1, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_RGBCAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_RGBCAC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, PurpleDetRange0);
	JSON(r_w_flag, CVI_U8, PurpleDetRange1);
	JSON(r_w_flag, CVI_U8, DePurpleStrMax0);
	JSON(r_w_flag, CVI_U8, DePurpleStrMin0);
	JSON(r_w_flag, CVI_U8, DePurpleStrMax1);
	JSON(r_w_flag, CVI_U8, DePurpleStrMin1);
	JSON(r_w_flag, CVI_U16, EdgeGlobalGain);
	JSON_A(r_w_flag, CVI_U8, EdgeGainIn, 3);
	JSON_A(r_w_flag, CVI_U8, EdgeGainOut, 3);
	JSON(r_w_flag, CVI_U16, LumaScale);
	JSON(r_w_flag, CVI_U16, UserDefineLuma);
	JSON(r_w_flag, CVI_U8, LumaBlendWgt);
	JSON(r_w_flag, CVI_U8, LumaBlendWgt2);
	JSON(r_w_flag, CVI_U8, LumaBlendWgt3);
	JSON(r_w_flag, CVI_U16, PurpleCb);
	JSON(r_w_flag, CVI_U16, PurpleCr);
	JSON(r_w_flag, CVI_U16, PurpleCb2);
	JSON(r_w_flag, CVI_U16, PurpleCr2);
	JSON(r_w_flag, CVI_U16, PurpleCb3);
	JSON(r_w_flag, CVI_U16, PurpleCr3);
	JSON(r_w_flag, CVI_U16, GreenCb);
	JSON(r_w_flag, CVI_U16, GreenCr);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, ISP_RGBCAC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_RGBCAC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - LCAC
// -----------------------------------------------------------------------------
static void ISP_LCAC_GAUSS_COEF_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LCAC_GAUSS_COEF_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, Wgt);
	JSON(r_w_flag, CVI_U8, Sigma);

	JSON_END(r_w_flag);
}

static void ISP_LCAC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LCAC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, DePurpleCrGain);
	JSON(r_w_flag, CVI_U16, DePurpleCbGain);
	JSON(r_w_flag, CVI_U8, DePurepleCrWgt0);
	JSON(r_w_flag, CVI_U8, DePurepleCbWgt0);
	JSON(r_w_flag, CVI_U8, DePurepleCrWgt1);
	JSON(r_w_flag, CVI_U8, DePurepleCbWgt1);
	JSON(r_w_flag, CVI_U8, EdgeCoringBase);
	JSON(r_w_flag, CVI_U8, EdgeCoringAdv);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_LCAC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LCAC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, DePurpleCrGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DePurpleCbGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurepleCrWgt0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurepleCbWgt0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurepleCrWgt1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurepleCbWgt1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeCoringBase, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeCoringAdv, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_LCAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LCAC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, CVI_U8, DePurpleCrStr0);
	JSON(r_w_flag, CVI_U8, DePurpleCbStr0);
	JSON(r_w_flag, CVI_U8, DePurpleCrStr1);
	JSON(r_w_flag, CVI_U8, DePurpleCbStr1);
	JSON(r_w_flag, CVI_U8, FilterTypeBase);
	JSON(r_w_flag, CVI_U8, EdgeGainBase0);
	JSON(r_w_flag, CVI_U8, EdgeGainBase1);
	JSON(r_w_flag, CVI_U8, EdgeStrWgtBase);
	JSON(r_w_flag, CVI_U8, DePurpleStrMaxBase);
	JSON(r_w_flag, CVI_U8, DePurpleStrMinBase);
	JSON(r_w_flag, CVI_U8, FilterScaleAdv);
	JSON(r_w_flag, CVI_U8, LumaWgt);
	JSON(r_w_flag, CVI_U8, FilterTypeAdv);
	JSON(r_w_flag, CVI_U8, EdgeGainAdv0);
	JSON(r_w_flag, CVI_U8, EdgeGainAdv1);
	JSON(r_w_flag, CVI_U8, EdgeStrWgtAdvG);
	JSON(r_w_flag, CVI_U8, DePurpleStrMaxAdv);
	JSON(r_w_flag, CVI_U8, DePurpleStrMinAdv);
	JSON(r_w_flag, ISP_LCAC_GAUSS_COEF_ATTR_S, EdgeWgtBase);
	JSON(r_w_flag, ISP_LCAC_GAUSS_COEF_ATTR_S, EdgeWgtAdv);
	JSON(r_w_flag, ISP_LCAC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_LCAC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - DIS
// -----------------------------------------------------------------------------
void ISP_DIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DIS_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, enable);
	JSON(r_w_flag, CVI_U32, movingSubjectLevel);
	JSON(r_w_flag, CVI_U32, horizontalLimit);
	JSON(r_w_flag, CVI_U32, verticalLimit);
	JSON(r_w_flag, CVI_BOOL, stillCrop);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DIS_CONFIG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DIS_CONFIG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, DIS_MODE_E, mode);
	JSON(r_w_flag, DIS_MOTION_LEVEL_E, motionLevel);
	JSON(r_w_flag, CVI_U32, cropRatio);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP RGB-Top
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ISP structure - MLSC
// -----------------------------------------------------------------------------
static void ISP_MESH_SHADING_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MESH_SHADING_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, MeshStr);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_MESH_SHADING_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MESH_SHADING_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, MeshStr, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_MESH_SHADING_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MESH_SHADING_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, OverflowProtection);
	JSON(r_w_flag, ISP_MESH_SHADING_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_MESH_SHADING_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

int ISP_MESH_SHADING_GAIN_LUT_ATTR_S_GET_MAX_SIZE()
{
	int gain_lut_size = 0, attr_size = 0;

	gain_lut_size += GET_OBJECT_STRING_SIZE("ColorTemperature", 5);
	gain_lut_size += GET_ARRAY_STRING_SIZE("RGain", CVI_ISP_LSC_GRID_POINTS, 4);
	gain_lut_size += GET_ARRAY_STRING_SIZE("GGain", CVI_ISP_LSC_GRID_POINTS, 4);
	gain_lut_size += GET_ARRAY_STRING_SIZE("BGain", CVI_ISP_LSC_GRID_POINTS, 4);

	attr_size += GET_OBJECT_STRING_SIZE("Size", 2);
	attr_size += GET_ARRAY_STRING_SIZE("LscGainLut", ISP_MLSC_COLOR_TEMPERATURE_SIZE, gain_lut_size);

	return attr_size;
}

void ISP_MESH_SHADING_GAIN_LUT_S_WRITE_JSON(ISP_MESH_SHADING_GAIN_LUT_S *plutAttr, char *json_str)
{
	char tmpstr[64] = {0};

	JSON_EX_START(json_str);
	snprintf(tmpstr, sizeof(tmpstr),"%c%s%c: %d, ", '"', "ColorTemperature", '"', plutAttr->ColorTemperature);
	strcat(json_str, tmpstr);
	PARSE_INT_ARRAY_TO_JSON_STR("RGain", plutAttr->RGain, CVI_ISP_LSC_GRID_POINTS, json_str);
	strcat(json_str, ", ");
	PARSE_INT_ARRAY_TO_JSON_STR("GGain", plutAttr->GGain, CVI_ISP_LSC_GRID_POINTS, json_str);
	strcat(json_str, ", ");
	PARSE_INT_ARRAY_TO_JSON_STR("BGain", plutAttr->BGain, CVI_ISP_LSC_GRID_POINTS, json_str);
	JSON_EX_END(json_str);

}

int ISP_MESH_SHADING_GAIN_LUT_ATTR_S_WRITE_JSON(ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pAttr, char *json_str)
{
	char tmpstr[64] = {0};

	JSON_EX_START(json_str);
	snprintf(tmpstr, sizeof(tmpstr),"%c%s%c: %d, ", '"', "Size", '"', pAttr->Size);
	strcat(json_str, tmpstr);

	snprintf(tmpstr, sizeof(tmpstr),"%c%s%c: [ ", '"', "LscGainLut", '"');
	strcat(json_str, tmpstr);

	ISP_MESH_SHADING_GAIN_LUT_S_WRITE_JSON(&pAttr->LscGainLut[0], json_str);
	for(int i = 1; i < ISP_MLSC_COLOR_TEMPERATURE_SIZE; i++){
		strcat(json_str, ", ");
		ISP_MESH_SHADING_GAIN_LUT_S_WRITE_JSON(&pAttr->LscGainLut[i], json_str);
	}
	strcat(json_str, " ]");
	JSON_EX_END(json_str);

	return strlen(json_str);
}

char *ISP_MESH_SHADING_GAIN_LUT_S_READ_JSON(ISP_MESH_SHADING_GAIN_LUT_S *pAttr, char *json_str)
{
	char *tmpStr = json_str;
	char *valPos = NULL;

	PARSE_JSON_STR_TO_INT_VALUE(ColorTemperature, json_str, pAttr);
	PARSE_JSON_STR_TO_INT_ARRAY(RGain, tmpStr, pAttr, CVI_ISP_LSC_GRID_POINTS);
	PARSE_JSON_STR_TO_INT_ARRAY(GGain, tmpStr, pAttr, CVI_ISP_LSC_GRID_POINTS);
	PARSE_JSON_STR_TO_INT_ARRAY(BGain, tmpStr, pAttr, CVI_ISP_LSC_GRID_POINTS);

	return tmpStr;
}

void ISP_MESH_SHADING_GAIN_LUT_ATTR_S_READ_JSON(ISP_MESH_SHADING_GAIN_LUT_ATTR_S *pAttr, const char *json_str)
{
	char *tmpStr = (char *)json_str;

	PARSE_JSON_STR_TO_INT_VALUE(Size, tmpStr, pAttr);
	tmpStr = strstr(tmpStr, "LscGainLut");

	if (tmpStr) {
		for(int i = 0; i < ISP_MLSC_COLOR_TEMPERATURE_SIZE; i++){
			tmpStr = strstr(tmpStr, "{");
			tmpStr = ISP_MESH_SHADING_GAIN_LUT_S_READ_JSON(&pAttr->LscGainLut[i], tmpStr);
			tmpStr = strstr(tmpStr, "}");
		}
	}
}

// -----------------------------------------------------------------------------
static void ISP_MESH_SHADING_GAIN_LUT_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_MESH_SHADING_GAIN_LUT_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, ColorTemperature);
	JSON_A(r_w_flag, CVI_U16, RGain, CVI_ISP_LSC_GRID_POINTS);
	JSON_A(r_w_flag, CVI_U16, GGain, CVI_ISP_LSC_GRID_POINTS);
	JSON_A(r_w_flag, CVI_U16, BGain, CVI_ISP_LSC_GRID_POINTS);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_MESH_SHADING_GAIN_LUT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_MESH_SHADING_GAIN_LUT_ATTR_S *data)
{
	if (r_w_flag == R_FLAG) {
		JSON_START(r_w_flag);

		if (cvi_json_object_is_type(obj, cvi_json_type_string)) {
			const char *json_str = cvi_json_object_get_string(obj);

			ISP_MESH_SHADING_GAIN_LUT_ATTR_S_READ_JSON(data, json_str);
		} else {
			JSON(r_w_flag, CVI_U8, Size);
			JSON_A(r_w_flag, ISP_MESH_SHADING_GAIN_LUT_S, LscGainLut, ISP_MLSC_COLOR_TEMPERATURE_SIZE);
		}
		JSON_END(r_w_flag);
	} else {
		int max_size = ISP_MESH_SHADING_GAIN_LUT_ATTR_S_GET_MAX_SIZE();
		char *json_str = (char *)malloc(max_size);

		if (json_str == NULL) {
			CVI_TRACE_JSON(LOG_WARNING, "%s\n", "Allocate memory fail");
			return ;
		}
		memset(json_str, 0, max_size);
		int json_len = ISP_MESH_SHADING_GAIN_LUT_ATTR_S_WRITE_JSON(data, json_str);
		JSON *json_obj = cvi_json_object_new_string_len(json_str, json_len);

		cvi_json_object_object_add(j, key, json_obj);
		free(json_str);
	}

}

// -----------------------------------------------------------------------------
// ISP structure - Saturation
// -----------------------------------------------------------------------------
static void ISP_SATURATION_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SATURATION_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, Saturation);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_SATURATION_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SATURATION_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, Saturation, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SATURATION_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, ISP_SATURATION_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_SATURATION_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CCM
// -----------------------------------------------------------------------------
static void ISP_COLORMATRIX_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_COLORMATRIX_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, ColorTemp);
	JSON_A(r_w_flag, CVI_S16, CCM, 9);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CCM_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, SatEnable);
	JSON_A(r_w_flag, CVI_S16, CCM, 9);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CCM_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, ISOActEnable);
	JSON(r_w_flag, CVI_BOOL, TempActEnable);
	JSON(r_w_flag, CVI_U8, CCMTabNum);
	JSON_A(r_w_flag, ISP_COLORMATRIX_ATTR_S, CCMTab, 7);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CCM_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_CCM_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CCM_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CCM_SATURATION_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CCM_SATURATION_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, SaturationLE);
	JSON(r_w_flag, CVI_U8, SaturationSE);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CCM_SATURATION_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CCM_SATURATION_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, SaturationLE, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, SaturationSE, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CCM_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CCM_SATURATION_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_CCM_SATURATION_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CCM_SATURATION_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Colortone
// -----------------------------------------------------------------------------
void ISP_COLOR_TONE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_COLOR_TONE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16RedCastGain);
	JSON(r_w_flag, CVI_U16, u16GreenCastGain);
	JSON(r_w_flag, CVI_U16, u16BlueCastGain);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - FSWDR
// -----------------------------------------------------------------------------
static void ISP_FSWDR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_FSWDR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, WDRCombineLongThr);
	JSON(r_w_flag, CVI_U16, WDRCombineShortThr);
	JSON(r_w_flag, CVI_U16, WDRCombineMaxWeight);
	JSON(r_w_flag, CVI_U16, WDRCombineMinWeight);
	JSON_A(r_w_flag, CVI_U8, WDRMtIn, 4);
	JSON_A(r_w_flag, CVI_U16, WDRMtOut, 4);
	JSON(r_w_flag, CVI_U16, WDRLongWgt);
	JSON(r_w_flag, CVI_U8, WDRCombineSNRAwareToleranceLevel);
	JSON(r_w_flag, CVI_U8, MergeModeAlpha);
	JSON(r_w_flag, CVI_U16, WDRMotionCombineLongThr);
	JSON(r_w_flag, CVI_U16, WDRMotionCombineShortThr);
	JSON(r_w_flag, CVI_U16, WDRMotionCombineMinWeight);
	JSON(r_w_flag, CVI_U16, WDRMotionCombineMaxWeight);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_FSWDR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_FSWDR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, WDRCombineLongThr, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRCombineShortThr, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRCombineMaxWeight, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRCombineMinWeight, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, WDRMtIn, 4 * ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRMtOut, 4 * ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRLongWgt, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, WDRCombineSNRAwareToleranceLevel, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, MergeModeAlpha, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRMotionCombineLongThr, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRMotionCombineShortThr, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRMotionCombineMinWeight, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, WDRMotionCombineMaxWeight, ISP_AUTO_LV_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_FSWDR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_FSWDR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, MotionCompEnable);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, CVI_BOOL, WDRDCMode);
	JSON(r_w_flag, CVI_BOOL, WDRLumaMode);
	JSON(r_w_flag, CVI_U8, WDRType);
	JSON(r_w_flag, CVI_BOOL, WDRCombineSNRAwareEn);
	JSON(r_w_flag, CVI_U16, WDRCombineSNRAwareLowThr);
	JSON(r_w_flag, CVI_U16, WDRCombineSNRAwareHighThr);
	JSON(r_w_flag, CVI_U16, WDRCombineSNRAwareSmoothLevel);
	JSON(r_w_flag, CVI_BOOL, LocalToneRefinedDCMode);
	JSON(r_w_flag, CVI_BOOL, LocalToneRefinedLumaMode);
	JSON(r_w_flag, CVI_U16, DarkToneRefinedThrL);
	JSON(r_w_flag, CVI_U16, DarkToneRefinedThrH);
	JSON(r_w_flag, CVI_U16, DarkToneRefinedMaxWeight);
	JSON(r_w_flag, CVI_U16, DarkToneRefinedMinWeight);
	JSON(r_w_flag, CVI_U16, BrightToneRefinedThrL);
	JSON(r_w_flag, CVI_U16, BrightToneRefinedThrH);
	JSON(r_w_flag, CVI_U16, BrightToneRefinedMaxWeight);
	JSON(r_w_flag, CVI_U16, BrightToneRefinedMinWeight);
	JSON(r_w_flag, CVI_U8, WDRMotionFusionMode);
	JSON(r_w_flag, CVI_BOOL, MtMode);
	JSON(r_w_flag, ISP_FSWDR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_FSWDR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - WDR Exposure
// -----------------------------------------------------------------------------
void ISP_WDR_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WDR_EXPOSURE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_OP_TYPE_E, enExpRatioType);
	JSON_A(r_w_flag, CVI_U32, au32ExpRatio, WDR_EXP_RATIO_NUM);
	JSON(r_w_flag, CVI_U32, u32ExpRatioMax);
	JSON(r_w_flag, CVI_U32, u32ExpRatioMin);
	JSON(r_w_flag, CVI_U16, u16Tolerance);
	JSON(r_w_flag, CVI_U16, u16Speed);
	JSON(r_w_flag, CVI_U16, u16RatioBias);
	JSON(r_w_flag, CVI_U8, u8SECompensation);
	JSON(r_w_flag, CVI_U16, u16SEHisThr);
	JSON(r_w_flag, CVI_U16, u16SEHisCntRatio1);
	JSON(r_w_flag, CVI_U16, u16SEHisCntRatio2);
	JSON(r_w_flag, CVI_U16, u16SEHis255CntThr1);
	JSON(r_w_flag, CVI_U16, u16SEHis255CntThr2);
	JSON_A(r_w_flag, CVI_U8, au8LEAdjustTargetMin, LV_TOTAL_NUM);
	JSON_A(r_w_flag, CVI_U8, au8LEAdjustTargetMax, LV_TOTAL_NUM);
	JSON_A(r_w_flag, CVI_U8, au8SEAdjustTargetMin, LV_TOTAL_NUM);
	JSON_A(r_w_flag, CVI_U8, au8SEAdjustTargetMax, LV_TOTAL_NUM);
	JSON(r_w_flag, CVI_U8, u8AdjustTargetDetectFrmNum);
	JSON(r_w_flag, CVI_U32, u32DiffPixelNum);
	JSON(r_w_flag, CVI_U16, u16LELowBinThr);
	JSON(r_w_flag, CVI_U16, u16LEHighBinThr);
	JSON(r_w_flag, CVI_U16, u16SELowBinThr);
	JSON(r_w_flag, CVI_U16, u16SEHighBinThr);
	JSON_A(r_w_flag, CVI_U8, au8FrameAvgLumaMin, LV_TOTAL_NUM);
	JSON_A(r_w_flag, CVI_U8, au8FrameAvgLumaMax, LV_TOTAL_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - DRC
// -----------------------------------------------------------------------------
static void ISP_DRC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DRC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, TargetYScale);
	JSON(r_w_flag, CVI_U16, HdrStrength);
	JSON(r_w_flag, CVI_U8, DEAdaptPercentile);
	JSON(r_w_flag, CVI_U8, DEAdaptTargetGain);
	JSON(r_w_flag, CVI_U8, DEAdaptGainUB);
	JSON(r_w_flag, CVI_U8, DEAdaptGainLB);
	JSON(r_w_flag, CVI_U8, BritInflectPtLuma);
	JSON(r_w_flag, CVI_U8, BritContrastLow);
	JSON(r_w_flag, CVI_U8, BritContrastHigh);
	JSON(r_w_flag, CVI_U8, SdrTargetY);
	JSON(r_w_flag, CVI_U8, SdrTargetYGain);
	JSON(r_w_flag, CVI_U16, SdrGlobalToneStr);
	JSON(r_w_flag, CVI_U8, SdrDEAdaptPercentile);
	JSON(r_w_flag, CVI_U8, SdrDEAdaptTargetGain);
	JSON(r_w_flag, CVI_U8, SdrDEAdaptGainLB);
	JSON(r_w_flag, CVI_U8, SdrDEAdaptGainUB);
	JSON(r_w_flag, CVI_U8, SdrBritInflectPtLuma);
	JSON(r_w_flag, CVI_U8, SdrBritContrastLow);
	JSON(r_w_flag, CVI_U8, SdrBritContrastHigh);
	JSON(r_w_flag, CVI_U8, TotalGain);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DRC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DRC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U32, TargetYScale, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, HdrStrength, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, DEAdaptPercentile, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, DEAdaptTargetGain, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, DEAdaptGainUB, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, DEAdaptGainLB, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, BritInflectPtLuma, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, BritContrastLow, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, BritContrastHigh, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrTargetY, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrTargetYGain, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, SdrGlobalToneStr, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrDEAdaptPercentile, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrDEAdaptTargetGain, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrDEAdaptGainLB, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrDEAdaptGainUB, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrBritInflectPtLuma, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrBritContrastLow, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, SdrBritContrastHigh, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, TotalGain, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DRC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DRC_ATTR_S *data)
{
	JSON_START(r_w_flag);


	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, CVI_BOOL, LocalToneEn);
	JSON(r_w_flag, CVI_BOOL, LocalToneRefineEn);
	JSON(r_w_flag, CVI_U8, ToneCurveSelect);
	JSON_A(r_w_flag, CVI_U16, CurveUserDefine, DRC_GLOBAL_USER_DEFINE_NUM);
	JSON_A(r_w_flag, CVI_U16, DarkUserDefine, DRC_DARK_USER_DEFINE_NUM);
	JSON_A(r_w_flag, CVI_U16, BrightUserDefine, DRC_BRIGHT_USER_DEFINE_NUM);
	JSON(r_w_flag, CVI_U32, ToneCurveSmooth);
	JSON(r_w_flag, CVI_U8, CoarseFltScale);
	JSON(r_w_flag, CVI_U8, SdrTargetYGainMode);
	JSON(r_w_flag, CVI_BOOL, DetailEnhanceEn);
	JSON_A(r_w_flag, CVI_U8, LumaGain, 33);
	JSON_A(r_w_flag, CVI_U8, DetailEnhanceMtIn, 4);
	JSON_A(r_w_flag, CVI_U16, DetailEnhanceMtOut, 4);
	JSON(r_w_flag, CVI_U8, OverShootThd);
	JSON(r_w_flag, CVI_U8, UnderShootThd);
	JSON(r_w_flag, CVI_U8, OverShootGain);
	JSON(r_w_flag, CVI_U8, UnderShootGain);
	JSON(r_w_flag, CVI_U8, OverShootThdMax);
	JSON(r_w_flag, CVI_U8, UnderShootThdMin);
	JSON(r_w_flag, CVI_BOOL, SoftClampEnable);
	JSON(r_w_flag, CVI_U8, SoftClampUB);
	JSON(r_w_flag, CVI_U8, SoftClampLB);
	JSON(r_w_flag, CVI_BOOL, dbg_182x_sim_enable);
	JSON(r_w_flag, CVI_U8, DarkMapStr);
	JSON(r_w_flag, CVI_U8, BritMapStr);
	JSON(r_w_flag, CVI_U8, SdrDarkMapStr);
	JSON(r_w_flag, CVI_U8, SdrBritMapStr);
	JSON_A(r_w_flag, CVI_U32, DRCMu, 32);
	JSON(r_w_flag, ISP_DRC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DRC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Gamma
// -----------------------------------------------------------------------------
void ISP_GAMMA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_GAMMA_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON_A(r_w_flag, CVI_U16, Table, GAMMA_NODE_NUM);
	JSON(r_w_flag, ISP_GAMMA_CURVE_TYPE_E, enCurveType);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_GAMMA_CURVE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_GAMMA_CURVE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_S16, Lv);
	JSON_A(r_w_flag, CVI_U16, Tbl, GAMMA_NODE_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AUTO_GAMMA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AUTO_GAMMA_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, GammaTabNum);
	JSON_A(r_w_flag, ISP_GAMMA_CURVE_ATTR_S, GammaTab, GAMMA_MAX_INTERPOLATION_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Dehaze
// -----------------------------------------------------------------------------
static void ISP_DEHAZE_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEHAZE_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, Strength);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DEHAZE_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEHAZE_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, Strength, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DEHAZE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DEHAZE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U16, CumulativeThr);
	JSON(r_w_flag, CVI_U16, MinTransMapValue);
	JSON(r_w_flag, CVI_BOOL, DehazeLumaEnable);
	JSON(r_w_flag, CVI_BOOL, DehazeSkinEnable);
	JSON(r_w_flag, CVI_U8, AirLightMixWgt);
	JSON(r_w_flag, CVI_U8, DehazeWgt);
	JSON(r_w_flag, CVI_U8, TransMapScale);
	JSON(r_w_flag, CVI_U8, AirlightDiffWgt);
	JSON(r_w_flag, CVI_U16, AirLightMax);
	JSON(r_w_flag, CVI_U16, AirLightMin);
	JSON(r_w_flag, CVI_U8, SkinCb);
	JSON(r_w_flag, CVI_U8, SkinCr);
	JSON(r_w_flag, CVI_U16, DehazeLumaCOEFFI);
	JSON(r_w_flag, CVI_U16, DehazeSkinCOEFFI);
	JSON(r_w_flag, CVI_U8, TransMapWgtWgt);
	JSON(r_w_flag, CVI_U8, TransMapWgtSigma);
	JSON(r_w_flag, ISP_DEHAZE_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DEHAZE_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CLUT
// -----------------------------------------------------------------------------
int ISP_CLUT_ATTR_S_GET_MAX_SIZE()
{
	int clut_max_size;

	clut_max_size = GET_OBJECT_STRING_SIZE("Enable",1);
	clut_max_size += GET_OBJECT_STRING_SIZE("UpdateInterval",2);
	clut_max_size += GET_ARRAY_STRING_SIZE("ClutR", ISP_CLUT_LUT_LENGTH, 4);
	clut_max_size += GET_ARRAY_STRING_SIZE("ClutG", ISP_CLUT_LUT_LENGTH, 4);
	clut_max_size += GET_ARRAY_STRING_SIZE("ClutB", ISP_CLUT_LUT_LENGTH, 4);

	return clut_max_size;
}

int ISP_CLUT_ATTR_S_WRITE_JSON(ISP_CLUT_ATTR_S *pAttr, char *json_str)
{
	char tmpstr[64] = {0};

	JSON_EX_START(json_str);
	snprintf(tmpstr, sizeof(tmpstr),"%c%s%c: %d, ", '"', "Enable", '"', pAttr->Enable);
	strcat(json_str, tmpstr);

	snprintf(tmpstr, sizeof(tmpstr),"%c%s%c: %d, ", '"', "UpdateInterval", '"', pAttr->UpdateInterval);
	strcat(json_str, tmpstr);

	PARSE_INT_ARRAY_TO_JSON_STR("ClutR", pAttr->ClutR, ISP_CLUT_LUT_LENGTH, json_str);
	strcat(json_str, ", ");
	PARSE_INT_ARRAY_TO_JSON_STR("ClutG", pAttr->ClutG, ISP_CLUT_LUT_LENGTH, json_str);
	strcat(json_str, ", ");
	PARSE_INT_ARRAY_TO_JSON_STR("ClutB", pAttr->ClutB, ISP_CLUT_LUT_LENGTH, json_str);

	JSON_EX_END(json_str);

	return strlen(json_str);
}

char *ISP_CLUT_ATTR_S_READ_JSON(ISP_CLUT_ATTR_S *pAttr, const char *json_str)
{
	char *tmpStr = (char *)json_str;
	char *valPos = NULL;

	PARSE_JSON_STR_TO_INT_VALUE(Enable, json_str, pAttr);
	PARSE_JSON_STR_TO_INT_VALUE(UpdateInterval, json_str, pAttr);
	PARSE_JSON_STR_TO_INT_ARRAY(ClutR, tmpStr, pAttr, ISP_CLUT_LUT_LENGTH);
	PARSE_JSON_STR_TO_INT_ARRAY(ClutG, tmpStr, pAttr, ISP_CLUT_LUT_LENGTH);
	PARSE_JSON_STR_TO_INT_ARRAY(ClutB, tmpStr, pAttr, ISP_CLUT_LUT_LENGTH);

	return tmpStr;
}

void ISP_CLUT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CLUT_ATTR_S *data)
{
	if (r_w_flag == R_FLAG) {
		JSON_START(r_w_flag);

		if (cvi_json_object_is_type(obj, cvi_json_type_string)) {
			const char *json_str = cvi_json_object_get_string(obj);

			ISP_CLUT_ATTR_S_READ_JSON(data, json_str);
		} else {
			JSON(r_w_flag, CVI_BOOL, Enable);
			JSON(r_w_flag, CVI_U8, UpdateInterval);
			JSON_A(r_w_flag, CVI_U16, ClutR, ISP_CLUT_LUT_LENGTH);
			JSON_A(r_w_flag, CVI_U16, ClutG, ISP_CLUT_LUT_LENGTH);
			JSON_A(r_w_flag, CVI_U16, ClutB, ISP_CLUT_LUT_LENGTH);
		}
		JSON_END(r_w_flag);
	} else {
		int max_size = ISP_CLUT_ATTR_S_GET_MAX_SIZE();
		char *json_str = (char *)malloc(max_size);

		if (json_str == NULL) {
			CVI_TRACE_JSON(LOG_WARNING, "%s\n", "Allocate memory fail");
			return ;
		}
		memset(json_str, 0, max_size);
		int json_len = ISP_CLUT_ATTR_S_WRITE_JSON(data, json_str);
		JSON *json_obj = cvi_json_object_new_string_len(json_str, json_len);

		cvi_json_object_object_add(j, key, json_obj);
		free(json_str);
	}
}


// -----------------------------------------------------------------------------
static void ISP_CLUT_SATURATION_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CLUT_SATURATION_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, SatIn, 4);
	JSON_A(r_w_flag, CVI_U16, SatOut, 4);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CLUT_SATURATION_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CLUT_SATURATION_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, SatIn, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, SatOut, 4 * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CLUT_SATURATION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CLUT_SATURATION_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, ISP_CLUT_SATURATION_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CLUT_SATURATION_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CSC
// -----------------------------------------------------------------------------
static void ISP_CSC_MATRX_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CSC_MATRX_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_S16, userCscCoef, CSC_MATRIX_SIZE);
	JSON_A(r_w_flag, CVI_S16, userCscOffset, CSC_OFFSET_SIZE);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CSC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CSC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_CSC_COLORGAMUT, enColorGamut);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, Hue);
	JSON(r_w_flag, CVI_U8, Luma);
	JSON(r_w_flag, CVI_U8, Contrast);
	JSON(r_w_flag, CVI_U8, Saturation);
	JSON(r_w_flag, ISP_CSC_MATRX_S, stUserMatrx);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - VC
// -----------------------------------------------------------------------------
void ISP_VC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_VC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON_A(r_w_flag, CVI_U8, MotionThreshold, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP YUV-Top
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ISP structure - DCI
// -----------------------------------------------------------------------------
static void ISP_DCI_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCI_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, ContrastGain);
	JSON(r_w_flag, CVI_U8, BlcThr);
	JSON(r_w_flag, CVI_U8, WhtThr);
	JSON(r_w_flag, CVI_U16, BlcCtrl);
	JSON(r_w_flag, CVI_U16, WhtCtrl);
	JSON(r_w_flag, CVI_U16, DciGainMax);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_DCI_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCI_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, ContrastGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BlcThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, WhtThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, BlcCtrl, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, WhtCtrl, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, DciGainMax, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DCI_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCI_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_BOOL, TuningMode);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, Method);
	JSON(r_w_flag, CVI_U32, Speed);
	JSON(r_w_flag, CVI_U16, DciStrength);
	JSON(r_w_flag, CVI_U16, DciGamma);
	JSON(r_w_flag, CVI_U8, DciOffset);
	JSON(r_w_flag, CVI_U8, ToleranceY);
	JSON(r_w_flag, CVI_U8, Sensitivity);
	JSON(r_w_flag, ISP_DCI_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_DCI_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - LDCI
// -----------------------------------------------------------------------------
static void ISP_LDCI_GAUSS_COEF_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LDCI_GAUSS_COEF_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, Wgt);
	JSON(r_w_flag, CVI_U8, Sigma);
	JSON(r_w_flag, CVI_U8, Mean);

	JSON_END(r_w_flag);
}

static void ISP_LDCI_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LDCI_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, LdciStrength);
	JSON(r_w_flag, CVI_U16, LdciRange);
	JSON(r_w_flag, CVI_U16, TprCoef);
	JSON(r_w_flag, CVI_U8, EdgeCoring);
	JSON(r_w_flag, CVI_U8, LumaWgtMax);
	JSON(r_w_flag, CVI_U8, LumaWgtMin);
	JSON(r_w_flag, CVI_U8, VarMapMax);
	JSON(r_w_flag, CVI_U8, VarMapMin);
	JSON(r_w_flag, CVI_U8, UvGainMax);
	JSON(r_w_flag, CVI_U8, UvGainMin);
	JSON(r_w_flag, CVI_U8, BrightContrastHigh);
	JSON(r_w_flag, CVI_U8, BrightContrastLow);
	JSON(r_w_flag, CVI_U8, DarkContrastHigh);
	JSON(r_w_flag, CVI_U8, DarkContrastLow);
	JSON(r_w_flag, ISP_LDCI_GAUSS_COEF_ATTR_S, LumaPosWgt);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_LDCI_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LDCI_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, LdciStrength, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, LdciRange, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, TprCoef, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeCoring, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaWgtMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaWgtMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, VarMapMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, VarMapMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UvGainMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UvGainMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BrightContrastHigh, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BrightContrastLow, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DarkContrastHigh, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DarkContrastLow, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, ISP_LDCI_GAUSS_COEF_ATTR_S, LumaPosWgt, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_LDCI_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_LDCI_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, GaussLPFSigma);
	JSON(r_w_flag, ISP_LDCI_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_LDCI_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - PreSharpen
// -----------------------------------------------------------------------------
static void ISP_PRESHARPEN_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_PRESHARPEN_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaAdpGain, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, DeltaAdpGain, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutOut, EE_LUT_NODE);
	JSON(r_w_flag, CVI_U8, GlobalGain);
	JSON(r_w_flag, CVI_U8, OverShootGain);
	JSON(r_w_flag, CVI_U8, UnderShootGain);
	JSON(r_w_flag, CVI_U8, HFBlendWgt);
	JSON(r_w_flag, CVI_U8, MFBlendWgt);
	JSON(r_w_flag, CVI_U8, OverShootThr);
	JSON(r_w_flag, CVI_U8, UnderShootThr);
	JSON(r_w_flag, CVI_U8, OverShootThrMax);
	JSON(r_w_flag, CVI_U8, UnderShootThrMin);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, HueShtCtrl, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, SatShtGainOut, EE_LUT_NODE);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_PRESHARPEN_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_PRESHARPEN_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaAdpGain, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DeltaAdpGain, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GlobalGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, HFBlendWgt, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MFBlendWgt, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootThrMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootThrMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, HueShtCtrl, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_PRESHARPEN_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_PRESHARPEN_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, CVI_BOOL, LumaAdpGainEn);
	JSON(r_w_flag, CVI_BOOL, DeltaAdpGainEn);
	JSON(r_w_flag, CVI_BOOL, NoiseSuppressEnable);
	JSON(r_w_flag, CVI_BOOL, SatShtCtrlEn);
	JSON(r_w_flag, CVI_BOOL, SoftClampEnable);
	JSON(r_w_flag, CVI_U8, SoftClampUB);
	JSON(r_w_flag, CVI_U8, SoftClampLB);
	JSON(r_w_flag, ISP_PRESHARPEN_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_PRESHARPEN_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - TNR
// -----------------------------------------------------------------------------
static void ISP_TNR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, TnrStrength0);
	JSON(r_w_flag, CVI_U8, MapThdLow0);
	JSON(r_w_flag, CVI_U8, MapThdHigh0);
	JSON(r_w_flag, CVI_U8, MtDetectUnit);
	JSON(r_w_flag, CVI_S16, BrightnessNoiseLevelLE);
	JSON(r_w_flag, CVI_S16, BrightnessNoiseLevelSE);
	JSON(r_w_flag, CVI_BOOL, MtFiltMode);
	JSON(r_w_flag, CVI_U16, MtFiltWgt);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, TnrStrength0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MapThdLow0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MapThdHigh0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MtDetectUnit, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_S16, BrightnessNoiseLevelLE, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_S16, BrightnessNoiseLevelSE, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_BOOL, MtFiltMode, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, MtFiltWgt, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, TuningMode);
	JSON(r_w_flag, CVI_BOOL, TnrMtMode);
	JSON(r_w_flag, CVI_BOOL, YnrCnrSharpenMtMode);
	JSON(r_w_flag, CVI_BOOL, PreSharpenMtMode);
	JSON(r_w_flag, CVI_U8, ChromaScalingDownMode);
	JSON(r_w_flag, CVI_BOOL, CompGainEnable);
	JSON(r_w_flag, ISP_TNR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_NOISE_MODEL_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_NOISE_MODEL_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, RNoiseLevel0);
	JSON(r_w_flag, CVI_U8, GNoiseLevel0);
	JSON(r_w_flag, CVI_U8, BNoiseLevel0);
	JSON(r_w_flag, CVI_U8, RNoiseLevel1);
	JSON(r_w_flag, CVI_U8, GNoiseLevel1);
	JSON(r_w_flag, CVI_U8, BNoiseLevel1);
	JSON(r_w_flag, CVI_U8, RNoiseHiLevel0);
	JSON(r_w_flag, CVI_U8, GNoiseHiLevel0);
	JSON(r_w_flag, CVI_U8, BNoiseHiLevel0);
	JSON(r_w_flag, CVI_U8, RNoiseHiLevel1);
	JSON(r_w_flag, CVI_U8, GNoiseHiLevel1);
	JSON(r_w_flag, CVI_U8, BNoiseHiLevel1);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_NOISE_MODEL_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_NOISE_MODEL_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, RNoiseLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GNoiseLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BNoiseLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, RNoiseLevel1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GNoiseLevel1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BNoiseLevel1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, RNoiseHiLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GNoiseHiLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BNoiseHiLevel0, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, RNoiseHiLevel1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GNoiseHiLevel1, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, BNoiseHiLevel1, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_NOISE_MODEL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_NOISE_MODEL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_TNR_NOISE_MODEL_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_NOISE_MODEL_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_LUMA_MOTION_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_LUMA_MOTION_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, L2mIn0, 4);
	JSON_A(r_w_flag, CVI_U8, L2mOut0, 4);
	JSON_A(r_w_flag, CVI_U16, L2mIn1, 4);
	JSON_A(r_w_flag, CVI_U8, L2mOut1, 4);
	JSON(r_w_flag, CVI_BOOL, MtLumaMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_LUMA_MOTION_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_LUMA_MOTION_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, L2mIn0, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, L2mOut0, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, L2mIn1, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, L2mOut1, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_BOOL, MtLumaMode, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_LUMA_MOTION_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_LUMA_MOTION_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_TNR_LUMA_MOTION_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_LUMA_MOTION_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_GHOST_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_GHOST_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, PrvMotion0, 4);
	JSON_A(r_w_flag, CVI_U8, PrtctWgt0, 4);
	JSON(r_w_flag, CVI_U8, MotionHistoryStr);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_GHOST_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_GHOST_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, PrvMotion0, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, PrtctWgt0, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionHistoryStr, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_GHOST_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_GHOST_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_TNR_GHOST_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_GHOST_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_MT_PRT_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MT_PRT_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, LowMtPrtLevelY);
	JSON(r_w_flag, CVI_U8, LowMtPrtLevelU);
	JSON(r_w_flag, CVI_U8, LowMtPrtLevelV);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInY, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInU, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInV, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutY, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutU, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutV, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvIn, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvOut, 4);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_MT_PRT_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MT_PRT_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LowMtPrtLevelY, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtLevelU, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtLevelV, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInY, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInU, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtInV, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutY, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutU, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtOutV, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvIn, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvOut, 4 * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_MT_PRT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MT_PRT_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, LowMtPrtEn);
	JSON(r_w_flag, CVI_BOOL, LowMtLowPassEnable);
	JSON(r_w_flag, CVI_BOOL, LowMtPrtAdvLumaEnable);
	JSON(r_w_flag, CVI_BOOL, LowMtPrtAdvMode);
	JSON(r_w_flag, CVI_U8, LowMtPrtAdvMax);
	JSON(r_w_flag, CVI_BOOL, LowMtPrtAdvDebugMode);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvDebugIn, 4);
	JSON_A(r_w_flag, CVI_U8, LowMtPrtAdvDebugOut, 4);

	JSON(r_w_flag, ISP_TNR_MT_PRT_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_MT_PRT_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_MOTION_ADAPT_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_MOTION_ADAPT_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, AdaptNrLumaStrIn, 4);
	JSON_A(r_w_flag, CVI_U8, AdaptNrLumaStrOut, 4);
	JSON_A(r_w_flag, CVI_U8, AdaptNrChromaStrIn, 4);
	JSON_A(r_w_flag, CVI_U8, AdaptNrChromaStrOut, 4);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_TNR_MOTION_ADAPT_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_TNR_MOTION_ADAPT_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, AdaptNrLumaStrIn, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, AdaptNrLumaStrOut, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, AdaptNrChromaStrIn, 4 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, AdaptNrChromaStrOut, 4 * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_TNR_MOTION_ADAPT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_TNR_MOTION_ADAPT_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_TNR_MOTION_ADAPT_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_TNR_MOTION_ADAPT_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - YNR
// -----------------------------------------------------------------------------
static void ISP_YNR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, WindowType);
	JSON(r_w_flag, CVI_U8, DetailSmoothMode);
	JSON(r_w_flag, CVI_U8, NoiseSuppressStr);
	JSON(r_w_flag, CVI_U8, FilterType);
	JSON(r_w_flag, CVI_U8, NoiseCoringMax);
	JSON(r_w_flag, CVI_U8, NoiseCoringBase);
	JSON(r_w_flag, CVI_U8, NoiseCoringAdv);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YNR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, WindowType, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailSmoothMode, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseSuppressStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FilterType, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseCoringMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseCoringBase, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseCoringAdv, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_YNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, CoringParamEnable);
	JSON(r_w_flag, CVI_BOOL, FiltModeEnable);
	JSON(r_w_flag, CVI_U16, FiltMode);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, ISP_YNR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_YNR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YNR_FILTER_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_FILTER_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, VarThr);
	JSON(r_w_flag, CVI_U16, CoringWgtLF);
	JSON(r_w_flag, CVI_U16, CoringWgtHF);
	JSON(r_w_flag, CVI_U8, NonDirFiltStr);
	JSON(r_w_flag, CVI_U8, VhDirFiltStr);
	JSON(r_w_flag, CVI_U8, AaDirFiltStr);
	JSON(r_w_flag, CVI_U8, CoringWgtMax);
	JSON(r_w_flag, CVI_U16, FilterMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YNR_FILTER_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_FILTER_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, VarThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, CoringWgtLF, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, CoringWgtHF, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NonDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, VhDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, AaDirFiltStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, CoringWgtMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, FilterMode, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_YNR_FILTER_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_FILTER_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_YNR_FILTER_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_YNR_FILTER_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YNR_MOTION_NR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_YNR_MOTION_NR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, MotionCoringWgtMax);
	JSON_A(r_w_flag, CVI_U16, MotionYnrLut, 16);
	JSON_A(r_w_flag, CVI_U16, MotionCoringWgt, 16);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YNR_MOTION_NR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_YNR_MOTION_NR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, MotionCoringWgtMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, MotionYnrLut, 16 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, MotionCoringWgt, 16 * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_YNR_MOTION_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YNR_MOTION_NR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_YNR_MOTION_NR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_YNR_MOTION_NR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CNR
// -----------------------------------------------------------------------------
static void ISP_CNR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, CnrStr);
	JSON(r_w_flag, CVI_U8, NoiseSuppressStr);
	JSON(r_w_flag, CVI_U8, NoiseSuppressGain);
	JSON(r_w_flag, CVI_U8, FilterType);
	JSON(r_w_flag, CVI_U8, MotionNrStr);
	JSON(r_w_flag, CVI_U8, LumaWgt);
	JSON(r_w_flag, CVI_U8, DetailSmoothMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CNR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, CnrStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseSuppressStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, NoiseSuppressGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, FilterType, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionNrStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaWgt, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DetailSmoothMode, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CNR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_CNR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CNR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CNR_MOTION_NR_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CNR_MOTION_NR_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, MotionCnrCoringLut, 16);
	JSON_A(r_w_flag, CVI_U8, MotionCnrStrLut, 16);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CNR_MOTION_NR_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_CNR_MOTION_NR_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, MotionCnrCoringLut, 16 * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCnrStrLut, 16 * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CNR_MOTION_NR_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CNR_MOTION_NR_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, MotionCnrEnable);
	JSON(r_w_flag, ISP_CNR_MOTION_NR_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CNR_MOTION_NR_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CAC
// -----------------------------------------------------------------------------
static void ISP_CAC_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CAC_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, DePurpleStr);
	JSON(r_w_flag, CVI_U8, EdgeGlobalGain);
	JSON(r_w_flag, CVI_U8, EdgeCoring);
	JSON(r_w_flag, CVI_U8, EdgeStrMin);
	JSON(r_w_flag, CVI_U8, EdgeStrMax);
	JSON(r_w_flag, CVI_U8, DePurpleCbStr);
	JSON(r_w_flag, CVI_U8, DePurpleCrStr);
	JSON(r_w_flag, CVI_U8, DePurpleStrMaxRatio);
	JSON(r_w_flag, CVI_U8, DePurpleStrMinRatio);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CAC_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CAC_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, DePurpleStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeGlobalGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeCoring, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeStrMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, EdgeStrMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCbStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleCrStr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleStrMaxRatio, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DePurpleStrMinRatio, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CAC_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CAC_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, PurpleDetRange);
	JSON(r_w_flag, CVI_U8, PurpleCb);
	JSON(r_w_flag, CVI_U8, PurpleCr);
	JSON(r_w_flag, CVI_U8, PurpleCb2);
	JSON(r_w_flag, CVI_U8, PurpleCr2);
	JSON(r_w_flag, CVI_U8, PurpleCb3);
	JSON(r_w_flag, CVI_U8, PurpleCr3);
	JSON(r_w_flag, CVI_U8, GreenCb);
	JSON(r_w_flag, CVI_U8, GreenCr);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON_A(r_w_flag, CVI_U8, EdgeGainIn, 3);
	JSON_A(r_w_flag, CVI_U8, EdgeGainOut, 3);
	JSON(r_w_flag, ISP_CAC_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CAC_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Sharpen
// -----------------------------------------------------------------------------
static void ISP_SHARPEN_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SHARPEN_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaAdpGain, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, DeltaAdpGain, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutOut, EE_LUT_NODE);
	JSON(r_w_flag, CVI_U8, GlobalGain);
	JSON(r_w_flag, CVI_U8, OverShootGain);
	JSON(r_w_flag, CVI_U8, UnderShootGain);
	JSON(r_w_flag, CVI_U8, HFBlendWgt);
	JSON(r_w_flag, CVI_U8, MFBlendWgt);
	JSON(r_w_flag, CVI_U8, OverShootThr);
	JSON(r_w_flag, CVI_U8, UnderShootThr);
	JSON(r_w_flag, CVI_U8, OverShootThrMax);
	JSON(r_w_flag, CVI_U8, UnderShootThrMin);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainOut, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, HueShtCtrl, SHARPEN_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainIn, EE_LUT_NODE);
	JSON_A(r_w_flag, CVI_U8, SatShtGainOut, EE_LUT_NODE);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_SHARPEN_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SHARPEN_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, LumaAdpGain, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, DeltaAdpGain, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, LumaCorLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionCorWgtLutOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, GlobalGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootGain, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, HFBlendWgt, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MFBlendWgt, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootThr, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, OverShootThrMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, UnderShootThrMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, MotionShtGainOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, HueShtCtrl, SHARPEN_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainIn, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U8, SatShtGainOut, EE_LUT_NODE * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_SHARPEN_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SHARPEN_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_U8, TuningMode);
	JSON(r_w_flag, CVI_BOOL, LumaAdpGainEn);
	JSON(r_w_flag, CVI_BOOL, DeltaAdpGainEn);
	JSON(r_w_flag, CVI_BOOL, NoiseSuppressEnable);
	JSON(r_w_flag, CVI_BOOL, SatShtCtrlEn);
	JSON(r_w_flag, CVI_BOOL, SoftClampEnable);
	JSON(r_w_flag, CVI_U8, SoftClampUB);
	JSON(r_w_flag, CVI_U8, SoftClampLB);
	JSON(r_w_flag, ISP_SHARPEN_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_SHARPEN_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CA
// -----------------------------------------------------------------------------
static void ISP_CA_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, ISORatio);
	JSON_A(r_w_flag, CVI_U16, YRatioLut, CA_LUT_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CA_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, ISORatio, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, YRatioLut, CA_LUT_NUM * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CA_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, CVI_BOOL, CaCpMode);
	JSON_A(r_w_flag, CVI_U8, CPLutY, CA_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, CPLutU, CA_LUT_NUM);
	JSON_A(r_w_flag, CVI_U8, CPLutV, CA_LUT_NUM);
	JSON(r_w_flag, ISP_CA_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CA_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - CA2
// -----------------------------------------------------------------------------
static void ISP_CA2_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA2_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, Ca2In, CA_LITE_NODE);
	JSON_A(r_w_flag, CVI_U16, Ca2Out, CA_LITE_NODE);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_CA2_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA2_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, Ca2In, CA_LITE_NODE * ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, Ca2Out, CA_LITE_NODE * ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_CA2_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CA2_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_CA2_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_CA2_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - YContrast
// -----------------------------------------------------------------------------
static void ISP_YCONTRAST_MANUAL_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YCONTRAST_MANUAL_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, ContrastLow);
	JSON(r_w_flag, CVI_U8, ContrastHigh);
	JSON(r_w_flag, CVI_U8, CenterLuma);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_YCONTRAST_AUTO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YCONTRAST_AUTO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U8, ContrastLow, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, ContrastHigh, ISP_AUTO_LV_NUM);
	JSON_A(r_w_flag, CVI_U8, CenterLuma, ISP_AUTO_LV_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_YCONTRAST_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_YCONTRAST_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, UpdateInterval);
	JSON(r_w_flag, ISP_YCONTRAST_MANUAL_ATTR_S, stManual);
	JSON(r_w_flag, ISP_YCONTRAST_AUTO_ATTR_S, stAuto);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP Other
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// ISP structure - NP
// -----------------------------------------------------------------------------
void ISP_CMOS_NOISE_CALIBRATION_S_JSON(int r_w_flag, JSON *j, char *key, ISP_CMOS_NOISE_CALIBRATION_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_FLOAT, CalibrationCoef,
		NOISE_PROFILE_ISO_NUM * NOISE_PROFILE_CHANNEL_NUM * NOISE_PROFILE_LEVEL_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP structure - Mono
// -----------------------------------------------------------------------------
void ISP_MONO_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MONO_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, Enable);
	JSON(r_w_flag, CVI_U8, UpdateInterval);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// ISP
// -----------------------------------------------------------------------------
void ISP_PARAMETER_BUFFER_JSON(int r_w_flag, JSON *j, char *key_word, ISP_Parameter_Structures *data, int idx)
{
	char key[64] = {0};
	JSON *obj = NULL;

	snprintf(key, sizeof(key), "%s%d", key_word, idx);
	JSON_START_ENTRANCE(r_w_flag);

	switch (idx)
	{
	case 1:
		//Pub attr
		JSON(r_w_flag, ISP_PUB_ATTR_S, pub_attr);

		// Pre-RAW
		JSON(r_w_flag, ISP_BLACK_LEVEL_ATTR_S, blc);
		JSON(r_w_flag, ISP_DP_DYNAMIC_ATTR_S, dpc_dynamic);
		JSON(r_w_flag, ISP_DP_STATIC_ATTR_S, dpc_static);
		JSON(r_w_flag, ISP_DP_CALIB_ATTR_S, DPCalib);
		JSON(r_w_flag, ISP_CROSSTALK_ATTR_S, crosstalk);
		// RAW-Top
		JSON(r_w_flag, ISP_NR_ATTR_S, bnr);
		JSON(r_w_flag, ISP_NR_FILTER_ATTR_S, bnr_filter);
		JSON(r_w_flag, ISP_DEMOSAIC_ATTR_S, demosaic);
		JSON(r_w_flag, ISP_DEMOSAIC_DEMOIRE_ATTR_S, demosaic_demoire);
		JSON(r_w_flag, ISP_RGBCAC_ATTR_S, rgbcac);
		JSON(r_w_flag, ISP_LCAC_ATTR_S, lcac);
		JSON(r_w_flag, ISP_DIS_ATTR_S, disAttr);
		JSON(r_w_flag, ISP_DIS_CONFIG_S, disConfig);

	// RGB-Top
		JSON(r_w_flag, ISP_MESH_SHADING_ATTR_S, mlsc);
		break;
	case 2:
		JSON(r_w_flag, ISP_MESH_SHADING_GAIN_LUT_ATTR_S, mlscLUT);
		break;
	case 3:
		JSON(r_w_flag, ISP_SATURATION_ATTR_S, Saturation);
		JSON(r_w_flag, ISP_CCM_ATTR_S, ccm);
		JSON(r_w_flag, ISP_CCM_SATURATION_ATTR_S, ccm_saturation);
		JSON(r_w_flag, ISP_COLOR_TONE_ATTR_S, colortone);
		JSON(r_w_flag, ISP_FSWDR_ATTR_S, fswdr);
		JSON(r_w_flag, ISP_WDR_EXPOSURE_ATTR_S, WDRExposure);
		JSON(r_w_flag, ISP_DRC_ATTR_S, drc);
		JSON(r_w_flag, ISP_GAMMA_ATTR_S, gamma);
		JSON(r_w_flag, ISP_AUTO_GAMMA_ATTR_S, autoGamma);
		JSON(r_w_flag, ISP_DEHAZE_ATTR_S, dehaze);
		break;
	case 4:
		JSON(r_w_flag, ISP_CLUT_ATTR_S, clut);
		break;
	case 5:
		JSON(r_w_flag, ISP_CLUT_SATURATION_ATTR_S, clut_saturation);
		JSON(r_w_flag, ISP_CSC_ATTR_S, csc);
		JSON(r_w_flag, ISP_VC_ATTR_S, vc_motion);

	// YUV-Top
		JSON(r_w_flag, ISP_DCI_ATTR_S, dci);
		JSON(r_w_flag, ISP_LDCI_ATTR_S, ldci);
		JSON(r_w_flag, ISP_PRESHARPEN_ATTR_S, presharpen);
		JSON(r_w_flag, ISP_TNR_ATTR_S, tnr);
		JSON(r_w_flag, ISP_TNR_NOISE_MODEL_ATTR_S, tnr_noise_model);
		JSON(r_w_flag, ISP_TNR_LUMA_MOTION_ATTR_S, tnr_luma_motion);
		JSON(r_w_flag, ISP_TNR_GHOST_ATTR_S, tnr_ghost);
		JSON(r_w_flag, ISP_TNR_MT_PRT_ATTR_S, tnr_mt_prt);
		JSON(r_w_flag, ISP_TNR_MOTION_ADAPT_ATTR_S, tnr_motion_adapt);
		JSON(r_w_flag, ISP_YNR_ATTR_S, ynr);
		JSON(r_w_flag, ISP_YNR_FILTER_ATTR_S, ynr_filter);
		JSON(r_w_flag, ISP_YNR_MOTION_NR_ATTR_S, ynr_motion);
		JSON(r_w_flag, ISP_CNR_ATTR_S, cnr);
		JSON(r_w_flag, ISP_CNR_MOTION_NR_ATTR_S, cnr_motion);
		JSON(r_w_flag, ISP_CAC_ATTR_S, cac);
		JSON(r_w_flag, ISP_SHARPEN_ATTR_S, sharpen);
		JSON(r_w_flag, ISP_CA_ATTR_S, ca);
		JSON(r_w_flag, ISP_CA2_ATTR_S, ca2);
		JSON(r_w_flag, ISP_YCONTRAST_ATTR_S, ycontrast);

		// Other
		JSON(r_w_flag, ISP_CMOS_NOISE_CALIBRATION_S, np);
		JSON(r_w_flag, ISP_MONO_ATTR_S, mono);
		break;
	default:
			break;
	}
	JSON_END_ENTRANCE(r_w_flag);
}

// -----------------------------------------------------------------------------
// AE structure
// -----------------------------------------------------------------------------
static void ISP_ME_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_ME_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_OP_TYPE_E, enExpTimeOpType);
	JSON(r_w_flag, ISP_OP_TYPE_E, enAGainOpType);
	JSON(r_w_flag, ISP_OP_TYPE_E, enDGainOpType);
	JSON(r_w_flag, ISP_OP_TYPE_E, enISPDGainOpType);
	JSON(r_w_flag, CVI_U32, u32ExpTime);
	JSON(r_w_flag, CVI_U32, u32AGain);
	JSON(r_w_flag, CVI_U32, u32DGain);
	JSON(r_w_flag, CVI_U32, u32ISPDGain);
	JSON(r_w_flag, ISP_OP_TYPE_E, enISONumOpType);
	JSON(r_w_flag, ISP_AE_GAIN_TYPE_E, enGainType);
	JSON(r_w_flag, CVI_U32, u32ISONum);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_RANGE_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_RANGE_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32Max);
	JSON(r_w_flag, CVI_U32, u32Min);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_ANTIFLICKER_S_JSON(int r_w_flag, JSON *j, char *key, ISP_ANTIFLICKER_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, ISP_AE_ANTIFLICKER_FREQUENCE_E, enFrequency);
	JSON(r_w_flag, ISP_ANTIFLICKER_MODE_E, enMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_SUBFLICKER_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SUBFLICKER_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U8, u8LumaDiff);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_DELAY_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_DELAY_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16BlackDelayFrame);
	JSON(r_w_flag, CVI_U16, u16WhiteDelayFrame);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_AE_RANGE_S, stExpTimeRange);
	JSON(r_w_flag, ISP_AE_RANGE_S, stAGainRange);
	JSON(r_w_flag, ISP_AE_RANGE_S, stDGainRange);
	JSON(r_w_flag, ISP_AE_RANGE_S, stISPDGainRange);
	JSON(r_w_flag, ISP_AE_RANGE_S, stSysGainRange);
	JSON(r_w_flag, CVI_U32, u32GainThreshold);
	JSON(r_w_flag, CVI_U8, u8Speed);
	JSON(r_w_flag, CVI_U16, u16BlackSpeedBias);
	JSON(r_w_flag, CVI_U8, u8Tolerance);
	JSON(r_w_flag, CVI_U8, u8Compensation);
	JSON(r_w_flag, CVI_U16, u16EVBias);
	JSON(r_w_flag, ISP_AE_STRATEGY_E, enAEStrategyMode);
	JSON(r_w_flag, CVI_U16, u16HistRatioSlope);
	JSON(r_w_flag, CVI_U8, u8MaxHistOffset);
	JSON(r_w_flag, ISP_AE_MODE_E, enAEMode);
	JSON(r_w_flag, ISP_ANTIFLICKER_S, stAntiflicker);
	JSON(r_w_flag, ISP_SUBFLICKER_S, stSubflicker);
	JSON(r_w_flag, ISP_AE_DELAY_S, stAEDelayAttr);
	JSON(r_w_flag, CVI_BOOL, bManualExpValue);
	JSON(r_w_flag, CVI_U32, u32ExpValue);
	JSON(r_w_flag, ISP_FSWDR_MODE_E, enFSWDRMode);
	JSON(r_w_flag, CVI_BOOL, bWDRQuick);
	JSON(r_w_flag, CVI_U16, u16ISOCalCoef);
	JSON(r_w_flag, ISP_AE_GAIN_TYPE_E, enGainType);
	JSON(r_w_flag, ISP_AE_RANGE_S, stISONumRange);
	JSON(r_w_flag, CVI_S16, s16IRCutOnLv);
	JSON(r_w_flag, CVI_S16, s16IRCutOffLv);
	JSON(r_w_flag, ISP_AE_IR_CUT_FORCE_STATUS, enIRCutStatus);
	JSON_A(r_w_flag, CVI_U8, au8AdjustTargetMin, LV_TOTAL_NUM);
	JSON_A(r_w_flag, CVI_U8, au8AdjustTargetMax, LV_TOTAL_NUM);
	JSON(r_w_flag, CVI_U16, u16LowBinThr);
	JSON(r_w_flag, CVI_U16, u16HighBinThr);
	JSON(r_w_flag, CVI_BOOL, bEnableFaceAE);
	JSON(r_w_flag, CVI_U8, u8FaceTargetLuma);
	JSON(r_w_flag, CVI_U8, u8FaceWeight);
	JSON(r_w_flag, CVI_U8, u8GridBvWeight);
	JSON_A(r_w_flag, CVI_U32, au32Reserve, RESERVE_SIZE);
	JSON(r_w_flag, CVI_U8, u8HighLightLumaThr);
	JSON(r_w_flag, CVI_U8, u8HighLightBufLumaThr);
	JSON(r_w_flag, CVI_U8, u8LowLightLumaThr);
	JSON(r_w_flag, CVI_U8, u8LowLightBufLumaThr);
	JSON(r_w_flag, CVI_BOOL, bHistogramAssist);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_EXPOSURE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bByPass);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U8, u8AERunInterval);
	JSON(r_w_flag, CVI_BOOL, bHistStatAdjust);
	JSON(r_w_flag, CVI_BOOL, bAERouteExValid);
	JSON(r_w_flag, ISP_ME_ATTR_S, stManual);
	JSON(r_w_flag, ISP_AE_ATTR_S, stAuto);
	JSON(r_w_flag, CVI_U8, u8DebugMode);
	JSON(r_w_flag, ISP_AE_METER_MODE_E, enMeterMode);
	JSON(r_w_flag, CVI_BOOL, bAEGainSepCfg);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_ROUTE_NODE_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_NODE_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32IntTime);
	JSON(r_w_flag, CVI_U32, u32SysGain);
	JSON(r_w_flag, ISP_IRIS_F_NO_E, enIrisFNO);
	JSON(r_w_flag, CVI_U32, u32IrisFNOLin);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AE_ROUTE_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32TotalNum);
	JSON_A(r_w_flag, ISP_AE_ROUTE_NODE_S, astRouteNode, ISP_AE_ROUTE_MAX_NODES);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_ROUTE_EX_NODE_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_EX_NODE_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32IntTime);
	JSON(r_w_flag, CVI_U32, u32Again);
	JSON(r_w_flag, CVI_U32, u32Dgain);
	JSON(r_w_flag, CVI_U32, u32IspDgain);
	JSON(r_w_flag, ISP_IRIS_F_NO_E, enIrisFNO);
	JSON(r_w_flag, CVI_U32, u32IrisFNOLin);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AE_ROUTE_EX_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_ROUTE_EX_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32TotalNum);
	JSON_A(r_w_flag, ISP_AE_ROUTE_EX_NODE_S, astRouteExNode, ISP_AE_ROUTE_EX_MAX_NODES);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_SMART_EXPOSURE_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_SMART_EXPOSURE_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_BOOL, bIRMode);
	JSON(r_w_flag, ISP_OP_TYPE_E, enSmartExpType);
	JSON(r_w_flag, CVI_U8, u8LumaTarget);
	JSON(r_w_flag, CVI_U16, u16ExpCoef);
	JSON(r_w_flag, CVI_U16, u16ExpCoefMax);
	JSON(r_w_flag, CVI_U16, u16ExpCoefMin);
	JSON(r_w_flag, CVI_U8, u8SmartInterval);
	JSON(r_w_flag, CVI_U8, u8SmartSpeed);
	JSON(r_w_flag, CVI_U16, u16SmartDelayNum);
	JSON(r_w_flag, CVI_U8, u8Weight);
	JSON(r_w_flag, CVI_U8, u8NarrowRatio);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_CROP_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_CROP_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16X);
	JSON(r_w_flag, CVI_U16, u16Y);
	JSON(r_w_flag, CVI_U16, u16W);
	JSON(r_w_flag, CVI_U16, u16H);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_FACE_CROP_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_FACE_CROP_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16X);
	JSON(r_w_flag, CVI_U16, u16Y);
	JSON(r_w_flag, CVI_U8, u16W);
	JSON(r_w_flag, CVI_U8, u16H);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AE_STATISTICS_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AE_STATISTICS_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bHisStatisticsEnable);
	JSON_A(r_w_flag, ISP_AE_CROP_S, stCrop, AE_MAX_NUM);
	JSON_A(r_w_flag, ISP_AE_FACE_CROP_S, stFaceCrop, FACE_WIN_NUM);
	JSON(r_w_flag, CVI_BOOL, fast2A_ena);
	JSON(r_w_flag, CVI_U8, fast2A_ae_low);
	JSON(r_w_flag, CVI_U8, fast2A_ae_high);
	JSON(r_w_flag, CVI_U16, fast2A_awb_top);
	JSON(r_w_flag, CVI_U16, fast2A_awb_bot);
	JSON(r_w_flag, CVI_U16, over_exp_thr);
	JSON_A(r_w_flag, CVI_U8, au8Weight, AE_WEIGHT_ZONE_ROW * AE_WEIGHT_ZONE_COLUMN);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_MI_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MI_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32HoldValue);
	JSON(r_w_flag, ISP_IRIS_F_NO_E, enIrisFNO);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_IRIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_IRIS_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, ISP_IRIS_TYPE_E, enIrisType);
	JSON(r_w_flag, ISP_IRIS_STATUS_E, enIrisStatus);
	JSON(r_w_flag, ISP_MI_ATTR_S, stMIAttr);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_DCIRIS_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_DCIRIS_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_S32, s32Kp);
	JSON(r_w_flag, CVI_S32, s32Ki);
	JSON(r_w_flag, CVI_S32, s32Kd);
	JSON(r_w_flag, CVI_U32, u32MinPwmDuty);
	JSON(r_w_flag, CVI_U32, u32MaxPwmDuty);
	JSON(r_w_flag, CVI_U32, u32OpenPwmDuty);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// AWB structure
// -----------------------------------------------------------------------------
static void ISP_MWB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_MWB_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16Rgain);
	JSON(r_w_flag, CVI_U16, u16Grgain);
	JSON(r_w_flag, CVI_U16, u16Gbgain);
	JSON(r_w_flag, CVI_U16, u16Bgain);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_CT_LIMIT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_CT_LIMIT_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, CVI_U16, u16HighRgLimit);
	JSON(r_w_flag, CVI_U16, u16HighBgLimit);
	JSON(r_w_flag, CVI_U16, u16LowRgLimit);
	JSON(r_w_flag, CVI_U16, u16LowBgLimit);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_CBCR_TRACK_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_CBCR_TRACK_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON_A(r_w_flag, CVI_U16, au16CrMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, au16CrMin, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, au16CbMax, ISP_AUTO_ISO_STRENGTH_NUM);
	JSON_A(r_w_flag, CVI_U16, au16CbMin, ISP_AUTO_ISO_STRENGTH_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_LUM_HISTGRAM_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_LUM_HISTGRAM_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON_A(r_w_flag, CVI_U8, au8HistThresh, AWB_LUM_HIST_NUM);
	JSON_A(r_w_flag, CVI_U16, au16HistWt, AWB_LUM_HIST_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16RefColorTemp);
	JSON_A(r_w_flag, CVI_U16, au16StaticWB, ISP_BAYER_CHN_NUM);
	JSON_A(r_w_flag, CVI_S32, as32CurvePara, AWB_CURVE_PARA_NUM);
	JSON(r_w_flag, ISP_AWB_ALG_TYPE_E, enAlgType);
	JSON(r_w_flag, CVI_U8, u8RGStrength);
	JSON(r_w_flag, CVI_U8, u8BGStrength);
	JSON(r_w_flag, CVI_U16, u16Speed);
	JSON(r_w_flag, CVI_U16, u16ZoneSel);
	JSON(r_w_flag, CVI_U16, u16HighColorTemp);
	JSON(r_w_flag, CVI_U16, u16LowColorTemp);
	JSON(r_w_flag, ISP_AWB_CT_LIMIT_ATTR_S, stCTLimit);
	JSON(r_w_flag, CVI_BOOL, bShiftLimitEn);
	JSON_A(r_w_flag, CVI_U16, u16ShiftLimit, AWB_CURVE_BOUND_NUM);
	JSON(r_w_flag, CVI_BOOL, bGainNormEn);
	JSON(r_w_flag, CVI_BOOL, bNaturalCastEn);
	JSON(r_w_flag, ISP_AWB_CBCR_TRACK_ATTR_S, stCbCrTrack);
	JSON(r_w_flag, ISP_AWB_LUM_HISTGRAM_ATTR_S, stLumaHist);
	JSON(r_w_flag, CVI_BOOL, bAWBZoneWtEn);
	JSON_A(r_w_flag, CVI_U8, au8ZoneWt, AWB_ZONE_WT_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_WB_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bByPass);
	JSON(r_w_flag, CVI_U8, u8AWBRunInterval);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, ISP_MWB_ATTR_S, stManual);
	JSON(r_w_flag, ISP_AWB_ATTR_S, stAuto);
	JSON(r_w_flag, ISP_AWB_ALG_E, enAlgType);
	JSON(r_w_flag, CVI_U8, u8DebugMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_EXTRA_LIGHTSOURCE_INFO_S_JSON(int r_w_flag, JSON *j, char *key,
	ISP_AWB_EXTRA_LIGHTSOURCE_INFO_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16WhiteRgain);
	JSON(r_w_flag, CVI_U16, u16WhiteBgain);
	JSON(r_w_flag, CVI_U16, u16ExpQuant);
	JSON(r_w_flag, CVI_U8, u8LightStatus);
	JSON(r_w_flag, CVI_U8, u8Radius);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_IN_OUT_ATTR_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_IN_OUT_ATTR_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, ISP_OP_TYPE_E, enOpType);
	JSON(r_w_flag, ISP_AWB_INDOOR_OUTDOOR_STATUS_E, enOutdoorStatus);
	JSON(r_w_flag, CVI_U32, u32OutThresh);
	JSON(r_w_flag, CVI_U16, u16LowStart);
	JSON(r_w_flag, CVI_U16, u16LowStop);
	JSON(r_w_flag, CVI_U16, u16HighStart);
	JSON(r_w_flag, CVI_U16, u16HighStop);
	JSON(r_w_flag, CVI_BOOL, bGreenEnhanceEn);
	JSON(r_w_flag, CVI_U8, u8OutShiftLimit);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_GRASS_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_GRASS_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8Mode);
	JSON(r_w_flag, CVI_U8, u8ThrLv);
	JSON(r_w_flag, CVI_U16, u16Rgain);
	JSON(r_w_flag, CVI_U16, u16Bgain);
	JSON(r_w_flag, CVI_U16, u16MapRgain);
	JSON(r_w_flag, CVI_U16, u16MapBgain);
	JSON(r_w_flag, CVI_U8, u8Radius);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_SKY_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_SKY_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8Mode);
	JSON(r_w_flag, CVI_U8, u8ThrLv);
	JSON(r_w_flag, CVI_U16, u16Rgain);
	JSON(r_w_flag, CVI_U16, u16Bgain);
	JSON(r_w_flag, CVI_U16, u16MapRgain);
	JSON(r_w_flag, CVI_U16, u16MapBgain);
	JSON(r_w_flag, CVI_U8, u8Radius);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_SKIN_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_SKIN_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8Mode);
	JSON(r_w_flag, CVI_U16, u16RgainDiff);
	JSON(r_w_flag, CVI_U16, u16BgainDiff);
	JSON(r_w_flag, CVI_U8, u8Radius);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_INTERFERENCE_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_INTERFERENCE_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8Mode);
	JSON(r_w_flag, CVI_U8, u8Limit);
	JSON(r_w_flag, CVI_U8, u8Radius);
	JSON(r_w_flag, CVI_U8, u8Ratio);
	JSON(r_w_flag, CVI_U8, u8Distance);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_REGION_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_REGION_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16Region1);
	JSON(r_w_flag, CVI_U16, u16Region2);
	JSON(r_w_flag, CVI_U16, u16Region3);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_CT_WGT_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_CT_WGT_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON_A(r_w_flag, CVI_U16, au16MultiCTBin, AWB_CT_BIN_NUM);
	JSON_A(r_w_flag, CVI_S8, s8ThrLv, AWB_CT_LV_NUM);
	JSON_A(r_w_flag, CVI_U16, au16MultiCTWt, AWB_CT_LV_NUM * AWB_CT_BIN_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ST_ISP_AWB_SHIFT_LV_S_JSON(int r_w_flag, JSON *j, char *key, struct ST_ISP_AWB_SHIFT_LV_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8LowLvMode);
	JSON_A(r_w_flag, CVI_U16, u16LowLvCT, ISP_AWB_COLORTEMP_NUM);
	JSON_A(r_w_flag, CVI_U16, u16LowLvThr, ISP_AWB_COLORTEMP_NUM);
	JSON_A(r_w_flag, CVI_U16, u16LowLvRatio, ISP_AWB_COLORTEMP_NUM);
	JSON(r_w_flag, CVI_U8, u8HighLvMode);
	JSON_A(r_w_flag, CVI_U16, u16HighLvCT, ISP_AWB_COLORTEMP_NUM);
	JSON_A(r_w_flag, CVI_U16, u16HighLvThr, ISP_AWB_COLORTEMP_NUM);
	JSON_A(r_w_flag, CVI_U16, u16HighLvRatio, ISP_AWB_COLORTEMP_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AWB_ATTR_EX_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_ATTR_EX_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U8, u8Tolerance);
	JSON(r_w_flag, CVI_U8, u8ZoneRadius);
	JSON(r_w_flag, CVI_U16, u16CurveLLimit);
	JSON(r_w_flag, CVI_U16, u16CurveRLimit);
	JSON(r_w_flag, CVI_BOOL, bExtraLightEn);
	JSON_A(r_w_flag, ISP_AWB_EXTRA_LIGHTSOURCE_INFO_S, stLightInfo, AWB_LS_NUM);
	JSON(r_w_flag, ISP_AWB_IN_OUT_ATTR_S, stInOrOut);
	JSON(r_w_flag, CVI_BOOL, bMultiLightSourceEn);
	JSON(r_w_flag, ISP_AWB_MULTI_LS_TYPE_E, enMultiLSType);
	JSON(r_w_flag, CVI_U16, u16MultiLSScaler);
	JSON(r_w_flag, CVI_U16, u16MultiLSThr);
	JSON(r_w_flag, CVI_U16, u16CALumaDiff);
	JSON(r_w_flag, CVI_U16, u16CAAdjustRatio);
	JSON_A(r_w_flag, CVI_U16, au16MultiCTBin, AWB_CT_BIN_NUM);
	JSON_A(r_w_flag, CVI_U16, au16MultiCTWt, AWB_CT_BIN_NUM);
	JSON(r_w_flag, CVI_BOOL, bFineTunEn);
	JSON(r_w_flag, CVI_U8, u8FineTunStrength);
	JSON(r_w_flag, ST_ISP_AWB_INTERFERENCE_S, stInterference);
	JSON(r_w_flag, ST_ISP_AWB_SKIN_S, stSkin);
	JSON(r_w_flag, ST_ISP_AWB_SKY_S, stSky);
	JSON(r_w_flag, ST_ISP_AWB_GRASS_S, stGrass);
	JSON(r_w_flag, ST_ISP_AWB_CT_WGT_S, stCtLv);
	JSON(r_w_flag, ST_ISP_AWB_SHIFT_LV_S, stShiftLv);
	JSON(r_w_flag, ST_ISP_AWB_REGION_S, stRegion);
	JSON(r_w_flag, CVI_U8, adjBgainMode);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_CROP_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_CROP_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16X);
	JSON(r_w_flag, CVI_U16, u16Y);
	JSON(r_w_flag, CVI_U16, u16W);
	JSON(r_w_flag, CVI_U16, u16H);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_WB_STATISTICS_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_STATISTICS_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_AWB_SWITCH_E, enAWBSwitch);
	JSON(r_w_flag, CVI_U16, u16ZoneRow);
	JSON(r_w_flag, CVI_U16, u16ZoneCol);
	JSON(r_w_flag, CVI_U16, u16ZoneBin);
	JSON_A(r_w_flag, CVI_U16, au16HistBinThresh, 4);
	JSON(r_w_flag, CVI_U16, u16WhiteLevel);
	JSON(r_w_flag, CVI_U16, u16BlackLevel);
	JSON(r_w_flag, CVI_U16, u16CbMax);
	JSON(r_w_flag, CVI_U16, u16CbMin);
	JSON(r_w_flag, CVI_U16, u16CrMax);
	JSON(r_w_flag, CVI_U16, u16CrMin);
	JSON(r_w_flag, ISP_AWB_CROP_S, stCrop);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AWB_Calibration_Gain_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_Calibration_Gain_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, u16AvgRgain, AWB_CALIB_PTS_NUM);
	JSON_A(r_w_flag, CVI_U16, u16AvgBgain, AWB_CALIB_PTS_NUM);
	JSON_A(r_w_flag, CVI_U16, u16ColorTemperature, AWB_CALIB_PTS_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AWB_Calibration_Gain_S_EX_JSON(int r_w_flag, JSON *j, char *key, ISP_AWB_Calibration_Gain_S_EX *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_U16, u16AvgRgain, AWB_CALIB_PTS_NUM_EX);
	JSON_A(r_w_flag, CVI_U16, u16AvgBgain, AWB_CALIB_PTS_NUM_EX);
	JSON_A(r_w_flag, CVI_U16, u16ColorTemperature, AWB_CALIB_PTS_NUM_EX);
	JSON_A(r_w_flag, CVI_U8, u8Weight, AWB_CALIB_PTS_NUM_EX);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// AF structure
// -----------------------------------------------------------------------------
static void ISP_AF_RAW_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_RAW_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, PreGammaEn);
	JSON_A(r_w_flag, CVI_U8, PreGammaTable, AF_GAMMA_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AF_PRE_FILTER_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_PRE_FILTER_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, PreFltEn);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AF_CROP_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_CROP_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16X);
	JSON(r_w_flag, CVI_U16, u16Y);
	JSON(r_w_flag, CVI_U16, u16W);
	JSON(r_w_flag, CVI_U16, u16H);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_AF_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_BOOL, bEnable);
	JSON(r_w_flag, CVI_U16, u16Hwnd);
	JSON(r_w_flag, CVI_U16, u16Vwnd);
	JSON(r_w_flag, CVI_U8, u8HFltShift);
	JSON_A(r_w_flag, CVI_S8, s8HVFltLpCoeff, FIR_H_GAIN_NUM);
	JSON(r_w_flag, ISP_AF_RAW_CFG_S, stRawCfg);
	JSON(r_w_flag, ISP_AF_PRE_FILTER_CFG_S, stPreFltCfg);
	JSON(r_w_flag, ISP_AF_CROP_S, stCrop);
	JSON(r_w_flag, CVI_U8, H0FltCoring);
	JSON(r_w_flag, CVI_U8, H1FltCoring);
	JSON(r_w_flag, CVI_U8, V0FltCoring);
	JSON(r_w_flag, CVI_U16, u16HighLumaTh);
	JSON(r_w_flag, CVI_U8, u8ThLow);
	JSON(r_w_flag, CVI_U8, u8ThHigh);
	JSON(r_w_flag, CVI_U8, u8GainLow);
	JSON(r_w_flag, CVI_U8, u8GainHigh);
	JSON(r_w_flag, CVI_U8, u8SlopLow);
	JSON(r_w_flag, CVI_U8, u8SlopHigh);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AF_H_PARAM_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_H_PARAM_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_S8, s8HFltHpCoeff, FIR_H_GAIN_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_AF_V_PARAM_S_JSON(int r_w_flag, JSON *j, char *key, ISP_AF_V_PARAM_S *data)
{
	JSON_START(r_w_flag);

	JSON_A(r_w_flag, CVI_S8, s8VFltHpCoeff, FIR_V_GAIN_NUM);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
static void ISP_FOCUS_STATISTICS_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_FOCUS_STATISTICS_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_AF_CFG_S, stConfig);
	JSON(r_w_flag, ISP_AF_H_PARAM_S, stHParam_FIR0);
	JSON(r_w_flag, ISP_AF_H_PARAM_S, stHParam_FIR1);
	JSON(r_w_flag, ISP_AF_V_PARAM_S, stVParam_FIR);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// 3A structure
// -----------------------------------------------------------------------------
static void ISP_STATISTICS_CTRL_U_JSON(int r_w_flag, JSON *j, char *key, ISP_STATISTICS_CTRL_U *data)
{
	JSON_START(r_w_flag);

	JSON_SB(r_w_flag, CVI_U64, bit1FEAeGloStat);
	JSON_SB(r_w_flag, CVI_U64, bit1FEAeLocStat);
	JSON_SB(r_w_flag, CVI_U64, bit1AwbStat1);
	JSON_SB(r_w_flag, CVI_U64, bit1AwbStat2);
	JSON_SB(r_w_flag, CVI_U64, bit1FEAfStat);
	JSON_SB(r_w_flag, CVI_U64, bit14Rsv);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_STATISTICS_CFG_S_JSON(int r_w_flag, JSON *j, char *key, ISP_STATISTICS_CFG_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, ISP_STATISTICS_CTRL_U, unKey);
	JSON(r_w_flag, ISP_AE_STATISTICS_CFG_S, stAECfg);
	JSON(r_w_flag, ISP_WB_STATISTICS_CFG_S, stWBCfg);
	JSON(r_w_flag, ISP_FOCUS_STATISTICS_CFG_S, stFocusCfg);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
// 3A
// -----------------------------------------------------------------------------
void ISP_3A_PARAMETER_BUFFER_JSON(int r_w_flag, JSON *j, char *key_word, ISP_3A_Parameter_Structures *data, int idx)
{
	char key[64] = {0};
	JSON *obj = NULL;

	snprintf(key, sizeof(key), "%s%d", key_word, idx);
	JSON_START_ENTRANCE(r_w_flag);
	switch (idx)
	{
	case 1:
		// AE
		JSON(r_w_flag, ISP_WDR_EXPOSURE_ATTR_S, WDRExpAttr);
		JSON(r_w_flag, ISP_EXPOSURE_ATTR_S, ExpAttr);
		JSON(r_w_flag, ISP_AE_ROUTE_S, AeRouteAttr);
		JSON(r_w_flag, ISP_AE_ROUTE_EX_S, AeRouteAttrEx);
		JSON(r_w_flag, ISP_SMART_EXPOSURE_ATTR_S, AeSmartExposureAttr);
		JSON(r_w_flag, ISP_IRIS_ATTR_S, AeIrisAttr);
		JSON(r_w_flag, ISP_DCIRIS_ATTR_S, AeDcirisAttr);
		JSON(r_w_flag, ISP_AE_ROUTE_S, AeRouteSFAttr);
		JSON(r_w_flag, ISP_AE_ROUTE_EX_S, AeRouteSFAttrEx);
		break;
	case 2:
		// AWB
		JSON(r_w_flag, ISP_WB_ATTR_S, WBAttr);
		JSON(r_w_flag, ISP_AWB_ATTR_EX_S, AWBAttrEx);
		JSON(r_w_flag, ISP_AWB_Calibration_Gain_S, WBCalib);
		JSON(r_w_flag, ISP_AWB_Calibration_Gain_S_EX, WBCalibEx);
		JSON(r_w_flag, ISP_STATISTICS_CFG_S, StatCfg);
		break;
	default:
			break;
	}
	JSON_END_ENTRANCE(r_w_flag);
}

// -----------------------------------------------------------------------------
// Read only
// -----------------------------------------------------------------------------
void ISP_EXP_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_EXP_INFO_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U32, u32ExpTime);
	JSON(r_w_flag, CVI_U32, u32ShortExpTime);
	JSON(r_w_flag, CVI_U32, u32MedianExpTime);
	JSON(r_w_flag, CVI_U32, u32LongExpTime);
	JSON(r_w_flag, CVI_U32, u32AGain);
	JSON(r_w_flag, CVI_U32, u32DGain);
	JSON(r_w_flag, CVI_U32, u32ISPDGain);
	JSON(r_w_flag, CVI_U32, u32Exposure);
	JSON(r_w_flag, CVI_BOOL, bExposureIsMAX);
	JSON(r_w_flag, CVI_S16, s16HistError);
	JSON_A(r_w_flag, CVI_U32, au32AE_Hist256Value, HIST_NUM);

	JSON(r_w_flag, CVI_U8, u8AveLum);
	JSON(r_w_flag, CVI_U32, u32LinesPer500ms);
	JSON(r_w_flag, CVI_U32, u32PirisFNO);
	JSON(r_w_flag, CVI_U32, u32Fps);
	JSON(r_w_flag, CVI_U32, u32ISO);
	JSON(r_w_flag, CVI_U32, u32ISOCalibrate);
	JSON(r_w_flag, CVI_U32, u32RefExpRatio);
	JSON(r_w_flag, CVI_U32, u32FirstStableTime);
	JSON(r_w_flag, ISP_AE_ROUTE_S, stAERoute);
	JSON(r_w_flag, ISP_AE_ROUTE_EX_S, stAERouteEx);
	JSON(r_w_flag, CVI_U8, u8WDRShortAveLuma);
	JSON(r_w_flag, CVI_U32, u32WDRExpRatio);
	JSON(r_w_flag, CVI_U8, u8LEFrameAvgLuma);
	JSON(r_w_flag, CVI_U8, u8SEFrameAvgLuma);
	JSON(r_w_flag, CVI_FLOAT, fLightValue);
	JSON(r_w_flag, CVI_U32, u32AGainSF);
	JSON(r_w_flag, CVI_U32, u32DGainSF);
	JSON(r_w_flag, CVI_U32, u32ISPDGainSF);
	JSON(r_w_flag, CVI_U32, u32ISOSF);
	JSON(r_w_flag, ISP_AE_ROUTE_S, stAERouteSF);
	JSON(r_w_flag, ISP_AE_ROUTE_EX_S, stAERouteSFEx);
	JSON(r_w_flag, CVI_BOOL, bGainSepStatus);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
void ISP_WB_INFO_S_JSON(int r_w_flag, JSON *j, char *key, ISP_WB_INFO_S *data)
{
	JSON_START(r_w_flag);

	JSON(r_w_flag, CVI_U16, u16Rgain);
	JSON(r_w_flag, CVI_U16, u16Grgain);
	JSON(r_w_flag, CVI_U16, u16Gbgain);
	JSON(r_w_flag, CVI_U16, u16Bgain);
	JSON(r_w_flag, CVI_U16, u16Saturation);
	JSON(r_w_flag, CVI_U16, u16ColorTemp);
	JSON_A(r_w_flag, CVI_U16, au16CCM, CCM_MATRIX_SIZE);
	JSON(r_w_flag, CVI_U16, u16LS0CT);
	JSON(r_w_flag, CVI_U16, u16LS1CT);
	JSON(r_w_flag, CVI_U16, u16LS0Area);
	JSON(r_w_flag, CVI_U16, u16LS1Area);
	JSON(r_w_flag, CVI_U8, u8MultiDegree);
	JSON(r_w_flag, CVI_U16, u16ActiveShift);
	JSON(r_w_flag, CVI_U32, u32FirstStableTime);
	JSON(r_w_flag, ISP_AWB_INDOOR_OUTDOOR_STATUS_E, enInOutStatus);
	JSON(r_w_flag, CVI_S16, s16Bv);

	JSON_END(r_w_flag);
}

// -----------------------------------------------------------------------------
