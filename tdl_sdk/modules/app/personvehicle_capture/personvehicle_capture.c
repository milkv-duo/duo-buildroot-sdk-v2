#include "personvehicle_capture.h"
#include "default_config.h"

#include <math.h>
#include "core/cvi_tdl_utils.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MEMORY_LIMIT (16 * 1024 * 1024) /* example: 16MB */

#define CAPTURE_TARGET_LIVE_TIME_EXTEND 5
#define UPDATE_VALUE_MIN 0.05
// TODO: Use cooldown to avoid too much updating
// #define UPDATE_COOLDOWN 3

static CVI_S32 clean_data(personvehicle_capture_t *personvehicle_cpt_info);
static void SHOW_CONFIG(personvehicle_capture_config_t *cfg);
void irregularReginJudge(const irregularRegins *regin_flags,
                         personvehicle_capture_t *personvehicle_cpt_info) {
  for (int i = 0; i < personvehicle_cpt_info->last_objects.size; i++) {
    int x = (personvehicle_cpt_info->last_objects.info[i].bbox.x1 +
             personvehicle_cpt_info->last_objects.info[i].bbox.x2) /
            2;
    int y = (personvehicle_cpt_info->last_objects.info[i].bbox.y1 +
             personvehicle_cpt_info->last_objects.info[i].bbox.y2) /
            2;
    int flag_x = floor(1.0 * x / regin_flags->w_size * 1.0);
    int flag_y = floor(1.0 * y / regin_flags->h_size * 1.0);
    if (regin_flags->regin_flags[flag_y * regin_flags->w_num + flag_x])
      personvehicle_cpt_info->last_objects.info[i].is_cross = 1;
    else
      personvehicle_cpt_info->last_objects.info[i].is_cross = 0;
  }
}
void getVehicleBufferRect(const cvtdl_counting_line_t *counting_line_t, randomRect *rect,
                          int expand_width) {
  statistics_mode s_mode = counting_line_t->s_mode;
  // A B
  rect->a_x = counting_line_t->A_x;
  rect->a_y = counting_line_t->A_y;
  rect->b_x = counting_line_t->B_x;
  rect->b_y = counting_line_t->B_y;
  if (counting_line_t->A_y == counting_line_t->B_y) {  // AY == BY
    rect->lt_x = counting_line_t->A_x;
    rect->lt_y = counting_line_t->A_y + expand_width;

    rect->rt_x = counting_line_t->B_x;
    rect->rt_y = counting_line_t->B_y + expand_width;

    rect->lb_x = counting_line_t->A_x;
    rect->lb_y = counting_line_t->A_y - expand_width;

    rect->rb_x = counting_line_t->B_x;
    rect->rb_y = counting_line_t->B_y - expand_width;

    if (s_mode == 0) {
      rect->f_x = 0;
      rect->f_y = -1;
    } else {
      rect->f_x = 0;
      rect->f_y = 1;
    }
  } else if (counting_line_t->A_x == counting_line_t->B_x) {  // AX = BX
    rect->lt_x = counting_line_t->A_x + expand_width;
    rect->lt_y = counting_line_t->A_y;

    rect->rt_x = counting_line_t->B_x + expand_width;
    rect->rt_y = counting_line_t->B_y;

    rect->lb_x = counting_line_t->A_x - expand_width;
    rect->lb_y = counting_line_t->A_y;

    rect->rb_x = counting_line_t->B_x - expand_width;
    rect->rb_y = counting_line_t->B_y;
    // // k b
    // rect->k = counting_line_t->A_x;
    // rect->b = -1000;
    if (s_mode == 0) {
      rect->f_x = 1;
      rect->f_y = 0;
    } else {
      rect->f_x = -1;
      rect->f_y = 0;
    }
  } else if (counting_line_t->A_y < counting_line_t->B_y) {
    float k = 1.0 * (counting_line_t->A_y - counting_line_t->B_y) /
              (counting_line_t->A_x - counting_line_t->B_x);
    float inv_k = atan(-1 / k);
    float x_e = abs(cos(inv_k) * expand_width);
    float y_e = abs(sin(inv_k) * expand_width);

    rect->lt_x = counting_line_t->A_x + x_e;
    rect->lt_y = counting_line_t->A_y - y_e;

    rect->rt_x = counting_line_t->B_x + x_e;
    rect->rt_y = counting_line_t->B_y - y_e;

    rect->lb_x = counting_line_t->A_x - x_e;
    rect->lb_y = counting_line_t->A_y + y_e;

    rect->rb_x = counting_line_t->B_x - x_e;
    rect->rb_y = counting_line_t->B_y + y_e;
    // // k b
    // rect->k = k;
    // rect->b = counting_line_t->A_y - k * counting_line_t->A_x;
    if (s_mode == 0) {
      rect->f_x = 1.0;
      rect->f_y = -1.0 / k;
    } else {
      rect->f_x = -1.0;
      rect->f_y = 1.0 / k;
    }
  } else if (counting_line_t->A_y > counting_line_t->B_y) {
    float k = 1.0 * (counting_line_t->A_y - counting_line_t->B_y) /
              (counting_line_t->A_x - counting_line_t->B_x);
    float inv_k = atan(-1 / k);
    float x_e = abs(cos(inv_k) * expand_width);
    float y_e = abs(sin(inv_k) * expand_width);

    rect->lt_x = counting_line_t->A_x - x_e;
    rect->lt_y = counting_line_t->A_y - y_e;

    rect->rt_x = counting_line_t->B_x - x_e;
    rect->rt_y = counting_line_t->B_y - y_e;

    rect->lb_x = counting_line_t->A_x + x_e;
    rect->lb_y = counting_line_t->A_y + y_e;

    rect->rb_x = counting_line_t->B_x + x_e;
    rect->rb_y = counting_line_t->B_y + y_e;
    // // k b
    // rect->k = k;
    // rect->b = counting_line_t->A_y - k * counting_line_t->A_x;
    if (s_mode == 0) {
      rect->f_x = -1.0;
      rect->f_y = 1.0 / k;
    } else {
      rect->f_x = 1;
      rect->f_y = -1.0 / k;
    }
  }
  printf("buffer rectangle: %f %f %f %f %f %f %f %f \n", rect->lt_x, rect->lt_y, rect->rt_x,
         rect->rt_y, rect->rb_x, rect->rb_y, rect->lb_x, rect->lb_y);
}

CVI_S32 _PersonVehicleCaptureIrregular_Run(personvehicle_capture_t *personvehicle_cpt_info,
                                           const cvitdl_handle_t tdl_handle,
                                           VIDEO_FRAME_INFO_S *frame) {
  if (personvehicle_cpt_info == NULL) {
    return CVI_TDL_FAILURE;
  }
  CVI_S32 ret;
  ret = clean_data(personvehicle_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    return CVI_TDL_FAILURE;
  }
  CVI_TDL_Free(&personvehicle_cpt_info->last_objects);
  CVI_TDL_Free(&personvehicle_cpt_info->last_trackers);
  personvehicle_cpt_info->ir_regins.w_size =
      frame->stVFrame.u32Width / personvehicle_cpt_info->ir_regins.w_num;
  personvehicle_cpt_info->ir_regins.h_size =
      frame->stVFrame.u32Height / personvehicle_cpt_info->ir_regins.h_num;

  /* set output signal to 0. */
  memset(personvehicle_cpt_info->_output, 0, sizeof(bool) * personvehicle_cpt_info->size);
  if (CVI_TDL_SUCCESS != CVI_TDL_Detection(tdl_handle, frame,
                                           personvehicle_cpt_info->od_model_index,
                                           &personvehicle_cpt_info->last_objects)) {
    return CVI_TDL_FAILURE;
  }
  for (int i = 0; i < personvehicle_cpt_info->last_objects.size; i++) {
    personvehicle_cpt_info->last_objects.info[i].is_cross = false;
  }
#ifndef NO_OPENCV
  if (personvehicle_cpt_info->enable_DeepSORT) {
    if (CVI_TDL_SUCCESS !=
        CVI_TDL_OSNet(tdl_handle, frame, &personvehicle_cpt_info->last_objects)) {
      return CVI_TDL_FAILURE;
    }
  }
#endif
  if (CVI_TDL_SUCCESS != CVI_TDL_DeepSORT_Obj(tdl_handle, &personvehicle_cpt_info->last_objects,
                                              &personvehicle_cpt_info->last_trackers,
                                              personvehicle_cpt_info->enable_DeepSORT)) {
    return CVI_TDL_FAILURE;
  }
  // regin judge
  irregularReginJudge(&personvehicle_cpt_info->ir_regins, personvehicle_cpt_info);

#if 0
  uint64_t mem_used;
  SUMMARY(person_cpt_info, &mem_used, true);
#endif

  /* update timestamp*/
  personvehicle_cpt_info->_time =
      (personvehicle_cpt_info->_time == 0xffffffffffffffff) ? 0 : personvehicle_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCaptureIrregular_Region(personvehicle_capture_t *personvehicle_cpt_info,
                                              int w_num, int h_num, bool *regin_flags) {
  personvehicle_cpt_info->ir_regins.h_num = h_num;
  personvehicle_cpt_info->ir_regins.w_num = w_num;
  memcpy(personvehicle_cpt_info->ir_regins.regin_flags, regin_flags, w_num * h_num * sizeof(_Bool));
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_Free(personvehicle_capture_t *personvehicle_cpt_info) {
  // printf("[APP::PersonCapture] Free PersonCapture Data\n");
  if (personvehicle_cpt_info != NULL) {
    _PersonVehicleCapture_CleanAll(personvehicle_cpt_info);

    free(personvehicle_cpt_info->data);
    CVI_TDL_Free(&personvehicle_cpt_info->last_objects);
    CVI_TDL_Free(&personvehicle_cpt_info->last_trackers);
    if (personvehicle_cpt_info->last_quality != NULL) {
      free(personvehicle_cpt_info->last_quality);
      personvehicle_cpt_info->last_quality = NULL;
    }
    free(personvehicle_cpt_info->_output);

    free(personvehicle_cpt_info);
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_Init(personvehicle_capture_t **personvehicle_cpt_info,
                                   uint32_t buffer_size) {
  printf("[APP::PersonVehicleCapture] Initialize (Buffer Size: %u)\n", buffer_size);
  personvehicle_capture_t *new_personvehicle_cpt_info =
      (personvehicle_capture_t *)malloc(sizeof(personvehicle_capture_t));
  memset(new_personvehicle_cpt_info, 0, sizeof(personvehicle_capture_t));
  new_personvehicle_cpt_info->last_quality = NULL;
  new_personvehicle_cpt_info->size = buffer_size;
  new_personvehicle_cpt_info->data =
      (personvehicle_cpt_data_t *)malloc(sizeof(personvehicle_cpt_data_t) * buffer_size);
  memset(new_personvehicle_cpt_info->data, 0, sizeof(personvehicle_cpt_data_t) * buffer_size);

  new_personvehicle_cpt_info->_output = (bool *)malloc(sizeof(bool) * buffer_size);
  memset(new_personvehicle_cpt_info->_output, 0, sizeof(bool) * buffer_size);

  _PersonVehicleCapture_GetDefaultConfig(&new_personvehicle_cpt_info->cfg);
  new_personvehicle_cpt_info->_m_limit = MEMORY_LIMIT;

  *personvehicle_cpt_info = new_personvehicle_cpt_info;
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_QuickSetUp(cvitdl_handle_t tdl_handle,
                                         personvehicle_capture_t *personvehicle_cpt_info,
                                         const char *od_model_name, const char *od_model_path,
                                         const char *reid_model_path) {
  CVI_S32 ret = CVI_TDL_SUCCESS;
  if (strcmp(od_model_name, "yolov3") == 0) {
    personvehicle_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
  } else if (strcmp(od_model_name, "yolov8") == 0) {
    personvehicle_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION;
  } else {
    return CVI_TDL_FAILURE;
  }

  ret |= CVI_TDL_OpenModel(tdl_handle, personvehicle_cpt_info->od_model_index, od_model_path);

  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, personvehicle_cpt_info->od_model_index, false);
  if (reid_model_path != NULL) {
    ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, reid_model_path);
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, false);
  }

  if (ret != CVI_TDL_SUCCESS) {
    printf("PersonCapture QuickSetUp failed\n");
    return ret;
  }
  personvehicle_cpt_info->enable_DeepSORT = reid_model_path != NULL;

  /* Init DeepSORT */
  CVI_TDL_DeepSORT_Init(tdl_handle, false);

  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, false);

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_GetDefaultConfig(personvehicle_capture_config_t *cfg) {
  cfg->miss_time_limit = MISS_TIME_LIMIT;
  cfg->thr_area_base = THRESHOLD_AREA_BASE;
  cfg->thr_area_min = THRESHOLD_AREA_MIN;
  cfg->thr_area_max = THRESHOLD_AREA_MAX;
  cfg->thr_aspect_ratio_min = THRESHOLD_ASPECT_RATIO_MIN;
  cfg->thr_aspect_ratio_max = THRESHOLD_ASPECT_RATIO_MAX;
  cfg->thr_quality = QUALITY_THRESHOLD;

  cfg->fast_m_interval = FAST_MODE_INTERVAL;
  cfg->fast_m_capture_num = FAST_MODE_CAPTURE_NUM;
  cfg->cycle_m_interval = CYCLE_MODE_INTERVAL;
  cfg->auto_m_time_limit = AUTO_MODE_TIME_LIMIT;
  cfg->auto_m_fast_cap = true;

  cfg->store_RGB888 = false;

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_SetConfig(personvehicle_capture_t *personvehicle_cpt_info,
                                        personvehicle_capture_config_t *cfg,
                                        cvitdl_handle_t tdl_handle) {
  memcpy(&personvehicle_cpt_info->cfg, cfg, sizeof(personvehicle_capture_config_t));
  cvtdl_deepsort_config_t deepsort_conf;
  CVI_TDL_DeepSORT_GetConfig(tdl_handle, &deepsort_conf, -1);
  deepsort_conf.ktracker_conf.max_unmatched_num = cfg->miss_time_limit;
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &deepsort_conf, -1, false);
  SHOW_CONFIG(&personvehicle_cpt_info->cfg);
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_Run(personvehicle_capture_t *personvehicle_cpt_info,
                                  const cvitdl_handle_t tdl_handle, VIDEO_FRAME_INFO_S *frame) {
  if (personvehicle_cpt_info == NULL) {
    printf("[APP::PersonVehicleCapture] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }
  printf("[APP::PersonVehicleCapture] RUN (MODE: %d, ReID: %d)\n", personvehicle_cpt_info->mode,
         personvehicle_cpt_info->enable_DeepSORT);
  CVI_S32 ret;
  ret = clean_data(personvehicle_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    printf("[APP::PersonVehicleCapture] clean data failed.\n");
    return CVI_TDL_FAILURE;
  }
  CVI_TDL_Free(&personvehicle_cpt_info->last_objects);
  CVI_TDL_Free(&personvehicle_cpt_info->last_trackers);
  /* set output signal to 0. */
  memset(personvehicle_cpt_info->_output, 0, sizeof(bool) * personvehicle_cpt_info->size);
  if (CVI_TDL_SUCCESS != CVI_TDL_Detection(tdl_handle, frame,
                                           personvehicle_cpt_info->od_model_index,
                                           &personvehicle_cpt_info->last_objects)) {
    return CVI_TDL_FAILURE;
  }
  for (int i = 0; i < personvehicle_cpt_info->last_objects.size; i++) {
    personvehicle_cpt_info->last_objects.info[i].is_cross = false;
  }
#ifndef NO_OPENCV
  if (personvehicle_cpt_info->enable_DeepSORT) {
    if (CVI_TDL_SUCCESS !=
        CVI_TDL_OSNet(tdl_handle, frame, &personvehicle_cpt_info->last_objects)) {
      return CVI_TDL_FAILURE;
    }
  }
#endif
  if (CVI_TDL_SUCCESS !=
      CVI_TDL_DeepSORT_Obj_Cross(
          tdl_handle, &personvehicle_cpt_info->last_objects, &personvehicle_cpt_info->last_trackers,
          personvehicle_cpt_info->enable_DeepSORT, &personvehicle_cpt_info->cross_line_t,
          &personvehicle_cpt_info->rect)) {
    return CVI_TDL_FAILURE;
  }
#if 0
  uint64_t mem_used;
  SUMMARY(person_cpt_info, &mem_used, true);
#endif

  /* update timestamp*/
  personvehicle_cpt_info->_time =
      (personvehicle_cpt_info->_time == 0xffffffffffffffff) ? 0 : personvehicle_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_Line(personvehicle_capture_t *personvehicle_cpt_info, int A_x,
                                   int A_y, int B_x, int B_y, statistics_mode s_mode) {
  personvehicle_cpt_info->cross_line_t.A_x = A_x;
  personvehicle_cpt_info->cross_line_t.A_y = A_y;
  personvehicle_cpt_info->cross_line_t.B_x = B_x;
  personvehicle_cpt_info->cross_line_t.B_y = B_y;
  personvehicle_cpt_info->cross_line_t.s_mode = s_mode;
  getVehicleBufferRect(&personvehicle_cpt_info->cross_line_t, &personvehicle_cpt_info->rect, 30);
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonVehicleCapture_CleanAll(personvehicle_capture_t *personvehicle_cpt_info) {
  /* Release tracking data */
  for (uint32_t j = 0; j < personvehicle_cpt_info->size; j++) {
    if (personvehicle_cpt_info->data[j].state != IDLE) {
      printf("[APP::PersonVehicleCapture] Clean PersonVehicle Info[%u]\n", j);
      CVI_TDL_Free(&personvehicle_cpt_info->data[j].image);
      CVI_TDL_Free(&personvehicle_cpt_info->data[j].info);
      personvehicle_cpt_info->data[j].state = IDLE;
    }
  }
  return CVI_TDL_SUCCESS;
}

static CVI_S32 clean_data(personvehicle_capture_t *personvehicle_cpt_info) {
  for (uint32_t j = 0; j < personvehicle_cpt_info->size; j++) {
    if (personvehicle_cpt_info->data[j].state == MISS) {
      printf("[APP::PersonVehicleCapture] Clean PersonVechicle Info[%u]\n", j);
      CVI_TDL_Free(&personvehicle_cpt_info->data[j].image);
      CVI_TDL_Free(&personvehicle_cpt_info->data[j].info);
      personvehicle_cpt_info->data[j].state = IDLE;
    }
  }
  return CVI_TDL_SUCCESS;
}

static void SHOW_CONFIG(personvehicle_capture_config_t *cfg) {
  printf("@@@ Person Capture Config @@@\n");
  printf(" - Miss Time Limit:   : %u\n", cfg->miss_time_limit);
  printf(" - Thr Area (Base)    : %d\n", cfg->thr_area_base);
  printf(" - Thr Area (Min)     : %d\n", cfg->thr_area_min);
  printf(" - Thr Area (Max)     : %d\n", cfg->thr_area_max);
  printf(" - Thr Aspect Ratio (Min) : %.2f\n", cfg->thr_aspect_ratio_min);
  printf(" - Thr Aspect Ratio (Max) : %.2f\n", cfg->thr_aspect_ratio_max);
  printf(" - Thr Quality        : %.2f\n", cfg->thr_quality);
  printf("[Fast] Interval     : %u\n", cfg->fast_m_interval);
  printf("[Fast] Capture Num  : %u\n", cfg->fast_m_capture_num);
  printf("[Cycle] Interval    : %u\n", cfg->cycle_m_interval);
  printf("[Auto] Time Limit   : %u\n\n", cfg->auto_m_time_limit);
  printf("[Auto] Fast Capture : %s\n\n", cfg->auto_m_fast_cap ? "True" : "False");
  printf(" - Store RGB888         : %s\n\n", cfg->store_RGB888 ? "True" : "False");
  return;
}
