#include <algorithm>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core_utils.hpp"
#include "cvi_sys.h"

#include <cmath>
#include "cvi_tdl_log.hpp"
#include "fall_det.hpp"

#define SCORE_THRESHOLD 0.4
#define FRAME_GAP 1.0
// #define FPS 30

void print_kps(std::vector<std::pair<float, float>> &kps, int index) {
  for (uint32_t i = 0; i < kps.size(); i++) {
    printf("[%d] %d: %.2f, %.2f\n", index, i, kps[i].first, kps[i].second);
  }
}

FallDet::FallDet(int id) {
  uid = id;
  for (int i = 0; i < 4; i++) {
    valid_list.push(0);
  }

  for (int i = 0; i < 3; i++) {
    speed_caches.push(0);
  }

  for (int i = 0; i < 6; i++) {
    statuses_cache.push(0);
  }
}

int FallDet::elem_count(std::queue<int> &q) {
  int num = 0;
  int q_size = q.size();

  for (int i = 0; i < q_size; i++) {
    int val = q.front();
    // printf("i: %d, val: %d\n", i, val);
    q.pop();

    num += val;
    q.push(val);
  }

  // printf("elem_count: %d\n", num);

  return num;
}

bool FallDet::keypoints_useful(cvtdl_pose17_meta_t *kps_meta) {
  if (history_neck.size() == FRAME_GAP + 3) {
    history_neck.erase(history_neck.begin());
    history_hip.erase(history_hip.begin());
  }
  // printf("kps_meta->score[5]%f [6]%f [7]%f [8]%f\n", kps_meta->score[5], kps_meta->score[6],
  // kps_meta->score[11], kps_meta->score[12]);
  if (kps_meta->score[5] > SCORE_THRESHOLD && kps_meta->score[6] > SCORE_THRESHOLD &&
      kps_meta->score[11] > SCORE_THRESHOLD && kps_meta->score[12] > SCORE_THRESHOLD) {
    float neck_x = (kps_meta->x[5] + kps_meta->x[6]) / 2.0f;
    float neck_y = (kps_meta->y[5] + kps_meta->y[6]) / 2.0f;
    float hip_x = (kps_meta->x[11] + kps_meta->x[12]) / 2.0f;
    float hip_y = (kps_meta->y[11] + kps_meta->y[12]) / 2.0f;

    std::pair<float, float> neck = std::make_pair(neck_x, neck_y);
    std::pair<float, float> hip = std::make_pair(hip_x, hip_y);

    history_neck.push_back(neck);
    history_hip.push_back(hip);

    // print_kps(history_neck, 0);
    // print_kps(history_hip, 1);
    return true;

  } else {
    history_neck.push_back(std::make_pair(0, 0));
    history_hip.push_back(std::make_pair(0, 0));
    // print_kps(history_neck, 0);
    // print_kps(history_hip, 1);
    return false;
  }
}

void FallDet::get_kps(std::vector<std::pair<float, float>> &val_list, int index, float *x,
                      float *y) {
  float tmp_x = 0;
  float tmp_y = 0;

  for (int i = index; i < index + 3; i++) {
    tmp_x += val_list[i].first;
    tmp_y += val_list[i].second;
  }

  *x = tmp_x / 3.0f;
  *y = tmp_y / 3.0f;
}

float FallDet::human_orientation() {
  float neck_x, neck_y, hip_x, hip_y;

  get_kps(history_neck, FRAME_GAP, &neck_x, &neck_y);
  get_kps(history_hip, FRAME_GAP, &hip_x, &hip_y);

  float human_angle = atan2(hip_y - neck_y, hip_x - neck_x) * 180.0 / M_PI - 90.0;

  return human_angle;
}

float FallDet::body_box_calculation(cvtdl_bbox_t *bbox) {
  return (bbox->x2 - bbox->x1) / (bbox->y2 - bbox->y1);
}

void FallDet::update_queue(std::queue<int> &q, int val) {
  q.pop();
  q.push(val);
}

float FallDet::speed_detection(cvtdl_bbox_t *bbox, cvtdl_pose17_meta_t *kps_meta, float fps) {
  // printf("speed_detection fps: %.2f\n", fps);
  float neck_x_before, neck_y_before, neck_x_cur, neck_y_cur;

  get_kps(history_neck, 0, &neck_x_before, &neck_y_before);
  get_kps(history_neck, FRAME_GAP, &neck_x_cur, &neck_y_cur);

  //   printf("neck_x_before:%.2f, neck_y_before:%.2f, neck_x_cur:%.2f, neck_y_cur:%.2f \n",
  //  neck_x_before, neck_y_before, neck_x_cur, neck_y_cur);

  float delta_position =
      sqrt(pow(neck_x_before - neck_x_cur, 2) + pow(neck_y_before - neck_y_cur, 2));

  if (neck_y_cur < neck_y_before) {
    delta_position = -delta_position;
  }

  //   printf("delta_position: %.2f\n", delta_position);

  std::vector<float> delta_val;

  float box_w = bbox->x2 - bbox->x1;
  float box_h = bbox->y2 - bbox->y1;

  if (kps_meta->score[13] < SCORE_THRESHOLD && kps_meta->score[14] < SCORE_THRESHOLD &&
      kps_meta->score[15] < SCORE_THRESHOLD && kps_meta->score[16] < SCORE_THRESHOLD) {
    box_h *= 1.8;
  } else if (kps_meta->score[15] < SCORE_THRESHOLD && kps_meta->score[16] < SCORE_THRESHOLD) {
    box_h *= 1.3;
  }

  float delta_body = sqrt(pow(box_w, 2) + pow(box_h, 2));

  delta_val.push_back(delta_body);
#ifdef DEBUG_FALL
  printf("delta_position: %.2f, delta_body: %.2f\n", delta_position, delta_body);
#endif
  if (kps_meta->score[12] > SCORE_THRESHOLD && kps_meta->score[14] > SCORE_THRESHOLD &&
      kps_meta->score[16] > SCORE_THRESHOLD) {
    float left_leg_up =
        sqrt(pow(kps_meta->x[12] - kps_meta->x[14], 2) + pow(kps_meta->y[12] - kps_meta->y[14], 2));
    float left_leg_bottom =
        sqrt(pow(kps_meta->x[16] - kps_meta->x[14], 2) + pow(kps_meta->y[16] - kps_meta->y[14], 2));
    float left_leg = (left_leg_up + left_leg_bottom) * 2.4;
#ifdef DEBUG_FALL
    printf("left_leg: %.2f\n", left_leg);
#endif
    delta_val.push_back(left_leg);
  }

  if (kps_meta->score[11] > SCORE_THRESHOLD && kps_meta->score[13] > SCORE_THRESHOLD &&
      kps_meta->score[15] > SCORE_THRESHOLD) {
    float right_leg_up =
        sqrt(pow(kps_meta->x[11] - kps_meta->x[13], 2) + pow(kps_meta->y[11] - kps_meta->y[13], 2));
    float right_leg_bottom =
        sqrt(pow(kps_meta->x[13] - kps_meta->x[15], 2) + pow(kps_meta->y[13] - kps_meta->y[15], 2));
    float right_leg = (right_leg_up + right_leg_bottom) * 2.4;
#ifdef DEBUG_FALL
    printf("right_leg: %.2f\n", right_leg);
#endif
    delta_val.push_back(right_leg);
  }

  if (kps_meta->score[6] > SCORE_THRESHOLD && kps_meta->score[8] > SCORE_THRESHOLD &&
      kps_meta->score[10] > SCORE_THRESHOLD) {
    float left_arm_up =
        sqrt(pow(kps_meta->x[6] - kps_meta->x[8], 2) + pow(kps_meta->y[6] - kps_meta->y[8], 2));
    float left_arm_bottom =
        sqrt(pow(kps_meta->x[8] - kps_meta->x[10], 2) + pow(kps_meta->y[8] - kps_meta->y[10], 2));
    float left_arm = (left_arm_up + left_arm_bottom) * 3.4;
#ifdef DEBUG_FALL
    printf("left_arm: %.2f\n", left_arm);
#endif
    delta_val.push_back(left_arm);
  }

  if (kps_meta->score[5] > SCORE_THRESHOLD && kps_meta->score[7] > SCORE_THRESHOLD &&
      kps_meta->score[9] > SCORE_THRESHOLD) {
    float right_arm_up =
        sqrt(pow(kps_meta->x[5] - kps_meta->x[7], 2) + pow(kps_meta->y[5] - kps_meta->y[7], 2));
    float right_arm_bottom =
        sqrt(pow(kps_meta->x[7] - kps_meta->x[9], 2) + pow(kps_meta->y[7] - kps_meta->y[9], 2));
    float right_arm = (right_arm_up + right_arm_bottom) * 3.4;
#ifdef DEBUG_FALL
    printf("right_arm: %.2f\n", right_arm);
#endif
    delta_val.push_back(right_arm);
  }

  double delta_sum = 0.0;

  for (uint32_t i = 0; i < delta_val.size(); i++) {
    delta_sum += delta_val[i];
  }
  double delta_mean = delta_sum / (float)delta_val.size();

#ifdef DEBUG_FALL
  printf("delta_mean: %.2f\n", delta_mean);
#endif

  // float delta_max = *std::max_element(delta_val.begin(),delta_val.end());

  //   printf("delta_body: %.2f\n", delta_body);

  float speed = 100.0 * delta_position / (delta_mean * (FRAME_GAP / fps));

  if (speed > SPEED_THRESHOLD) {
    update_queue(speed_caches, 1);
  } else {
    update_queue(speed_caches, 0);
  }

  if (elem_count(speed_caches) >= 2) {
    is_moving = true;
  } else {
    is_moving = false;
  }

  return speed;
}

int FallDet::action_analysis(float human_angle, float aspect_ratio, float moving_speed) {
  /*
  state_list[0]: Stand_still
  state_list[1]: Stand_walking
  state_list[2]: Fall
  state_list[3]: Lie
  state_list[4]: Sit

  */
  float status_score[5] = {0.0};

  if (human_angle > -HUMAN_ANGLE_THRESHOLD && human_angle < HUMAN_ANGLE_THRESHOLD) {
    status_score[0] += 0.8;
    status_score[1] += 0.8;
    status_score[4] += 0.8;
  } else {
    status_score[2] += 0.8;
    status_score[3] += 0.8;
  }

  if (aspect_ratio < ASPECT_RATIO_THRESHOLD) {
    status_score[0] += 0.8;
    status_score[1] += 0.8;
  } else if (aspect_ratio > 1.0f / ASPECT_RATIO_THRESHOLD) {
    status_score[3] += 0.8;
  } else {
    status_score[2] += 0.8;
    status_score[4] += 0.8;
  }

  if (moving_speed < SPEED_THRESHOLD) {
    status_score[0] += 0.8;
    status_score[1] += 0.8;
    status_score[3] += 0.8;
    status_score[4] += 0.8;
  } else {
    status_score[2] += 0.8;
  }

  if (is_moving) {
    status_score[1] += 0.8;
    status_score[2] += 0.8;
  } else {
    status_score[0] += 0.8;
    status_score[3] += 0.8;
    status_score[4] += 0.8;
  }

  int max_position = std::max_element(status_score, status_score + 5) - status_score;

  return max_position;
}

bool FallDet::alert_decision(int status) {
  update_queue(statuses_cache, status);

  if (elem_count(statuses_cache) >= 3) {
    return true;
  } else {
    return false;
  }
}

void FallDet::detect(cvtdl_object_info_t *meta, float fps) {
  meta->pedestrian_properity->fall = false;
  if (keypoints_useful(&meta->pedestrian_properity->pose_17)) {
#ifdef DEBUG_FALL
    printf("---------keypoints_useful----------\n");
#endif
    update_queue(valid_list, 1);

    if (elem_count(valid_list) == 4) {
      //   printf("into cal\n");

      float human_angle = human_orientation();
      float aspect_ratio = body_box_calculation(&meta->bbox);
      float speed = speed_detection(&meta->bbox, &meta->pedestrian_properity->pose_17, fps);
      int status = action_analysis(human_angle, aspect_ratio, speed);

      // meta->human_angle = human_angle;
      // meta->aspect_ratio = aspect_ratio;
      // meta->speed = speed;
      // meta->is_moving = (int)is_moving;
      // meta->status = status;

#ifdef DEBUG_FALL
      printf("human_angle:%.2f, aspect_ratio:%.2f, speed:%.2f, status:%d\n", human_angle,
             aspect_ratio, speed, status);
#endif
      int is_fall = status == 2 ? 1 : 0;

      if (alert_decision(is_fall)) {
        meta->pedestrian_properity->fall = true;
      }
    }
  } else {
    update_queue(valid_list, 0);
  }
}
