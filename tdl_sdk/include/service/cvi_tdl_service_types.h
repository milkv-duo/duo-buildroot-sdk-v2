#ifndef _CVI_TDL_SERVICE_TYPES_H_
#define _CVI_TDL_SERVICE_TYPES_H_

#include "core/core/cvtdl_core_types.h"

#include <stdint.h>

/**
 * \defgroup core_cvitdlservice CVI_TDL Service Module
 */

/** @enum cvtdl_service_feature_matching_e
 *  @ingroup core_cvitdlservice
 *  @brief Supported feature matching method in Service
 *
 * @var cvtdl_service_feature_matching_e::COS_SIMILARITY
 * Do feature matching using inner product method.
 */
typedef enum { COS_SIMILARITY } cvtdl_service_feature_matching_e;

/** @struct cvtdl_service_feature_array_t
 *  @ingroup core_cvitdlservice
 *  @brief Feature array structure used in Service
 *
 * @var cvtdl_service_feature_array_t::ptr
 * ptr is the raw 1-D array of the feature array. Format is feature 1, feature 2...
 * @var cvtdl_service_feature_array_t::feature_length
 * feature length is the length of one single feature.
 * @var cvtdl_service_feature_array_t::data_num
 * data_num is how many features the data array has.
 * @var cvtdl_service_feature_array_t::type
 * type is the data type of the feature array.
 */
typedef struct {
  int8_t* ptr;
  uint32_t feature_length;
  uint32_t data_num;
  feature_type_e type;
} cvtdl_service_feature_array_t;

/** @enum cvtdl_area_detect_e
 *  @ingroup core_cvitdlservice
 *  @brief The intersect state for intersect detection.
 */
typedef enum {
  UNKNOWN = 0,
  NO_INTERSECT,
  ON_LINE,
  CROSS_LINE_POS,
  CROSS_LINE_NEG,
  INSIDE_POLYGON,
  OUTSIDE_POLYGON
} cvtdl_area_detect_e;

/** @struct cvtdl_service_brush_t
 *  @ingroup core_cvitdlservice
 *  @brief Brush structure for bounding box drawing
 *
 */
typedef struct {
  struct {
    float r;
    float g;
    float b;
  } color;
  uint32_t size;
} cvtdl_service_brush_t;

#endif  // End of _CVI_TDL_SERVICE_TYPES_H_