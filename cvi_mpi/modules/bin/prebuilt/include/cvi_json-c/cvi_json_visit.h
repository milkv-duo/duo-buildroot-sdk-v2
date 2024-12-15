
#ifndef _cvi_json_c_cvi_json_visit_h_
#define _cvi_json_c_cvi_json_visit_h_

/**
 * @file
 * @brief Methods for walking a tree of objects.
 */
#include "cvi_json_object.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int(cvi_json_c_visit_userfunc)(cvi_json_object *jso, int flags, cvi_json_object *parent_jso,
                                   const char *jso_key, size_t *jso_index, void *userarg);

/**
 * Visit each object in the JSON hierarchy starting at jso.
 * For each object, userfunc is called, passing the object and userarg.
 * If the object has a parent (i.e. anything other than jso itself)
 * its parent will be passed as parent_jso, and either jso_key or jso_index
 * will be set, depending on whether the parent is an object or an array.
 *
 * Nodes will be visited depth first, but containers (arrays and objects)
 * will be visited twice, the second time with JSON_C_VISIT_SECOND set in
 * flags.
 *
 * userfunc must return one of the defined return values, to indicate
 * whether and how to continue visiting nodes, or one of various ways to stop.
 *
 * Returns 0 if nodes were visited successfully, even if some were
 *  intentionally skipped due to what userfunc returned.
 * Returns <0 if an error occurred during iteration, including if
 *  userfunc returned JSON_C_VISIT_RETURN_ERROR.
 */
#ifndef REMOVE_UNUSED_FUNCTION
JSON_EXPORT int cvi_json_c_visit(cvi_json_object *jso, int future_flags, cvi_json_c_visit_userfunc *userfunc,
                             void *userarg);
#endif
/**
 * Passed to cvi_json_c_visit_userfunc as one of the flags values to indicate
 * that this is the second time a container (array or object) is being
 * called, after all of it's members have been iterated over.
 */
#define JSON_C_VISIT_SECOND 0x02

/**
 * This cvi_json_c_visit_userfunc return value indicates that iteration
 * should proceed normally.
 */
#define JSON_C_VISIT_RETURN_CONTINUE 0

/**
 * This cvi_json_c_visit_userfunc return value indicates that iteration
 * over the members of the current object should be skipped.
 * If the current object isn't a container (array or object), this
 * is no different than JSON_C_VISIT_RETURN_CONTINUE.
 */
#define JSON_C_VISIT_RETURN_SKIP 7547

/**
 * This cvi_json_c_visit_userfunc return value indicates that iteration
 * of the fields/elements of the <b>containing</b> object should stop
 * and continue "popped up" a level of the object hierarchy.
 * For example, returning this when handling arg will result in
 * arg3 and any other fields being skipped.   The next call to userfunc
 * will be the JSON_C_VISIT_SECOND call on "foo", followed by a userfunc
 * call on "bar".
 * <pre>
 * {
 *   "foo": {
 *     "arg1": 1,
 *     "arg2": 2,
 *     "arg3": 3,
 *     ...
 *   },
 *   "bar": {
 *     ...
 *   }
 * }
 * </pre>
 */
#define JSON_C_VISIT_RETURN_POP 767

/**
 * This cvi_json_c_visit_userfunc return value indicates that iteration
 * should stop immediately, and cause cvi_json_c_visit to return success.
 */
#define JSON_C_VISIT_RETURN_STOP 7867

/**
 * This cvi_json_c_visit_userfunc return value indicates that iteration
 * should stop immediately, and cause cvi_json_c_visit to return an error.
 */
#define JSON_C_VISIT_RETURN_ERROR -1

#ifdef __cplusplus
}
#endif

#endif /* _cvi_json_c_cvi_json_visit_h_ */
