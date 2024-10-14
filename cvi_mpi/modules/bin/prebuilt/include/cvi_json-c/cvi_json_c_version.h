/*
 * Copyright (c) 2012,2017 Eric Haszlakiewicz
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 */

/**
 * @file
 * @brief Methods for retrieving the cvi_json-c version.
 */
#ifndef _cvi_json_c_version_h_
#define _cvi_json_c_version_h_

#ifdef __cplusplus
extern "C" {
#endif

#define JSON_C_MAJOR_VERSION 0
#define JSON_C_MINOR_VERSION 15
#define JSON_C_MICRO_VERSION 99
#define JSON_C_VERSION_NUM \
	((JSON_C_MAJOR_VERSION << 16) | (JSON_C_MINOR_VERSION << 8) | JSON_C_MICRO_VERSION)
#define JSON_C_VERSION "0.15.99"

#ifndef JSON_EXPORT
#if defined(_MSC_VER) && defined(JSON_C_DLL)
#define JSON_EXPORT __declspec(dllexport)
#else
#define JSON_EXPORT extern
#endif
#endif
#ifndef REMOVE_UNUSED_FUNCTION
#define REMOVE_UNUSED_FUNCTION
#endif
#ifndef REMOVE_UNUSED_FUNCTION
/**
 * @see JSON_C_VERSION
 * @return the version of the cvi_json-c library as a string
 */
JSON_EXPORT const char *cvi_json_c_version(void); /* Returns JSON_C_VERSION */

/**
 * The cvi_json-c version encoded into an int, with the low order 8 bits
 * being the micro version, the next higher 8 bits being the minor version
 * and the next higher 8 bits being the major version.
 * For example, 7.12.99 would be 0x00070B63.
 *
 * @see JSON_C_VERSION_NUM
 * @return the version of the cvi_json-c library as an int
 */
JSON_EXPORT int cvi_json_c_version_num(void); /* Returns JSON_C_VERSION_NUM */
#endif
#ifdef __cplusplus
}
#endif

#endif
