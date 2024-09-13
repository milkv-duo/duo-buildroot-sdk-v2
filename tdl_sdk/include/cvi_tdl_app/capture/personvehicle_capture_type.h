#ifndef _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_TYPE_H_
#define _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_TYPE_H_

#include "capture_type.h"
#include "core/cvi_tdl_core.h"

typedef struct {
  cvtdl_object_info_t info;
  float quality;
  tracker_state_e state;
  uint32_t miss_counter;
  cvtdl_image_t image;

  bool _capture;
  uint64_t _timestamp;
  uint32_t _out_counter;
} personvehicle_cpt_data_t;

typedef struct {
  int thr_area_base;
  int thr_area_min;
  int thr_area_max;
  float thr_aspect_ratio_min;
  float thr_aspect_ratio_max;
  float thr_quality;

  uint32_t miss_time_limit;
  uint32_t fast_m_interval;
  uint32_t fast_m_capture_num;
  uint32_t cycle_m_interval;
  uint32_t auto_m_time_limit;
  bool auto_m_fast_cap;

  bool store_RGB888;
} personvehicle_capture_config_t;

typedef struct {
  capture_mode_e mode;
  personvehicle_capture_config_t cfg;

  uint32_t size;
  personvehicle_cpt_data_t *data;
  cvtdl_object_t last_objects;
  float *last_quality;
  cvtdl_tracker_t last_trackers;

  CVI_TDL_SUPPORTED_MODEL_E od_model_index;
  bool enable_DeepSORT; /* don't set manually */

  bool *_output;   // output signal (# = .size)
  uint64_t _time;  // timer
  uint32_t _m_limit;
  cvtdl_counting_line_t cross_line_t;
  randomRect rect;
  irregularRegins ir_regins;
} personvehicle_capture_t;

#endif  // End of _CVI_TDL_APP_PERSONVEHICLE_CAPTURE_TYPE_H_
