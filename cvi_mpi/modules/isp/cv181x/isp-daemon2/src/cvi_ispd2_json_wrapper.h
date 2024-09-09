/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_json_wrapper.h
 * Description:
 */

#ifndef _CVI_ISPD2_JSON_WRAPPER_H_
#define _CVI_ISPD2_JSON_WRAPPER_H_

#ifdef USE_CVI_JSONC_LIBRARY
#include "cvi_json.h"
#	define	JSONObject								struct cvi_json_object

#else
#include "json.h"
#	define	JSONObject								struct json_object
#	define	JSONTokener								struct json_tokener

// function
#	define	ISPD2_json_tokener_new					json_tokener_new
#	define	ISPD2_json_tokener_free					json_tokener_free
#	define	ISPD2_json_tokener_parse				json_tokener_parse
#	define	ISPD2_json_tokener_parse_ex				json_tokener_parse_ex
#	define	ISPD2_json_tokener_get_parse_end		json_tokener_get_parse_end
#	define	ISPD2_json_tokener_error_desc			json_tokener_error_desc

#	define	ISPD2_json_tokener_set_flags			json_tokener_set_flags

#	define	ISPD2_json_pointer_get					json_pointer_get
#	define	ISPD2_json_pointer_set					json_pointer_set

#	define	ISPD2_json_object_to_fd					json_object_to_fd
#	define	ISPD2_json_object_to_json_string_ext	json_object_to_json_string_ext
#	define	ISPD2_json_object_to_file_ext			json_object_to_file_ext
#	define	ISPD2_json_object_to_json_string_length	json_object_to_json_string_length

#	define	ISPD2_json_object_from_file				json_object_from_file

#	define	ISPD2_json_object_get					json_object_get
#	define	ISPD2_json_object_put					json_object_put
#	define	ISPD2_json_object_is_type				json_object_is_type
#	define	ISPD2_json_object_get_string			json_object_get_string
#	define	ISPD2_json_object_get_int				json_object_get_int
#	define	ISPD2_json_object_get_double			json_object_get_double

#	define	ISPD2_json_object_new_object			json_object_new_object
#	define	ISPD2_json_object_new_array				json_object_new_array
#	define	ISPD2_json_object_new_null				json_object_new_null
#	define	ISPD2_json_object_new_int				json_object_new_int
#	define	ISPD2_json_object_new_uint64			json_object_new_uint64
#	define	ISPD2_json_object_new_double			json_object_new_double
#	define	ISPD2_json_object_new_string			json_object_new_string

#	define	ISPD2_json_object_array_add				json_object_array_add
#	define	ISPD2_json_object_array_length			json_object_array_length
#	define	ISPD2_json_object_array_get_idx			json_object_array_get_idx

#	define	ISPD2_json_object_object_add			json_object_object_add
#	define	ISPD2_json_object_object_length			json_object_object_length
#	define	ISPD2_json_object_object_get_ex			json_object_object_get_ex

#	define	ISPD2_json_util_get_last_err			json_util_get_last_err

// enum
#	define	ISPD2_json_type_null					json_type_null
#	define	ISPD2_json_type_object					json_type_object
#	define	ISPD2_json_type_array					json_type_array
#	define	ISPD2_json_tokener_success				json_tokener_success

#define ISPD2_json_tokener_flags (JSON_TOKENER_STRICT | JSON_TOKENER_ALLOW_TRAILING_CHARS)
#endif //

#endif // _CVI_ISPD2_JSON_WRAPPER_H_
