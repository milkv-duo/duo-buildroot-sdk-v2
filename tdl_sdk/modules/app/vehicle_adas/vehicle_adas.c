#include "vehicle_adas.h"

#include <math.h>
#include <sys/time.h>
#include "core/cvi_tdl_utils.h"
#include "cvi_tdl_log.hpp"
#include "cvi_venc.h"
#include "service/cvi_tdl_service.h"

#define AVG_CAR_HEIGHT 1.4
#define AVG_BUS_HEIGHT 2.6
#define AVG_TRUCK_HEIGHT 3.5
#define AVG_RIDER_HEIGHT 1.4
#define AVG_PERSON_HEIGHT 1.6
#define AVG_DEFAULT_HEIGHT 1.2

#define ALPHA_ADAS 0.475

int update_easy_queue(int *data, int new_val, int size) {
  int sum = 0;
  for (int i = 0; i < size - 1; i++) {
    data[i] = data[i + 1];
    sum += data[i];
  }
  data[size - 1] = new_val;
  sum += new_val;
  return sum;
}

CVI_S32 _ADAS_Init(adas_info_t **adas_info, uint32_t buffer_size, int det_type) {
  if (*adas_info != NULL) {
    LOGW("[APP::ADAS] already exist.\n");
    return CVI_TDL_SUCCESS;
  }
  LOGI("[APP::ADAS] Initialize (Buffer Size: %u)\n", buffer_size);
  adas_info_t *new_adas_info = (adas_info_t *)malloc(sizeof(adas_info_t));
  memset(new_adas_info, 0, sizeof(adas_info_t));
  new_adas_info->size = buffer_size;
  new_adas_info->miss_time_limit = 10;
  new_adas_info->is_static = true;
  new_adas_info->FPS = 25;
  new_adas_info->departure_time = 1;
  new_adas_info->det_type = det_type;

  new_adas_info->data = (adas_data_t *)malloc(sizeof(adas_data_t) * buffer_size);
  memset(new_adas_info->data, 0, sizeof(adas_data_t) * buffer_size);

  *adas_info = new_adas_info;

  return CVI_SUCCESS;
}

CVI_S32 _ADAS_Free(adas_info_t *adas_info) {
  LOGI("[APP::ADAS] Free FaceCapture Data\n");
  if (adas_info != NULL) {
    /* Release tracking data */
    for (uint32_t j = 0; j < adas_info->size; j++) {
      if (adas_info->data[j].t_state != IDLE) {
        LOGI("[APP::ADAS] Clean Face Info[%u]\n", j);
        CVI_TDL_Free(&adas_info->data[j].info);
        adas_info->data[j].t_state = IDLE;
      }
    }

    free(adas_info->data);
    CVI_TDL_Free(&adas_info->last_objects);
    CVI_TDL_Free(&adas_info->last_trackers);
    CVI_TDL_Free(&adas_info->lane_meta);
    free(adas_info);
  }

  return CVI_TDL_SUCCESS;
}

float sum_acc(float *data, int size) {
  float sum = 0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum;
}

float obj_dis(cvtdl_object_info_t *info, float height) {
  float obj_height;
  switch (info->classes) {
    case 0:
      obj_height = AVG_CAR_HEIGHT;
      break;
    case 1:
      obj_height = AVG_BUS_HEIGHT;
      break;
    case 2:
      obj_height = AVG_TRUCK_HEIGHT;
      break;
    case 3:
      obj_height = AVG_RIDER_HEIGHT;
      break;
    case 4:
      obj_height = AVG_PERSON_HEIGHT;
      break;
    default:
      obj_height = AVG_DEFAULT_HEIGHT;
  }

  float dis = ALPHA_ADAS * obj_height * height / (info->bbox.y2 - info->bbox.y1);

  if (info->bbox.y2 / height > 0.83) {
    dis *= 0.6;
  }

  return dis;
}

void update_dis(cvtdl_object_info_t *info, adas_data_t *data, float height, bool first_time) {
  float cur_dis = obj_dis(info, height);

  if (!first_time) {
    cur_dis = 0.9 * data->dis + 0.1 * cur_dis;
  } else {
    data->dis_tmp = cur_dis;
  }

  data->dis = cur_dis;
  info->adas_properity.dis = cur_dis;
}

void update_apeed(cvtdl_object_info_t *info, adas_data_t *data, float fps) {
  info->adas_properity.speed = data->speed;

  if (data->counter >= 3) {
    float cur_speed = (data->dis - data->dis_tmp) / ((float)(data->counter - 1) / fps);

    data->speed = cur_speed;

    // printf(" data->dis  %.2f   data->dis_tmp   %.2f  cur_speed: %.2f\n", data->dis,
    // data->dis_tmp, cur_speed); data->speed = cur_speed;
    info->adas_properity.speed = cur_speed;

    data->dis_tmp = data->dis;
    data->counter = 0;
  }
}

void update_object_state(cvtdl_object_info_t *info, adas_data_t *data, adas_info_t *adas_info,
                         uint32_t width, int obj_index, bool self_static) {
  info->adas_properity.state = NORMAL;
  int *center_info = adas_info->center_info;

  float cur_start_score = 0;
  float cur_warning_score = 0;
  if (self_static && center_info[0] == obj_index && info->adas_properity.dis < 4.0f &&
      data->speed > 0.25) {
    cur_start_score = 1.0f;
  }

  data->start_score = 0.9 * data->start_score + 0.1 * cur_start_score;

  if (data->start_score > adas_info->FPS / 50.0) {
    info->adas_properity.state = START;
  }

  bool center = center_info[0] == obj_index;
  if ((center && info->adas_properity.dis < 6.0 && data->speed < info->adas_properity.dis / -2.0) ||
      (center && info->adas_properity.dis < 2.6)) {
    cur_warning_score = 1.0f;
  }
  data->warning_score = 0.9 * data->warning_score + 0.1 * cur_warning_score;
  if (data->warning_score > adas_info->FPS / 50.0) {
    info->adas_properity.state = COLLISION_WARNING;
  }
}

void update_frame_state(adas_info_t *adas_info) {
  if (adas_info->gsensor_data.counter > adas_info->gsensor_tmp_data.counter) {
    if (adas_info->gsensor_data.counter > 1) {
      int diff = abs(adas_info->gsensor_data.x - adas_info->gsensor_tmp_data.x) +
                 abs(adas_info->gsensor_data.y - adas_info->gsensor_tmp_data.y) +
                 abs(adas_info->gsensor_data.z - adas_info->gsensor_tmp_data.z);

      int gsensor_period_sum = update_easy_queue(adas_info->gsensor_queue, diff, 10);

      if (gsensor_period_sum > 22) {
        adas_info->is_static = false;
      } else {
        adas_info->is_static = true;
      }
    }
    adas_info->gsensor_tmp_data = adas_info->gsensor_data;
  }
}

void obj_filter(cvtdl_object_t *obj_meta) {
  int size = obj_meta->size;
  if (size > 0) {
    int valid[size];
    int count = 0;
    memset(valid, 0, sizeof(valid));
    for (int i = 0; i < size; i++) {
      if ((obj_meta->info[i].bbox.x2 - obj_meta->info[i].bbox.x1) /
              (obj_meta->info[i].bbox.y2 - obj_meta->info[i].bbox.y1) <
          3) {
        valid[i] = 1;
        count += 1;
      }
    }

    if (count < size) {
      obj_meta->size = count;
      cvtdl_object_info_t *new_info = NULL;

      if (count > 0 && obj_meta->info) {
        new_info = (cvtdl_object_info_t *)malloc(sizeof(cvtdl_object_info_t) * count);
        memset(new_info, 0, sizeof(cvtdl_object_info_t) * count);

        count = 0;
        for (int oid = 0; oid < size; oid++) {
          if (valid[oid]) {
            CVI_TDL_CopyObjectInfo(&obj_meta->info[oid], &new_info[count]);
            count++;
          }
          CVI_TDL_Free(&obj_meta->info[oid]);
        }
      }

      free(obj_meta->info);
      obj_meta->info = new_info;
    }
  }
}

static CVI_S32 clean_data(adas_info_t *adas_info) {
  for (uint32_t j = 0; j < adas_info->size; j++) {
    if (adas_info->data[j].t_state == MISS) {
      LOGI("[APP::VehicleAdas] Clean Vehicle Info[%u]\n", j);
      CVI_TDL_Free(&adas_info->data[j].info);
      adas_info->data[j].info.unique_id = 0;

      adas_info->data[j].t_state = IDLE;
      adas_info->data[j].counter = 0;
      adas_info->data[j].miss_counter = 0;
      adas_info->data[j].dis = 0;
      adas_info->data[j].dis_tmp = 0;
    }
  }
  return CVI_TDL_SUCCESS;
}

static CVI_S32 update_lane_state(adas_info_t *adas_info, uint32_t height, uint32_t width) {
  cvtdl_lane_t *lane_meta = &adas_info->lane_meta;

  float cur_score = 0;

  for (uint32_t i = 0; i < lane_meta->size; i++) {
    cvtdl_lane_point_t *point = &lane_meta->lane[i];

    float k = (point->y[1] - point->y[0]) / (point->x[1] - point->x[0]);
    k = k > 0 ? k : -k;
    float x_i = ((float)height * 0.78 - point->y[0]) / (point->y[1] - point->y[0]) *
                    (point->x[1] - point->x[0]) +
                point->x[0];

    if (x_i > (float)width * 0.3 && x_i < (float)width * 0.7 && k > 1.2) {
      cur_score = 1.0f;
      break;
    }
  }

  adas_info->lane_score = 0.95 * adas_info->lane_score + 0.05 * cur_score;
  float departure_time = adas_info->departure_time - 0.4;
  if (adas_info->lane_score > adas_info->FPS / 50.0) {
    adas_info->lane_counter++;
    if ((float)adas_info->lane_counter / adas_info->FPS > departure_time) {
      adas_info->lane_state = 1;
    } else {
      adas_info->lane_state = 0;
    }
  } else {
    adas_info->lane_counter = 0;
    adas_info->lane_state = 0;
  }
  adas_info->lane_meta.lane_state = adas_info->lane_state;

  // printf("adas_info->lane_state: %d\n", adas_info->lane_state);
  return CVI_TDL_SUCCESS;
}

void front_obj_index(cvtdl_object_t *obj_meta, cvtdl_lane_t *lane_meta, int *center_info,
                     float width, int det_lane) {
  center_info[0] = -1;
  float dis = width;

  float left_x = 0.33 * width;
  float right_x = 0.66 * width;

  if (det_lane && lane_meta->size == 2) {
    float xmin = lane_meta->lane[0].y[0] < lane_meta->lane[0].y[1] ? lane_meta->lane[0].x[0]
                                                                   : lane_meta->lane[0].x[1];
    float xmax = lane_meta->lane[1].y[0] < lane_meta->lane[1].y[1] ? lane_meta->lane[1].x[0]
                                                                   : lane_meta->lane[1].x[1];

    if (xmin > xmax) {
      float tmp = xmin;
      xmin = xmax;
      xmax = tmp;
    }

    if (xmax - xmin > 0.15 * width && xmax - xmin < 0.35 * width) {  // valid lane
      left_x = xmin;
      right_x = xmax;
    }
  }

  center_info[1] = left_x;
  center_info[2] = right_x;

  float center = (left_x + right_x) / 2.0f;

  for (uint32_t i = 0; i < obj_meta->size; i++) {
    if (obj_meta->info[i].classes < 3) {  //机动车

      float c_x = (obj_meta->info[i].bbox.x1 + obj_meta->info[i].bbox.x2) / 2.0f;

      if (c_x > left_x && c_x < right_x) {
        float cur_dis = center > c_x ? center - c_x : c_x - center;

        if (cur_dis < dis) {
          center_info[0] = i;
          dis = cur_dis;
        }
      }
    }
  }
}

uint64_t update_unique_id_with_classes(cvtdl_object_t *obj_meta) {
  for (uint32_t i = 0; i < obj_meta->size; i++) {
    uint64_t uid = obj_meta->info[i].unique_id;
    int classes = obj_meta->info[i].classes;

    char str[16];
    sprintf(str, "%ld", uid);
    int num = strlen(str);

    uint64_t trk_id = classes * pow(10, num) + uid;
    obj_meta->info[i].unique_id = trk_id;
  }
  return CVI_TDL_SUCCESS;
}

static CVI_S32 update_data(adas_info_t *adas_info, VIDEO_FRAME_INFO_S *frame,
                           cvtdl_object_t *obj_meta, cvtdl_tracker_t *tracker_meta) {
  LOGI("[APP::VehicleAdas] Update Data\n");

  update_unique_id_with_classes(obj_meta);
  front_obj_index(obj_meta, &adas_info->lane_meta, adas_info->center_info, frame->stVFrame.u32Width,
                  adas_info->det_type);

  for (uint32_t i = 0; i < obj_meta->size; i++) {
    uint64_t trk_id = obj_meta->info[i].unique_id;
    int match_idx = -1;
    int idle_idx = -1;
    int update_idx = -1;
    //  printf("obj_meta->size: %d\n", adas_info->size);

    for (uint32_t j = 0; j < adas_info->size; j++) {
      if (adas_info->data[j].t_state == ALIVE && adas_info->data[j].info.unique_id == trk_id) {
        match_idx = (int)j;
        break;
      }
    }

    if (match_idx != -1) {
      adas_info->data[match_idx].miss_counter = 0;
      update_dis(&obj_meta->info[i], &adas_info->data[match_idx], frame->stVFrame.u32Height, false);

      update_idx = match_idx;
    } else {
      for (uint32_t j = 0; j < adas_info->size; j++) {
        if (adas_info->data[j].t_state == IDLE) {
          idle_idx = (int)j;
          update_dis(&obj_meta->info[i], &adas_info->data[idle_idx], frame->stVFrame.u32Height,
                     true);
          update_idx = idle_idx;
          break;
        }
      }
    }

    if (match_idx == -1 && idle_idx == -1) {
      LOGD("no valid buffer\n");
      continue;
    }

    memcpy(&adas_info->data[update_idx].info, &obj_meta->info[i], sizeof(cvtdl_object_info_t));
    adas_info->data[update_idx].t_state = ALIVE;

    adas_info->data[update_idx].counter += 1;

    if (match_idx != -1) {
      update_apeed(&obj_meta->info[i], &adas_info->data[match_idx], adas_info->FPS);
      update_object_state(&obj_meta->info[i], &adas_info->data[match_idx], adas_info,
                          obj_meta->width, i, adas_info->is_static);
    }
  }

  for (uint32_t j = 0; j < adas_info->size; j++) {
    bool found = false;
    for (uint32_t k = 0; k < obj_meta->size; k++) {
      if (adas_info->data[j].info.unique_id == obj_meta->info[k].unique_id) {
        found = true;
        break;
      }
    }

    if (!found && adas_info->data[j].info.unique_id != 0) {
      adas_info->data[j].miss_counter += 1;
      adas_info->data[j].counter += 1;

      if (adas_info->data[j].miss_counter >= adas_info->miss_time_limit) {
        LOGD("to delete track:%u\n", (uint32_t)adas_info->data[j].info.unique_id);
        adas_info->data[j].t_state = MISS;
      }
    }
  }

  return CVI_TDL_SUCCESS;
}

CVI_S32 _ADAS_Run(adas_info_t *adas_info, const cvitdl_handle_t tdl_handle,
                  VIDEO_FRAME_INFO_S *frame) {
  if (adas_info == NULL) {
    LOGE("[APP::VehicleAdas] is not initialized.\n");
    return CVI_TDL_FAILURE;
  }

  if (frame->stVFrame.u32Length[0] == 0) {
    LOGE("[APP::VehicleAdas] got empty frame.\n");
    return CVI_TDL_FAILURE;
  }

  CVI_S32 ret;
  ret = clean_data(adas_info);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::VehicleAdas] clean data failed.\n");
    return CVI_TDL_FAILURE;
  }

  CVI_TDL_Free(&adas_info->last_trackers);
  CVI_TDL_Free(&adas_info->last_objects);

  if (frame->stVFrame.u32Length[0] == 0) {
    LOGE("input frame turn into empty\n");
    return CVI_TDL_FAILURE;
  }

  if (CVI_SUCCESS != CVI_TDL_Detection(tdl_handle, frame, CVI_TDL_SUPPORTED_MODEL_YOLOV8_DETECTION,
                                       &adas_info->last_objects)) {
    // CVI_TDL_Release_VideoFrame(tdl_handle, CVI_TDL_SUPPORTED_MODEL_PERSON_VEHICLE_DETECTION,
    // frame,
    //  true);
    printf("PersonVehicle detection failed\n");
    return CVI_TDL_FAILURE;
  }

  obj_filter(&adas_info->last_objects);

  CVI_TDL_DeepSORT_Obj(tdl_handle, &adas_info->last_objects, &adas_info->last_trackers, false);

  if (adas_info->det_type) {
    if (adas_info->lane_model_type == 0) {
      ret = CVI_TDL_Lane_Det(tdl_handle, frame, &adas_info->lane_meta);
    } else if (adas_info->lane_model_type == 1) {
      ret = CVI_TDL_LSTR_Det(tdl_handle, frame, &adas_info->lane_meta);
    }
    if (ret != CVI_SUCCESS) {
      // CVI_TDL_Release_VideoFrame(tdl_handle, CVI_TDL_SUPPORTED_MODEL_LANE_DET, frame, true);
      printf("lane detection failed\n");
      return CVI_TDL_FAILURE;
    }

    update_lane_state(adas_info, frame->stVFrame.u32Height, frame->stVFrame.u32Width);
    update_frame_state(adas_info);
  }

  ret = update_data(adas_info, frame, &adas_info->last_objects, &adas_info->last_trackers);
  if (ret != CVI_TDL_SUCCESS) {
    LOGE("[APP::ADAS] update data failed.\n");
    return CVI_TDL_FAILURE;
  }

  return CVI_TDL_SUCCESS;
}