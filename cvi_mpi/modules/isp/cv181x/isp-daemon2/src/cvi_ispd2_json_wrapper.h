/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2022. All rights reserved.
 *
 * File Name: cvi_ispd2_json_wrapper.h
 * Description:
 */

#ifndef _CVI_ISPD2_JSON_WRAPPER_H_
#define _CVI_ISPD2_JSON_WRAPPER_H_


#include "cvi_json.h"
#	define	JSONObject								struct cvi_json_object
#	define	JSONTokener								struct cvi_json_tokener

// function
#	define	ISPD2_json_tokener_new					cvi_json_tokener_new
#	define	ISPD2_json_tokener_free					cvi_json_tokener_free
#	define	ISPD2_json_tokener_parse				cvi_json_tokener_parse
#	define	ISPD2_json_tokener_parse_ex				cvi_json_tokener_parse_ex
#	define	ISPD2_json_tokener_get_parse_end		cvi_json_tokener_get_parse_end
#	define	ISPD2_json_tokener_error_desc			cvi_json_tokener_error_desc
#	define	ISPD2_json_tokener_set_flags			cvi_json_tokener_set_flags
#	define	ISPD2_json_pointer_get					cvi_json_pointer_get
#	define	ISPD2_json_pointer_set					cvi_json_pointer_set
#	define	ISPD2_json_object_to_fd					cvi_json_object_to_fd
#	define	ISPD2_json_object_to_json_string_ext	cvi_json_object_to_cvi_json_string_ext
#	define	ISPD2_json_object_to_file_ext			cvi_json_object_to_file_ext
#	define	ISPD2_json_object_to_json_string_length	cvi_json_object_to_cvi_json_string_length
#	define	ISPD2_json_object_from_file				cvi_json_object_from_file
#	define	ISPD2_json_object_get					cvi_json_object_get
#	define	ISPD2_json_object_put					cvi_json_object_put
#	define	ISPD2_json_object_is_type				cvi_json_object_is_type
#	define	ISPD2_json_object_get_string			cvi_json_object_get_string
#	define	ISPD2_json_object_get_string_len		cvi_json_object_get_string_len
#	define	ISPD2_json_object_get_int				cvi_json_object_get_int
#	define	ISPD2_json_object_get_double			cvi_json_object_get_double
#	define	ISPD2_json_object_get_object			cvi_json_object_get_object
#	define	ISPD2_json_object_new_object			cvi_json_object_new_object
#	define	ISPD2_json_object_new_array				cvi_json_object_new_array
#	define	ISPD2_json_object_new_null				cvi_json_object_new_null
#	define	ISPD2_json_object_new_int				cvi_json_object_new_int
#	define	ISPD2_json_object_new_uint64			cvi_json_object_new_uint64
#	define	ISPD2_json_object_new_double			cvi_json_object_new_double
#	define	ISPD2_json_object_new_string			cvi_json_object_new_string
#	define	ISPD2_json_object_array_add				cvi_json_object_array_add
#	define	ISPD2_json_object_array_length			cvi_json_object_array_length
#	define	ISPD2_json_object_array_get_idx			cvi_json_object_array_get_idx
#	define	ISPD2_json_object_object_add			cvi_json_object_object_add
#	define	ISPD2_json_object_object_length			cvi_json_object_object_length
#	define	ISPD2_json_object_object_get_ex			cvi_json_object_object_get_ex
#	define	ISPD2_json_util_get_last_err			cvi_json_util_get_last_err

// enum
#	define	ISPD2_json_type_null					cvi_json_type_null
#	define	ISPD2_json_type_object					cvi_json_type_object
#	define	ISPD2_json_type_array					cvi_json_type_array
#	define	ISPD2_json_tokener_success				cvi_json_tokener_success

#define ISPD2_json_tokener_flags (JSON_TOKENER_STRICT | JSON_TOKENER_ALLOW_TRAILING_CHARS)


#endif // _CVI_ISPD2_JSON_WRAPPER_H_
