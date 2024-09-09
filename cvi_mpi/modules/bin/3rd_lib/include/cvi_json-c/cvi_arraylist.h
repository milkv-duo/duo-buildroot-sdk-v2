/*
 * $Id: cvi_arraylist.h,v 1.4 2006/01/26 02:16:28 mclark Exp $
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
 * @brief Internal methods for working with cvi_json_type_array objects.
 *        Although this is exposed by the cvi_json_object_get_array() method,
 *        it is not recommended for direct use.
 */
#ifndef _cvi_json_c_arraylist_h_
#define _cvi_json_c_arraylist_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define ARRAY_LIST_DEFAULT_SIZE 32
#ifndef REMOVE_UNUSED_FUNCTION
#define REMOVE_UNUSED_FUNCTION
#endif
typedef void(array_list_free_fn)(void *data);

struct array_list
{
	void **array;
	size_t length;
	size_t size;
	array_list_free_fn *free_fn;
};
typedef struct array_list array_list;

/**
 * Allocate an array_list of the default size (32).
 * @deprecated Use cvi_array_list_new2() instead.
 */
#ifndef REMOVE_UNUSED_FUNCTION
extern struct array_list *array_list_new(array_list_free_fn *free_fn);
#endif
/**
 * Allocate an array_list of the desired size.
 *
 * If possible, the size should be chosen to closely match
 * the actual number of elements expected to be used.
 * If the exact size is unknown, there are tradeoffs to be made:
 * - too small - the array_list code will need to call realloc() more
 *   often (which might incur an additional memory copy).
 * - too large - will waste memory, but that can be mitigated
 *   by calling cvi_array_list_shrink() once the final size is known.
 *
 * @see cvi_array_list_shrink
 */
extern struct array_list *cvi_array_list_new2(array_list_free_fn *free_fn, int initial_size);

extern void cvi_array_list_free(struct array_list *al);

extern void *cvi_array_list_get_idx(struct array_list *al, size_t i);
#ifndef REMOVE_UNUSED_FUNCTION
extern int array_list_put_idx(struct array_list *al, size_t i, void *data);
#endif
extern int cvi_array_list_add(struct array_list *al, void *data);

extern size_t cvi_array_list_length(struct array_list *al);
#ifndef REMOVE_UNUSED_FUNCTION
extern void array_list_sort(struct array_list *arr, int (*compar)(const void *, const void *));

extern void *array_list_bsearch(const void **key, struct array_list *arr,
                                int (*compar)(const void *, const void *));

extern int array_list_del_idx(struct array_list *arr, size_t idx, size_t count);
#endif
/**
 * Shrink the array list to just enough to fit the number of elements in it,
 * plus empty_slots.
 */
extern int cvi_array_list_shrink(struct array_list *arr, size_t empty_slots);

#ifdef __cplusplus
}
#endif

#endif
