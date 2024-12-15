
/**
 * @file
 * @brief Do not use, cvi_json-c internal, may be changed or removed at any time.
 */
#ifndef _cvi_json_inttypes_h_
#define _cvi_json_inttypes_h_

#include "cvi_json_config.h"

#ifdef JSON_C_HAVE_INTTYPES_H
/* inttypes.h includes stdint.h */
#include <inttypes.h>

#else
#include <stdint.h>

#define PRId64 "I64d"
#define SCNd64 "I64d"
#define PRIu64 "I64u"

#endif

#endif
