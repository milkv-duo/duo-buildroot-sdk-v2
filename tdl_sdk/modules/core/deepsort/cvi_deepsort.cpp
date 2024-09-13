#include "cvi_deepsort.hpp"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "cvi_deepsort_utils.hpp"
#include "cvi_tdl_log.hpp"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <map>
#include <set>
#include <sstream>

#define DEFAULT_X_CONSTRAINT_A_MIN 0.25
#define DEFAULT_X_CONSTRAINT_A_MAX 4.0
#define DEFAULT_X_CONSTRAINT_H_MIN 32
#define DEFAULT_X_CONSTRAINT_H_MAX 512

/* helper functions */
// static void FACE_QUALITY_ASSESSMENT(cvtdl_face_t *face);
static void show_deepsort_config(cvtdl_deepsort_config_t &ds_conf);
bool isCrossPointInRect(const randomRect *rect, float p_x, float p_y) {
  // a×b=(x1y2-x2y1)
  float a = (rect->lt_x - rect->lb_x) * (p_y - rect->lb_y) -
            (rect->lt_y - rect->lb_y) * (p_x - rect->lb_x);  // LTLB X PLB
  float b = (rect->rt_x - rect->lt_x) * (p_y - rect->lt_y) -
            (rect->rt_y - rect->lt_y) * (p_x - rect->lt_x);  // RTLT X PLT
  float c = (rect->rb_x - rect->rt_x) * (p_y - rect->rt_y) -
            (rect->rb_y - rect->rt_y) * (p_x - rect->rt_x);  // RBRT X PRT -- LTLB X PLB
  float d = (rect->lb_x - rect->rb_x) * (p_y - rect->rb_y) -
            (rect->lb_y - rect->rb_y) * (p_x - rect->rb_x);  // LBRB X PRB -- RTLT X PLT

  return (a > 0 && b > 0 && c > 0 && d > 0) || (a < 0 && b < 0 && c < 0 && d < 0);
}

float crossProductBorder(float A_x, float A_y, float B_x, float B_y, float C_x, float C_y) {
  return (B_x - A_x) * (C_y - A_y) - (B_y - A_y) * (C_x - A_x);
}
bool isLineIntersectBoarder(const randomRect *rect, float old_x, float old_y, float cur_x,
                            float cur_y) {
  double cp1 = crossProductBorder(rect->a_x, rect->a_y, rect->b_x, rect->b_y, old_x, old_y);
  double cp2 = crossProductBorder(rect->a_x, rect->a_y, rect->b_x, rect->b_y, cur_x, cur_y);
  double cp3 = crossProductBorder(old_x, old_y, cur_x, cur_y, rect->a_x, rect->a_y);
  double cp4 = crossProductBorder(old_x, old_y, cur_x, cur_y, rect->b_x, rect->b_y);
  // // positive and negative symbols
  if ((cp1 * cp2 <= 0) && (cp3 * cp4 <= 0)) return true;
  return false;
}

DeepSORT::DeepSORT(bool use_specific_counter) {
  sp_counter = use_specific_counter;
  id_counter = 0;
  kf_ = KalmanFilter();

  default_conf = get_DefaultConfig();
}

DeepSORT::~DeepSORT() {}
CVI_S32 DeepSORT::track_cross(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid,
                              const cvtdl_counting_line_t *cross_line_t, const randomRect *rect) {
  /** statistic what classes ID in bbox and tracker,
   *  and counting bbox number for each class */

  CVI_S32 ret = CVI_TDL_SUCCESS;
  std::map<int, int> class_bbox_counter;
  std::set<int> class_ids_bbox;
  std::set<int> class_ids_trackers;
  for (uint32_t i = 0; i < obj->size; i++) {
    auto iter = class_bbox_counter.find(obj->info[i].classes);
    if (iter != class_bbox_counter.end()) {
      iter->second += 1;
    } else {
      class_bbox_counter.insert(std::pair<int, int>(obj->info[i].classes, 1));
      class_ids_bbox.insert(obj->info[i].classes);
    }
  }

  for (size_t j = 0; j < k_trackers.size(); j++) {
    class_ids_trackers.insert(k_trackers[j].class_id);
  }

  CVI_TDL_MemAlloc(obj->size, tracker);

  /** run tracking function for each class ID in bbox
   */
  for (auto const &e : class_bbox_counter) {
    /** pick up all bboxes and features data for this class ID
     */
    std::vector<BBOX> bboxes;
    std::vector<FEATURE> features;
    std::vector<int> idx_table;
    for (uint32_t i = 0; i < obj->size; i++) {
      if (obj->info[i].classes == e.first) {
        idx_table.push_back(static_cast<int>(i));
        BBOX bbox_;
        uint32_t feature_size = obj->info[i].feature.size;
        FEATURE feature_(feature_size);
        bbox_(0, 0) = obj->info[i].bbox.x1;
        bbox_(0, 1) = obj->info[i].bbox.y1;
        bbox_(0, 2) = obj->info[i].bbox.x2 - obj->info[i].bbox.x1;
        bbox_(0, 3) = obj->info[i].bbox.y2 - obj->info[i].bbox.y1;
        if (obj->info[i].feature.type != TYPE_INT8) {
          LOGE("Feature Type not support now.\n");
          return CVI_TDL_ERR_INVALID_ARGS;
        }
        int type_size = getFeatureTypeSize(obj->info[i].feature.type);
        for (uint32_t d = 0; d < feature_size; d++) {
          feature_(d) = static_cast<float>(obj->info[i].feature.ptr[d * type_size]);
        }
        bboxes.push_back(bbox_);
        features.push_back(feature_);
      }
    }
    assert(idx_table.size() == static_cast<size_t>(e.second));

    /** run tracking function
     *    - ReID flag is only avaliable for PERSON now.
     */
    Tracking_Result2 result_(bboxes.size());
    ret = track_impl_cross(result_, bboxes, features, 0.3, e.first,
                           use_reid && (e.first == CVI_TDL_DET_TYPE_PERSON), NULL, cross_line_t,
                           rect);
    if (CVI_TDL_SUCCESS != ret) {
      return ret;
    }

    for (size_t i = 0; i < result_.size(); i++) {
      int idx = idx_table[i];
      bool &matched = std::get<0>(result_[i]);
      uint64_t &t_id = std::get<1>(result_[i]);
      k_tracker_state_e &t_state = std::get<2>(result_[i]);
      BBOX &t_bbox = std::get<3>(result_[i]);
      bool &is_C = std::get<4>(result_[i]);

      if (!matched) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
      } else if (t_state == k_tracker_state_e::PROBATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
      } else if (t_state == k_tracker_state_e::ACCREDITATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
      } else {
        LOGE("Tracker State Unknow.\n");
        return CVI_TDL_ERR_INVALID_ARGS;
      }
      tracker->info[idx].bbox.x1 = t_bbox(0);
      tracker->info[idx].bbox.y1 = t_bbox(1);
      tracker->info[idx].bbox.x2 = t_bbox(0) + t_bbox(2);
      tracker->info[idx].bbox.y2 = t_bbox(1) + t_bbox(3);
      obj->info[idx].unique_id = t_id;
      obj->info[idx].is_cross = is_C;
      tracker->info[idx].id = t_id;
    }
  }

  /** update tracker state even though there is no relative bbox (by class ID) at this time.
   */
  for (std::set<int>::iterator it = class_ids_trackers.begin(); it != class_ids_trackers.end();
       ++it) {
    if (class_ids_bbox.find(*it) == class_ids_bbox.end()) {
      std::vector<BBOX> bboxes;
      std::vector<FEATURE> features;
      Tracking_Result result_;
      if (CVI_SUCCESS != track_impl(result_, bboxes, features, 0.3, *it, use_reid)) {
        return CVI_TDL_FAILURE;
      }
    }
  }

  return CVI_TDL_SUCCESS;
}
CVI_S32 DeepSORT::track_impl(Tracking_Result &high_result, Tracking_Result &low_result,
                             const std::vector<BBOX> &HighBBoxes,
                             const std::vector<BBOX> &LowBBoxes,
                             const std::vector<FEATURE> &HighFeatures,
                             const std::vector<FEATURE> &LowFeatures, float crowd_iou_thresh,
                             int class_id, bool use_reid, float *Quality) {
  cvtdl_deepsort_config_t *conf;
  auto it_conf = specific_conf.find(class_id);
  if (it_conf != specific_conf.end()) {
    conf = &it_conf->second;
  } else {
    conf = &default_conf;
  }
  if (conf->ktracker_conf.enable_QA_feature_update && Quality == NULL) {
    LOGE("Enable QA feature upate, but Quality is not initialized.");
    return CVI_TDL_FAILURE;
  }
  std::vector<int> tracker_ids;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    tracker_ids.push_back(k_trackers[i].id);
  }

  LOGD("Kalman Trackers predict\n");
  for (KalmanTracker &tracker_ : k_trackers) {
    // kf_.predict(tracker_.kalman_state, tracker_.x, tracker_.P, conf->kfilter_conf);
    tracker_.predict(kf_, conf);
  }
  check_bound_state(conf);
  /*****************************     high score bbox match   start
   * *************************************/
  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> high_unmatched_bbox_idxes;
  for (size_t i = 0; i < HighBBoxes.size(); i++) {
    high_unmatched_bbox_idxes.push_back(i);
  }
  std::vector<int> unmatched_tracker_idxes;
  if (class_id != -1) {
    for (std::vector<int>::iterator iter = accreditation_tracker_idxes.begin();
         iter != accreditation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
    /* Append probation trackers */
    for (std::vector<int>::iterator iter = probation_tracker_idxes.begin();
         iter != probation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
  } else {
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                   accreditation_tracker_idxes.begin(),
                                   accreditation_tracker_idxes.end());
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(), probation_tracker_idxes.begin(),
                                   probation_tracker_idxes.end());
  }
  LOGD("Cascade Match\n");
  /* Match accreditation trackers */
  /* - Cascade Match */
  /* - Feature Consine Distance */
  /* - Kalman Mahalanobis Distance */
  for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
    if (high_unmatched_bbox_idxes.empty()) {
      break;
    }
    cost_matrix_algo_e cost_method =
        (use_reid) ? Feature_CosineDistance : Kalman_MahalanobisDistance;
    std::vector<int> t_tracker_idxes;
    for (size_t tmp_i = 0; tmp_i < unmatched_tracker_idxes.size(); tmp_i++) {
      if (k_trackers[unmatched_tracker_idxes[tmp_i]].unmatched_times == t) {
        if (cost_method == Feature_CosineDistance &&
            !k_trackers[unmatched_tracker_idxes[tmp_i]].init_feature) {
          continue;
        }
        t_tracker_idxes.push_back(unmatched_tracker_idxes[tmp_i]);
      }
    }
    if (t_tracker_idxes.empty()) {
      continue;
    }
    MatchResult match_result = match(
        HighBBoxes, HighFeatures, t_tracker_idxes, high_unmatched_bbox_idxes, conf->kfilter_conf,
        cost_method, (use_reid) ? conf->max_distance_consine : conf->kfilter_conf.chi2_threshold);
    if (match_result.matched_pairs.empty()) {
      continue;
    }
    /* Remove matched idx from bbox_idxes and unmatched_tracker_idxes */
    matched_pairs.insert(matched_pairs.end(), match_result.matched_pairs.begin(),
                         match_result.matched_pairs.end());

    for (size_t m_i = 0; m_i < match_result.matched_pairs.size(); m_i++) {
      int m_tracker_idx = match_result.matched_pairs[m_i].first;
      int m_bbox_idx = match_result.matched_pairs[m_i].second;
      unmatched_tracker_idxes.erase(std::remove(unmatched_tracker_idxes.begin(),
                                                unmatched_tracker_idxes.end(), m_tracker_idx),
                                    unmatched_tracker_idxes.end());
      high_unmatched_bbox_idxes.erase(std::remove(high_unmatched_bbox_idxes.begin(),
                                                  high_unmatched_bbox_idxes.end(), m_bbox_idx),
                                      high_unmatched_bbox_idxes.end());
    }
  }

  /* Remove trackers' idx, which unmatched_times > T, from
   * unmatched_tracker_idxes */
  for (auto it = unmatched_tracker_idxes.begin(); it != unmatched_tracker_idxes.end();) {
    if (k_trackers[*it].unmatched_times > conf->max_unmatched_times_for_bbox_matching ||
        k_trackers[*it].bounding) {
      it = unmatched_tracker_idxes.erase(it);
    } else {
      it++;
    }
  }
#if 0
  /* Append probation trackers */
  unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
    probation_tracker_idxes.begin(), probation_tracker_idxes.end());
#endif

  /* Match remain trackers */
  /* - BBox IoU Distance */
  MatchResult match_result_bbox =
      match(HighBBoxes, HighFeatures, unmatched_tracker_idxes, high_unmatched_bbox_idxes,
            conf->kfilter_conf, BBox_IoUDistance, conf->max_distance_iou);

  /* Match remain trackers */
  matched_pairs.insert(matched_pairs.end(), match_result_bbox.matched_pairs.begin(),
                       match_result_bbox.matched_pairs.end());
  high_unmatched_bbox_idxes = match_result_bbox.unmatched_bbox_idxes;
  unmatched_tracker_idxes = match_result_bbox.unmatched_tracker_idxes;

  /* Update the kalman trackers (Matched) */
  LOGD("Update the high score kalman trackers (Matched)");
  for (size_t i = 0; i < matched_pairs.size(); i++) {
    int tracker_idx = matched_pairs[i].first;
    int bbox_idx = matched_pairs[i].second;
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    const BBOX &bbox_ = HighBBoxes[bbox_idx];
    // tracker_.update_state(true, conf->ktracker_conf.max_unmatched_num,
    //                       conf->ktracker_conf.accreditation_threshold);
    // kf_.update(tracker_.kalman_state, tracker_.x, tracker_.P, bbox_tlwh2xyah(bbox_),
    //            conf->kfilter_conf);
    stRect rct(bbox_(0), bbox_(1), bbox_(2), bbox_(3));
    tracker_.update(kf_, &rct, conf);

    bool quality_ok = true;
    if (Quality != nullptr && Quality[bbox_idx] == 0) quality_ok = false;
    if ((!conf->ktracker_conf.enable_QA_feature_update && quality_ok) ||
        (conf->ktracker_conf.enable_QA_feature_update &&
         Quality[bbox_idx] > conf->ktracker_conf.feature_update_quality_threshold)) {
      const FEATURE &feature_ = HighFeatures[bbox_idx];
      tracker_.update_feature(feature_, conf->ktracker_conf.feature_budget_size,
                              conf->ktracker_conf.feature_update_interval);
    }
    high_result[bbox_idx] =
        std::make_tuple(true, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
  }
  /*****************************     high score bbox match   end
   * *************************************/

  /*****************************************************************************************************/

  /*****************************     low score bbox match   start
   * *************************************/
  // matched_pairs.clear();
  // release memory
  // matched_pairs.clear();
  // matched_pairs.shrink_to_fit();

  if (unmatched_tracker_idxes.size()) {
    std::vector<std::pair<int, int>> second_matched_pairs;
    std::vector<int> low_unmatched_bbox_idxes;
    for (size_t i = 0; i < LowBBoxes.size(); i++) {
      low_unmatched_bbox_idxes.push_back(i);
    }
    LOGD("Cascade Match\n");
    /* Match accreditation trackers */
    /* - Cascade Match */
    /* - Feature Consine Distance */
    /* - Kalman Mahalanobis Distance */

    for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
      if (low_unmatched_bbox_idxes.empty()) {
        break;
      }
      cost_matrix_algo_e cost_method =
          (use_reid) ? Feature_CosineDistance : Kalman_MahalanobisDistance;
      std::vector<int> t_tracker_idxes;
      for (size_t tmp_i = 0; tmp_i < unmatched_tracker_idxes.size(); tmp_i++) {
        if (k_trackers[unmatched_tracker_idxes[tmp_i]].unmatched_times == t) {
          if (cost_method == Feature_CosineDistance &&
              !k_trackers[unmatched_tracker_idxes[tmp_i]].init_feature) {
            continue;
          }
          t_tracker_idxes.push_back(unmatched_tracker_idxes[tmp_i]);
        }
      }
      if (t_tracker_idxes.empty()) {
        continue;
      }
      MatchResult match_result = match(
          LowBBoxes, LowFeatures, t_tracker_idxes, low_unmatched_bbox_idxes, conf->kfilter_conf,
          cost_method, (use_reid) ? conf->max_distance_consine : conf->kfilter_conf.chi2_threshold);
      if (match_result.matched_pairs.empty()) {
        continue;
      }
      /* Remove matched idx from bbox_idxes and unmatched_tracker_idxes */
      second_matched_pairs.insert(second_matched_pairs.end(), match_result.matched_pairs.begin(),
                                  match_result.matched_pairs.end());

      for (size_t m_i = 0; m_i < match_result.matched_pairs.size(); m_i++) {
        int m_tracker_idx = match_result.matched_pairs[m_i].first;
        int m_bbox_idx = match_result.matched_pairs[m_i].second;
        unmatched_tracker_idxes.erase(std::remove(unmatched_tracker_idxes.begin(),
                                                  unmatched_tracker_idxes.end(), m_tracker_idx),
                                      unmatched_tracker_idxes.end());
        low_unmatched_bbox_idxes.erase(std::remove(low_unmatched_bbox_idxes.begin(),
                                                   low_unmatched_bbox_idxes.end(), m_bbox_idx),
                                       low_unmatched_bbox_idxes.end());
      }
    }
    std::vector<int> tmp_tracker_idxes; /* unmatch trackers' index in cascade match */
    /* Remove trackers' idx, which unmatched_times > T, from
     * unmatched_tracker_idxes */
    for (auto it = unmatched_tracker_idxes.begin(); it != unmatched_tracker_idxes.end();) {
      if (k_trackers[*it].unmatched_times > conf->max_unmatched_times_for_bbox_matching ||
          k_trackers[*it].bounding) {
        tmp_tracker_idxes.push_back(*it);
        it = unmatched_tracker_idxes.erase(it);
      } else {
        it++;
      }
    }
    /* Match remain trackers */
    /* - BBox IoU Distance */
    match_result_bbox =
        match(LowBBoxes, LowFeatures, unmatched_tracker_idxes, low_unmatched_bbox_idxes,
              conf->kfilter_conf, BBox_IoUDistance, conf->max_distance_iou);
    /* Match remain trackers */
    second_matched_pairs.insert(second_matched_pairs.end(), match_result_bbox.matched_pairs.begin(),
                                match_result_bbox.matched_pairs.end());
    unmatched_tracker_idxes = tmp_tracker_idxes;
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                   match_result_bbox.unmatched_tracker_idxes.begin(),
                                   match_result_bbox.unmatched_tracker_idxes.end());
    /* Update the kalman trackers (Matched) */
    LOGD("Update the  low score kalman trackers (Matched)");
    for (size_t i = 0; i < second_matched_pairs.size(); i++) {
      int tracker_idx = second_matched_pairs[i].first;

      int bbox_idx = second_matched_pairs[i].second;

      KalmanTracker &tracker_ = k_trackers[tracker_idx];

      const BBOX &bbox_ = LowBBoxes[bbox_idx];
      stRect rct(bbox_(0), bbox_(1), bbox_(2), bbox_(3));
      tracker_.update(kf_, &rct, conf);

      const FEATURE &feature_ = LowBBoxes[bbox_idx];

      tracker_.update_feature(feature_, conf->ktracker_conf.feature_budget_size,
                              conf->ktracker_conf.feature_update_interval);
      low_result[bbox_idx] =
          std::make_tuple(true, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
    }
  }
  /*****************************     low score bbox match   end
   * *************************************/

  /* Create new kalman trackers (Unmatched BBoxes) */
  LOGD("Create new kalman trackers (Unmatched BBoxes)");
  for (size_t i = 0; i < high_unmatched_bbox_idxes.size(); i++) {
    int bbox_idx = high_unmatched_bbox_idxes[i];
    uint64_t new_id = get_nextID(class_id);
    const BBOX &bbox_ = HighBBoxes[bbox_idx];
    // KalmanTracker tracker_(new_id, bbox_, feature_);
    if (conf->ktracker_conf.enable_QA_feature_init &&
        Quality[bbox_idx] < conf->ktracker_conf.feature_init_quality_threshold) {
      const FEATURE empty_feature(0);
      KalmanTracker tracker_(new_id, class_id, bbox_, empty_feature, conf->ktracker_conf);
      k_trackers.push_back(tracker_);
      high_result[bbox_idx] =
          std::make_tuple(false, tracker_.id, k_tracker_state_e::MISS, tracker_.getBBox_TLWH());
    } else {
      const FEATURE &feature_ = HighFeatures[bbox_idx];
      KalmanTracker tracker_(new_id, class_id, bbox_, feature_, conf->ktracker_conf);
      k_trackers.push_back(tracker_);
      high_result[bbox_idx] =
          std::make_tuple(false, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
    }
  }

  /* Update the kalman trackers (Unmatched) */
  LOGD("Update the kalman trackers (Unmatched)");
  for (size_t i = 0; i < unmatched_tracker_idxes.size(); i++) {
    int tracker_idx = unmatched_tracker_idxes[i];
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    tracker_.update(kf_, nullptr, conf);
    // tracker_.update_state(false, conf->ktracker_conf.max_unmatched_num,
    //                       conf->ktracker_conf.accreditation_threshold);
  }
  /* Check kalman trackers state, and remove invalid trackers */
  LOGD("Check kalman trackers state, and remove invalid trackers");
  for (auto it_ = k_trackers.begin(); it_ != k_trackers.end();) {
    if (it_->tracker_state == k_tracker_state_e::MISS) {
      it_ = k_trackers.erase(it_);
    } else {
      it_++;
    }
  }

  /* Update accreditation & probation tracker idxes */
  LOGD("Update accreditation & probation tracker idxes");
  accreditation_tracker_idxes.clear();
  probation_tracker_idxes.clear();
  for (size_t i = 0; i < k_trackers.size(); i++) {
    if (k_trackers[i].tracker_state == k_tracker_state_e::ACCREDITATION) {
      accreditation_tracker_idxes.push_back(i);
    } else if (k_trackers[i].tracker_state == k_tracker_state_e::PROBATION) {
      probation_tracker_idxes.push_back(i);
    } else {
      assert(0);
    }
  }

  return CVI_TDL_SUCCESS;
}
CVI_S32 DeepSORT::byte_track(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid,
                             float low_score, float high_score) {
  /** statistic what classes ID in bbox and tracker,
   *  and counting bbox number for each class */

  CVI_S32 ret = CVI_TDL_SUCCESS;
  std::map<int, int> class_bbox_counter;
  std::set<int> class_ids_bbox;
  std::set<int> class_ids_trackers;
  for (uint32_t i = 0; i < obj->size; i++) {
    auto iter = class_bbox_counter.find(obj->info[i].classes);
    if (iter != class_bbox_counter.end()) {
      iter->second += 1;
    } else {
      class_bbox_counter.insert(std::pair<int, int>(obj->info[i].classes, 1));
      class_ids_bbox.insert(obj->info[i].classes);
    }
  }

  for (size_t j = 0; j < k_trackers.size(); j++) {
    class_ids_trackers.insert(k_trackers[j].class_id);
  }

  CVI_TDL_MemAlloc(obj->size, tracker);

  /** run tracking function for each class ID in bbox
   */
  for (auto const &e : class_bbox_counter) {
    /** pick up all bboxes and features data for this class ID
     */
    std::vector<BBOX> high_bboxes;
    std::vector<BBOX> low_bboxes;

    std::vector<FEATURE> high_features;
    std::vector<FEATURE> low_features;

    std::vector<int> idx_table;
    for (uint32_t i = 0; i < obj->size; i++) {
      if (obj->info[i].classes == e.first) {
        if (obj->info[i].bbox.score > high_score) {
          idx_table.push_back(static_cast<int>(i));
          BBOX bbox_;
          uint32_t feature_size = obj->info[i].feature.size;
          FEATURE feature_(feature_size);
          bbox_(0, 0) = obj->info[i].bbox.x1;
          bbox_(0, 1) = obj->info[i].bbox.y1;
          bbox_(0, 2) = obj->info[i].bbox.x2 - obj->info[i].bbox.x1;
          bbox_(0, 3) = obj->info[i].bbox.y2 - obj->info[i].bbox.y1;
          if (obj->info[i].feature.type != TYPE_INT8) {
            LOGE("Feature Type not support now.\n");
            return CVI_TDL_ERR_INVALID_ARGS;
          }
          int type_size = getFeatureTypeSize(obj->info[i].feature.type);
          for (uint32_t d = 0; d < feature_size; d++) {
            feature_(d) = static_cast<float>(obj->info[i].feature.ptr[d * type_size]);
          }
          high_bboxes.push_back(bbox_);
          high_features.push_back(feature_);
        } else if (obj->info[i].bbox.score > low_score) {
          idx_table.push_back(static_cast<int>(i));
          BBOX bbox_;
          uint32_t feature_size = obj->info[i].feature.size;
          FEATURE feature_(feature_size);
          bbox_(0, 0) = obj->info[i].bbox.x1;
          bbox_(0, 1) = obj->info[i].bbox.y1;
          bbox_(0, 2) = obj->info[i].bbox.x2 - obj->info[i].bbox.x1;
          bbox_(0, 3) = obj->info[i].bbox.y2 - obj->info[i].bbox.y1;
          if (obj->info[i].feature.type != TYPE_INT8) {
            LOGE("Feature Type not support now.\n");
            return CVI_TDL_ERR_INVALID_ARGS;
          }
          int type_size = getFeatureTypeSize(obj->info[i].feature.type);
          for (uint32_t d = 0; d < feature_size; d++) {
            feature_(d) = static_cast<float>(obj->info[i].feature.ptr[d * type_size]);
          }
          low_bboxes.push_back(bbox_);
          low_features.push_back(feature_);
        }
      }
    }
    assert(idx_table.size() == static_cast<size_t>(e.second));

    /** run tracking function
     *    - ReID flag is only avaliable for PERSON now.
     */
    Tracking_Result high_result_(high_bboxes.size());
    Tracking_Result low_result_(low_bboxes.size());
    ret = track_impl(high_result_, low_result_, high_bboxes, low_bboxes, high_features,
                     low_features, 0.3, e.first, use_reid && (e.first == CVI_TDL_DET_TYPE_PERSON));
    if (CVI_TDL_SUCCESS != ret) {
      return ret;
    }
    /* high bbox update state */
    for (size_t i = 0; i < high_result_.size(); i++) {
      int idx = idx_table[i];
      bool &matched = std::get<0>(high_result_[i]);
      uint64_t &t_id = std::get<1>(high_result_[i]);
      k_tracker_state_e &t_state = std::get<2>(high_result_[i]);
      BBOX &t_bbox = std::get<3>(high_result_[i]);
      if (!matched) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
      } else if (t_state == k_tracker_state_e::PROBATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
      } else if (t_state == k_tracker_state_e::ACCREDITATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
      } else {
        LOGE("Tracker State Unknow.\n");
        return CVI_TDL_ERR_INVALID_ARGS;
      }
      tracker->info[idx].bbox.x1 = t_bbox(0);
      tracker->info[idx].bbox.y1 = t_bbox(1);
      tracker->info[idx].bbox.x2 = t_bbox(0) + t_bbox(2);
      tracker->info[idx].bbox.y2 = t_bbox(1) + t_bbox(3);
      obj->info[idx].unique_id = t_id;
      tracker->info[idx].id = t_id;
    }
    /* low bbox update state */
    for (size_t i = 0; i < low_result_.size(); i++) {
      int i_ = i + high_result_.size();
      int idx = idx_table[i_];
      bool &matched = std::get<0>(low_result_[i]);
      uint64_t &t_id = std::get<1>(low_result_[i]);
      k_tracker_state_e &t_state = std::get<2>(low_result_[i]);
      BBOX &t_bbox = std::get<3>(low_result_[i]);
      if (matched) {
        if (t_state == k_tracker_state_e::PROBATION) {
          tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
        } else if (t_state == k_tracker_state_e::ACCREDITATION) {
          tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
        } else {
          LOGE("Tracker State Unknow.\n");
          return CVI_TDL_ERR_INVALID_ARGS;
        }
        tracker->info[idx].bbox.x1 = t_bbox(0);
        tracker->info[idx].bbox.y1 = t_bbox(1);
        tracker->info[idx].bbox.x2 = t_bbox(0) + t_bbox(2);
        tracker->info[idx].bbox.y2 = t_bbox(1) + t_bbox(3);
        obj->info[idx].unique_id = t_id;
        tracker->info[idx].id = t_id;
      } else {
        obj->info[idx].unique_id = 0;
        tracker->info[idx].id = 0;
      }
    }
  }
  /** update tracker state even though there is no relative bbox (by class ID) at this time.
   */
  for (std::set<int>::iterator it = class_ids_trackers.begin(); it != class_ids_trackers.end();
       ++it) {
    if (class_ids_bbox.find(*it) == class_ids_bbox.end()) {
      std::vector<BBOX> bboxes;
      std::vector<FEATURE> features;
      Tracking_Result result_;
      if (CVI_SUCCESS != track_impl(result_, bboxes, features, 0.3, *it, use_reid)) {
        return CVI_TDL_FAILURE;
      }
    }
  }
  return CVI_TDL_SUCCESS;
}
CVI_S32 DeepSORT::track(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid) {
  /** statistic what classes ID in bbox and tracker,
   *  and counting bbox number for each class */

  CVI_S32 ret = CVI_TDL_SUCCESS;
  std::map<int, int> class_bbox_counter;
  std::set<int> class_ids_bbox;
  std::set<int> class_ids_trackers;
  for (uint32_t i = 0; i < obj->size; i++) {
    auto iter = class_bbox_counter.find(obj->info[i].classes);
    if (iter != class_bbox_counter.end()) {
      iter->second += 1;
    } else {
      class_bbox_counter.insert(std::pair<int, int>(obj->info[i].classes, 1));
      class_ids_bbox.insert(obj->info[i].classes);
    }
  }

  for (size_t j = 0; j < k_trackers.size(); j++) {
    class_ids_trackers.insert(k_trackers[j].class_id);
  }

  CVI_TDL_MemAlloc(obj->size, tracker);

  /** run tracking function for each class ID in bbox
   */
  for (auto const &e : class_bbox_counter) {
    /** pick up all bboxes and features data for this class ID
     */
    std::vector<BBOX> bboxes;
    std::vector<FEATURE> features;
    std::vector<int> idx_table;
    for (uint32_t i = 0; i < obj->size; i++) {
      if (obj->info[i].classes == e.first) {
        idx_table.push_back(static_cast<int>(i));
        BBOX bbox_;
        uint32_t feature_size = obj->info[i].feature.size;
        FEATURE feature_(feature_size);
        bbox_(0, 0) = obj->info[i].bbox.x1;
        bbox_(0, 1) = obj->info[i].bbox.y1;
        bbox_(0, 2) = obj->info[i].bbox.x2 - obj->info[i].bbox.x1;
        bbox_(0, 3) = obj->info[i].bbox.y2 - obj->info[i].bbox.y1;
        if (obj->info[i].feature.type != TYPE_INT8) {
          LOGE("Feature Type not support now.\n");
          return CVI_TDL_ERR_INVALID_ARGS;
        }
        int type_size = getFeatureTypeSize(obj->info[i].feature.type);
        for (uint32_t d = 0; d < feature_size; d++) {
          feature_(d) = static_cast<float>(obj->info[i].feature.ptr[d * type_size]);
        }
        bboxes.push_back(bbox_);
        features.push_back(feature_);
      }
    }
    assert(idx_table.size() == static_cast<size_t>(e.second));

    /** run tracking function
     *    - ReID flag is only avaliable for PERSON now.
     */
    Tracking_Result result_(bboxes.size());
    ret = track_impl(result_, bboxes, features, 0.3, e.first,
                     use_reid && (e.first == CVI_TDL_DET_TYPE_PERSON));
    if (CVI_TDL_SUCCESS != ret) {
      return ret;
    }

    for (size_t i = 0; i < result_.size(); i++) {
      int idx = idx_table[i];
      bool &matched = std::get<0>(result_[i]);
      uint64_t &t_id = std::get<1>(result_[i]);
      k_tracker_state_e &t_state = std::get<2>(result_[i]);
      BBOX &t_bbox = std::get<3>(result_[i]);
      if (!matched) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
        obj->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
      } else if (t_state == k_tracker_state_e::PROBATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
        obj->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
      } else if (t_state == k_tracker_state_e::ACCREDITATION) {
        tracker->info[idx].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
        obj->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
      } else {
        LOGE("Tracker State Unknow.\n");
        return CVI_TDL_ERR_INVALID_ARGS;
      }
      tracker->info[idx].bbox.x1 = t_bbox(0);
      tracker->info[idx].bbox.y1 = t_bbox(1);
      tracker->info[idx].bbox.x2 = t_bbox(0) + t_bbox(2);
      tracker->info[idx].bbox.y2 = t_bbox(1) + t_bbox(3);
      obj->info[idx].unique_id = t_id;
      tracker->info[idx].id = t_id;
    }
  }

  /** update tracker state even though there is no relative bbox (by class ID) at this time.
   */
  for (std::set<int>::iterator it = class_ids_trackers.begin(); it != class_ids_trackers.end();
       ++it) {
    if (class_ids_bbox.find(*it) == class_ids_bbox.end()) {
      std::vector<BBOX> bboxes;
      std::vector<FEATURE> features;
      Tracking_Result result_;
      if (CVI_SUCCESS != track_impl(result_, bboxes, features, 0.3, *it, use_reid)) {
        return CVI_TDL_FAILURE;
      }
    }
  }

  return CVI_TDL_SUCCESS;
}

CVI_S32 DeepSORT::track(cvtdl_face_t *face, cvtdl_tracker_t *tracker) {
#ifdef DEBUG_TRACK
  std::cout << "start to track,face num:" << face->size << std::endl;
  show_INFO_KalmanTrackers();
#endif
  CVI_S32 ret = CVI_TDL_SUCCESS;
  std::vector<BBOX> bboxes;
  std::vector<FEATURE> features;
  uint32_t bbox_num = face->size;

  bool use_reid = true;
  for (uint32_t i = 0; i < bbox_num; i++) {
    if (face->info[i].feature.size == 0 ||
        face->info[0].feature.size != face->info[i].feature.size) {
      use_reid = false;
      break;
    }
  }
  for (uint32_t i = 0; i < bbox_num; i++) {
    BBOX bbox_;
    bbox_(0, 0) = face->info[i].bbox.x1;
    bbox_(0, 1) = face->info[i].bbox.y1;
    bbox_(0, 2) = face->info[i].bbox.x2 - face->info[i].bbox.x1;
    bbox_(0, 3) = face->info[i].bbox.y2 - face->info[i].bbox.y1;
    bboxes.push_back(bbox_);
    uint32_t feature_size = (use_reid) ? face->info[i].feature.size : 0;
    FEATURE feature_(feature_size);
    if (use_reid) {
      if (face->info[i].feature.type != TYPE_INT8) {
        LOGE("Feature Type not support now.\n");
        return CVI_TDL_FAILURE;
      }
      int type_size = getFeatureTypeSize(face->info[i].feature.type);
      for (uint32_t d = 0; d < feature_size; d++) {
        feature_(d) = static_cast<float>(face->info[i].feature.ptr[d * type_size]);
      }
    }
    features.push_back(feature_);
  }

  float *bbox_quality = (float *)malloc(sizeof(float) * bbox_num);
  for (uint32_t i = 0; i < bbox_num; i++) {
    bbox_quality[i] = face->info[i].face_quality;
    if (face->info[i].pose_score == 0) {
      bbox_quality[i] = 0;
    }
  }

  Tracking_Result result_(bboxes.size());
  ret = track_impl(result_, bboxes, features, 0.1, -1, use_reid, bbox_quality);
  if (CVI_TDL_SUCCESS != ret) {
    free(bbox_quality);
    printf("ERROR\n");
    return ret;
  }
  free(bbox_quality);

  CVI_TDL_MemAlloc(k_trackers.size(), tracker);
  for (uint32_t i = 0; i < k_trackers.size(); i++) {
    memset(&tracker->info[i], 0, sizeof(tracker->info[i]));
    auto *p_track = &k_trackers[i];
    if (p_track->ages_ == 1) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
    } else if (p_track->tracker_state == k_tracker_state_e::PROBATION) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
    } else if (p_track->tracker_state == k_tracker_state_e::ACCREDITATION) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
    } else {
      LOGE("Tracker State Unknow.\n");
      printf("track unknown type error\n");
      continue;
    }
    BBOX t_bbox = p_track->getBBox_TLWH();
    tracker->info[i].bbox.x1 = t_bbox(0);
    tracker->info[i].bbox.y1 = t_bbox(1);
    tracker->info[i].bbox.x2 = t_bbox(0) + t_bbox(2);
    tracker->info[i].bbox.y2 = t_bbox(1) + t_bbox(3);
    tracker->info[i].id = p_track->id;
    tracker->info[i].out_num = p_track->out_nums;
  }

  assert(result_.size() == static_cast<size_t>(bbox_num));
  for (size_t i = 0; i < result_.size(); i++) {
    bool &matched = std::get<0>(result_[i]);
    uint64_t &t_id = std::get<1>(result_[i]);
    k_tracker_state_e &t_state = std::get<2>(result_[i]);
    BBOX &t_bbox = std::get<3>(result_[i]);
    if (!matched) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
    } else if (t_state == k_tracker_state_e::PROBATION) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
    } else if (t_state == k_tracker_state_e::ACCREDITATION) {
      tracker->info[i].state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
    } else {
      LOGE("Tracker State Unknow.\n");
      return CVI_TDL_ERR_INVALID_ARGS;
    }
    tracker->info[i].bbox.x1 = t_bbox(0);
    tracker->info[i].bbox.y1 = t_bbox(1);
    tracker->info[i].bbox.x2 = t_bbox(0) + t_bbox(2);
    tracker->info[i].bbox.y2 = t_bbox(1) + t_bbox(3);
    face->info[i].unique_id = t_id;
    tracker->info[i].id = t_id;
  }
#ifdef DEBUG_TRACK
  std::cout << "finish track,face num:" << face->size << std::endl;
  show_INFO_KalmanTrackers();
#endif
  return CVI_TDL_SUCCESS;
}

CVI_S32 DeepSORT::track_impl_cross(Tracking_Result2 &result, const std::vector<BBOX> &BBoxes,
                                   const std::vector<FEATURE> &Features, float crowd_iou_thresh,
                                   int class_id, bool use_reid, float *Quality,
                                   const cvtdl_counting_line_t *cross_line_t,
                                   const randomRect *rect) {
  cvtdl_deepsort_config_t *conf;
  auto it_conf = specific_conf.find(class_id);
  if (it_conf != specific_conf.end()) {
    conf = &it_conf->second;
  } else {
    conf = &default_conf;
  }
  if (conf->ktracker_conf.enable_QA_feature_update && Quality == NULL) {
    LOGE("Enable QA feature upate, but Quality is not initialized.");
    return CVI_TDL_FAILURE;
  }

  std::vector<int> tracker_ids;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    tracker_ids.push_back(k_trackers[i].id);
  }

  LOGD("Kalman Trackers predict\n");
  for (KalmanTracker &tracker_ : k_trackers) {
    // kf_.predict(tracker_.kalman_state, tracker_.x, tracker_.P, conf->kfilter_conf);
    tracker_.predict(kf_, conf);
  }
  check_bound_state(conf);

  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> unmatched_bbox_idxes;
  for (size_t i = 0; i < BBoxes.size(); i++) {
    unmatched_bbox_idxes.push_back(i);
  }

  std::vector<int> unmatched_tracker_idxes;
  if (class_id != -1) {
    for (std::vector<int>::iterator iter = accreditation_tracker_idxes.begin();
         iter != accreditation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
    /* Append probation trackers */
    for (std::vector<int>::iterator iter = probation_tracker_idxes.begin();
         iter != probation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
  } else {
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                   accreditation_tracker_idxes.begin(),
                                   accreditation_tracker_idxes.end());
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(), probation_tracker_idxes.begin(),
                                   probation_tracker_idxes.end());
  }

  LOGD("Cascade Match\n");
  /* Match accreditation trackers */
  /* - Cascade Match */
  /* - Feature Consine Distance */
  /* - Kalman Mahalanobis Distance */
  for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
    if (unmatched_bbox_idxes.empty()) {
      break;
    }
    cost_matrix_algo_e cost_method =
        (use_reid) ? Feature_CosineDistance : Kalman_MahalanobisDistance;
    std::vector<int> t_tracker_idxes;
    for (size_t tmp_i = 0; tmp_i < unmatched_tracker_idxes.size(); tmp_i++) {
      if (k_trackers[unmatched_tracker_idxes[tmp_i]].unmatched_times == t) {
        if (cost_method == Feature_CosineDistance &&
            !k_trackers[unmatched_tracker_idxes[tmp_i]].init_feature) {
          continue;
        }
        t_tracker_idxes.push_back(unmatched_tracker_idxes[tmp_i]);
      }
    }
    if (t_tracker_idxes.empty()) {
      continue;
    }
    MatchResult match_result = match(
        BBoxes, Features, t_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf, cost_method,
        (use_reid) ? conf->max_distance_consine : conf->kfilter_conf.chi2_threshold);
    if (match_result.matched_pairs.empty()) {
      continue;
    }
    /* Remove matched idx from bbox_idxes and unmatched_tracker_idxes */
    matched_pairs.insert(matched_pairs.end(), match_result.matched_pairs.begin(),
                         match_result.matched_pairs.end());

    for (size_t m_i = 0; m_i < match_result.matched_pairs.size(); m_i++) {
      int m_tracker_idx = match_result.matched_pairs[m_i].first;
      int m_bbox_idx = match_result.matched_pairs[m_i].second;
      unmatched_tracker_idxes.erase(std::remove(unmatched_tracker_idxes.begin(),
                                                unmatched_tracker_idxes.end(), m_tracker_idx),
                                    unmatched_tracker_idxes.end());
      unmatched_bbox_idxes.erase(
          std::remove(unmatched_bbox_idxes.begin(), unmatched_bbox_idxes.end(), m_bbox_idx),
          unmatched_bbox_idxes.end());
    }
  }

  std::vector<int> tmp_tracker_idxes; /* unmatch trackers' index in cascade match */
  /* Remove trackers' idx, which unmatched_times > T, from
   * unmatched_tracker_idxes */
  for (auto it = unmatched_tracker_idxes.begin(); it != unmatched_tracker_idxes.end();) {
    if (k_trackers[*it].unmatched_times > conf->max_unmatched_times_for_bbox_matching ||
        k_trackers[*it].bounding) {
      tmp_tracker_idxes.push_back(*it);
      it = unmatched_tracker_idxes.erase(it);
    } else {
      it++;
    }
  }

#if 0
  /* Append probation trackers */
  unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
    probation_tracker_idxes.begin(), probation_tracker_idxes.end());
#endif

  /* Match remain trackers */
  /* - BBox IoU Distance */
  MatchResult match_result_bbox =
      match(BBoxes, Features, unmatched_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf,
            BBox_IoUDistance, conf->max_distance_iou);

  /* Match remain trackers */
  matched_pairs.insert(matched_pairs.end(), match_result_bbox.matched_pairs.begin(),
                       match_result_bbox.matched_pairs.end());
  unmatched_bbox_idxes = match_result_bbox.unmatched_bbox_idxes;
  unmatched_tracker_idxes = tmp_tracker_idxes;
  unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                 match_result_bbox.unmatched_tracker_idxes.begin(),
                                 match_result_bbox.unmatched_tracker_idxes.end());
  // MatchResult match_recall = refine_uncrowd(BBoxes, Features, unmatched_tracker_idxes,
  //                                           unmatched_bbox_idxes, crowd_iou_thresh);

  // /* Match remain trackers */
  // matched_pairs.insert(matched_pairs.end(), match_recall.matched_pairs.begin(),
  //                      match_recall.matched_pairs.end());
  // unmatched_bbox_idxes = match_recall.unmatched_bbox_idxes;
  // unmatched_tracker_idxes = match_recall.unmatched_tracker_idxes;
  /* Update the kalman trackers (Matched) */
  LOGD("Update the kalman trackers (Matched)");
  for (size_t i = 0; i < matched_pairs.size(); i++) {
    int tracker_idx = matched_pairs[i].first;
    int bbox_idx = matched_pairs[i].second;
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    const BBOX &bbox_ = BBoxes[bbox_idx];
    {
      float cur_x = bbox_[0] + bbox_[2] * 0.5;
      float cur_y = bbox_[1] + bbox_[3] * 0.5;

      float pre_x = tracker_.old_x;
      float pre_y = tracker_.old_y;

      if (isLineIntersectBoarder(rect, pre_x, pre_y, cur_x, cur_y)) {
        float tmp_x = cur_x - pre_x;
        float tmp_y = cur_y - pre_y;
        if (tmp_x * rect->f_x + tmp_y * rect->f_y > 0 && tracker_.cross_gap == 0) {
          tracker_.cross_gap = 40;
          tracker_.is_cross = true;
          tracker_.first_x = 0;
          tracker_.first_y = 0;
        } else if (cross_line_t->s_mode == 2 && tracker_.cross_gap == 0) {
          tracker_.cross_gap = 40;
          tracker_.is_cross = true;
          tracker_.first_x = 0;
          tracker_.first_y = 0;
        }
      }
      if (tracker_.cross_gap > 0)
        tracker_.is_cross = true;
      else
        tracker_.is_cross = false;
      tracker_.cross_gap = (tracker_.cross_gap >= 1) ? tracker_.cross_gap - 1 : 0;
      tracker_.old_x = cur_x;
      tracker_.old_y = cur_y;
    }
    stRect rct(bbox_(0), bbox_(1), bbox_(2), bbox_(3));
    tracker_.update(kf_, &rct, conf);

    bool quality_ok = true;
    if (Quality != nullptr && Quality[bbox_idx] == 0) quality_ok = false;
    if ((!conf->ktracker_conf.enable_QA_feature_update && quality_ok) ||
        (conf->ktracker_conf.enable_QA_feature_update &&
         Quality[bbox_idx] > conf->ktracker_conf.feature_update_quality_threshold)) {
      const FEATURE &feature_ = Features[bbox_idx];
      tracker_.update_feature(feature_, conf->ktracker_conf.feature_budget_size,
                              conf->ktracker_conf.feature_update_interval);
    }
    if (tracker_.is_cross) {
      result[bbox_idx] =
          std::make_tuple(true, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH(), true);
    } else {
      result[bbox_idx] = std::make_tuple(true, tracker_.id, tracker_.tracker_state,
                                         tracker_.getBBox_TLWH(), false);
    }
  }

  /* Update the kalman trackers (Unmatched) */
  LOGD("Update the kalman trackers (Unmatched)");
  for (size_t i = 0; i < unmatched_tracker_idxes.size(); i++) {
    int tracker_idx = unmatched_tracker_idxes[i];
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    tracker_.update(kf_, nullptr, conf);
    // tracker_.update_state(false, conf->ktracker_conf.max_unmatched_num,
    //                       conf->ktracker_conf.accreditation_threshold);
  }

  /* Check kalman trackers state, and remove invalid trackers */
  LOGD("Check kalman trackers state, and remove invalid trackers");
  for (auto it_ = k_trackers.begin(); it_ != k_trackers.end();) {
    if (it_->tracker_state == k_tracker_state_e::MISS) {
      it_ = k_trackers.erase(it_);
    } else {
      it_++;
    }
  }

  /* Create new kalman trackers (Unmatched BBoxes) */
  LOGD("Create new kalman trackers (Unmatched BBoxes)");
  for (size_t i = 0; i < unmatched_bbox_idxes.size(); i++) {
    int bbox_idx = unmatched_bbox_idxes[i];
    uint64_t new_id = get_nextID(class_id);
    const BBOX &bbox_ = BBoxes[bbox_idx];
    // KalmanTracker tracker_(new_id, bbox_, feature_);
    if (conf->ktracker_conf.enable_QA_feature_init &&
        Quality[bbox_idx] < conf->ktracker_conf.feature_init_quality_threshold) {
      const FEATURE empty_feature(0);
      KalmanTracker tracker_(new_id, class_id, bbox_, empty_feature, conf->ktracker_conf);
      tracker_.old_x = bbox_[0] + bbox_[2] * 0.5;
      tracker_.old_y = bbox_[1] + bbox_[3] * 0.5;
      k_trackers.push_back(tracker_);
      result[bbox_idx] = std::make_tuple(false, tracker_.id, tracker_.tracker_state,
                                         tracker_.getBBox_TLWH(), false);
    } else {
      const FEATURE &feature_ = Features[bbox_idx];
      KalmanTracker tracker_(new_id, class_id, bbox_, feature_, conf->ktracker_conf);
      tracker_.old_x = bbox_[0] + bbox_[2] * 0.5;
      tracker_.old_y = bbox_[1] + bbox_[3] * 0.5;
      k_trackers.push_back(tracker_);
      result[bbox_idx] = std::make_tuple(false, tracker_.id, tracker_.tracker_state,
                                         tracker_.getBBox_TLWH(), false);
    }
  }

  /* Update accreditation & probation tracker idxes */
  LOGD("Update accreditation & probation tracker idxes");
  accreditation_tracker_idxes.clear();
  probation_tracker_idxes.clear();
  for (size_t i = 0; i < k_trackers.size(); i++) {
    if (k_trackers[i].tracker_state == k_tracker_state_e::ACCREDITATION) {
      accreditation_tracker_idxes.push_back(i);
    } else if (k_trackers[i].tracker_state == k_tracker_state_e::PROBATION) {
      probation_tracker_idxes.push_back(i);
    } else {
      assert(0);
    }
  }

  return CVI_TDL_SUCCESS;
}
CVI_S32 DeepSORT::track_impl(Tracking_Result &result, const std::vector<BBOX> &BBoxes,
                             const std::vector<FEATURE> &Features, float crowd_iou_thresh,
                             int class_id, bool use_reid, float *Quality) {
  cvtdl_deepsort_config_t *conf;
  auto it_conf = specific_conf.find(class_id);
  if (it_conf != specific_conf.end()) {
    conf = &it_conf->second;
  } else {
    conf = &default_conf;
  }
  if (conf->ktracker_conf.enable_QA_feature_update && Quality == NULL) {
    LOGE("Enable QA feature upate, but Quality is not initialized.");
    return CVI_TDL_FAILURE;
  }

  std::vector<int> tracker_ids;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    tracker_ids.push_back(k_trackers[i].id);
  }

  LOGD("Kalman Trackers predict\n");
  for (KalmanTracker &tracker_ : k_trackers) {
    // kf_.predict(tracker_.kalman_state, tracker_.x, tracker_.P, conf->kfilter_conf);
    if (tracker_.class_id == class_id) {
      tracker_.predict(kf_, conf);
    }
  }
  check_bound_state(conf);

  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> unmatched_bbox_idxes;
  for (size_t i = 0; i < BBoxes.size(); i++) {
    unmatched_bbox_idxes.push_back(i);
  }

  std::vector<int> unmatched_tracker_idxes;
  if (class_id != -1) {
    for (std::vector<int>::iterator iter = accreditation_tracker_idxes.begin();
         iter != accreditation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
    /* Append probation trackers */
    for (std::vector<int>::iterator iter = probation_tracker_idxes.begin();
         iter != probation_tracker_idxes.end(); iter++) {
      if (k_trackers[*iter].class_id == class_id) {
        unmatched_tracker_idxes.push_back(*iter);
      }
    }
  } else {
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                   accreditation_tracker_idxes.begin(),
                                   accreditation_tracker_idxes.end());
    unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(), probation_tracker_idxes.begin(),
                                   probation_tracker_idxes.end());
  }

  LOGD("Cascade Match\n");
  /* Match accreditation trackers */
  /* - Cascade Match */
  /* - Feature Consine Distance */
  /* - Kalman Mahalanobis Distance */
  for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
    if (unmatched_bbox_idxes.empty()) {
      break;
    }
    cost_matrix_algo_e cost_method =
        (use_reid) ? Feature_CosineDistance : Kalman_MahalanobisDistance;
    std::vector<int> t_tracker_idxes;
    for (size_t tmp_i = 0; tmp_i < unmatched_tracker_idxes.size(); tmp_i++) {
      if (k_trackers[unmatched_tracker_idxes[tmp_i]].unmatched_times == t) {
        if (cost_method == Feature_CosineDistance &&
            !k_trackers[unmatched_tracker_idxes[tmp_i]].init_feature) {
          continue;
        }
        t_tracker_idxes.push_back(unmatched_tracker_idxes[tmp_i]);
      }
    }
    if (t_tracker_idxes.empty()) {
      continue;
    }
    MatchResult match_result = match(
        BBoxes, Features, t_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf, cost_method,
        (use_reid) ? conf->max_distance_consine : conf->kfilter_conf.chi2_threshold);
    if (match_result.matched_pairs.empty()) {
      continue;
    }
    /* Remove matched idx from bbox_idxes and unmatched_tracker_idxes */
    matched_pairs.insert(matched_pairs.end(), match_result.matched_pairs.begin(),
                         match_result.matched_pairs.end());

    for (size_t m_i = 0; m_i < match_result.matched_pairs.size(); m_i++) {
      int m_tracker_idx = match_result.matched_pairs[m_i].first;
      int m_bbox_idx = match_result.matched_pairs[m_i].second;
      unmatched_tracker_idxes.erase(std::remove(unmatched_tracker_idxes.begin(),
                                                unmatched_tracker_idxes.end(), m_tracker_idx),
                                    unmatched_tracker_idxes.end());
      unmatched_bbox_idxes.erase(
          std::remove(unmatched_bbox_idxes.begin(), unmatched_bbox_idxes.end(), m_bbox_idx),
          unmatched_bbox_idxes.end());
    }
  }

  std::vector<int> tmp_tracker_idxes; /* unmatch trackers' index in cascade match */
  /* Remove trackers' idx, which unmatched_times > T, from
   * unmatched_tracker_idxes */
  for (auto it = unmatched_tracker_idxes.begin(); it != unmatched_tracker_idxes.end();) {
    if (k_trackers[*it].unmatched_times > conf->max_unmatched_times_for_bbox_matching ||
        k_trackers[*it].bounding) {
      tmp_tracker_idxes.push_back(*it);
      it = unmatched_tracker_idxes.erase(it);
    } else {
      it++;
    }
  }

#if 0
  /* Append probation trackers */
  unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
    probation_tracker_idxes.begin(), probation_tracker_idxes.end());
#endif

  /* Match remain trackers */
  /* - BBox IoU Distance */
  MatchResult match_result_bbox =
      match(BBoxes, Features, unmatched_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf,
            BBox_IoUDistance, conf->max_distance_iou);

  /* Match remain trackers */
  matched_pairs.insert(matched_pairs.end(), match_result_bbox.matched_pairs.begin(),
                       match_result_bbox.matched_pairs.end());
  unmatched_bbox_idxes = match_result_bbox.unmatched_bbox_idxes;
  unmatched_tracker_idxes = tmp_tracker_idxes;
  unmatched_tracker_idxes.insert(unmatched_tracker_idxes.end(),
                                 match_result_bbox.unmatched_tracker_idxes.begin(),
                                 match_result_bbox.unmatched_tracker_idxes.end());
  // MatchResult match_recall = refine_uncrowd(BBoxes, Features, unmatched_tracker_idxes,
  //                                           unmatched_bbox_idxes, crowd_iou_thresh);

  // /* Match remain trackers */
  // matched_pairs.insert(matched_pairs.end(), match_recall.matched_pairs.begin(),
  //                      match_recall.matched_pairs.end());
  // unmatched_bbox_idxes = match_recall.unmatched_bbox_idxes;
  // unmatched_tracker_idxes = match_recall.unmatched_tracker_idxes;
  /* Update the kalman trackers (Matched) */
  LOGD("Update the kalman trackers (Matched)");
  for (size_t i = 0; i < matched_pairs.size(); i++) {
    int tracker_idx = matched_pairs[i].first;
    int bbox_idx = matched_pairs[i].second;
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    const BBOX &bbox_ = BBoxes[bbox_idx];
    // tracker_.update_state(true, conf->ktracker_conf.max_unmatched_num,
    //                       conf->ktracker_conf.accreditation_threshold);
    // kf_.update(tracker_.kalman_state, tracker_.x, tracker_.P, bbox_tlwh2xyah(bbox_),
    //            conf->kfilter_conf);
    stRect rct(bbox_(0), bbox_(1), bbox_(2), bbox_(3));
    tracker_.update(kf_, &rct, conf);

    bool quality_ok = true;
    if (Quality != nullptr && Quality[bbox_idx] == 0) quality_ok = false;
    if ((!conf->ktracker_conf.enable_QA_feature_update && quality_ok) ||
        (conf->ktracker_conf.enable_QA_feature_update &&
         Quality[bbox_idx] > conf->ktracker_conf.feature_update_quality_threshold)) {
      const FEATURE &feature_ = Features[bbox_idx];
      tracker_.update_feature(feature_, conf->ktracker_conf.feature_budget_size,
                              conf->ktracker_conf.feature_update_interval);
    }
    result[bbox_idx] =
        std::make_tuple(true, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
  }

  /* Update the kalman trackers (Unmatched) */
  LOGD("Update the kalman trackers (Unmatched)");
  for (size_t i = 0; i < unmatched_tracker_idxes.size(); i++) {
    int tracker_idx = unmatched_tracker_idxes[i];
    KalmanTracker &tracker_ = k_trackers[tracker_idx];
    tracker_.update(kf_, nullptr, conf);
    // tracker_.update_state(false, conf->ktracker_conf.max_unmatched_num,
    //                       conf->ktracker_conf.accreditation_threshold);
  }

  /* Check kalman trackers state, and remove invalid trackers */
  LOGD("Check kalman trackers state, and remove invalid trackers");
  for (auto it_ = k_trackers.begin(); it_ != k_trackers.end();) {
    if (it_->tracker_state == k_tracker_state_e::MISS) {
      it_ = k_trackers.erase(it_);
    } else {
      it_++;
    }
  }

  /* Create new kalman trackers (Unmatched BBoxes) */
  LOGD("Create new kalman trackers (Unmatched BBoxes)");
  for (size_t i = 0; i < unmatched_bbox_idxes.size(); i++) {
    int bbox_idx = unmatched_bbox_idxes[i];
    uint64_t new_id = get_nextID(class_id);
    const BBOX &bbox_ = BBoxes[bbox_idx];
    // KalmanTracker tracker_(new_id, bbox_, feature_);
    if (conf->ktracker_conf.enable_QA_feature_init &&
        Quality[bbox_idx] < conf->ktracker_conf.feature_init_quality_threshold) {
      const FEATURE empty_feature(0);
      KalmanTracker tracker_(new_id, class_id, bbox_, empty_feature, conf->ktracker_conf);
      k_trackers.push_back(tracker_);
      result[bbox_idx] =
          std::make_tuple(false, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
    } else {
      const FEATURE &feature_ = Features[bbox_idx];
      KalmanTracker tracker_(new_id, class_id, bbox_, feature_, conf->ktracker_conf);
      k_trackers.push_back(tracker_);
      result[bbox_idx] =
          std::make_tuple(false, tracker_.id, tracker_.tracker_state, tracker_.getBBox_TLWH());
    }
  }

  /* Update accreditation & probation tracker idxes */
  LOGD("Update accreditation & probation tracker idxes");
  accreditation_tracker_idxes.clear();
  probation_tracker_idxes.clear();
  for (size_t i = 0; i < k_trackers.size(); i++) {
    if (k_trackers[i].tracker_state == k_tracker_state_e::ACCREDITATION) {
      accreditation_tracker_idxes.push_back(i);
    } else if (k_trackers[i].tracker_state == k_tracker_state_e::PROBATION) {
      probation_tracker_idxes.push_back(i);
    } else {
      assert(0);
    }
  }

  return CVI_TDL_SUCCESS;
}
void DeepSORT::check_bound_state(cvtdl_deepsort_config_t *conf) {
  if (!conf->kfilter_conf.enable_bounding_stay) return;
  stRect imgroi(0, 0, image_width_, image_height_);
  for (KalmanTracker &tracker_ : k_trackers) {
    BBOX box = tracker_.getBBox_TLWH();
    stRect track_rct = tlwh2rect(box);
    float iou = cal_iou(imgroi, track_rct);
    if (iou < bounding_iou_thresh_) {
      tracker_.bounding = true;
      printf("track:%d leaving image\n", (int)tracker_.id);
    }
  }
}
MatchResult DeepSORT::match(const std::vector<BBOX> &BBoxes, const std::vector<FEATURE> &Features,
                            const std::vector<int> &Tracker_IDXes,
                            const std::vector<int> &BBox_IDXes,
                            cvtdl_kalman_filter_config_t &kf_conf, cost_matrix_algo_e cost_method,
                            float max_distance) {
  MatchResult result_;

  if (Tracker_IDXes.empty() || BBox_IDXes.empty()) {
    result_.unmatched_tracker_idxes = Tracker_IDXes;
    result_.unmatched_bbox_idxes = BBox_IDXes;
    return result_;
  }

  COST_MATRIX cost_matrix;
  switch (cost_method) {
    case Feature_CosineDistance: {
      LOGD("Feature Cost Matrix (Consine Distance)");
      cost_matrix = KalmanTracker::getCostMatrix_Feature(k_trackers, BBoxes, Features,
                                                         Tracker_IDXes, BBox_IDXes);
      // gating cost matrix with different methods
      if (track_face_) {
        KalmanTracker::restrictCostMatrix_BBox(cost_matrix, k_trackers, BBoxes, Tracker_IDXes,
                                               BBox_IDXes, max_distance);
      } else {
        KalmanTracker::restrictCostMatrix_Mahalanobis(
            cost_matrix, kf_, k_trackers, BBoxes, Tracker_IDXes, BBox_IDXes, kf_conf, max_distance);
      }

    } break;
    case Kalman_MahalanobisDistance: {
      LOGD("Kalman Cost Matrix (Mahalanobis Distance)");
      cost_matrix = KalmanTracker::getCostMatrix_Mahalanobis(kf_, k_trackers, BBoxes, Tracker_IDXes,
                                                             BBox_IDXes, kf_conf, max_distance);
#ifdef DEBUG_TRACK
      std::cout << "mahah cost matrix:\n" << cost_matrix << std::endl;
#endif
    } break;
    case BBox_IoUDistance: {
      LOGD("BBox Cost Matrix (IoU Distance)");
      cost_matrix = KalmanTracker::getCostMatrix_BBox(k_trackers, BBoxes, Features, Tracker_IDXes,
                                                      BBox_IDXes);
#ifdef DEBUG_TRACK
      std::cout << "iou cost matrix:\n" << cost_matrix << std::endl;
#endif
      restrict_cost_matrix(cost_matrix, max_distance);
    } break;
    default:
      LOGE("Unknown cost method %d", cost_method);
      return result_;
  }

  CVIMunkres cvi_munkres_solver(&cost_matrix);
  if (cvi_munkres_solver.solve() == MUNKRES_FAILURE) {
    LOGW("MUNKRES algorithm failed.");
    // return empty results if failed to solve
    result_.unmatched_tracker_idxes.clear();
    result_.unmatched_bbox_idxes.clear();
    return result_;
  }

  int bbox_num = BBox_IDXes.size();
  int tracker_num = Tracker_IDXes.size();
  bool *matched_tracker_i = new bool[tracker_num];
  bool *matched_bbox_j = new bool[bbox_num];
  memset(matched_tracker_i, false, tracker_num * sizeof(bool));
  memset(matched_bbox_j, false, bbox_num * sizeof(bool));

  for (int i = 0; i < tracker_num; i++) {
    int bbox_j = cvi_munkres_solver.m_match_result[i];
    if (bbox_j != -1) {
      if (cost_matrix(i, bbox_j) < max_distance) {
        matched_tracker_i[i] = true;
        matched_bbox_j[bbox_j] = true;
        int tracker_idx = Tracker_IDXes[i];
        int bbox_idx = BBox_IDXes[bbox_j];
        result_.matched_pairs.push_back(std::make_pair(tracker_idx, bbox_idx));
      }
    }
  }

  for (int i = 0; i < tracker_num; i++) {
    if (!matched_tracker_i[i]) {
      int tracker_idx = Tracker_IDXes[i];
      result_.unmatched_tracker_idxes.push_back(tracker_idx);
    }
  }

  for (int j = 0; j < bbox_num; j++) {
    if (!matched_bbox_j[j]) {
      int bbox_idx = BBox_IDXes[j];
      result_.unmatched_bbox_idxes.push_back(bbox_idx);
    }
  }

  delete[] matched_tracker_i;
  delete[] matched_bbox_j;

  return result_;
}

MatchResult DeepSORT::refine_uncrowd(const std::vector<BBOX> &BBoxes,
                                     const std::vector<FEATURE> &Features,
                                     const std::vector<int> &Tracker_IDXes,
                                     const std::vector<int> &BBox_IDXes, float iou_thresh) {
  MatchResult result_;
  if (Tracker_IDXes.empty() || BBox_IDXes.empty()) {
    result_.unmatched_tracker_idxes = Tracker_IDXes;
    result_.unmatched_bbox_idxes = BBox_IDXes;
    return result_;
  }

  std::vector<BBOX> all_track_boxes;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    all_track_boxes.push_back(k_trackers[i].getBBox_TLWH());
  }

  int bbox_num = BBox_IDXes.size();
  int tracker_num = Tracker_IDXes.size();
  bool *matched_tracker_i = new bool[tracker_num];
  bool *matched_bbox_j = new bool[bbox_num];
  memset(matched_tracker_i, false, tracker_num * sizeof(bool));
  memset(matched_bbox_j, false, bbox_num * sizeof(bool));
  for (size_t j = 0; j < BBox_IDXes.size(); j++) {
    // match with unmatched tracks
    if (matched_bbox_j[j]) continue;
    int didx = BBox_IDXes[j];
    bool is_det_crowded = is_bbox_crowded(BBoxes, didx, 1.5);

    if (is_det_crowded) {
      // std::cout<<"crowd bbox:"<<BBoxes[didx]<<std::endl;
      continue;
    }
    for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
      if (matched_tracker_i[i]) continue;
      int tidx = Tracker_IDXes[i];
      if (k_trackers[tidx].unmatched_times > 2) continue;
      float iou = cal_iou_bbox(BBoxes[didx], all_track_boxes[tidx]);

      if (iou > iou_thresh) {
        float boxsim = compute_box_sim_bbox(BBoxes[didx], all_track_boxes[tidx]);
        // std::cout<<"dbox:"<<BBoxes[didx]<<",tbox:"<<all_track_boxes[tidx]<<",sim:"<<boxsim<<std::endl;
        if (boxsim > 0.75 && !is_bbox_crowded(all_track_boxes, tidx, 1.5)) {
          matched_tracker_i[i] = true;
          matched_bbox_j[j] = true;
          std::cout << "recall uncrowded,tracker:" << tidx << ",bboxidx:" << didx << std::endl;
          result_.matched_pairs.push_back(std::make_pair(tidx, didx));
        }
      }
    }
  }
  for (int i = 0; i < tracker_num; i++) {
    if (!matched_tracker_i[i]) {
      int tracker_idx = Tracker_IDXes[i];
      result_.unmatched_tracker_idxes.push_back(tracker_idx);
    }
  }

  for (int j = 0; j < bbox_num; j++) {
    if (!matched_bbox_j[j]) {
      int bbox_idx = BBox_IDXes[j];
      result_.unmatched_bbox_idxes.push_back(bbox_idx);
    }
  }

  delete[] matched_tracker_i;
  delete[] matched_bbox_j;
  return result_;
}

uint64_t DeepSORT::get_nextID(int class_id) {
  if (!sp_counter) {
    if (id_counter == 0xffffffff) {  // overflow condition
      id_counter = 0;
    }
    id_counter += 1;
    return id_counter;
  } else {
    auto it_ = specific_id_counter.find(class_id);
    if (it_ != specific_id_counter.end()) {
      if (it_->second == 0xffffffff) {  // overflow condition
        it_->second = 0;
      }
      it_->second += 1;
      return it_->second;
    } else {
      specific_id_counter[class_id] = 1;
      return 1;
    }
  }
}

void DeepSORT::cleanCounter() {
  id_counter = 0;
  for (auto &it : specific_id_counter) {
    it.second = 0;
  }
}

CVI_S32 DeepSORT::getConfig(cvtdl_deepsort_config_t *ds_conf, int cvitdl_obj_type) {
  if (ds_conf == NULL) {
    LOGE("input config is NULL.");
    return CVI_TDL_FAILURE;
  }
  if (cvitdl_obj_type == -1) {
    memcpy(ds_conf, &default_conf, sizeof(cvtdl_deepsort_config_t));
    return CVI_TDL_SUCCESS;
  }
  if (specific_conf.find(cvitdl_obj_type) == specific_conf.end()) {
    LOGE("specific config[%d] not found.", cvitdl_obj_type);
    return CVI_TDL_FAILURE;
  }
  memcpy(ds_conf, &specific_conf[cvitdl_obj_type], sizeof(cvtdl_deepsort_config_t));
  return CVI_TDL_SUCCESS;
}

CVI_S32 DeepSORT::setConfig(cvtdl_deepsort_config_t *ds_conf, int cvitdl_obj_type,
                            bool show_config) {
  if (ds_conf == NULL) {
    LOGE("Input config is NULL.");
    return CVI_TDL_FAILURE;
  }
  if (ds_conf->kfilter_conf.enable_X_constraint_1) {
    LOGW("Enable X constraint 1 is not verified now.");
  }
  switch (ds_conf->kfilter_conf.confidence_level) {
    case L005: {
      ds_conf->kfilter_conf.chi2_threshold = chi2_005[4];
    } break;
    case L010: {
      ds_conf->kfilter_conf.chi2_threshold = chi2_010[4];
    } break;
    case L025: {
      ds_conf->kfilter_conf.chi2_threshold = chi2_025[4];
    } break;
    case L050: {
      ds_conf->kfilter_conf.chi2_threshold = chi2_050[4];
    } break;
    case L100: {
      ds_conf->kfilter_conf.chi2_threshold = chi2_100[4];
    } break;
    default:
      LOGE("unknown mahalanobis confidence[%d]", ds_conf->kfilter_conf.confidence_level);
      return CVI_TDL_FAILURE;
  }
  if (cvitdl_obj_type == -1) {
    memcpy(&default_conf, ds_conf, sizeof(cvtdl_deepsort_config_t));
  } else {
    specific_conf[cvitdl_obj_type] = *ds_conf;
  }
  if (show_config) {
    show_deepsort_config(*ds_conf);
  }
  return CVI_TDL_SUCCESS;
}

cvtdl_deepsort_config_t DeepSORT::get_DefaultConfig() {
  cvtdl_deepsort_config_t conf;
  conf.max_distance_consine = 0.2;
  conf.max_distance_iou = 0.7;
  conf.max_unmatched_times_for_bbox_matching = 2;
  conf.enable_internal_FQ = false;

  conf.ktracker_conf.accreditation_threshold = 3;
  conf.ktracker_conf.feature_budget_size = 8;
  conf.ktracker_conf.feature_update_interval = 1;
  conf.ktracker_conf.max_unmatched_num = 40;
  conf.ktracker_conf.enable_QA_feature_update = false;
  conf.ktracker_conf.enable_QA_feature_init = false;
  conf.ktracker_conf.feature_update_quality_threshold = 0.75;
  conf.ktracker_conf.feature_init_quality_threshold = 0.75;

  conf.ktracker_conf.P_alpha[0] = 2 * 1 / 20.0;
  conf.ktracker_conf.P_alpha[1] = 2 * 1 / 20.0;
  conf.ktracker_conf.P_alpha[2] = 0.0;
  conf.ktracker_conf.P_alpha[3] = 2 * 1 / 20.0;
  conf.ktracker_conf.P_alpha[4] = 10 * 1 / 160.0;
  conf.ktracker_conf.P_alpha[5] = 10 * 1 / 160.0;
  conf.ktracker_conf.P_alpha[6] = 0.0;
  conf.ktracker_conf.P_alpha[7] = 10 * 1 / 160.0;
  conf.ktracker_conf.P_x_idx[0] = 3;
  conf.ktracker_conf.P_x_idx[1] = 3;
  conf.ktracker_conf.P_x_idx[2] = -1;
  conf.ktracker_conf.P_x_idx[3] = 3;
  conf.ktracker_conf.P_x_idx[4] = 3;
  conf.ktracker_conf.P_x_idx[5] = 3;
  conf.ktracker_conf.P_x_idx[6] = -1;
  conf.ktracker_conf.P_x_idx[7] = 3;
  conf.ktracker_conf.P_beta[0] = 0.0;
  conf.ktracker_conf.P_beta[1] = 0.0;
  conf.ktracker_conf.P_beta[2] = 0.01;
  conf.ktracker_conf.P_beta[3] = 0.0;
  conf.ktracker_conf.P_beta[4] = 0.0;
  conf.ktracker_conf.P_beta[5] = 0.0;
  conf.ktracker_conf.P_beta[6] = 1e-5;
  conf.ktracker_conf.P_beta[7] = 0.0;

  conf.kfilter_conf.confidence_level = L050;
  conf.kfilter_conf.chi2_threshold = 0.0;
  conf.kfilter_conf.enable_X_constraint_0 = false;
  conf.kfilter_conf.enable_X_constraint_1 = false;
  conf.kfilter_conf.enable_bounding_stay = false;

  conf.kfilter_conf.X_constraint_min[0] = 0.0;
  conf.kfilter_conf.X_constraint_min[1] = 0.0;
  conf.kfilter_conf.X_constraint_min[2] = DEFAULT_X_CONSTRAINT_A_MIN;
  conf.kfilter_conf.X_constraint_min[3] = DEFAULT_X_CONSTRAINT_H_MIN;
  conf.kfilter_conf.X_constraint_min[4] = -0.1 * DEFAULT_X_CONSTRAINT_A_MAX;
  conf.kfilter_conf.X_constraint_min[5] = -0.1 * DEFAULT_X_CONSTRAINT_A_MAX;
  conf.kfilter_conf.X_constraint_min[6] = -0.25;
  conf.kfilter_conf.X_constraint_min[7] = -0.25;

  conf.kfilter_conf.X_constraint_max[0] = 0.0;
  conf.kfilter_conf.X_constraint_max[1] = 0.0;
  conf.kfilter_conf.X_constraint_max[2] = DEFAULT_X_CONSTRAINT_A_MAX;
  conf.kfilter_conf.X_constraint_max[3] = DEFAULT_X_CONSTRAINT_H_MAX;
  conf.kfilter_conf.X_constraint_max[4] = -1. * conf.kfilter_conf.X_constraint_min[4];
  conf.kfilter_conf.X_constraint_max[5] = -1. * conf.kfilter_conf.X_constraint_min[5];
  conf.kfilter_conf.X_constraint_max[6] = -1. * conf.kfilter_conf.X_constraint_min[6];
  conf.kfilter_conf.X_constraint_max[7] = -1. * conf.kfilter_conf.X_constraint_min[7];

  conf.kfilter_conf.Q_alpha[0] = 1 / 20.0;
  conf.kfilter_conf.Q_alpha[1] = 1 / 20.0;
  conf.kfilter_conf.Q_alpha[2] = 0.0;
  conf.kfilter_conf.Q_alpha[3] = 1 / 20.0;
  conf.kfilter_conf.Q_alpha[4] = 1 / 160.0;
  conf.kfilter_conf.Q_alpha[5] = 1 / 160.0;
  conf.kfilter_conf.Q_alpha[6] = 0.0;
  conf.kfilter_conf.Q_alpha[7] = 1 / 160.0;
  conf.kfilter_conf.Q_x_idx[0] = 3;
  conf.kfilter_conf.Q_x_idx[1] = 3;
  conf.kfilter_conf.Q_x_idx[2] = -1;
  conf.kfilter_conf.Q_x_idx[3] = 3;
  conf.kfilter_conf.Q_x_idx[4] = 3;
  conf.kfilter_conf.Q_x_idx[5] = 3;
  conf.kfilter_conf.Q_x_idx[6] = -1;
  conf.kfilter_conf.Q_x_idx[7] = 3;
  conf.kfilter_conf.Q_beta[0] = 0.0;
  conf.kfilter_conf.Q_beta[1] = 0.0;
  conf.kfilter_conf.Q_beta[2] = 0.1;
  conf.kfilter_conf.Q_beta[3] = 0.0;
  conf.kfilter_conf.Q_beta[4] = 0.0;
  conf.kfilter_conf.Q_beta[5] = 0.0;
  conf.kfilter_conf.Q_beta[6] = 1e-5;
  conf.kfilter_conf.Q_beta[7] = 0.0;

  conf.kfilter_conf.R_alpha[0] = 1 / 20.0;
  conf.kfilter_conf.R_alpha[1] = 1 / 20.0;
  conf.kfilter_conf.R_alpha[2] = 0.0;
  conf.kfilter_conf.R_alpha[3] = 1 / 20.0;
  conf.kfilter_conf.R_x_idx[0] = 3;
  conf.kfilter_conf.R_x_idx[1] = 3;
  conf.kfilter_conf.R_x_idx[2] = -1;
  conf.kfilter_conf.R_x_idx[3] = 3;
  conf.kfilter_conf.R_beta[0] = 0.0;
  conf.kfilter_conf.R_beta[1] = 0.0;
  conf.kfilter_conf.R_beta[2] = 0.1;
  conf.kfilter_conf.R_beta[3] = 0.0;

  return conf;
}
void DeepSORT::set_image_size(uint32_t imgw, uint32_t imgh) {
  image_width_ = imgw;
  image_height_ = imgh;
}
/* DEBUG CODE*/
void DeepSORT::show_INFO_KalmanTrackers() {
  for (size_t i = 0; i < k_trackers.size(); i++) {
    KalmanTracker &tracker_ = k_trackers[i];

    std::cout << "[" << std::setw(3) << i << "] Tracker ID: " << tracker_.id
              << ",class:" << tracker_.class_id << std::endl;
    if (tracker_.pair_track_infos_.size() > 0) {
      std::cout << "pairinfo,trackid:" << tracker_.pair_track_infos_.begin()->first << std::endl;
    }
    std::cout << "\t" << std::setw(20) << "tracker state = " << tracker_.get_INFO_TrackerState()
              << std::endl;
    std::cout << "\t" << std::setw(20) << "kalman state = " << tracker_.get_INFO_KalmanState()
              << std::endl;
    // std::cout << "\t" << std::setw(20) << "bbox = " << tracker_.bbox << std::endl;
    std::cout << "\t" << std::setw(20) << "unmatched times = " << tracker_.unmatched_times
              << std::endl;
    std::cout << "\t" << std::setw(20) << "matched counter = " << tracker_.get_MatchedCounter()
              << std::endl;
    std::cout << "\t" << std::setw(30)
              << "feauture update counter = " << tracker_.get_FeatureUpdateCounter() << std::endl;
    std::cout << "\t" << std::setw(30) << "Kalman x_ = \n" << tracker_.x.transpose() << std::endl;
    std::cout << "\t" << std::setw(30) << "Kalman P_ = \n" << tracker_.P << std::endl;
  }
}

std::vector<KalmanTracker> DeepSORT::get_Trackers_UnmatchedLastTime() const {
  std::vector<KalmanTracker> unmatched_trackers;
  for (const KalmanTracker &tracker_ : k_trackers) {
    if (tracker_.unmatched_times > 0) {
      unmatched_trackers.push_back(tracker_);
    }
  }
  return unmatched_trackers;
}

bool DeepSORT::get_Tracker_ByID(uint64_t id, KalmanTracker &tracker) const {
  for (const KalmanTracker &tracker_ : k_trackers) {
    if (tracker_.id == id) {
      tracker = tracker_;
      return true;
    }
  }
  return false;
}

std::string DeepSORT::get_TrackersInfo_UnmatchedLastTime(std::string &str_info) const {
  std::stringstream ss_info;
  std::vector<KalmanTracker> unmatched_trackers = get_Trackers_UnmatchedLastTime();
  ss_info << unmatched_trackers.size() << std::endl;
  for (size_t i = 0; i < unmatched_trackers.size(); i++) {
    KalmanTracker &tracker_ = unmatched_trackers[i];
    if (tracker_.tracker_state != k_tracker_state_e::ACCREDITATION) {
      assert(0);
    }
    BBOX tracker_bbox = tracker_.getBBox_TLWH();
    ss_info << tracker_.id << ",-1,-1,-1,-1," << tracker_.tracker_state << ",";
    ss_info << static_cast<int>(tracker_bbox(0, 0)) << "," << static_cast<int>(tracker_bbox(0, 1))
            << "," << static_cast<int>(tracker_bbox(0, 0) + tracker_bbox(0, 2)) << ","
            << static_cast<int>(tracker_bbox(0, 1) + tracker_bbox(0, 3)) << "\n";
  }

  str_info = ss_info.str();
  return str_info;
}

CVI_S32 DeepSORT::get_trackers_inactive(cvtdl_tracker_t *tracker) const {
  std::vector<KalmanTracker> unmatched_trackers = get_Trackers_UnmatchedLastTime();
  if (unmatched_trackers.size() == 0) return CVI_TDL_SUCCESS;
  CVI_TDL_MemAlloc(static_cast<uint32_t>(unmatched_trackers.size()), tracker);
  for (uint32_t i = 0; i < static_cast<uint32_t>(unmatched_trackers.size()); i++) {
    KalmanTracker &u_tkr = unmatched_trackers[i];
    if (u_tkr.tracker_state != k_tracker_state_e::ACCREDITATION) {
      LOGE("[BUG] This is an illegal condition.\n");
      return CVI_TDL_FAILURE;
    }
    BBOX tracker_bbox = u_tkr.getBBox_TLWH();
    tracker->info[i].id = u_tkr.id;
    tracker->info[i].state = CVI_TRACKER_STABLE;
    tracker->info[i].bbox.x1 = tracker_bbox(0, 0);
    tracker->info[i].bbox.y1 = tracker_bbox(0, 1);
    tracker->info[i].bbox.x2 = tracker_bbox(0, 0) + tracker_bbox(0, 2);
    tracker->info[i].bbox.y2 = tracker_bbox(0, 1) + tracker_bbox(0, 3);
  }

  return CVI_TDL_SUCCESS;
}

// static void FACE_QUALITY_ASSESSMENT(cvtdl_face_t *face) {
//   for (uint32_t i = 0; i < face->size; i++) {
//     cvtdl_bbox_t &bbox = face->info[i].bbox;
//     float a = (bbox.x2 - bbox.x1) / (bbox.y2 - bbox.y1);
//     face->info[i].face_quality = 1.0 - fabs(a - 1.0);
//   }
// }

static void show_deepsort_config(cvtdl_deepsort_config_t &ds_conf) {
  std::cout << "[DeepSORT] Max Distance Consine : " << ds_conf.max_distance_consine << std::endl
            << "[DeepSORT] Max Distance IoU     : " << ds_conf.max_distance_iou << std::endl
            << "[DeepSORT] Max Unmatched Times for BBox Matching : "
            << ds_conf.max_unmatched_times_for_bbox_matching << std::endl
            << "[Kalman Tracker] Max Unmatched Num       : "
            << ds_conf.ktracker_conf.max_unmatched_num << std::endl
            << "[Kalman Tracker] Accreditation Threshold : "
            << ds_conf.ktracker_conf.accreditation_threshold << std::endl
            << "[Kalman Tracker] Feature Budget Size     : "
            << ds_conf.ktracker_conf.feature_budget_size << std::endl
            << "[Kalman Tracker] Feature Update Interval : "
            << ds_conf.ktracker_conf.feature_update_interval << std::endl
            << "[Kalman Tracker] P-alpha : " << std::endl
            << std::setw(6) << ds_conf.ktracker_conf.P_alpha[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.ktracker_conf.P_alpha[i];
  }
  std::cout << std::endl
            << "[Kalman Tracker] P-beta : " << std::endl
            << std::setw(6) << ds_conf.ktracker_conf.P_beta[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.ktracker_conf.P_beta[i];
  }
  std::cout << std::endl
            << "[Kalman Tracker] P-x_idx : " << std::endl
            << std::setw(6) << ds_conf.ktracker_conf.P_x_idx[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.ktracker_conf.P_x_idx[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] Q-alpha : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.Q_alpha[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.Q_alpha[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] Q-beta : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.Q_beta[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.Q_beta[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] Q-x_idx : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.Q_x_idx[0];
  for (int i = 1; i < 8; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.Q_x_idx[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] R-alpha : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.R_alpha[0];
  for (int i = 1; i < 4; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.R_alpha[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] R-beta : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.R_beta[0];
  for (int i = 1; i < 4; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.R_beta[i];
  }
  std::cout << std::endl
            << "[Kalman Filter] R-x_idx : " << std::endl
            << std::setw(6) << ds_conf.kfilter_conf.R_x_idx[0];
  for (int i = 1; i < 4; i++) {
    std::cout << "," << std::setw(6) << ds_conf.kfilter_conf.R_x_idx[i];
  }
  std::cout << std::endl;
}
