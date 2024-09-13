#include "person_capture.h"
#include <inttypes.h>
#include <math.h>
#include "core/cvi_tdl_utils.h"
#include "cvi_tdl_log.hpp"
#include "default_config.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MEMORY_LIMIT (16 * 1024 * 1024) /* example: 16MB */

#define CAPTURE_TARGET_LIVE_TIME_EXTEND 5
#define UPDATE_VALUE_MIN 0.05
// TODO: Use cooldown to avoid too much updating
// #define UPDATE_COOLDOWN 3

/* person capture functions (core) */
static CVI_S32 update_data(person_capture_t *person_cpt_info, cvtdl_object_t *obj_meta,
                           cvtdl_tracker_t *tracker_meta, float *quality);
static CVI_S32 clean_data(person_capture_t *person_cpt_info);
static CVI_S32 capture_target(person_capture_t *person_cpt_info, VIDEO_FRAME_INFO_S *frame,
                              cvtdl_object_t *obj_meta);
static void quality_assessment(person_capture_t *person_cpt_info, float *quality);

/* person capture functions (helper) */
static bool is_qualified(person_capture_t *person_cpt_info, float quality, float current_quality);

/* other helper functions */
static bool IS_MEMORY_ENOUGH(uint32_t mem_limit, uint64_t mem_used, cvtdl_image_t *current_image,
                             cvtdl_bbox_t *new_bbox, PIXEL_FORMAT_E fmt);
static void SUMMARY(person_capture_t *person_cpt_info, uint64_t *size, bool show_detail);
static void SHOW_CONFIG(person_capture_config_t *cfg);

void getBufferRect(const cvtdl_counting_line_t *counting_line_t, randomRect *rect,
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
    // // k b horizontal line
    rect->k = 0;
    rect->b = counting_line_t->A_y;
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
    //  k b Vertical line
    rect->k = counting_line_t->A_x;
    rect->b = -1000;
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
    rect->k = k;
    rect->b = counting_line_t->A_y - k * counting_line_t->A_x;
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
    rect->k = k;
    rect->b = counting_line_t->A_y - k * counting_line_t->A_x;
    if (s_mode == 0) {
      rect->f_x = -1.0;
      rect->f_y = 1.0 / k;
    } else {
      rect->f_x = 1;
      rect->f_y = -1.0 / k;
    }
  }
  printf("buffer rectangle: %f %f %f %f %f %f %f %f %f %f\n", rect->lt_x, rect->lt_y, rect->rt_x,
         rect->rt_y, rect->rb_x, rect->rb_y, rect->lb_x, rect->lb_y, rect->k, rect->b);
}

CVI_S32 _PersonCapture_Free(person_capture_t *person_cpt_info) {
  LOGI("[APP::PersonCapture] Free PersonCapture Data\n");
  if (person_cpt_info != NULL) {
    _PersonCapture_CleanAll(person_cpt_info);

    free(person_cpt_info->data);
    CVI_TDL_Free(&person_cpt_info->last_objects);
    CVI_TDL_Free(&person_cpt_info->last_trackers);
    if (person_cpt_info->last_quality != NULL) {
      free(person_cpt_info->last_quality);
      person_cpt_info->last_quality = NULL;
    }
    free(person_cpt_info->_output);

    free(person_cpt_info);
  }
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_Init(person_capture_t **person_cpt_info, uint32_t buffer_size) {
  if (*person_cpt_info != NULL) {
    printf("[APP::PersonCapture] already exist.\n");
    return CVI_TDL_SUCCESS;
  }
  printf("[APP::PersonCapture] Initialize (Buffer Size: %u)\n", buffer_size);
  person_capture_t *new_person_cpt_info = (person_capture_t *)malloc(sizeof(person_capture_t));
  memset(new_person_cpt_info, 0, sizeof(person_capture_t));
  new_person_cpt_info->last_quality = NULL;
  new_person_cpt_info->size = buffer_size;
  new_person_cpt_info->data = (person_cpt_data_t *)malloc(sizeof(person_cpt_data_t) * buffer_size);
  memset(new_person_cpt_info->data, 0, sizeof(person_cpt_data_t) * buffer_size);

  new_person_cpt_info->_output = (bool *)malloc(sizeof(bool) * buffer_size);
  memset(new_person_cpt_info->_output, 0, sizeof(bool) * buffer_size);

  _PersonCapture_GetDefaultConfig(&new_person_cpt_info->cfg);
  new_person_cpt_info->_m_limit = MEMORY_LIMIT;

  *person_cpt_info = new_person_cpt_info;
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_QuickSetUp(cvitdl_handle_t tdl_handle, person_capture_t *person_cpt_info,
                                  const char *od_model_name, const char *od_model_path,
                                  const char *reid_model_path) {
  CVI_S32 ret = CVI_TDL_SUCCESS;

  if (strcmp(od_model_name, "mobiledetv2-person-vehicle") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_VEHICLE;
  } else if (strcmp(od_model_name, "mobiledetv2-person-pets") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PERSON_PETS;
  } else if (strcmp(od_model_name, "mobiledetv2-coco80") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_COCO80;
  } else if (strcmp(od_model_name, "mobiledetv2-pedestrian") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_MOBILEDETV2_PEDESTRIAN;
  } else if (strcmp(od_model_name, "yolov3") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_YOLOV3;
  } else if (strcmp(od_model_name, "yolov8") == 0) {
    person_cpt_info->od_model_index = CVI_TDL_SUPPORTED_MODEL_HEAD_PERSON_DETECTION;
  } else {
    return CVI_TDL_FAILURE;
  }

  ret |= CVI_TDL_OpenModel(tdl_handle, person_cpt_info->od_model_index, od_model_path);
  CVI_TDL_SetSkipVpssPreprocess(tdl_handle, person_cpt_info->od_model_index, false);
  if (reid_model_path != NULL) {
    ret |= CVI_TDL_OpenModel(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, reid_model_path);
    CVI_TDL_SetSkipVpssPreprocess(tdl_handle, CVI_TDL_SUPPORTED_MODEL_OSNET, false);
  }
  if (ret != CVI_TDL_SUCCESS) {
    printf("PersonCapture QuickSetUp failed\n");
    return ret;
  }
  person_cpt_info->enable_DeepSORT = reid_model_path != NULL;

  /* Init DeepSORT */
  CVI_TDL_DeepSORT_Init(tdl_handle, false);

  cvtdl_deepsort_config_t ds_conf;
  CVI_TDL_DeepSORT_GetDefaultConfig(&ds_conf);
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &ds_conf, -1, false);

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_GetDefaultConfig(person_capture_config_t *cfg) {
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

CVI_S32 _PersonCapture_SetConfig(person_capture_t *person_cpt_info, person_capture_config_t *cfg,
                                 cvitdl_handle_t tdl_handle) {
  memcpy(&person_cpt_info->cfg, cfg, sizeof(person_capture_config_t));
  cvtdl_deepsort_config_t deepsort_conf;
  CVI_TDL_DeepSORT_GetConfig(tdl_handle, &deepsort_conf, -1);
  deepsort_conf.ktracker_conf.max_unmatched_num = cfg->miss_time_limit;
  CVI_TDL_DeepSORT_SetConfig(tdl_handle, &deepsort_conf, -1, false);
  SHOW_CONFIG(&person_cpt_info->cfg);
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_Run(person_capture_t *person_cpt_info, const cvitdl_handle_t tdl_handle,
                           VIDEO_FRAME_INFO_S *frame) {
  if (person_cpt_info == NULL) {
    printf("[APP::PersonCapture] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }
  printf("[APP::PersonCapture] RUN (MODE: %d, ReID: %d)\n", person_cpt_info->mode,
         person_cpt_info->enable_DeepSORT);
  CVI_S32 ret;
  ret = clean_data(person_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    printf("[APP::PersonCapture] clean data failed.\n");
    return CVI_TDL_FAILURE;
  }
  CVI_TDL_Free(&person_cpt_info->last_objects);
  CVI_TDL_Free(&person_cpt_info->last_trackers);
  /* set output signal to 0. */
  memset(person_cpt_info->_output, 0, sizeof(bool) * person_cpt_info->size);

  if (CVI_TDL_SUCCESS != CVI_TDL_Detection(tdl_handle, frame, person_cpt_info->od_model_index,
                                           &person_cpt_info->last_objects)) {
    return CVI_TDL_FAILURE;
  }
  if (person_cpt_info->enable_DeepSORT) {
    if (CVI_TDL_SUCCESS != CVI_TDL_OSNet(tdl_handle, frame, &person_cpt_info->last_objects)) {
      return CVI_TDL_FAILURE;
    }
  }
  if (CVI_TDL_SUCCESS != CVI_TDL_DeepSORT_Obj(tdl_handle, &person_cpt_info->last_objects,
                                              &person_cpt_info->last_trackers,
                                              person_cpt_info->enable_DeepSORT)) {
    return CVI_TDL_FAILURE;
  }

  if (person_cpt_info->last_quality != NULL) {
    free(person_cpt_info->last_quality);
    person_cpt_info->last_quality = NULL;
  }
  person_cpt_info->last_quality =
      (float *)malloc(sizeof(float) * person_cpt_info->last_objects.size);
  memset(person_cpt_info->last_quality, 0, sizeof(float) * person_cpt_info->last_objects.size);

  quality_assessment(person_cpt_info, person_cpt_info->last_quality);
#if 0
  for (uint32_t i = 0; i < person_cpt_info->last_objects.size; i++) {
    cvtdl_bbox_t *bbox = &person_cpt_info->last_objects.info[i].bbox;
    printf("[%u][%d] quality[%.2f], x1[%.2f], y1[%.2f], x2[%.2f], y2[%.2f], h[%.2f], w[%.2f]\n", i,
           person_cpt_info->last_objects.info[i].classes, person_cpt_info->last_quality[i],
           bbox->x1, bbox->y1, bbox->x2, bbox->y2, (bbox->y2 - bbox->y1), (bbox->x2 - bbox->x1));
  }
#endif

  ret = update_data(person_cpt_info, &person_cpt_info->last_objects,
                    &person_cpt_info->last_trackers, person_cpt_info->last_quality);
  if (ret != CVI_TDL_SUCCESS) {
    printf("[APP::PersonCapture] update data failed.\n");
    return CVI_TDL_FAILURE;
  }
  ret = capture_target(person_cpt_info, frame, &person_cpt_info->last_objects);
  if (ret != CVI_TDL_SUCCESS) {
    printf("[APP::PersonCapture] capture target failed.\n");
    return CVI_TDL_FAILURE;
  }

#if 0
  uint64_t mem_used;
  SUMMARY(person_cpt_info, &mem_used, true);
#endif

  /* update timestamp*/
  person_cpt_info->_time =
      (person_cpt_info->_time == 0xffffffffffffffff) ? 0 : person_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}
CVI_S32 _ConsumerCounting_Line(person_capture_t *person_cpt_info, int A_x, int A_y, int B_x,
                               int B_y, statistics_mode s_mode) {
  person_cpt_info->counting_line_t.A_x = A_x;
  person_cpt_info->counting_line_t.A_y = A_y;
  person_cpt_info->counting_line_t.B_x = B_x;
  person_cpt_info->counting_line_t.B_y = B_y;
  person_cpt_info->counting_line_t.s_mode = s_mode;
  getBufferRect(&person_cpt_info->counting_line_t, &person_cpt_info->rect, 10);
  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_SetMode(person_capture_t *person_cpt_info, capture_mode_e mode) {
  person_cpt_info->mode = mode;
  return CVI_TDL_SUCCESS;
}

CVI_S32 _ConsumerCounting_Run(person_capture_t *person_cpt_info, const cvitdl_handle_t tdl_handle,
                              VIDEO_FRAME_INFO_S *frame) {
  if (person_cpt_info == NULL) {
    printf("[APP::ConsumerCounting] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }
  printf("[APP::ConsumerCounting] RUN (MODE: %d, ReID: %d)\n", person_cpt_info->mode,
         person_cpt_info->enable_DeepSORT);
  CVI_S32 ret;
  ret = clean_data(person_cpt_info);
  if (ret != CVI_TDL_SUCCESS) {
    printf("[APP::ConsumerCounting] clean data failed.\n");
    return CVI_TDL_FAILURE;
  }
  CVI_TDL_Free(&person_cpt_info->last_objects);
  CVI_TDL_Free(&person_cpt_info->last_trackers);
  CVI_TDL_Free(&person_cpt_info->last_head);
  CVI_TDL_Free(&person_cpt_info->last_ped);

  /* set output signal to 0. */
  memset(person_cpt_info->_output, 0, sizeof(bool) * person_cpt_info->size);
  if (CVI_TDL_SUCCESS != CVI_TDL_Detection(tdl_handle, frame, person_cpt_info->od_model_index,
                                           &person_cpt_info->last_objects)) {
    return CVI_TDL_FAILURE;
  }
#ifndef NO_OPENCV
  if (person_cpt_info->enable_DeepSORT) {
    if (CVI_TDL_SUCCESS != CVI_TDL_OSNet(tdl_handle, frame, &person_cpt_info->last_objects)) {
      return CVI_TDL_FAILURE;
    }
  }
#endif
  if (person_cpt_info->od_model_index == CVI_TDL_SUPPORTED_MODEL_HEAD_PERSON_DETECTION) {
    if (CVI_TDL_SUCCESS != CVI_TDL_DeepSORT_Head_FusePed(
                               tdl_handle, &person_cpt_info->last_objects,
                               &person_cpt_info->last_trackers, person_cpt_info->enable_DeepSORT,
                               &person_cpt_info->last_head, &person_cpt_info->last_ped,
                               &person_cpt_info->counting_line_t, &person_cpt_info->rect))
      return CVI_TDL_FAILURE;
  } else {
    if (CVI_TDL_SUCCESS != CVI_TDL_DeepSORT_Obj(tdl_handle, &person_cpt_info->last_objects,
                                                &person_cpt_info->last_trackers,
                                                person_cpt_info->enable_DeepSORT)) {
      return CVI_TDL_FAILURE;
    }
  }

  /* update timestamp*/
  person_cpt_info->_time =
      (person_cpt_info->_time == 0xffffffffffffffff) ? 0 : person_cpt_info->_time + 1;

  return CVI_TDL_SUCCESS;
}

CVI_S32 _PersonCapture_CleanAll(person_capture_t *person_cpt_info) {
  /* Release tracking data */
  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (person_cpt_info->data[j].state != IDLE) {
      printf("[APP::PersonCapture] Clean Person Info[%u]\n", j);
      CVI_TDL_Free(&person_cpt_info->data[j].image);
      CVI_TDL_Free(&person_cpt_info->data[j].info);
      person_cpt_info->data[j].state = IDLE;
    }
  }
  return CVI_TDL_SUCCESS;
}

static void quality_assessment(person_capture_t *person_cpt_info, float *quality) {
  for (uint32_t i = 0; i < person_cpt_info->last_objects.size; i++) {
    cvtdl_bbox_t *bbox = &person_cpt_info->last_objects.info[i].bbox;
    float aspect_ratio = (bbox->y2 - bbox->y1) / (bbox->x2 - bbox->x1);
    if (aspect_ratio < person_cpt_info->cfg.thr_aspect_ratio_min ||
        aspect_ratio > person_cpt_info->cfg.thr_aspect_ratio_max) {
      quality[i] = 0.;
      continue;
    }
    float area = (bbox->y2 - bbox->y1) * (bbox->x2 - bbox->x1);
    if (area < person_cpt_info->cfg.thr_area_min || area > person_cpt_info->cfg.thr_area_max) {
      quality[i] = 0.;
      continue;
    }
    float area_score = MIN(1.0, area / person_cpt_info->cfg.thr_area_base);
    quality[i] = area_score;
  }
  return;
}

static CVI_S32 update_data(person_capture_t *person_cpt_info, cvtdl_object_t *obj_meta,
                           cvtdl_tracker_t *tracker_meta, float *quality) {
  printf("[APP::PersonCapture] Update Data\n");
  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (person_cpt_info->data[j].state == ALIVE) {
      person_cpt_info->data[j].miss_counter += 1;
    }
  }
  for (uint32_t i = 0; i < tracker_meta->size; i++) {
    /* we only consider the stable tracker in this sample code. */
    if (tracker_meta->info[i].state != CVI_TRACKER_STABLE) {
      continue;
    }

    uint64_t trk_id = obj_meta->info[i].unique_id;
    /* check whether the tracker id exist or not. */
    int match_idx = -1;
    for (uint32_t j = 0; j < person_cpt_info->size; j++) {
      if (person_cpt_info->data[j].state == ALIVE &&
          person_cpt_info->data[j].info.unique_id == trk_id) {
        match_idx = (int)j;
        break;
      }
    }
    if (match_idx == -1) {
      /* if not found, create new one. */
      bool is_created = false;
      /* search available index for new tracker. */
      for (uint32_t j = 0; j < person_cpt_info->size; j++) {
        if (person_cpt_info->data[j].state == IDLE && person_cpt_info->data[j]._capture == false) {
          // printf("[APP::PersonCapture] Create Target Info[%u]\n", j);
          person_cpt_info->data[j].miss_counter = 0;
          memcpy(&person_cpt_info->data[j].info, &obj_meta->info[i], sizeof(cvtdl_object_info_t));
          person_cpt_info->data[j].quality = quality[i];
          /* set useless heap data structure to 0 */
          memset(&person_cpt_info->data[j].info.feature, 0, sizeof(cvtdl_feature_t));

          /* always capture target in the first frame. */
          person_cpt_info->data[j]._capture = true;
          person_cpt_info->data[j]._timestamp = person_cpt_info->_time;
          person_cpt_info->data[j]._out_counter = 0;
          is_created = true;
          break;
        }
      }
      /* fail to create */
      if (!is_created) {
        printf("[APP::PersonCapture] Buffer overflow! (Ignore person[%u])\n", i);
      }
    } else {
      person_cpt_info->data[match_idx].miss_counter = 0;
      bool capture = false;
      uint64_t _time = person_cpt_info->_time - person_cpt_info->data[match_idx]._timestamp;
      float current_quality = person_cpt_info->data[match_idx].quality;
      switch (person_cpt_info->mode) {
        case AUTO: {
          bool time_out = (person_cpt_info->cfg.auto_m_time_limit != 0) &&
                          (_time > person_cpt_info->cfg.auto_m_time_limit);
          if (!time_out) {
            float update_quality_threshold = (current_quality < person_cpt_info->cfg.thr_quality)
                                                 ? 0.
                                                 : MIN(1.0, current_quality + UPDATE_VALUE_MIN);
            capture = is_qualified(person_cpt_info, quality[i], update_quality_threshold);
          }
        } break;
        case FAST: {
          if (person_cpt_info->data[match_idx]._out_counter <
              person_cpt_info->cfg.fast_m_capture_num) {
            if (_time < person_cpt_info->cfg.fast_m_interval) {
              float update_quality_threshold;
              if (current_quality < person_cpt_info->cfg.thr_quality) {
                update_quality_threshold = -1;
              } else {
                update_quality_threshold = ((current_quality + UPDATE_VALUE_MIN) >= 1.0)
                                               ? current_quality
                                               : current_quality + UPDATE_VALUE_MIN;
              }
              capture = is_qualified(person_cpt_info, quality[i], update_quality_threshold);
            } else {
              capture = true;
              person_cpt_info->data[match_idx]._timestamp = person_cpt_info->_time;
            }
          }
        } break;
        case CYCLE: {
          if (_time < person_cpt_info->cfg.cycle_m_interval) {
            float update_quality_threshold;
            if (current_quality < person_cpt_info->cfg.thr_quality) {
              update_quality_threshold = -1;
            } else {
              update_quality_threshold = ((current_quality + UPDATE_VALUE_MIN) >= 1.0)
                                             ? current_quality
                                             : current_quality + UPDATE_VALUE_MIN;
            }
            capture = is_qualified(person_cpt_info, quality[i], update_quality_threshold);
          } else {
            capture = true;
            person_cpt_info->data[match_idx]._timestamp = person_cpt_info->_time;
          }
        } break;
        default: {
          // NOTE: consider non-free tracker because it won't set MISS
          printf("Unsupported type.\n");
          return CVI_TDL_ERR_INVALID_ARGS;
        } break;
      }
      /* if found, check whether the quality(or feature) need to be update. */
      if (capture) {
        printf("[APP::PersonCapture] Update Person Info[%u]\n", match_idx);
        memcpy(&person_cpt_info->data[match_idx].info, &obj_meta->info[i],
               sizeof(cvtdl_object_info_t));
        /* set useless heap data structure to 0 */
        memset(&person_cpt_info->data[match_idx].info.feature, 0, sizeof(cvtdl_feature_t));

        person_cpt_info->data[match_idx].quality = quality[i];
        person_cpt_info->data[match_idx]._capture = true;
      }
      switch (person_cpt_info->mode) {
        case AUTO: {
          /* We use fast interval for the first capture */
          if (person_cpt_info->cfg.auto_m_fast_cap) {
            if (_time >= person_cpt_info->cfg.fast_m_interval &&
                person_cpt_info->data[match_idx]._out_counter < 1 &&
                is_qualified(person_cpt_info, person_cpt_info->data[match_idx].quality, -1)) {
              person_cpt_info->_output[match_idx] = true;
              person_cpt_info->data[match_idx]._out_counter += 1;
            }
          }
          if (person_cpt_info->cfg.auto_m_time_limit != 0) {
            /* Time's up */
            if (_time == person_cpt_info->cfg.auto_m_time_limit &&
                is_qualified(person_cpt_info, person_cpt_info->data[match_idx].quality, -1)) {
              person_cpt_info->_output[match_idx] = true;
              person_cpt_info->data[match_idx]._out_counter += 1;
            }
          }
        } break;
        case FAST: {
          if (person_cpt_info->data[match_idx]._out_counter <
              person_cpt_info->cfg.fast_m_capture_num) {
            if (_time == person_cpt_info->cfg.fast_m_interval - 1 &&
                is_qualified(person_cpt_info, person_cpt_info->data[match_idx].quality, -1)) {
              person_cpt_info->_output[match_idx] = true;
              person_cpt_info->data[match_idx]._out_counter += 1;
            }
          }
        } break;
        case CYCLE: {
          if (_time == person_cpt_info->cfg.cycle_m_interval - 1 &&
              is_qualified(person_cpt_info, person_cpt_info->data[match_idx].quality, -1)) {
            person_cpt_info->_output[match_idx] = true;
            person_cpt_info->data[match_idx]._out_counter += 1;
          }
        } break;
        default:
          return CVI_TDL_FAILURE;
      }
    }
  }

  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    /* NOTE: For more flexible application, we do not remove the tracker immediately when time out
     */
    if (person_cpt_info->data[j].state == ALIVE &&
        person_cpt_info->data[j].miss_counter >
            person_cpt_info->cfg.miss_time_limit + CAPTURE_TARGET_LIVE_TIME_EXTEND) {
      person_cpt_info->data[j].state = MISS;
      switch (person_cpt_info->mode) {
        case AUTO: {
          if (is_qualified(person_cpt_info, person_cpt_info->data[j].quality, -1)) {
            person_cpt_info->_output[j] = true;
          }
        } break;
        case FAST:
        case CYCLE: {
          /* at least capture 1 target */
          if (is_qualified(person_cpt_info, person_cpt_info->data[j].quality, -1) &&
              person_cpt_info->data[j]._out_counter == 0) {
            person_cpt_info->_output[j] = true;
          }
        } break;
        default:
          return CVI_TDL_FAILURE;
          break;
      }
    }
  }

  return CVI_TDL_SUCCESS;
}

static CVI_S32 clean_data(person_capture_t *person_cpt_info) {
  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (person_cpt_info->data[j].state == MISS) {
      printf("[APP::PersonCapture] Clean Person Info[%u]\n", j);
      CVI_TDL_Free(&person_cpt_info->data[j].image);
      CVI_TDL_Free(&person_cpt_info->data[j].info);
      person_cpt_info->data[j].state = IDLE;
    }
  }
  return CVI_TDL_SUCCESS;
}

static CVI_S32 capture_target(person_capture_t *person_cpt_info, VIDEO_FRAME_INFO_S *frame,
                              cvtdl_object_t *obj_meta) {
  printf("[APP::PersonCapture] Capture Target\n");
  if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_RGB_888_PLANAR &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_NV21 &&
      frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YUV_PLANAR_420) {
    printf("Pixel format [%d] is not supported.\n", frame->stVFrame.enPixelFormat);
    return CVI_TDL_ERR_INVALID_ARGS;
  }

  bool capture = false;
  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (person_cpt_info->data[j]._capture) {
      capture = true;
      break;
    }
  }
  if (!capture) {
    return CVI_TDL_SUCCESS;
  }
  /* Estimate memory used */
  uint64_t mem_used;
  SUMMARY(person_cpt_info, &mem_used, false);

  bool do_unmap = false;
  size_t image_size =
      frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
  if (frame->stVFrame.pu8VirAddr[0] == NULL) {
    frame->stVFrame.pu8VirAddr[0] =
        (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
    frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
    do_unmap = true;
  }

  for (uint32_t j = 0; j < person_cpt_info->size; j++) {
    if (!(person_cpt_info->data[j]._capture)) {
      continue;
    }
    bool first_capture = false;
    if (person_cpt_info->data[j].state != ALIVE) {
      /* first capture */
      person_cpt_info->data[j].state = ALIVE;
      first_capture = true;
    }
    printf("Capture Target[%u] (%s)!\n", j, (first_capture) ? "INIT" : "UPDATE");

    /* Check remaining memory space */
    if (!IS_MEMORY_ENOUGH(person_cpt_info->_m_limit, mem_used, &person_cpt_info->data[j].image,
                          &person_cpt_info->data[j].info.bbox, frame->stVFrame.enPixelFormat)) {
      printf("Memory is not enough. (drop)\n");
      if (first_capture) {
        person_cpt_info->data[j].state = IDLE;
      }
      continue;
    }
    CVI_TDL_Free(&person_cpt_info->data[j].image);

    CVI_TDL_CropImage(frame, &person_cpt_info->data[j].image, &person_cpt_info->data[j].info.bbox,
                      person_cpt_info->cfg.store_RGB888);

    person_cpt_info->data[j]._capture = false;
  }
  if (do_unmap) {
    CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], image_size);
    frame->stVFrame.pu8VirAddr[0] = NULL;
    frame->stVFrame.pu8VirAddr[1] = NULL;
    frame->stVFrame.pu8VirAddr[2] = NULL;
  }
  return CVI_TDL_SUCCESS;
}

static bool is_qualified(person_capture_t *person_cpt_info, float quality, float current_quality) {
  if (quality >= person_cpt_info->cfg.thr_quality && quality > current_quality) {
    return true;
  }
  return false;
}

static bool IS_MEMORY_ENOUGH(uint32_t mem_limit, uint64_t mem_used, cvtdl_image_t *current_image,
                             cvtdl_bbox_t *new_bbox, PIXEL_FORMAT_E fmt) {
  mem_used -= (current_image->length[0] + current_image->length[1] + current_image->length[2]);
  uint32_t new_h = (uint32_t)roundf(new_bbox->y2) - (uint32_t)roundf(new_bbox->y1) + 1;
  uint32_t new_w = (uint32_t)roundf(new_bbox->x2) - (uint32_t)roundf(new_bbox->x1) + 1;
  uint64_t new_size;
  CVI_TDL_EstimateImageSize(&new_size, ((new_h + 1) >> 1) << 1, ((new_w + 1) >> 1) << 1, fmt);
  if (mem_limit - mem_used < new_size) {
    return false;
  } else {
    // printf("<remaining: %u, needed: %u>\n", mem_limit - mem_used, new_s * new_h);
    return true;
  }
}

static void SUMMARY(person_capture_t *person_cpt_info, uint64_t *size, bool show_detail) {
  *size = 0;
  if (show_detail) {
    printf("@@@@ SUMMARY @@@@\n");
  }
  for (uint32_t i = 0; i < person_cpt_info->size; i++) {
    tracker_state_e state = person_cpt_info->data[i].state;
    if (state == IDLE) {
      if (show_detail) {
        printf("Data[%u] state[IDLE]\n", i);
      }
    } else {
      uint64_t m = person_cpt_info->data[i].image.length[0];
      m += person_cpt_info->data[i].image.length[1];
      m += person_cpt_info->data[i].image.length[2];
      if (show_detail) {
        printf("Data[%u] state[%s], h[%u], w[%u], size[%" PRIu64 "]\n", i,
               (state == ALIVE) ? "ALIVE" : "MISS", person_cpt_info->data[i].image.height,
               person_cpt_info->data[i].image.width, m);
      }
      *size += m;
    }
  }
  if (show_detail) {
    printf("MEMORY USED: %" PRIu64 "\n\n", *size);
  }
}

static void SHOW_CONFIG(person_capture_config_t *cfg) {
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
