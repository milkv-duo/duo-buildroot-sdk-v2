#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem.h"
#include "core_utils.hpp"
#include "cvi_sys.h"
#include "face_utils.hpp"

#include <cmath>
#include "cvi_tdl_log.hpp"
#include "fall_detection.hpp"

#define HISTORYPART_UPDATE 3  // 10//5
#define CURRENT_UPDATE 27     // 20//25

// todo : multiple persons; only consider 1 person here and take the first
FallMD::FallMD() {
  for (uint32_t initial_index = 0; initial_index < HISTORY_UPDATE; ++initial_index) {
    this->history_q_extra_pred_x.push(0.0);
    this->history_q_extra_pred_y.push(0.0);
    this->history_bbox_x1.push(0.0);
    this->history_bbox_x2.push(0.0);
    this->history_bbox_y1.push(0.0);
    this->history_bbox_y2.push(0.0);
  }
  this->isFall = false;
};

int FallMD::detect(cvtdl_object_t *obj) {
  float history_extra_pred_x = 0.0;
  float history_extra_pred_y = 0.0;
  float history_bbox_x1 = 0.0;
  float history_bbox_x2 = 0.0;
  float history_bbox_y1 = 0.0;
  float history_bbox_y2 = 0.0;

  float current_extra_pred_x = 0.0;
  float current_extra_pred_y = 0.0;
  float current_bbox_x1 = 0.0;
  float current_bbox_x2 = 0.0;
  float current_bbox_y1 = 0.0;
  float current_bbox_y2 = 0.0;

  // take the history informations we need
  for (uint32_t initial_index = 0; initial_index < HISTORY_UPDATE; ++initial_index) {
    float tmp_extra_pred_x = this->history_q_extra_pred_x.front();
    float tmp_extra_pred_y = this->history_q_extra_pred_y.front();
    float tmp_bbox_x1 = this->history_bbox_x1.front();
    float tmp_bbox_x2 = this->history_bbox_x2.front();
    float tmp_bbox_y1 = this->history_bbox_y1.front();
    float tmp_bbox_y2 = this->history_bbox_y2.front();

    this->history_q_extra_pred_x.pop();
    this->history_q_extra_pred_y.pop();
    this->history_bbox_x1.pop();
    this->history_bbox_x2.pop();
    this->history_bbox_y1.pop();
    this->history_bbox_y2.pop();

    if (initial_index < HISTORYPART_UPDATE) {
      history_extra_pred_x += tmp_extra_pred_x;
      history_extra_pred_y += tmp_extra_pred_y;
      history_bbox_x1 += tmp_bbox_x1;
      history_bbox_x2 += tmp_bbox_x2;
      history_bbox_y1 += tmp_bbox_y1;
      history_bbox_y2 += tmp_bbox_y2;
    }
    if (initial_index > CURRENT_UPDATE) {
      current_extra_pred_x += tmp_extra_pred_x;
      current_extra_pred_y += tmp_extra_pred_y;
      current_bbox_x1 += tmp_bbox_x1;
      current_bbox_x2 += tmp_bbox_x2;
      current_bbox_y1 += tmp_bbox_y1;
      current_bbox_y2 += tmp_bbox_y2;
    }
    this->history_q_extra_pred_x.push(tmp_extra_pred_x);
    this->history_q_extra_pred_y.push(tmp_extra_pred_y);
    this->history_bbox_x1.push(tmp_bbox_x1);
    this->history_bbox_x2.push(tmp_bbox_x2);
    this->history_bbox_y1.push(tmp_bbox_y1);
    this->history_bbox_y2.push(tmp_bbox_y2);
  }

  history_extra_pred_x /= HISTORYPART_UPDATE;
  history_extra_pred_y /= HISTORYPART_UPDATE;
  history_bbox_x1 /= HISTORYPART_UPDATE;
  history_bbox_x2 /= HISTORYPART_UPDATE;
  history_bbox_y1 /= HISTORYPART_UPDATE;
  history_bbox_y2 /= HISTORYPART_UPDATE;

  /*
  std::cout << "\nhistory_extra_pred_x : " << history_extra_pred_x << std::endl;
  std::cout << "\nhistory_extra_pred_y : " << history_extra_pred_y << std::endl;
  std::cout << "\nhistory_bbox_x1 : " << history_bbox_x1 << std::endl;
  std::cout << "\nhistory_bbox_x2 : " << history_bbox_x2 << std::endl;
  std::cout << "\nhistory_bbox_y1 : " << history_bbox_y1 << std::endl;
  std::cout << "\nhistory_bbox_y2 : " << history_bbox_y2 << std::endl;
  */

  if (obj->size > 0) {
    // for (uint32_t i = 0; i < obj->size; ++i) {
    // assume only the same one person
    for (uint32_t i = 0; i < 1; ++i) {
      // initialize fall_score from history fall status
      if (obj->info[i].pedestrian_properity == NULL) continue;

      obj->info[i].pedestrian_properity->fall = this->isFall;
      std::vector<cv::Point2f> kp_preds(17);
      cvtdl_pose17_meta_t pose = obj->info[i].pedestrian_properity->pose_17;
      for (int kpi = 0; kpi < 17; ++kpi) {
        kp_preds[kpi].x = pose.x[kpi];
        kp_preds[kpi].y = pose.y[kpi];
      }

      cv::Point2f extra_pred;
      if (kp_preds[5].x == 0 && kp_preds[6].x == 0) {
        continue;
      } else if (kp_preds[6].x == 0) {
        extra_pred.x = kp_preds[5].x;
      } else if (kp_preds[5].x == 0) {
        extra_pred.x = kp_preds[6].x;
      } else {
        extra_pred.x = (kp_preds[5].x + kp_preds[6].x) / 2;
      }

      if (kp_preds[5].y == 0 && kp_preds[6].y == 0) {
        continue;
      } else if (kp_preds[6].y == 0) {
        extra_pred.y = kp_preds[5].y;
      } else if (kp_preds[5].y == 0) {
        extra_pred.y = kp_preds[6].y;
      } else {
        extra_pred.y = (kp_preds[5].y + kp_preds[6].y) / 2;
      }

      kp_preds.push_back(extra_pred);

      /*
      std::cout << "kp_preds[5].x : " << kp_preds[5].x << std::endl;
      std::cout << "kp_preds[5].y : " << kp_preds[5].y << std::endl;
      std::cout << "kp_preds[6].x : " << kp_preds[6].x << std::endl;
      std::cout << "kp_preds[6].y : " << kp_preds[6].y << std::endl;
      */

      cvtdl_bbox_t bbox = obj->info[i].bbox;

      /*
      std::cout << "\n extra_pred.x : " << extra_pred.x << std::endl;
      std::cout << "\n extra_pred.y : " << extra_pred.y << std::endl;
      std::cout << "\n bbox.x1 : " << bbox.x1 << std::endl;
      std::cout << "\n bbox.x2 : " << bbox.x2 << std::endl;
      std::cout << "\n bbox.y1 : " << bbox.y1 << std::endl;
      std::cout << "\n bbox.y2 : " << bbox.y2 << std::endl;
      */

      current_extra_pred_x += extra_pred.x;
      current_extra_pred_y += extra_pred.y;
      current_bbox_x1 += bbox.x1;
      current_bbox_x2 += bbox.x2;
      current_bbox_y1 += bbox.y1;
      current_bbox_y2 += bbox.y2;

      current_extra_pred_x /= (HISTORY_UPDATE - CURRENT_UPDATE);
      current_extra_pred_y /= (HISTORY_UPDATE - CURRENT_UPDATE);
      current_bbox_x1 /= (HISTORY_UPDATE - CURRENT_UPDATE);
      current_bbox_x2 /= (HISTORY_UPDATE - CURRENT_UPDATE);
      current_bbox_y1 /= (HISTORY_UPDATE - CURRENT_UPDATE);
      current_bbox_y2 /= (HISTORY_UPDATE - CURRENT_UPDATE);

      /*
      std::cout << "\n current_extra_pred_x : " << current_extra_pred_x << std::endl;
      std::cout << "\n current_extra_pred_y : " << current_extra_pred_y << std::endl;
      std::cout << "\n current_bbox_x1 : " << current_bbox_x1 << std::endl;
      std::cout << "\n current_bbox_x2 : " << current_bbox_x2 << std::endl;
      std::cout << "\n current_bbox_y1 : " << current_bbox_y1 << std::endl;
      std::cout << "\n current_bbox_y2 : " << current_bbox_y2 << std::endl;
      */

      // fall detection
      // std::cout << "result1 : " << ((current_bbox_x2 - current_bbox_x1) >= 1.2*(current_bbox_y2 -
      // current_bbox_y1)) << std::endl;  std::cout << "result2 : " <<
      // sqrt(pow((current_extra_pred_x
      // - history_extra_pred_x), 2)+pow((current_extra_pred_y-history_extra_pred_y), 2)) <<
      // std::endl;  std::cout << "result3 : " << sqrt(pow(history_bbox_x2-history_bbox_x1,
      // 2)+pow(history_bbox_y2-history_bbox_y1, 2)) << std::endl;
      if (this->history_q_extra_pred_x.front() != 0.0 &&
          this->history_q_extra_pred_y.front() != 0.0) {
        if ((current_bbox_x2 - current_bbox_x1) >= 1.2 * (current_bbox_y2 - current_bbox_y1)) {
          if (sqrt(pow((current_extra_pred_x - history_extra_pred_x), 2) +
                   pow((current_extra_pred_y - history_extra_pred_y), 2)) >=
              0.5 * sqrt(pow(history_bbox_x2 - history_bbox_x1, 2) +
                         pow(history_bbox_y2 - history_bbox_y1, 2))) {
            this->isFall = true;
            obj->info[i].pedestrian_properity->fall = this->isFall;
          } else {
            // keep original fall status
          }
        } else {
          this->isFall = false;
          obj->info[i].pedestrian_properity->fall = this->isFall;
        }
      } else {
        LOGI("History initialization, don't make fall prediction\n");
        // keep original fall status
      }
      // update history
      this->history_q_extra_pred_x.push(extra_pred.x);
      this->history_q_extra_pred_y.push(extra_pred.y);
      this->history_bbox_x1.push(bbox.x1);
      this->history_bbox_x2.push(bbox.x2);
      this->history_bbox_y1.push(bbox.y1);
      this->history_bbox_y2.push(bbox.y2);
    }
  } else {
    // update history
    this->history_q_extra_pred_x.push(this->history_q_extra_pred_x.back());
    this->history_q_extra_pred_y.push(this->history_q_extra_pred_y.back());
    this->history_bbox_x1.push(this->history_bbox_x1.back());
    this->history_bbox_x2.push(this->history_bbox_x2.back());
    this->history_bbox_y1.push(this->history_bbox_y1.back());
    this->history_bbox_y2.push(this->history_bbox_y2.back());
  }
  return CVI_TDL_SUCCESS;
}