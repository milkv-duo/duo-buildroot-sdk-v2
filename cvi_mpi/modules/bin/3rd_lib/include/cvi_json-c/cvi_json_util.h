/*
 * $Id: cvi_json_util.h,v 1.4 2006/01/30 23:07:57 mclark Exp $
 *
 * Copyright (c) 2004, 2005 Metaparadigm Pte. Ltd.
 * Michael Clark <michael@metaparadigm.com>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See COPYING for details.
 *
 */

/**
 * @file
 * @brief Miscllaneous utility functions and macros.
 */
#ifndef _cvi_json_util_h_
#define _cvi_json_util_h_

#include "cvi_json_object.h"

#ifndef cvi_json_min
#define cvi_json_min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef cvi_json_max
#define cvi_json_max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define JSON_FILE_BUF_SIZE 4096

/* utility functions */
/**
 * Read the full contents of the given file, then convert it to a
 * cvi_json_object using cvi_json_tokener_parse().
 *
 * Returns NULL on failure.  See cvi_json_util_get_last_err() for details.
 */
#ifndef REMOVE_UNUSED_FUNCTION
JSON_EXPORT struct cvi_json_object *cvi_json_object_from_file(const char *filename);

/**
 * Create a JSON object from already opened file descriptor.
 *
 * This function can be helpful, when you opened the file already,
 * e.g. when you have a temp file.
 * Note, that the fd must be readable at the actual position, i.e.
 * use lseek(fd, 0, SEEK_SET) before.
 *
 * The depth argument specifies the maximum object depth to pass to
 * cvi_json_tokener_new_ex().  When depth == -1, JSON_TOKENER_DEFAULT_DEPTH
 * is used instead.
 *
 * Returns NULL on failure.  See cvi_json_util_get_last_err() for details.
 */
JSON_EXPORT struct cvi_json_object *cvi_json_object_from_fd_ex(int fd, int depth);

/**
 * Create a JSON object from an already opened file descriptor, using
 * the default maximum object depth. (JSON_TOKENER_DEFAULT_DEPTH)
 *
 * See cvi_json_object_from_fd_ex() for details.
 */
JSON_EXPORT struct cvi_json_object *cvi_json_object_from_fd(int fd);

/**
 * Equivalent to:
 *   cvi_json_object_to_file_ext(filename, obj, JSON_C_TO_STRING_PLAIN);
 *
 * Returns -1 if something fails.  See cvi_json_util_get_last_err() for details.
 */
JSON_EXPORT int cvi_json_object_to_file(const char *filename, struct cvi_json_object *obj);

/**
 * Open and truncate the given file, creating it if necessary, then
 * convert the cvi_json_object to a string and write it to the file.
 *
 * Returns -1 if something fails.  See cvi_json_util_get_last_err() for details.
 */
JSON_EXPORT int cvi_json_object_to_file_ext(const char *filename, struct cvi_json_object *obj, int flags);
/**
 * Convert the cvi_json_object to a string and write it to the file descriptor.
 * Handles partial writes and will keep writing until done, or an error
 * occurs.
 *
 * @param fd an open, writable file descriptor to write to
 * @param obj the object to serializer and write
 * @param flags flags to pass to cvi_json_object_to_cvi_json_string_ext()
 * @return -1 if something fails.  See cvi_json_util_get_last_err() for details.
 */
JSON_EXPORT int cvi_json_object_to_fd(int fd, struct cvi_json_object *obj, int flags);

/**
 * Return the last error from various cvi_json-c functions, including:
 * cvi_json_object_to_file{,_ext}, cvi_json_object_to_fd() or
 * cvi_json_object_from_{file,fd}, or NULL if there is none.
 */

JSON_EXPORT const char *cvi_json_util_get_last_err(void);
#endif
/* these parsing helpers return zero on success */
JSON_EXPORT int cvi_json_parse_int64(const char *buf, int64_t *retval);
JSON_EXPORT int cvi_json_parse_uint64(const char *buf, uint64_t *retval);
/**
 * @deprecated
 */
#ifndef REMOVE_UNUSED_FUNCTION
JSON_EXPORT int cvi_json_parse_double(const char *buf, double *retval);

/**
 * Return a string describing the type of the object.
 * e.g. "int", or "object", etc...
 */

JSON_EXPORT const char *cvi_json_type_to_name(enum cvi_json_type o_type);
#endif
#ifdef __cplusplus
}
#endif

#endif
