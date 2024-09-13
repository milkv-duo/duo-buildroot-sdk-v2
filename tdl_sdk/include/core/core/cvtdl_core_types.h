#ifndef _CVI_CORE_TYPES_H_
#define _CVI_CORE_TYPES_H_

#include <stdint.h>
#include <stdlib.h>
#include "cvi_comm.h"
/**
 * \defgroup core_cvitdlcore CVI_TDL Core Module
 */

/** @enum feature_type_e
 * @ingroup core_cvitdlcore
 * @brief A variable type enum to present the data type stored in cvtdl_feature_t.
 * @see cvtdl_feature_t
 */
typedef enum {
  TYPE_INT8 = 0, /**< Equals to int8_t. */
  TYPE_UINT8,    /**< Equals to uint8_t. */
  TYPE_INT16,    /**< Equals to int16_t. */
  TYPE_UINT16,   /**< Equals to uint16_t. */
  TYPE_INT32,    /**< Equals to int32_t. */
  TYPE_UINT32,   /**< Equals to uint32_t. */
  TYPE_BF16,     /**< Equals to bf17. */
  TYPE_FLOAT     /**< Equals to float. */
} feature_type_e;

/** @enum meta_rescale_type_e
 * @ingroup core_cvitdlcore
 * @brief A variable type enum that records the resize padding method.
 */
typedef enum { RESCALE_UNKNOWN, RESCALE_NOASPECT, RESCALE_CENTER, RESCALE_RB } meta_rescale_type_e;

typedef enum { DOWN_UP = 0, UP_DOWN, Two_Way } statistics_mode;
/** @struct cvtdl_bbox_t
 * @ingroup core_cvitdlcore
 * @brief A structure to describe an area in a given image with confidence score.
 *
 * @var cvtdl_bbox_t::x1
 * The left-upper x coordinate.
 * @var cvtdl_bbox_t::y1
 * The left-upper y coordinate.
 * @var cvtdl_bbox_t::x2
 * The right-bottom x coordinate.
 * @var cvtdl_bbox_t::y2
 * The right-bottom y coordinate.
 * @var cvtdl_bbox_t::score
 * The confidence score.
 */

typedef struct {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
} cvtdl_bbox_t;

/** @struct cvtdl_feature_t
 * @ingroup core_cvitdlcore
 * @brief A structure to describe feature. Note that the length of the buffer is size *
 * getFeatureTypeSize(type)
 *
 * @var cvtdl_feature_t::ptr
 * The raw pointer of a feature. Need to convert to correct type with feature_type_e.
 * @var cvtdl_feature_t::size
 * The buffer size of ptr in unit of type.
 * @var cvtdl_feature_t::type
 * An enum to describe the type of ptr.
 * @see feature_type_e
 * @see getFeatureTypeSize()
 */
typedef struct {
  int8_t* ptr;
  uint32_t size;
  feature_type_e type;
} cvtdl_feature_t;

/** @struct cvtdl_pts_t
 * @ingroup core_cvitdlcore
 * @brief A structure to describe (x, y) array.
 *
 * @var cvtdl_pts_t::x
 * The raw pointer of the x coordinate.
 * @var cvtdl_pts_t::y
 * The raw pointer of the x coordinate.
 * @var cvtdl_pts_t::size
 * The buffer size of x and y in the unit of float.
 */
typedef struct {
  float* x;
  float* y;
  uint32_t size;
  float score;
} cvtdl_pts_t;

/** @struct cvtdl_4_pts_t
 * @ingroup core_cvitdlcore
 * @brief A structure to describe 4 2d points.
 *
 * @var cvtdl_pts_t::x
 * The raw pointer of the x coordinate.
 * @var cvtdl_pts_t::y
 * The raw pointer of the x coordinate.
 */
typedef struct {
  float x[4];
  float y[4];
} cvtdl_4_pts_t;

typedef struct _Point_t {
  int x1;
  int y1;
  int x2;
  int y2;
} Point_t;

typedef struct _MDROI_t {
  uint8_t num;
  Point_t pnt[4];
} MDROI_t;

/** @enum cvtdl_trk_state_type_t
 * @ingroup core_cvitdlcore
 * @brief Enum describing the tracking state.
 */
typedef enum {
  CVI_TRACKER_NEW = 0,
  CVI_TRACKER_UNSTABLE,
  CVI_TRACKER_STABLE,
} cvtdl_trk_state_type_t;

/** @struct cvtdl_tracker_info_t
 * @ingroup core_cvitdlcore
 * @brief Tracking info of a object.
 *
 * @var cvtdl_tracker_info_t:id:
 * The tracker ID number.
 * @var cvtdl_tracker_info_t::state
 * The tracking state of the object (or face).
 * @var cvtdl_tracker_info_t::bbox
 * The MOT algorithm computed bbox.
 */
typedef struct {
  uint64_t id;
  cvtdl_trk_state_type_t state;
  cvtdl_bbox_t bbox;
  int out_num;
} cvtdl_tracker_info_t;

/** @struct cvtdl_tracker_t
 * @ingroup core_cvitdlcore
 * @brief Tracking meta.
 *
 * @var cvtdl_tracker_t::size
 * The size of the info.
 * @var cvtdl_tracker_t::info
 * The object tracking array.
 */
typedef struct {
  uint32_t size;               // number of detections of current frame
  cvtdl_tracker_info_t* info;  // track info for current frame detection
} cvtdl_tracker_t;

/** @struct cvtdl_image_t
 * @ingroup core_cvitdlcore
 * @brief image stucture.
 *
 * @var cvtdl_image_t::height
 * The height of the image.
 * @var cvtdl_image_t::width
 * The width of the image.
 * @var cvtdl_image_t::stride
 * The stride of the image.
 * @var cvtdl_image_t::pix
 * The pixel data of the image.
 */
typedef struct {
  PIXEL_FORMAT_E pix_format;
  uint8_t* pix[3];
  uint32_t stride[3];
  uint32_t length[3];
  uint32_t height;
  uint32_t width;
  uint8_t* full_img;
  uint32_t full_length;
} cvtdl_image_t;

/** @struct InputPreParam
 *  @ingroup core_cvitdlcore
 *  @brief Config the detection model preprocess.
 *  @param InputPreParam::factor
 *  Preprocess factor, one dimension matrix, r g b channel
 *  @param InputPreParam::mean
 *  Preprocess mean, one dimension matrix, r g b channel
 *  @param InputPreParam::keep_aspect_ratio
 *  Preprocess config  scale
 *  @param InputPreParam:: use_crop
 *  Preprocess config crop
 *  @param InputPreParam::format
 *  Preprocess pixcel format config
 *  @param InputPreParam::VpssPreParam
 *  Vpss preprocess param
 */
typedef struct {
  float factor[3];
  float mean[3];
  bool keep_aspect_ratio;
  bool use_crop;
  PIXEL_FORMAT_E format;
  meta_rescale_type_e rescale_type;
  VPSS_SCALE_COEF_E resize_method;
} InputPreParam;

/**
 * @brief A helper function to get the unit size of feature_type_e.
 * @ingroup core_cvitdlcore
 *
 * @param type Input feature_type_e.
 * @return const int The unit size of a variable type.
 */

inline int __attribute__((always_inline)) getFeatureTypeSize(feature_type_e type) {
  uint32_t size = 1;
  switch (type) {
    case TYPE_INT8:
    case TYPE_UINT8:
      break;
    case TYPE_INT16:
    case TYPE_UINT16:
    case TYPE_BF16:
      size = 2;
      break;
    case TYPE_INT32:
    case TYPE_UINT32:
    case TYPE_FLOAT:
      size = 4;
      break;
  }
  return size;
}

#define DLL_EXPORT __attribute__((visibility("default")))
#endif
