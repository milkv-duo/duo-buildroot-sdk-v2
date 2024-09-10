#pragma once

#include "cvi_deepsort_types_internal.hpp"
#include "cvi_distance_metric.hpp"
#include "cvi_kalman_filter.hpp"
#include "cvi_kalman_tracker.hpp"
#include "cvi_munkres.hpp"

#include "core/cvi_tdl_core.h"

struct MatchResult {
  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> unmatched_bbox_idxes;
  std::vector<int> unmatched_tracker_idxes;
};

/* Result Format: [i] <is_matched, tracker_id, tracker_state, tracker_bbox> */
typedef std::vector<std::tuple<bool, uint64_t, k_tracker_state_e, BBOX>> Tracking_Result;
typedef std::vector<std::tuple<bool, uint64_t, k_tracker_state_e, BBOX, bool>> Tracking_Result2;

class DeepSORT {
 public:
  DeepSORT() = delete;
  DeepSORT(bool use_specific_counter);
  ~DeepSORT();

  static cvtdl_deepsort_config_t get_DefaultConfig();

  void set_image_size(uint32_t imgw, uint32_t imgh);
  void check_bound_state(cvtdl_deepsort_config_t *conf);
  CVI_S32 track_impl(Tracking_Result &result, const std::vector<BBOX> &BBoxes,
                     const std::vector<FEATURE> &Features, float crowd_iou_thresh,
                     int class_id = -1, bool use_reid = true, float *Quality = NULL);
  CVI_S32 track_impl_cross(Tracking_Result2 &result, const std::vector<BBOX> &BBoxes,
                           const std::vector<FEATURE> &Features, float crowd_iou_thresh,
                           int class_id, bool use_reid, float *Quality,
                           const cvtdl_counting_line_t *cross_line_t, const randomRect *rect);
  CVI_S32 track(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid = true);
  CVI_S32 track_cross(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid,
                      const cvtdl_counting_line_t *cross_line_t, const randomRect *rect);
  CVI_S32 track(cvtdl_face_t *face, cvtdl_tracker_t *tracker);
  // byte track

  CVI_S32 byte_track(cvtdl_object_t *obj, cvtdl_tracker_t *tracker, bool use_reid,
                     float low_score = 0.3, float high_score = 0.5);
  CVI_S32 track_impl(Tracking_Result &high_result, Tracking_Result &low_result,
                     const std::vector<BBOX> &HighBBoxes, const std::vector<BBOX> &LowBBoxes,
                     const std::vector<FEATURE> &HighFeatures,
                     const std::vector<FEATURE> &LowFeatures, float crowd_iou_thresh,
                     int class_id = -1, bool use_reid = true, float *Quality = NULL);

  void update_tracks(cvtdl_deepsort_config_t *conf,
                     std::map<int, std::vector<stObjInfo>> &cls_objs);
  void consumer_counting_fun(stObjInfo obj, int index, const cvtdl_counting_line_t *counting_line_t,
                             const randomRect *rect);
  void get_pair_trackids(std::map<int, std::vector<stObjInfo>> &cls_objs,
                         std::map<uint64_t, uint64_t> &pair_trackids);
  void update_pair_info(std::vector<stObjInfo> &dets_a, std::vector<stObjInfo> &dets_b,
                        ObjectType typea, ObjectType typeb, float corre_thresh);
  CVI_S32 face_head_iou_score(cvtdl_face_t *faces, cvtdl_face_t *heads);

  CVI_S32 track_fuse(cvtdl_object_t *obj, cvtdl_face_t *face, cvtdl_tracker_t *tracker);
  CVI_S32 track_headfuse(cvtdl_object_t *origin_obj, cvtdl_tracker_t *tracker, bool use_reid,
                         cvtdl_object_t *head, cvtdl_object_t *ped,
                         const cvtdl_counting_line_t *counting_line_t, const randomRect *rect);
  void update_out_num(cvtdl_tracker_t *tracker);

  CVI_S32 getConfig(cvtdl_deepsort_config_t *ds_conf, int cvitdl_obj_type = -1);
  CVI_S32 setConfig(cvtdl_deepsort_config_t *ds_conf, int cvitdl_obj_type = -1,
                    bool show_config = false);
  void cleanCounter();

  CVI_S32 get_trackers_inactive(cvtdl_tracker_t *tracker) const;
  void set_timestamp(uint32_t ts) { current_timestamp_ = ts; }

  /* DEBUG CODE */
  // TODO: refactor these functions.
  void show_INFO_KalmanTrackers();
  std::vector<KalmanTracker> get_Trackers_UnmatchedLastTime() const;
  bool get_Tracker_ByID(uint64_t id, KalmanTracker &tracker) const;
  std::string get_TrackersInfo_UnmatchedLastTime(std::string &str_info) const;

 private:
  bool sp_counter;
  uint64_t id_counter;
  uint64_t frame_id_ = 0;
  std::map<int, uint64_t> specific_id_counter;
  std::vector<KalmanTracker> k_trackers;
  KalmanFilter kf_;
  uint32_t image_width_;
  uint32_t image_height_;
  // consumer counting
  uint32_t entry_num = 0;
  uint32_t miss_num = 0;
  float bounding_iou_thresh_ = 0.5;
  uint32_t current_timestamp_ = 0;
  uint32_t last_timestamp_ = 0;
  // byte_kalman::KalmanFilter byte_kf_;
  std::vector<int> accreditation_tracker_idxes;  // confirmed trackids
  std::vector<int> probation_tracker_idxes;      // temp trackids

  /* DeepSORT config */
  cvtdl_deepsort_config_t default_conf;
  std::map<int, cvtdl_deepsort_config_t> specific_conf;
  std::map<uint64_t, int> track_indices_;
  std::map<uint64_t, std::vector<float>> old_coordinate;

  uint64_t get_nextID(int class_id);
  MatchResult get_match_result(MatchResult &prev_match, const std::vector<BBOX> &BBoxes,
                               const std::vector<FEATURE> &Features, bool use_reid,
                               float crowd_iou_thresh, cvtdl_deepsort_config_t *conf);
  MatchResult get_match_result_consumer_counting(MatchResult &prev_match,
                                                 const std::vector<BBOX> &BBoxes,
                                                 const std::vector<FEATURE> &Features,
                                                 bool use_reid, float crowd_iou_thresh,
                                                 cvtdl_deepsort_config_t *conf, bool is_ped,
                                                 std::vector<stObjInfo> &objs);
  MatchResult match(const std::vector<BBOX> &BBoxes, const std::vector<FEATURE> &Features,
                    const std::vector<int> &Tracker_IDXes, const std::vector<int> &BBox_IDXes,
                    cvtdl_kalman_filter_config_t &kf_conf,
                    cost_matrix_algo_e cost_method = Feature_CosineDistance,
                    float max_distance = __FLT_MAX__);
  MatchResult refine_uncrowd(const std::vector<BBOX> &BBoxes, const std::vector<FEATURE> &Features,
                             const std::vector<int> &Tracker_IDXes,
                             const std::vector<int> &BBox_IDXes, float iou_thresh);
  void compute_distance();
  void solve_assignment();
  bool track_face_ = false;
};
