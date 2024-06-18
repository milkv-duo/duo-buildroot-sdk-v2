/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_json_struct_common.c
 * Description:
 *
 */

#include "cvi_json_struct_comm.h"
#include "string.h"
#include "stdbool.h"

char param_point[150] = { 0 };

void JSON_CHECK_RANGE(char *name, int *value, int min, int max)
{
	if (*value < min) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %d[%d,%d]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = min;
	} else if (*value > max) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %d[%d,%d]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = max;
	}
}

void JSON_CHECK_RANGE_OF_32(char *name, long long *value, long long min, long long max)
{
	if (*value < min) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %lld[%lld,%lld]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = min;
	} else if (*value > max) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %lld[%lld,%lld]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = max;
	}
}

void JSON_CHECK_RANGE_OF_U64(char *name, unsigned long long *value, unsigned long long min, unsigned long long max)
{
	if (*value < min) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %llu[%llu,%llu]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = min;
	} else if (*value > max) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %llu[%llu,%llu]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = max;
	}
}

void JSON_CHECK_RANGE_OF_DOUBLE(char *name, double *value, double min, double max)
{
	if (*value < min) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %f[%f,%f]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = min;
	} else if (*value > max) {
		CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:OVER_RANGE %d(L) %s%s = %f[%f,%f]\n",
			__LINE__, param_point, name, *value, min, max);
		*value = max;
	}
}

void JSON_PRINT_ERR_NOT_EXIST(char *name)
{
	CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:NOT_EXIST %d(L) %s%s\n",
		__LINE__, param_point, name);
}

void JSON_PRINT_ERR_DATA_TYPE(char *name)
{
	CVI_TRACE_JSON(CVI_DBG_ERR, "JSON_READ_ERR:DATA_TYPE %d(L) %s%s\n",
		__LINE__, param_point, name);
}

cvi_json_bool cvi_json_object_object_get_ex2(struct cvi_json_object *obj, const char *key,
					struct cvi_json_object **value)
{
	if (cvi_json_object_is_type(obj, cvi_json_type_array)) {
		char *str = strrchr(key, '[');
		int index = atoi(str + 1);
		*value = cvi_json_object_array_get_idx(obj, index);
		return true;
	} else {
		return cvi_json_object_object_get_ex(obj, key, value);
	}
}

void CVI_BOOL_JSON(int r_w_flag, JSON *j, char *key, CVI_BOOL *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}
			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, 0, 1);
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

void CVI_U8_JSON(int r_w_flag, JSON *j, char *key, CVI_U8 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, 0, CVI_U8_MAX);
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

void CVI_S8_JSON(int r_w_flag, JSON *j, char *key, CVI_S8 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, CVI_S8_MIN, CVI_S8_MAX);
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

void CVI_U16_JSON(int r_w_flag, JSON *j, char *key, CVI_U16 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}

			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, 0, CVI_U16_MAX);
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

void CVI_S16_JSON(int r_w_flag, JSON *j, char *key, CVI_S16 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			int temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}
			temp = cvi_json_object_get_int(obj);
			JSON_CHECK_RANGE(key, &temp, CVI_S16_MIN, CVI_S16_MAX);
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

void CVI_U32_JSON(int r_w_flag, JSON *j, char *key, CVI_U32 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			long long temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}
			temp = cvi_json_object_get_int64(obj);
			JSON_CHECK_RANGE_OF_32(key, &temp, 0, CVI_U32_MAX);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int64(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

void CVI_S32_JSON(int r_w_flag, JSON *j, char *key, CVI_S32 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			long long temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}

			temp = cvi_json_object_get_int64(obj);
			JSON_CHECK_RANGE_OF_32(key, &temp, CVI_S32_MIN, CVI_S32_MAX);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_int64(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

void CVI_U64_JSON(int r_w_flag, JSON *j, char *key, CVI_U64 *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			unsigned long long temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_int) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}
			temp = cvi_json_object_get_uint64(obj);
			JSON_CHECK_RANGE_OF_U64(key, &temp, 0, CVI_U32_MAX);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_uint64(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

void CVI_FLOAT_JSON(int r_w_flag, JSON *j, char *key, CVI_FLOAT *value)
{
	JSON *obj = 0;

	if (r_w_flag == R_FLAG) {
		if (cvi_json_object_object_get_ex2(j, key, &obj)) {
			double temp;
			cvi_json_type cvi_type = cvi_json_object_get_type(obj);

			if (cvi_type != cvi_json_type_double) {
				JSON_PRINT_ERR_DATA_TYPE(key);
				return;
			}
			temp = cvi_json_object_get_double(obj);
			JSON_CHECK_RANGE_OF_DOUBLE(key, &temp, CVI_FLOAT_MIN, CVI_FLOAT_MAX);
			*value = temp;
		} else {
			JSON_PRINT_ERR_NOT_EXIST(key);
		}
	} else {
		obj = cvi_json_object_new_double(*value);
		if (cvi_json_object_is_type(j, cvi_json_type_array)) {
			cvi_json_object_array_add(j, obj);
		} else {
			cvi_json_object_object_add(j, key, obj);
		}
	}
}

JSON *JSON_TokenerParse(const char *buffer)
{
	return cvi_json_tokener_parse(buffer);
}

JSON *JSON_GetNewObject(void)
{
	return cvi_json_object_new_object();
}

CVI_S32 JSON_GetJsonStrLen(JSON *json_object)
{
	return strlen(cvi_json_object_to_cvi_json_string(json_object)) + 1;
}

const char *JSON_GetJsonStrContent(JSON *json_object)
{
	return cvi_json_object_to_cvi_json_string(json_object);
}

CVI_S32 JSON_ObjectPut(JSON *json_object)
{
	cvi_json_object_put(json_object);

	return CVI_SUCCESS;
}
