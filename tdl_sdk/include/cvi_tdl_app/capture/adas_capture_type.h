#ifndef _CVI_TDL_APP_ADAS_CAPTURE_TYPE_HPP_
#define _CVI_TDL_APP_ADAS_CAPTURE_TYPE_HPP_

// #ifdef __cplusplus
// extern "C" {
// #endif

#include "capture_type.h"
#include "core/cvi_tdl_core.h"

// #ifdef __cplusplus
// }

typedef struct {
  cvtdl_object_info_t info;
  tracker_state_e t_state;
  float speed;
  float dis;
  float dis_tmp;

  float start_score;
  float warning_score;

  uint32_t miss_counter;

  uint32_t counter;

} adas_data_t;

typedef struct {
  int x;
  int y;
  int z;
  uint64_t counter;
} gsensor_data_t;

typedef struct {
  uint32_t size;
  float FPS;
  uint32_t lane_model_type;
  uint32_t lane_counter;
  uint32_t departure_time;
  float lane_score;
  int det_type;

  adas_data_t *data;
  cvtdl_lane_t lane_meta;

  cvtdl_object_t last_objects;
  cvtdl_tracker_t last_trackers;

  uint32_t miss_time_limit;

  int center_info[3];
  adas_state_e lane_state;

  gsensor_data_t gsensor_data;
  gsensor_data_t gsensor_tmp_data;
  int gsensor_queue[10];

  bool is_static;

} adas_info_t;

// #endif

#endif  // End of _CVI_TDL_APP_ADAS_CAPTURE_TYPE_HPP_
