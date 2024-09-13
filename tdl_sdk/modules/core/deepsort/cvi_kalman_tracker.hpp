#pragma once

#include <vector>
#include "cvi_deepsort_types_internal.hpp"
#include "cvi_distance_metric.hpp"
#include "cvi_kalman_filter.hpp"
#include "cvi_kalman_types.hpp"
#include "cvi_tracker.hpp"

typedef enum { MISS = 0, PROBATION, ACCREDITATION } k_tracker_state_e;

// clang-format off
/** NOTE: reference https://people.richland.edu/james/lecture/m170/tbl-chi.html */
const float chi2_005[5] = {0,   7.879,  10.597,  12.838,  14.860};
const float chi2_010[5] = {0,   6.635,   9.210,  11.345,  13.277};
const float chi2_025[5] = {0,   5.024,   7.378,   9.348,  11.143};
const float chi2_050[5] = {0,   3.841,   5.991,   7.815,   9.488};
const float chi2_100[5] = {0,   2.706,   4.605,   6.251,   7.779};
// clang-format on

class KalmanTracker : public Tracker {
 public:
  std::vector<FEATURE> features;
  kalman_state_e kalman_state;
  k_tracker_state_e tracker_state;
  bool bounding;
  bool init_feature;
  K_STATE_V x;
  K_COVARIANCE_M P;
  int unmatched_times;
  int false_update_times_;
  uint64_t ages_ = 0;
  int matched_counter;
  uint32_t out_nums = 0;
  int label;
  // consumer counting flag
  float first_x = 0, first_y = 0;  // The first direction vector
  float old_x = 0, old_y = 0;      // The pre coordinate
  bool is_cross = false;
  int cross_gap = 0;
  int counting_gap = 0;
  // int miss_gap = 0;
  bool is_entry = false;
  bool is_leave = false;
  std::map<uint64_t, stCorrelateInfo> pair_track_infos_;

  KalmanTracker() = delete;
  KalmanTracker(const uint64_t &id, const int &class_id, const BBOX &bbox, const FEATURE &feature,
                const cvtdl_kalman_tracker_config_t &ktracker_conf);
  ~KalmanTracker();

  void update_state(bool is_matched, int max_unmatched_num = 40, int accreditation_thr = 3);
  void update_feature(const FEATURE &feature, int feature_budget_size = 8,
                      int feature_update_interval = 1);

  uint64_t get_pair_trackid();
  void false_update_from_pair(KalmanFilter &kf, KalmanTracker *p_other,
                              cvtdl_deepsort_config_t *conf);
  void update_pair_info(KalmanTracker *p_other);
  void predict(KalmanFilter &kf, cvtdl_deepsort_config_t *conf);
  void update(KalmanFilter &kf, const stRect *p_bbox, cvtdl_deepsort_config_t *conf);
  BBOX getBBox_TLWH() const;

  static COST_MATRIX getCostMatrix_Feature(const std::vector<KalmanTracker> &KTrackers,
                                           const std::vector<BBOX> &BBoxes,
                                           const std::vector<FEATURE> &Features,
                                           const std::vector<int> &Tracker_IDXes,
                                           const std::vector<int> &BBox_IDXes);

  static COST_MATRIX getCostMatrix_BBox(const std::vector<KalmanTracker> &KTrackers,
                                        const std::vector<BBOX> &BBoxes,
                                        const std::vector<FEATURE> &Features,
                                        const std::vector<int> &Tracker_IDXes,
                                        const std::vector<int> &BBox_IDXes);

  static COST_MATRIX getCostMatrix_Mahalanobis(const KalmanFilter &KF_,
                                               const std::vector<KalmanTracker> &K_Trackers,
                                               const std::vector<BBOX> &BBoxes,
                                               const std::vector<int> &Tracker_IDXes,
                                               const std::vector<int> &BBox_IDXes,
                                               const cvtdl_kalman_filter_config_t &kfilter_conf,
                                               float upper_bound);

  static void restrictCostMatrix_Mahalanobis(COST_MATRIX &cost_matrix, const KalmanFilter &KF_,
                                             const std::vector<KalmanTracker> &K_Trackers,
                                             const std::vector<BBOX> &BBoxes,
                                             const std::vector<int> &Tracker_IDXes,
                                             const std::vector<int> &BBox_IDXes,
                                             const cvtdl_kalman_filter_config_t &kfilter_conf,
                                             float upper_bound);

  static void restrictCostMatrix_BBox(COST_MATRIX &cost_matrix,
                                      const std::vector<KalmanTracker> &KTrackers,
                                      const std::vector<BBOX> &BBoxes,
                                      const std::vector<int> &Tracker_IDXes,
                                      const std::vector<int> &BBox_IDXes, float upper_bound);
  /* DEBUG CODE */
  int get_FeatureUpdateCounter() const;
  int get_MatchedCounter() const;
  std::string get_INFO_KalmanState() const;
  std::string get_INFO_TrackerState() const;

 private:
  int feature_update_counter;
};
