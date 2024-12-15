/*
 * Copyright (c) 2020 Eric Hawicz
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 */

#ifndef _cvi_json_types_h_
#define _cvi_json_types_h_

/**
 * @file
 * @brief Basic types used in a few places in cvi_json-c, but you should include "cvi_json_object.h" instead.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef JSON_EXPORT
#if defined(_MSC_VER) && defined(JSON_C_DLL)
#define JSON_EXPORT __declspec(dllexport)
#else
#define JSON_EXPORT extern
#endif
#endif

struct printbuf;
#define REMOVE_UNUSED_FUNCTION
/**
 * A structure to use with cvi_json_object_object_foreachC() loops.
 * Contains key, val and entry members.
 */
struct cvi_json_object_iter
{
	char *key;
	struct cvi_json_object *val;
	struct lh_entry *entry;
};
typedef struct cvi_json_object_iter cvi_json_object_iter;

typedef int cvi_json_bool;

/**
 * @brief The core type for all type of JSON objects handled by cvi_json-c
 */
typedef struct cvi_json_object cvi_json_object;

/**
 * Type of custom user delete functions.  See cvi_json_object_set_serializer.
 */
typedef void(cvi_json_object_delete_fn)(struct cvi_json_object *jso, void *userdata);

/**
 * Type of a custom serialization function.  See cvi_json_object_set_serializer.
 */
typedef int(cvi_json_object_to_cvi_json_string_fn)(struct cvi_json_object *jso, struct printbuf *pb, int level,
                                           int flags);

/* supported object types */

typedef enum cvi_json_type
{
	/* If you change this, be sure to update cvi_json_type_to_name() too */
	cvi_json_type_null,
	cvi_json_type_boolean,
	cvi_json_type_double,
	cvi_json_type_int,
	cvi_json_type_object,
	cvi_json_type_array,
	cvi_json_type_string
} cvi_json_type;

#ifdef __cplusplus
}
#endif

#endif
