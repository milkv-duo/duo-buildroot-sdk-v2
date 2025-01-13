#include "cvi_kalman_tracker.hpp"
#include "cvi_deepsort_utils.hpp"

#include <math.h>
#include <iostream>
#include "cvi_tdl_log.hpp"
#define USE_COSINE_DISTANCE_FOR_FEATURE true

KalmanTracker::~KalmanTracker() {}

KalmanTracker::KalmanTracker(const uint64_t &id, const int &class_id, const BBOX &bbox,
                             const FEATURE &feature,
                             const cvtdl_kalman_tracker_config_t &ktracker_conf) {
  this->id = id;
  this->class_id = class_id;
  int feature_size = feature.size();
  if (feature_size > 0) {
    if (USE_COSINE_DISTANCE_FOR_FEATURE) {
      FEATURE tmp_feature = feature;
      normalize_feature(tmp_feature);
      this->features.push_back(tmp_feature);
    } else {
      assert(0);
      this->features.push_back(feature);
    }
    this->init_feature = true;
  } else {
    this->init_feature = false;
  }
  this->feature_update_counter = 0;

  this->tracker_state = k_tracker_state_e::PROBATION;
  this->matched_counter = 1;
  this->unmatched_times = 0;
  this->bounding = false;

  this->kalman_state = kalman_state_e::UPDATED;
  BBOX bbox_xyah = bbox_tlwh2xyah(bbox);
  this->x.block(0, 0, DIM_Z, 1) = bbox_xyah.transpose();
  this->x.block(DIM_Z, 0, DIM_Z, 1) = Eigen::MatrixXf::Zero(DIM_Z, 1);
  this->P = Eigen::MatrixXf::Zero(DIM_X, DIM_X);
  for (int i = 0; i < DIM_X; i++) {
    float X_base = (ktracker_conf.P_x_idx[i] == -1) ? 0.0 : x[ktracker_conf.P_x_idx[i]];
    this->P(i, i) = pow(ktracker_conf.P_alpha[i] * X_base + ktracker_conf.P_beta[i], 2);
  }
  false_update_times_ = 0;
  ages_ = 1;
}

void KalmanTracker::update_feature(const FEATURE &feature, int feature_budget_size,
                                   int feature_update_interval) {
  if (!init_feature) {
    FEATURE tmp_feature = feature;
    normalize_feature(tmp_feature);
    features.push_back(tmp_feature);
    init_feature = true;
    feature_update_counter = 0;
    return;
  }
  feature_update_counter += 1;
  if (feature_update_counter >= feature_update_interval) {
    if (USE_COSINE_DISTANCE_FOR_FEATURE) {
      FEATURE tmp_feature = feature;
      normalize_feature(tmp_feature);
      features.push_back(tmp_feature);
    } else {
      assert(0);
      features.push_back(feature);
    }
    feature_update_counter = 0;
    if (features.size() > static_cast<size_t>(feature_budget_size)) {
      features.erase(features.begin());
    }
  }
}

void KalmanTracker::update_state(bool is_matched, int max_unmatched_num, int accreditation_thr) {
  ages_ += 1;
  if (is_matched) {
    if (tracker_state == k_tracker_state_e::PROBATION) {
      matched_counter += 1;
      if (matched_counter >= accreditation_thr) {
        tracker_state = k_tracker_state_e::ACCREDITATION;
      }
    }
    unmatched_times = 0;
    false_update_times_ = 0;
  } else {
    if (tracker_state == k_tracker_state_e::PROBATION) {
      tracker_state = k_tracker_state_e::MISS;
      return;
    }
    unmatched_times += 1;
    kalman_state = kalman_state_e::UPDATED;
    if (unmatched_times > max_unmatched_num) {
      tracker_state = k_tracker_state_e::MISS;
    }
  }
}

COST_MATRIX KalmanTracker::getCostMatrix_Feature(const std::vector<KalmanTracker> &KTrackers,
                                                 const std::vector<BBOX> &BBoxes,
                                                 const std::vector<FEATURE> &Features,
                                                 const std::vector<int> &Tracker_IDXes,
                                                 const std::vector<int> &BBox_IDXes) {
  assert(!Tracker_IDXes.empty() && !BBox_IDXes.empty());
  COST_MATRIX cost_m(Tracker_IDXes.size(), BBox_IDXes.size());
  uint32_t feature_size = Features[0].cols();
  FEATURES features_m_(BBox_IDXes.size(), feature_size);
  for (size_t i = 0; i < BBox_IDXes.size(); i++) {
    int bbox_idx = BBox_IDXes[i];
    FEATURE tmp_feature_ = Features[bbox_idx];
    if (USE_COSINE_DISTANCE_FOR_FEATURE) {
      normalize_feature(tmp_feature_);
    }
    features_m_.row(i) = tmp_feature_;
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    int tracker_idx = Tracker_IDXes[i];
    const std::vector<FEATURE> &t_features = KTrackers[tracker_idx].features;
    assert(t_features.size() > 0);

    FEATURES tracker_features(t_features.size(), feature_size);
    for (size_t t = 0; t < t_features.size(); t++) {
      tracker_features.row(t) = t_features[t];
    }
    COST_MATRIX distance_m = cosine_distance(tracker_features, features_m_);
    ROW_VECTOR distance_v = get_min_colwise(distance_m);

    cost_m.row(i) = distance_v;
  }

  return cost_m;
}

COST_MATRIX KalmanTracker::getCostMatrix_BBox(const std::vector<KalmanTracker> &KTrackers,
                                              const std::vector<BBOX> &BBoxes,
                                              const std::vector<FEATURE> &Features,
                                              const std::vector<int> &Tracker_IDXes,
                                              const std::vector<int> &BBox_IDXes) {
  assert(!Tracker_IDXes.empty() && !BBox_IDXes.empty());
  COST_MATRIX cost_m(Tracker_IDXes.size(), BBox_IDXes.size());

  BBOXES bbox_m_(BBox_IDXes.size(), 4);
  for (size_t i = 0; i < BBox_IDXes.size(); i++) {
    int bbox_idx = BBox_IDXes[i];
    bbox_m_.row(i) = BBoxes[bbox_idx];
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    int tracker_idx = Tracker_IDXes[i];
    BBOX tracker_bbox = KTrackers[tracker_idx].getBBox_TLWH();
    COST_VECTOR distance_v = iou_distance(tracker_bbox, bbox_m_);
    cost_m.row(i) = distance_v;
  }

  return cost_m;
}

COST_MATRIX KalmanTracker::getCostMatrix_Mahalanobis(
    const KalmanFilter &KF_, const std::vector<KalmanTracker> &K_Trackers,
    const std::vector<BBOX> &BBoxes, const std::vector<int> &Tracker_IDXes,
    const std::vector<int> &BBox_IDXes, const cvtdl_kalman_filter_config_t &kfilter_conf,
    float upper_bound) {
#if 0
  float chi2_threshold = kfilter_conf.chi2_threshold;
#endif
  COST_MATRIX cost_m(Tracker_IDXes.size(), BBox_IDXes.size());

  BBOXES measurement_bboxes(BBox_IDXes.size(), 4);
  for (size_t i = 0; i < BBox_IDXes.size(); i++) {
    int bbox_idx = BBox_IDXes[i];
    measurement_bboxes.row(i) = bbox_tlwh2xyah(BBoxes[bbox_idx]);
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    int tracker_idx = Tracker_IDXes[i];
    const KalmanTracker &tracker_ = K_Trackers[tracker_idx];
    ROW_VECTOR maha2_d = KF_.mahalanobis(tracker_.kalman_state, tracker_.x, tracker_.P,
                                         measurement_bboxes, kfilter_conf);
    for (int j = 0; j < maha2_d.cols(); j++) {
      cost_m(i, j) = (maha2_d(0, j) > upper_bound) ? upper_bound : maha2_d(0, j);
    }
  }

  return cost_m;
}

void KalmanTracker::restrictCostMatrix_Mahalanobis(
    COST_MATRIX &cost_matrix, const KalmanFilter &KF_, const std::vector<KalmanTracker> &K_Trackers,
    const std::vector<BBOX> &BBoxes, const std::vector<int> &Tracker_IDXes,
    const std::vector<int> &BBox_IDXes, const cvtdl_kalman_filter_config_t &kfilter_conf,
    float upper_bound) {
  // float chi2_threshold = chi2inv95[4];
  BBOXES measurement_bboxes(BBox_IDXes.size(), 4);
  for (size_t i = 0; i < BBox_IDXes.size(); i++) {
    int bbox_idx = BBox_IDXes[i];
    measurement_bboxes.row(i) = bbox_tlwh2xyah(BBoxes[bbox_idx]);
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    int tracker_idx = Tracker_IDXes[i];
    const KalmanTracker &tracker_ = K_Trackers[tracker_idx];
    ROW_VECTOR maha2_d = KF_.mahalanobis(tracker_.kalman_state, tracker_.x, tracker_.P,
                                         measurement_bboxes, kfilter_conf);
    // std::cout << "[" << i << "] mahalanobis:" << std::endl << maha2_d << std::endl;
    for (int j = 0; j < maha2_d.cols(); j++) {
      if (maha2_d(0, j) > kfilter_conf.chi2_threshold) {
        cost_matrix(i, j) = upper_bound;
      }
    }
  }
}

void KalmanTracker::restrictCostMatrix_BBox(COST_MATRIX &cost_matrix,
                                            const std::vector<KalmanTracker> &KTrackers,
                                            const std::vector<BBOX> &BBoxes,
                                            const std::vector<int> &Tracker_IDXes,
                                            const std::vector<int> &BBox_IDXes, float upper_bound) {
  assert(!Tracker_IDXes.empty() && !BBox_IDXes.empty());
  COST_MATRIX cost_m(Tracker_IDXes.size(), BBox_IDXes.size());

  BBOXES bbox_m_(BBox_IDXes.size(), 4);
  for (size_t i = 0; i < BBox_IDXes.size(); i++) {
    int bbox_idx = BBox_IDXes[i];
    bbox_m_.row(i) = BBoxes[bbox_idx];
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    int tracker_idx = Tracker_IDXes[i];
    BBOX tracker_bbox = KTrackers[tracker_idx].getBBox_TLWH();
    COST_VECTOR distance_v = iou_distance(tracker_bbox, bbox_m_);
    cost_m.row(i) = distance_v;
  }
  for (size_t i = 0; i < Tracker_IDXes.size(); i++) {
    for (size_t j = 0; j < BBox_IDXes.size(); j++) {
      if (cost_m(i, j) > 0.9) {
        // std::cout << "restrict iou,trackbox:" << KTrackers[Tracker_IDXes[i]].getBBox_TLWH()
        //           << ",detbox:" << bbox_m_.row(j) << ",ioudist:" << cost_m(i, j) << std::endl;
        cost_matrix(i, j) = upper_bound;
      }
    }
  }
}

BBOX KalmanTracker::getBBox_TLWH() const {
  BBOX bbox_tlwh;
  bbox_tlwh(2) = x(2) * x(3);  // H
  bbox_tlwh(3) = x(3);         // W
  bbox_tlwh(0) = x(0) - 0.5 * bbox_tlwh(2);
  bbox_tlwh(1) = x(1) - 0.5 * bbox_tlwh(3);
  return bbox_tlwh;
}

/* DEBUG CODE */
int KalmanTracker::get_FeatureUpdateCounter() const { return feature_update_counter; }

int KalmanTracker::get_MatchedCounter() const { return matched_counter; }

std::string KalmanTracker::get_INFO_KalmanState() const {
  switch (kalman_state) {
    case kalman_state_e::UPDATED:
      return std::string("UPDATED");
    case kalman_state_e::PREDICTED:
      return std::string("PREDICTED");
    default:
      assert(0);
  }
  return "ERROR";
}

std::string KalmanTracker::get_INFO_TrackerState() const {
  switch (tracker_state) {
    case k_tracker_state_e::PROBATION:
      return std::string("PROBATION");
    case k_tracker_state_e::ACCREDITATION:
      return std::string("ACCREDITATION");
    case k_tracker_state_e::MISS:
      return std::string("MISS");
    default:
      assert(0);
  }
  return "ERROR";
}

uint64_t KalmanTracker::get_pair_trackid() {
  if (pair_track_infos_.size() == 0) {
    return 0;
  }
  return pair_track_infos_.begin()->first;
}
void KalmanTracker::predict(KalmanFilter &kf, cvtdl_deepsort_config_t *conf) {
  kf.predict(kalman_state, x, P, conf->kfilter_conf);
  unmatched_times += 1;
  ages_ += 1;
}
//
void KalmanTracker::update(KalmanFilter &kf, const stRect *p_tlwh_bbox,
                           cvtdl_deepsort_config_t *conf) {
  if (p_tlwh_bbox != nullptr) {
    unmatched_times = 0;
    matched_counter += 1;
    false_update_times_ = 0;
    BBOX twh_box;
    twh_box(0) = p_tlwh_bbox->x;
    twh_box(1) = p_tlwh_bbox->y;
    twh_box(2) = p_tlwh_bbox->width;
    twh_box(3) = p_tlwh_bbox->height;
    BBOX xyah = bbox_tlwh2xyah(twh_box);
    kf.update(kalman_state, x, P, xyah, conf->kfilter_conf);
    if (tracker_state == k_tracker_state_e::PROBATION &&
        matched_counter >= conf->ktracker_conf.accreditation_threshold) {
      tracker_state = k_tracker_state_e::ACCREDITATION;
    }

  } else {
    kalman_state = kalman_state_e::UPDATED;
#ifdef DEBUG_TRACK
    printf("missed track id:%d, tracker_state: %d, unmatched_times:%d, max_unmatched_num:%d\n", id,
           tracker_state, unmatched_times, conf->ktracker_conf.max_unmatched_num);
#endif
    if (tracker_state == k_tracker_state_e::PROBATION) {
      tracker_state = k_tracker_state_e::MISS;
    } else if (unmatched_times >= conf->ktracker_conf.max_unmatched_num) {
      tracker_state = k_tracker_state_e::MISS;
    }
  }
}
void KalmanTracker::false_update_from_pair(KalmanFilter &kf, KalmanTracker *p_other,
                                           cvtdl_deepsort_config_t *conf) {
#ifdef DEBUG_TRACK
  std::cout << "false update pairtrack:" << id << ",with:" << p_other->id << std::endl;
#endif

  if (p_other->pair_track_infos_.count(id) == 0) {
    LOGE("false update current trackid:%d not found in pair track:%d\n", (int)id, (int)p_other->id);
    return;
  }
  BBOX false_box;
  stCorrelateInfo corre = p_other->pair_track_infos_[id];
  // LOG(INFO)<<"falseframe:"<<cur_frame_<<",curtrack:"<<track_id_<<",pairedtrack:"<<p_pair_track->track_id_<<",offscalex:"<<
  // corre.offset_scale_x<<
  // ",offscaley:"<<
  // corre.offset_scale_y<<",sizescalex:"<<corre.pair_size_scale_x<<",sizescaley:"<<corre.pair_size_scale_y;

  BBOX pairbox = p_other->getBBox_TLWH();
  false_box(0) = p_other->x(0) + pairbox(2) * corre.offset_scale_x;
  false_box(1) = p_other->x(1) + pairbox(3) * corre.offset_scale_y;
  false_box(2) = x(2);
  false_box(3) = pairbox(3) * corre.pair_size_scale_y;

  // to keep this track alive
  if (unmatched_times >= conf->ktracker_conf.max_unmatched_num - 10) {
    unmatched_times = conf->ktracker_conf.max_unmatched_num - 10;
  }
  false_update_times_ += 1;
  kalman_state = kalman_state_e::UPDATED;
  // only linear update xyah
  // this->x.block(0, 0, DIM_Z, 1) = this->x.block(0, 0, DIM_Z, 1) * 0.6 + false_box.transpose() *
  // 0.4;
  kf.update(kalman_state, x, P, false_box, conf->kfilter_conf);
}
void KalmanTracker::update_pair_info(KalmanTracker *p_other) {
#ifdef DEBUG_TRACK
  std::cout << "update pairtrack:" << id << ",with:" << p_other->id << std::endl;
#endif

  BBOX cur_box = getBBox_TLWH();
  BBOX pair_box = p_other->getBBox_TLWH();

  if (p_other->pair_track_infos_.count(id) == 0) {
    stCorrelateInfo corre;
    memset((void *)&corre, 0, sizeof(corre));
    p_other->pair_track_infos_[id] = corre;
  }
  update_corre(pair_box, cur_box, p_other->pair_track_infos_[id], 0.5);

  if (pair_track_infos_.count(p_other->id) == 0) {
    stCorrelateInfo corre;
    memset((void *)&corre, 0, sizeof(corre));
    pair_track_infos_[p_other->id] = corre;
  }
  update_corre(cur_box, pair_box, pair_track_infos_[p_other->id], 0.5);
}
