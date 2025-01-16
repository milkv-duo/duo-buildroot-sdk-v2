
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "cvi_deepsort.hpp"
#include "cvi_deepsort_utils.hpp"
#include "cvi_tdl_log.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <map>
#include <set>

stRect extract_box(const stObjInfo &obj) {
  stRect box;
  box.x = obj.box.x1;
  box.y = obj.box.y1;
  box.width = obj.box.x2 - box.x;
  box.height = obj.box.y2 - box.y;
  return box;
}

stRect extract_box(const cvtdl_bbox_t &bbox) {
  stRect box;
  box.x = bbox.x1;
  box.y = bbox.y1;
  box.width = bbox.x2 - box.x;
  box.height = bbox.y2 - box.y;
  return box;
}

BBOX cvt_box(const stObjInfo &obj) {
  BBOX box;
  box(0) = obj.box.x1;
  box(1) = obj.box.y1;
  box(2) = obj.box.x2;
  box(3) = obj.box.y2;
  return box;
}
BBOX cvt_tlwh_box(const stObjInfo &obj) {
  BBOX box;
  box(0) = obj.box.x1;
  box(1) = obj.box.y1;
  box(2) = obj.box.x2 - obj.box.x1;
  box(3) = obj.box.y2 - obj.box.y1;
  return box;
}
std::vector<BBOX> cvt_boxes(std::vector<stObjInfo> &objs) {
  std::vector<BBOX> BBoxes;
  for (size_t i = 0; i < objs.size(); i++) {
    BBOX box = cvt_tlwh_box(objs[i]);
    BBoxes.push_back(box);
  }
  return BBoxes;
}

bool isPointInRect(const randomRect *rect, float p_x, float p_y) {
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

bool isCloser(const randomRect *rect, float p_x, float p_y) {
  float k = rect->k;
  float b = rect->b;
  float dis;
  if (k == 0) {
    dis = std::abs(p_y - b);
  } else if (b == -1000) {
    dis = std::abs(p_x - k);
  } else {
    dis = std::abs(k * p_x - p_y + b) / sqrt(1 + k * k);
  }
  // printf("k:%f,b:%f,dis:%f");
  return dis < 30;
}

float crossProduct(float A_x, float A_y, float B_x, float B_y, float C_x, float C_y) {
  return (B_x - A_x) * (C_y - A_y) - (B_y - A_y) * (C_x - A_x);
}
bool isLineIntersect(const randomRect *rect, float old_x, float old_y, float cur_x, float cur_y) {
  double cp1 = crossProduct(rect->a_x, rect->a_y, rect->b_x, rect->b_y, old_x, old_y);
  double cp2 = crossProduct(rect->a_x, rect->a_y, rect->b_x, rect->b_y, cur_x, cur_y);

  double cp3 = crossProduct(old_x, old_y, cur_x, cur_y, rect->a_x, rect->a_y);
  double cp4 = crossProduct(old_x, old_y, cur_x, cur_y, rect->b_x, rect->b_y);
  // positive and negative symbols
  if ((cp1 * cp2 <= 0) && (cp3 * cp4 <= 0)) return true;
  return false;
}
MatchResult get_init_match_result(const std::vector<stObjInfo> &dets,
                                  const std::vector<KalmanTracker> &trackers,
                                  std::map<uint64_t, int> &tid_index_map, int label) {
  MatchResult res;
  std::vector<int> track_flags;

  int num_type = 0;
  for (size_t i = 0; i < trackers.size(); i++) {
    auto &t = trackers[i];
    if (t.class_id == label || label == -1) {
      num_type += 1;
    }
    track_flags.push_back(0);
  }

  for (size_t i = 0; i < dets.size(); i++) {
    uint64_t tid = dets[i].track_id;

    if (tid != 0) {
      if (tid_index_map.count(tid) == 0) {
        printf("error,trackid:%d not found in tid_index_map,type:%d\n", (int)tid, dets[i].classes);
        continue;
      }
      int index = tid_index_map[tid];
      track_flags[index] = 1;
      continue;
    }

    res.unmatched_bbox_idxes.push_back(int(i));
  }

  for (size_t i = 0; i < trackers.size(); i++) {
    auto &t = trackers[i];
    if ((t.class_id == label || label == -1) && t.tracker_state != k_tracker_state_e::MISS &&
        track_flags[i] == 0) {
      res.unmatched_tracker_idxes.push_back(i);
    }
  }
#ifdef DEBUG_TRACK
  std::cout << "detsize:" << dets.size() << ",trackersize:" << num_type
            << ",unmatchedtracknum:" << res.unmatched_tracker_idxes.size() << std::endl;
#endif
  return res;
}

void DeepSORT::consumer_counting_fun(stObjInfo obj, int index,
                                     const cvtdl_counting_line_t *counting_line_t,
                                     const randomRect *rect) {
  float pre_x = k_trackers[index].old_x;
  float pre_y = k_trackers[index].old_y;
  float cur_x = (obj.box.x1 + obj.box.x2) / 2.0;
  float cur_y = (obj.box.y1 + obj.box.y2) / 2.0;
  if (k_trackers[index].label == OBJ_PERSON) {
    cur_y = obj.box.y1 * 1.2;
  }
  if (isLineIntersect(rect, pre_x, pre_y, cur_x, cur_y)) {
    float tmp_x = cur_x - k_trackers[index].old_x;
    float tmp_y = cur_y - k_trackers[index].old_y;
    if ((tmp_x * rect->f_x + tmp_y * rect->f_y > 0) && k_trackers[index].counting_gap == 0 &&
        !k_trackers[index].is_entry) {
      entry_num++;
      k_trackers[index].counting_gap = 50;
      k_trackers[index].is_entry = true;
    } else if ((tmp_x * rect->f_x + tmp_y * rect->f_y < 0) && k_trackers[index].counting_gap == 0 &&
               !k_trackers[index].is_leave) {
      miss_num++;
      k_trackers[index].counting_gap = 50;
      k_trackers[index].is_leave = true;
    }
  }
}

void DeepSORT::update_pair_info(std::vector<stObjInfo> &dets_a, std::vector<stObjInfo> &dets_b,
                                ObjectType typea, ObjectType typeb, float corre_thresh) {
  // generate pair score matrix
  if (dets_a.size() == 0 || dets_b.size() == 0) {
    return;
  }
  COST_MATRIX pair_cost(dets_a.size(), dets_b.size());

  float cost_thresh = 1 - corre_thresh;
  for (size_t i = 0; i < dets_a.size(); i++) {
    stRect boxa = extract_box(dets_a[i]);

    for (size_t j = 0; j < dets_b.size(); j++) {
      stRect boxb = extract_box(dets_b[j]);
      pair_cost(i, j) = 1 - cal_object_pair_score(boxa, boxb, typea, typeb);
    }
  }
#ifdef DEBUG_TRACK
  std::cout << "pair cost matrix:\n"
            << pair_cost << ",typea:" << typea << ",typeb:" << typeb << std::endl;
#endif
  // use hungeria solver
  CVIMunkres cvi_munkres_solver(&pair_cost);
  if (cvi_munkres_solver.solve() == MUNKRES_FAILURE) {
    return;
  }

  int num_a = (int)dets_a.size();
  for (int i = 0; i < num_a; i++) {
    int bbox_j = cvi_munkres_solver.m_match_result[i];
    if (bbox_j != -1 && pair_cost(i, bbox_j) < cost_thresh) {
#ifdef DEBUG_TRACK
      std::cout << "found pair boxa:" << i << ",boxj:" << bbox_j << ",cost:" << pair_cost(i, bbox_j)
                << std::endl;
#endif
      dets_a[i].pair_obj_id = bbox_j;
      dets_a[i].pair_type = typeb;
      dets_b[bbox_j].pair_obj_id = i;
      dets_b[bbox_j].pair_type = typea;
    }
  }
}

CVI_S32 DeepSORT::face_head_iou_score(cvtdl_face_t *faces, cvtdl_face_t *heads) {
  // generate pair score matrix
  if (faces->size == 0 || heads->size == 0) {
    return CVI_TDL_SUCCESS;
  }
  COST_MATRIX pair_cost(faces->size, heads->size);

  for (size_t i = 0; i < faces->size; i++) {
    stRect boxa = extract_box(faces->info[i].bbox);

    for (size_t j = 0; j < heads->size; j++) {
      stRect boxb = extract_box(heads->info[j].bbox);
      float cur_iou = cal_iou(boxa, boxb);
      pair_cost(i, j) = 1 - cur_iou;
    }
  }
#ifdef DEBUG_TRACK
  std::cout << "pair cost matrix:\n" << pair_cost << std::endl;
#endif
  // use hungeria solver
  CVIMunkres cvi_munkres_solver(&pair_cost);
  if (cvi_munkres_solver.solve() == MUNKRES_FAILURE) {
    return CVI_TDL_FAILURE;
  }

  for (int i = 0; i < (int)faces->size; i++) {
    int bbox_j = cvi_munkres_solver.m_match_result[i];
    if (bbox_j != -1) {
#ifdef DEBUG_TRACK
      std::cout << "found pair boxa:" << i << ",boxj:" << bbox_j << ",cost:" << pair_cost(i, bbox_j)
                << std::endl;
#endif
      faces->info[i].pose_score = 1 - pair_cost(i, bbox_j);
      // printf("faces->info[i].pose_score: %f\n", faces->info[i].pose_score);
    }
  }

  return CVI_TDL_SUCCESS;
}

MatchResult DeepSORT::get_match_result(MatchResult &prev_match, const std::vector<BBOX> &BBoxes,
                                       const std::vector<FEATURE> &Features, bool use_reid,
                                       float crowd_iou_thresh, cvtdl_deepsort_config_t *conf) {
  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> unmatched_bbox_idxes = prev_match.unmatched_bbox_idxes;

  std::vector<int> unmatched_tracker_idxes = prev_match.unmatched_tracker_idxes;

  /* Match accreditation trackers */
  /* - Cascade Match */
  /* - Feature Consine Distance */
  /* - Kalman Mahalanobis Distance */
  for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
    if (unmatched_bbox_idxes.empty()) {
      break;
    }
    // std::cout<<"cascade matching:"<<t<<std::endl;
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
    float chithresh = conf->kfilter_conf.chi2_threshold - t * 0.1;
    MatchResult match_result =
        match(BBoxes, Features, t_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf,
              cost_method, (use_reid) ? conf->max_distance_consine : chithresh);
    if (match_result.matched_pairs.empty()) {
      continue;
    }
#ifdef DEBUG_TRACK
    std::cout << "matched with maha,num:" << match_result.matched_pairs.size() << std::endl;
#endif
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

  // std::cout<<"prepare for ioumatch,"<<unmatched_tracker_idxes.size()<<std::endl;
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
#ifdef DEBUG_TRACK
  std::cout << "to ioumatch ,boxsize:" << BBoxes.size() << ",featsize:" << Features.size()
            << ",unmatchedt:" << unmatched_tracker_idxes.size()
            << ",unmatchedbox:" << unmatched_bbox_idxes.size() << std::endl;
#endif
  /* Match remain trackers */
  /* - BBOX IoU Distance */
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
#ifdef DEBUG_TRACK
  for (auto ti : unmatched_tracker_idxes) {
    std::cout << "unmatched ti:" << ti << ",trackid:" << k_trackers[ti].id << std::endl;
  }
#endif

  MatchResult match_recall = refine_uncrowd(BBoxes, Features, unmatched_tracker_idxes,
                                            unmatched_bbox_idxes, crowd_iou_thresh);

  match_recall.matched_pairs.insert(match_recall.matched_pairs.end(), matched_pairs.begin(),
                                    matched_pairs.end());
  match_recall.matched_pairs.insert(match_recall.matched_pairs.end(),
                                    prev_match.matched_pairs.begin(),
                                    prev_match.matched_pairs.end());

  return match_recall;
}

MatchResult DeepSORT::get_match_result_consumer_counting(MatchResult &prev_match,
                                                         const std::vector<BBOX> &BBoxes,
                                                         const std::vector<FEATURE> &Features,
                                                         bool use_reid, float crowd_iou_thresh,
                                                         cvtdl_deepsort_config_t *conf, bool is_ped,
                                                         std::vector<stObjInfo> &objs) {
  std::vector<std::pair<int, int>> matched_pairs;
  std::vector<int> unmatched_bbox_idxes = prev_match.unmatched_bbox_idxes;

  std::vector<int> unmatched_tracker_idxes = prev_match.unmatched_tracker_idxes;

  /* Match accreditation trackers */
  /* - Cascade Match */
  /* - Feature Consine Distance */
  /* - Kalman Mahalanobis Distance */
  for (int t = 0; t < conf->ktracker_conf.max_unmatched_num; t++) {
    if (unmatched_bbox_idxes.empty()) {
      break;
    }
    // std::cout<<"cascade matching:"<<t<<std::endl;
    cost_matrix_algo_e cost_method =
        (use_reid) ? Feature_CosineDistance : Kalman_MahalanobisDistance;
    if (is_ped) cost_method = BBox_IoUDistance;
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
    float chithresh = conf->kfilter_conf.chi2_threshold - t * 0.1;
    MatchResult match_result =
        match(BBoxes, Features, t_tracker_idxes, unmatched_bbox_idxes, conf->kfilter_conf,
              cost_method, (use_reid) ? conf->max_distance_consine : chithresh);
    if (match_result.matched_pairs.empty()) {
      continue;
    }
#ifdef DEBUG_TRACK
    std::cout << "matched with maha,num:" << match_result.matched_pairs.size() << std::endl;
#endif
    /* Remove matched idx from bbox_idxes and unmatched_tracker_idxes */
    matched_pairs.insert(matched_pairs.end(), match_result.matched_pairs.begin(),
                         match_result.matched_pairs.end());

    for (size_t m_i = 0; m_i < match_result.matched_pairs.size(); m_i++) {
      int m_tracker_idx = match_result.matched_pairs[m_i].first;
      int m_bbox_idx = match_result.matched_pairs[m_i].second;
      if (is_ped) {
        uint64_t pair_head_id = k_trackers[m_tracker_idx].get_pair_trackid();
        uint64_t id = objs[objs[m_bbox_idx].pair_obj_id].track_id;
        if (pair_head_id != 0 && pair_head_id != id) {
          continue;
        }
      }

      unmatched_tracker_idxes.erase(std::remove(unmatched_tracker_idxes.begin(),
                                                unmatched_tracker_idxes.end(), m_tracker_idx),
                                    unmatched_tracker_idxes.end());
      unmatched_bbox_idxes.erase(
          std::remove(unmatched_bbox_idxes.begin(), unmatched_bbox_idxes.end(), m_bbox_idx),
          unmatched_bbox_idxes.end());
    }
  }

  // std::cout<<"prepare for ioumatch,"<<unmatched_tracker_idxes.size()<<std::endl;
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
#ifdef DEBUG_TRACK
  std::cout << "to ioumatch ,boxsize:" << BBoxes.size() << ",featsize:" << Features.size()
            << ",unmatchedt:" << unmatched_tracker_idxes.size()
            << ",unmatchedbox:" << unmatched_bbox_idxes.size() << std::endl;
#endif
  /* Match remain trackers */
  /* - BBOX IoU Distance */
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
#ifdef DEBUG_TRACK
  for (auto ti : unmatched_tracker_idxes) {
    std::cout << "unmatched ti:" << ti << ",trackid:" << k_trackers[ti].id << std::endl;
  }
#endif

  MatchResult match_recall = refine_uncrowd(BBoxes, Features, unmatched_tracker_idxes,
                                            unmatched_bbox_idxes, crowd_iou_thresh);

  match_recall.matched_pairs.insert(match_recall.matched_pairs.end(), matched_pairs.begin(),
                                    matched_pairs.end());
  match_recall.matched_pairs.insert(match_recall.matched_pairs.end(),
                                    prev_match.matched_pairs.begin(),
                                    prev_match.matched_pairs.end());

  return match_recall;
}

void DeepSORT::get_pair_trackids(std::map<int, std::vector<stObjInfo>> &cls_objs,
                                 std::map<uint64_t, uint64_t> &pair_tracks) {
  auto swapf = [](uint64_t &a, uint64_t &b) {
    if (b < a) {
      uint64_t tmp = a;
      a = b;
      b = tmp;
    }
  };
  for (auto &kv : cls_objs) {
    std::vector<stObjInfo> &objs = kv.second;
    for (auto &obj : objs) {
      uint64_t tid = obj.track_id;
      if (tid == 0) continue;
      int pair_objid = obj.pair_obj_id;
#ifdef DEBUG_TRACK
      std::cout << "label:" << kv.first << ",tid:" << tid << ",pairobjid:" << pair_objid
                << std::endl;
#endif
      if (pair_objid == -1) continue;
      uint64_t pair_trackid = cls_objs[obj.pair_type][obj.pair_obj_id].track_id;
      if (pair_trackid == 0) continue;
      swapf(tid, pair_trackid);
      if (pair_tracks.count(tid)) {
        if (pair_tracks[tid] != pair_trackid) {
          std::cout << "found duplicate pair,tid:" << tid << ",srcpair:" << pair_tracks[tid]
                    << ",new:" << pair_trackid << std::endl;
        }
        continue;
      }
      pair_tracks[tid] = pair_trackid;
#ifdef DEBUG_TRACK
      std::cout << "generate pair track:" << tid << ",pairtid:" << pair_trackid << std::endl;
#endif
    }
  }
  for (auto &track : k_trackers) {
    uint64_t tid = track.id;
    uint64_t pair_trackid = track.get_pair_trackid();
    if (track.unmatched_times == 0 && pair_trackid != 0) {
      swapf(tid, pair_trackid);
      if (pair_tracks.count(tid) == 0) {
        pair_tracks[tid] = pair_trackid;
      }
    }
  }
}

void DeepSORT::update_tracks(cvtdl_deepsort_config_t *conf,
                             std::map<int, std::vector<stObjInfo>> &cls_objs) {
  // update matched tracks
  std::map<uint64_t, int> processed_tracks;

  for (auto &kv : cls_objs) {
    // check
    std::vector<stObjInfo> &objs = kv.second;

    // track_result[label].resize(objs.size());
    for (auto &obj : objs) {
      uint64_t trackid = obj.track_id;

      if (trackid == 0) continue;
      if (track_indices_.count(trackid) == 0) {
        std::cout << "error trackid not found:" << trackid << std::endl;
        continue;
      }
      processed_tracks[trackid] = 1;

      int index = track_indices_[trackid];
      stRect rct(obj.box.x1, obj.box.y1, obj.box.x2 - obj.box.x1, obj.box.y2 - obj.box.y1);

      k_trackers[index].update(kf_, &rct, conf);
    }
  }

  // create new tracks
  for (auto &kv : cls_objs) {
    int label = kv.first;
    std::vector<stObjInfo> &objs = kv.second;
    // std::cout<<"process type:"<<kv.first<<",num:"<<objs.size()<<std::endl;
    for (auto &obj : objs) {
      uint64_t trackid = obj.track_id;
      if (trackid != 0) continue;

      BBOX box = cvt_tlwh_box(obj);
      const FEATURE empty_feature(0);
      uint64_t new_id = get_nextID(label);
      KalmanTracker tracker_(new_id, label, box, empty_feature, conf->ktracker_conf);
      tracker_.label = label;
      if (label == OBJ_HEAD) {
        tracker_.old_x = (obj.box.x1 + obj.box.x2) / 2.0;
        tracker_.old_y = (obj.box.y1 + obj.box.y2) / 2.0;
      } else if (label == OBJ_PERSON) {
        tracker_.old_x = (obj.box.x1 + obj.box.x2) / 2.0;
        tracker_.old_y = obj.box.y1 * 1.2;
      }
      k_trackers.push_back(tracker_);
      track_indices_[new_id] = k_trackers.size() - 1;
      KalmanTracker *p_track = &k_trackers[k_trackers.size() - 1];

      obj.track_id = new_id;
      processed_tracks[new_id] = 1;

#ifdef DEBUG_TRACK
      std::cout << "create new track:" << new_id << ",label:" << label
                << ",pairobj:" << obj.pair_obj_id << ",srcidx:" << obj.src_idx << std::endl;
      std::cout << "p_track addr:" << (void *)p_track << std::endl;
#endif
      // if new track has pair track,should add pair releations
      if (obj.pair_obj_id == -1) continue;

      if (obj.pair_obj_id >= (int)cls_objs[obj.pair_type].size()) {
        std::cout << "error pairobjid:" << obj.pair_obj_id << ",type:" << obj.pair_type
                  << ",objsize:" << cls_objs[obj.pair_type].size() << std::endl;
        continue;
      }

      uint64_t pair_obj_trackid = cls_objs[obj.pair_type][obj.pair_obj_id].track_id;
      if (pair_obj_trackid == 0) continue;
      if (track_indices_.count(pair_obj_trackid) == 0) {
        std::cout << "error pair_obj_trackid not found:" << pair_obj_trackid << std::endl;
        continue;
      }

      int tid = track_indices_[pair_obj_trackid];
      KalmanTracker *p_pair_track = &k_trackers[tid];
      if (p_pair_track->unmatched_times == 0 &&
          p_pair_track->tracker_state == k_tracker_state_e::ACCREDITATION && obj.box.score > 0.5) {
        // turn the track state as confirmed
        p_track->tracker_state = k_tracker_state_e::ACCREDITATION;
        std::cout << "confirm track directly ,track:" << p_track->id << ",pair:" << p_pair_track->id
                  << std::endl;
      }
    }
  }

  std::map<uint64_t, uint64_t> pair_tracks;
  get_pair_trackids(cls_objs, pair_tracks);
  // update pair track relationship
  std::map<uint64_t, int> matched_tracks = processed_tracks;
  for (auto &p : pair_tracks) {
#ifdef DEBUG_TRACK
    std::cout << "to process paired track:" << p.first << " and " << p.second << std::endl;
#endif

    KalmanTracker *p_tracka = &k_trackers[track_indices_[p.first]];
    KalmanTracker *p_trackb = &k_trackers[track_indices_[p.second]];

    // std::cout<<"tracka
    // addr:"<<(void*)p_tracka<<",addb:"<<(void*)p_trackb<<",id:"<<p_tracka->id<<","<<p_trackb->id<<std::endl;

    if (p_trackb->unmatched_times == 0 && p_tracka->unmatched_times != 0) {
      // tracka was not matched,trackb was matched
      // use bbox guessed from trackb to update tracka
      p_tracka->false_update_from_pair(kf_, p_trackb, conf);
      // std::cout<<"fa"
      matched_tracks[p.first] = 1;
      matched_tracks[p.second] = 1;
    } else if (p_trackb->unmatched_times != 0 && p_tracka->unmatched_times == 0) {
      p_trackb->false_update_from_pair(kf_, p_tracka, conf);
      matched_tracks[p.first] = 1;
      matched_tracks[p.second] = 1;
    } else if (p_trackb->unmatched_times == 0 && p_tracka->unmatched_times == 0) {
      p_tracka->update_pair_info(p_trackb);
      matched_tracks[p.first] = 1;
      matched_tracks[p.second] = 1;
    }
  }

  // update tracks not matched
  for (auto &tracker : k_trackers) {
    if (matched_tracks.count(tracker.id)) continue;
#ifdef DEBUG_TRACK
    std::cout << "to updat missed track:" << tracker.id << ",frameid:" << frame_id_
              << ",age:" << tracker.ages_ << ",pairtrack:" << tracker.get_pair_trackid()
              << std::endl;
#endif
    tracker.update(kf_, nullptr, conf);
  }
  std::vector<uint64_t> erased_tids;
  for (auto it_ = k_trackers.begin(); it_ != k_trackers.end();) {
    if (it_->tracker_state == k_tracker_state_e::MISS) {
#ifdef DEBUG_TRACK
      std::cout << "erase track:" << it_->id << ",frameid:" << frame_id_ << ",age:" << it_->ages_
                << ",pairtrack:" << it_->get_pair_trackid() << std::endl;
#endif
      erased_tids.push_back(it_->id);
      it_ = k_trackers.erase(it_);

    } else {
      it_++;
    }
  }
  for (auto &track : k_trackers) {
    uint64_t pair_tid = track.get_pair_trackid();
    if (pair_tid == 0) continue;
    if (std::find(erased_tids.begin(), erased_tids.end(), pair_tid) != erased_tids.end()) {
      track.pair_track_infos_.clear();

#ifdef DEBUG_TRACK
      std::cout << "to delete pairinfo for track:" << track.id << ",pairtid:" << pair_tid
                << ",frameid:" << frame_id_ << std::endl;
#endif
    }
  }
}

CVI_S32 DeepSORT::track_fuse(cvtdl_object_t *ped, cvtdl_face_t *face, cvtdl_tracker_t *tracker) {
#ifdef DEBUG_CAPTURE
  std::cout << "start to track_fuse,face num:" << face->size << ",pedsize:" << ped->size
            << ",frameid:" << frame_id_ << std::endl;
  show_INFO_KalmanTrackers();
#endif

  cvtdl_deepsort_config_t *conf;
  auto it_conf = specific_conf.find(0);
  if (it_conf != specific_conf.end()) {
    conf = &it_conf->second;
  } else {
    conf = &default_conf;
  }
  // predict all tracks
  const int face_label = OBJ_FACE;
  const int ped_label = OBJ_PERSON;

  int tidx = 0;
  track_indices_.clear();
  for (KalmanTracker &tracker_ : k_trackers) {
    track_indices_[tracker_.id] = tidx++;
    tracker_.predict(kf_, conf);
  }

  check_bound_state(conf);

  std::map<int, std::vector<stObjInfo>> cls_objs;
  // assemble data
  for (uint32_t i = 0; i < face->size; i++) {
    stObjInfo obj;
    obj.box = face->info[i].bbox;
    obj.src_idx = (int)i;
    obj.classes = face_label;
    obj.pair_obj_id = -1;
    obj.track_id = 0;

    cls_objs[face_label].push_back(obj);
  }

  for (uint32_t i = 0; i < ped->size; i++) {
    stObjInfo obj;
    int lb = ped_label;  // ped->info[i].classes;
    obj.box = ped->info[i].bbox;
    obj.src_idx = (int)i;
    obj.classes = lb;
    obj.pair_obj_id = -1;
    obj.track_id = 0;
    cls_objs[lb].push_back(obj);
  }

  update_pair_info(cls_objs[face_label], cls_objs[ped_label], OBJ_FACE, OBJ_PERSON, 0.1);

  std::vector<BBOX> face_boxes = cvt_boxes(cls_objs[face_label]);
  std::vector<FEATURE> face_feats(face_boxes.size());

  MatchResult face_res =
      get_init_match_result(cls_objs[face_label], k_trackers, track_indices_, face_label);

  face_res = get_match_result(face_res, face_boxes, face_feats, false, 0.1, conf);

  std::map<uint64_t, int> face_track_indices;
  for (auto &m : face_res.matched_pairs) {
    int t_idx = m.first;
    int det_idx = m.second;

    int pair_ped_idx = cls_objs[face_label][det_idx].pair_obj_id;
    cls_objs[face_label][det_idx].track_id = k_trackers[t_idx].id;
    face_track_indices[k_trackers[t_idx].id] = det_idx;
#ifdef DEBUG_TRACK
    std::cout << "matched face:" << det_idx << ",track:" << k_trackers[t_idx].id
              << ",pairped:" << pair_ped_idx << std::endl;
#endif
    if (pair_ped_idx != -1) {
      uint64_t pair_ped_trackid = k_trackers[t_idx].get_pair_trackid();
      if (pair_ped_trackid != 0) {
        cls_objs[ped_label][pair_ped_idx].track_id = pair_ped_trackid;
#ifdef DEBUG_TRACK
        std::cout << "recall peddet:" << pair_ped_idx << " with pedtrack:" << pair_ped_trackid
                  << std::endl;
#endif
      }
    }
  }

  std::vector<BBOX> ped_boxes = cvt_boxes(cls_objs[ped_label]);
  std::vector<FEATURE> ped_feats(ped_boxes.size());

  MatchResult ped_res =
      get_init_match_result(cls_objs[ped_label], k_trackers, track_indices_, ped_label);

  cvtdl_deepsort_config_t ped_cfg = *conf;
  ped_cfg.max_distance_iou = 0.6;
  ped_cfg.kfilter_conf.chi2_threshold *= 0.85;
  ped_cfg.ktracker_conf.max_unmatched_num = 15;
  ped_res = get_match_result(ped_res, ped_boxes, ped_feats, false, 0.7, &ped_cfg);
  std::map<uint64_t, BBOX> src_track_box;
  for (auto &kf : k_trackers) {
    if (kf.class_id == face_label) {
      src_track_box[kf.id] = kf.getBBox_TLWH();
    }
  }
#ifdef DEBUG_TRACK
  std::cout << "ped matched pairs:" << ped_res.matched_pairs.size() << std::endl;
#endif
  // getchar();
  // use ped track res to recall face
  for (auto &m : ped_res.matched_pairs) {
    int t_idx = m.first;
    int det_idx = m.second;
    int pair_face_idx = cls_objs[ped_label][det_idx].pair_obj_id;
    uint64_t pair_face_trackid = k_trackers[t_idx].get_pair_trackid();

    if (face_track_indices.count(pair_face_trackid) > 0) {
      stRect box_face = extract_box(cls_objs[face_label][face_track_indices[pair_face_trackid]]);
      stRect box_ped = tlwh2rect(k_trackers[t_idx].getBBox_TLWH());

      if (self_iou(box_face, box_ped) < 0.8) {
        LOGI("skip det_idx:%d, track_id: %d\n", det_idx, k_trackers[t_idx].id);
        continue;
      }
    }

    cls_objs[ped_label][det_idx].track_id = k_trackers[t_idx].id;
#ifdef DEBUG_TRACK
    std::cout << "matched ped:" << det_idx << ",track:" << k_trackers[t_idx].id
              << ",pairface:" << pair_face_idx << std::endl;
#endif
    if (pair_face_idx != -1) {
#ifdef DEBUG_TRACK
      std::cout << "pedtrack:" << k_trackers[t_idx].id << ",pairfacetrack:" << pair_face_trackid
                << std::endl;
#endif
      if (pair_face_trackid == 0) continue;
      uint64_t face_trackid = cls_objs[face_label][pair_face_idx].track_id;
      if (face_trackid != 0 && face_trackid != pair_face_trackid) {
        LOGE("error matched ped pairface trackid:%d,paired_trackid:%d\n", (int)face_trackid,
             (int)pair_face_trackid);
      } else {
#ifdef DEBUG_TRACK
        std::cout << "recall face with ped,faceidx:" << pair_face_idx
                  << ",trackid:" << pair_face_trackid << std::endl;
#endif
        cls_objs[face_label][pair_face_idx].track_id = pair_face_trackid;
      }
    }
  }

  update_tracks(conf, cls_objs);
  track_indices_.clear();
  std::vector<int> face_tracks_inds;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    track_indices_[k_trackers[i].id] = (int)i;
    if (k_trackers[i].class_id == face_label) {
      face_tracks_inds.push_back(i);
    }
  }
  for (auto &kv : cls_objs) {
    int label = kv.first;

    for (auto &obj : kv.second) {
      if (label == OBJ_FACE) {
        face->info[obj.src_idx].unique_id = obj.track_id;
      } else {
        ped->info[obj.src_idx].unique_id = obj.track_id;
      }
    }
  }

  std::map<uint64_t, uint32_t> face_trackid_idx_map;
  for (uint32_t i = 0; i < face->size; i++) {
    uint64_t trackid = face->info[i].unique_id;

    face_trackid_idx_map[trackid] = i;
    if (track_indices_.count(trackid) == 0) {
      LOGE("track not found in trackindst,:%d\n", (int)trackid);
      continue;
    }
    int index = track_indices_[trackid];
    if (index >= (int)k_trackers.size()) {
      std::cout << "error,index overflow,index:" << index << ",size:" << k_trackers.size()
                << ",trackid:" << trackid << std::endl;
      continue;
    }
    auto *p_track = &k_trackers[index];
    if (p_track->ages_ == 1) {
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
    } else if (p_track->tracker_state == k_tracker_state_e::PROBATION) {
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
    } else if (p_track->tracker_state == k_tracker_state_e::ACCREDITATION) {
      face->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
    } else {
      LOGE("Tracker State Unknow.\n");
      printf("track unknown type error\n");
      continue;
    }
  }
  int tsdiff = current_timestamp_ - last_timestamp_;
  float sec = tsdiff / 1000.0;
  CVI_TDL_MemAlloc(face_tracks_inds.size(), tracker);
  for (uint32_t i = 0; i < tracker->size; i++) {
    memset(&tracker->info[i], 0, sizeof(tracker->info[i]));
    auto *p_track = &k_trackers[face_tracks_inds[i]];
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
    float velx = 0;
    float vely = 0;
    // printf("src velx, velx = 0\n");
    if (src_track_box.count(p_track->id)) {
      BBOX pre_box = src_track_box[p_track->id];
      float meansize = (pre_box(2) + pre_box(3) + t_bbox(2) + t_bbox(3)) / 4;
      velx = (t_bbox(0) - pre_box(0)) / meansize / sec;
      vely = (t_bbox(1) - pre_box(1)) / meansize / sec;
      // printf("meansize: %f, sec:%f\n", meansize, sec);
      // printf("velx: %f, t_bbox(0):%f, pre_box(0):%f\n", velx, t_bbox(0), pre_box(0));
      // printf("vely: %f, t_bbox(1):%f, pre_box(1):%f\n", vely, t_bbox(1), pre_box(1));
    }
    if (face_trackid_idx_map.count(p_track->id)) {
      uint32_t faceid = face_trackid_idx_map[p_track->id];
      // printf("to update velx vely\n");
      face->info[faceid].velx = velx;
      face->info[faceid].vely = vely;
    }
    tracker->info[i].bbox.x1 = t_bbox(0);
    tracker->info[i].bbox.y1 = t_bbox(1);
    tracker->info[i].bbox.x2 = t_bbox(0) + t_bbox(2);
    tracker->info[i].bbox.y2 = t_bbox(1) + t_bbox(3);
    tracker->info[i].id = p_track->id;
    tracker->info[i].out_num = p_track->out_nums;
  }
  frame_id_ += 1;
  last_timestamp_ = current_timestamp_;
#ifdef DEBUG_TRACK
  std::cout << "finish track,face num:" << face->size << std::endl;
  show_INFO_KalmanTrackers();
#endif
  return CVI_TDL_SUCCESS;
}

void DeepSORT::update_out_num(cvtdl_tracker_t *tracker) {
  for (uint32_t i = 0; i < tracker->size; i++) {
    uint32_t trackid = tracker->info[i].id;
    int index = track_indices_[trackid];
    if (index >= (int)k_trackers.size()) {
      std::cout << "error,index overflow,index:" << index << ",size:" << k_trackers.size()
                << ",trackid:" << trackid << std::endl;
      continue;
    }
    auto *p_track = &k_trackers[index];
    p_track->out_nums = tracker->info[i].out_num;
  }
}

CVI_S32 DeepSORT::track_headfuse(cvtdl_object_t *origin_obj, cvtdl_tracker_t *tracker,
                                 bool use_reid, cvtdl_object_t *head, cvtdl_object_t *ped,
                                 const cvtdl_counting_line_t *counting_line_t,
                                 const randomRect *rect) {
  {
    std::map<int, std::vector<cvtdl_object_info_t>> tmp_objs;
    for (uint32_t i = 0; i < origin_obj->size; i++) {
      if (origin_obj->info[i].classes == 0)
        tmp_objs[0].push_back(origin_obj->info[i]);
      else
        tmp_objs[1].push_back(origin_obj->info[i]);
    }
    CVI_TDL_MemAllocInit(tmp_objs[0].size(), head);
    CVI_TDL_MemAllocInit(tmp_objs[1].size(), ped);
    memset(head->info, 0, sizeof(cvtdl_object_info_t) * head->size);
    head->rescale_type = origin_obj->rescale_type;
    head->height = origin_obj->height;
    head->width = origin_obj->width;
    for (uint32_t i = 0; i < tmp_objs[0].size(); i++) {
      memcpy(&head->info[i].bbox, &tmp_objs[0][i].bbox, sizeof(cvtdl_bbox_t));
      head->info[i].classes = 0;
    }

    memset(ped->info, 0, sizeof(cvtdl_object_info_t) * ped->size);
    ped->rescale_type = origin_obj->rescale_type;
    ped->height = origin_obj->height;
    ped->width = origin_obj->width;
    for (uint32_t i = 0; i < tmp_objs[1].size(); i++) {
      memcpy(&ped->info[i].bbox, &tmp_objs[1][i].bbox, sizeof(cvtdl_bbox_t));
      ped->info[i].classes = 1;
    }
  }
  CVI_TDL_Free(origin_obj);

#ifdef DEBUG_CAPTURE
  std::cout << "start to track_fuse,head num:" << head->size << ",pedsize:" << ped->size
            << ",frameid:" << frame_id_ << std::endl;
  show_INFO_KalmanTrackers();
#endif

  cvtdl_deepsort_config_t *conf;
  auto it_conf = specific_conf.find(0);
  if (it_conf != specific_conf.end()) {
    conf = &it_conf->second;
  } else {
    conf = &default_conf;
  }
  // predict all tracks
  const int head_label = OBJ_HEAD;
  const int ped_label = OBJ_PERSON;

  int tidx = 0;
  track_indices_.clear();
  for (KalmanTracker &tracker_ : k_trackers) {
    track_indices_[tracker_.id] = tidx++;
    tracker_.predict(kf_, conf);
  }

  check_bound_state(conf);

  std::map<int, std::vector<stObjInfo>> cls_objs;
  // assemble data
  for (uint32_t i = 0; i < head->size; i++) {
    stObjInfo obj;
    obj.box = head->info[i].bbox;
    obj.src_idx = (int)i;
    obj.classes = head_label;
    obj.pair_obj_id = -1;
    obj.track_id = 0;

    cls_objs[head_label].push_back(obj);
  }

  for (uint32_t i = 0; i < ped->size; i++) {
    stObjInfo obj;
    int lb = ped_label;  // ped->info[i].classes;
    obj.box = ped->info[i].bbox;
    obj.src_idx = (int)i;
    obj.classes = lb;
    obj.pair_obj_id = -1;
    obj.track_id = 0;
    cls_objs[lb].push_back(obj);
  }

  update_pair_info(cls_objs[head_label], cls_objs[ped_label], OBJ_HEAD, OBJ_PERSON, 0.1);

  std::vector<BBOX> head_boxes = cvt_boxes(cls_objs[head_label]);
  std::vector<FEATURE> head_feats(head_boxes.size());

  MatchResult head_res =
      get_init_match_result(cls_objs[head_label], k_trackers, track_indices_, head_label);

  head_res = get_match_result_consumer_counting(head_res, head_boxes, head_feats, false, 0.1, conf,
                                                false, cls_objs[head_label]);
  for (auto &m : head_res.matched_pairs) {
    int t_idx = m.first;
    int det_idx = m.second;
    int pair_ped_idx = cls_objs[head_label][det_idx].pair_obj_id;
    cls_objs[head_label][det_idx].track_id = k_trackers[t_idx].id;

#ifdef DEBUG_TRACK
    std::cout << "matched face:" << det_idx << ",track:" << k_trackers[t_idx].id
              << ",pairped:" << pair_ped_idx << std::endl;
#endif
    if (pair_ped_idx != -1) {
      uint64_t pair_ped_trackid = k_trackers[t_idx].get_pair_trackid();
      // printf("trackid:%d  pair_id:%d\n",k_trackers[t_idx].id, pair_ped_trackid);
      if (pair_ped_trackid != 0) {
        cls_objs[ped_label][pair_ped_idx].track_id = pair_ped_trackid;
        // #ifdef DEBUG_TRACK
        //         std::cout << "recall peddet:" << pair_ped_idx << " with pedtrack:" <<
        //         pair_ped_trackid
        //                   << std::endl;
        // #endif
      }
    }
  }

  std::vector<BBOX> ped_boxes = cvt_boxes(cls_objs[ped_label]);
  std::vector<FEATURE> ped_feats(ped_boxes.size());

  MatchResult ped_res =
      get_init_match_result(cls_objs[ped_label], k_trackers, track_indices_, ped_label);

  cvtdl_deepsort_config_t ped_cfg = *conf;
  ped_cfg.max_distance_iou = 0.6;
  ped_cfg.kfilter_conf.chi2_threshold = 0.6;
  ped_cfg.ktracker_conf.max_unmatched_num = 15;
  ped_res = get_match_result_consumer_counting(ped_res, ped_boxes, ped_feats, false, 0.7, &ped_cfg,
                                               true, cls_objs[ped_label]);

#ifdef DEBUG_TRACK
  std::cout << "ped matched pairs:" << ped_res.matched_pairs.size() << std::endl;
#endif
  // getchar();
  // use ped track res to recall head
  std::map<uint64_t, int> pair_heads;
  for (auto &m : ped_res.matched_pairs) {
    int t_idx = m.first;
    int det_idx = m.second;
    int pair_head_idx = cls_objs[ped_label][det_idx].pair_obj_id;

    cls_objs[ped_label][det_idx].track_id = k_trackers[t_idx].id;
#ifdef DEBUG_TRACK
    std::cout << "matched ped:" << det_idx << ",track:" << k_trackers[t_idx].id
              << ",pairhead:" << pair_head_idx << std::endl;
#endif
    if (pair_head_idx != -1) {
      uint64_t pair_head_trackid = k_trackers[t_idx].get_pair_trackid();
#ifdef DEBUG_TRACK
      std::cout << "pedtrack:" << k_trackers[t_idx].id << ",pairheadtrack:" << pair_head_trackid
                << std::endl;
#endif
      if (pair_head_trackid == 0) continue;
      uint64_t head_trackid = cls_objs[head_label][pair_head_idx].track_id;
      // printf("********** trackid:%d  pair_id:%d\n",pair_head_trackid, k_trackers[t_idx].id);
      if (head_trackid != 0 && head_trackid != pair_head_trackid) {
        LOGE("error matched ped pairhead trackid:%d,paired_trackid:%d\n", (int)head_trackid,
             (int)pair_head_trackid);
        k_trackers[t_idx].pair_track_infos_.clear();
        k_trackers[track_indices_[pair_head_trackid]].pair_track_infos_.clear();
      } else {
#ifdef DEBUG_TRACK
        std::cout << "recall head with ped,headidx:" << pair_head_idx
                  << ",trackid:" << pair_head_trackid << std::endl;
#endif
        cls_objs[head_label][pair_head_idx].track_id = pair_head_trackid;
      }
    }
  }

  // consumer counting  head
  for (auto &obj : cls_objs[head_label]) {
    uint64_t trackid = obj.track_id;
    if (trackid == 0) continue;
    if (track_indices_.count(trackid) == 0) {
      std::cout << "error trackid not found:" << trackid << std::endl;
      continue;
    }
    int index = track_indices_[trackid];
    float cur_x = (obj.box.x1 + obj.box.x2) / 2.0;
    float cur_y = (obj.box.y1 + obj.box.y2) / 2.0;
    //  counting num
    uint64_t pair_track_id = k_trackers[index].get_pair_trackid();
    int pair_tracker_idx =
        track_indices_.count(pair_track_id) == 0 ? -1 : track_indices_[pair_track_id];

    if (pair_tracker_idx != -1 && k_trackers[pair_tracker_idx].counting_gap > 0) {
      k_trackers[index].is_entry = k_trackers[pair_tracker_idx].is_entry;
      k_trackers[index].is_leave = k_trackers[pair_tracker_idx].is_leave;
    } else if (k_trackers[index].is_entry && k_trackers[index].is_leave) {
      ;
    } else {
      consumer_counting_fun(obj, index, counting_line_t, rect);
    }
    k_trackers[index].old_x = cur_x;
    k_trackers[index].old_y = cur_y;
    k_trackers[index].counting_gap = std::max(0, k_trackers[index].counting_gap - 1);
    stRect rct(obj.box.x1, obj.box.y1, obj.box.x2 - obj.box.x1, obj.box.y2 - obj.box.y1);
    // printf("trackid:%d  pair_id:%d\n",trackid, pair_track_id);
    k_trackers[index].update(kf_, &rct, conf);
  }
  // consumer counting  ped
  for (auto &obj : cls_objs[ped_label]) {
    uint64_t trackid = obj.track_id;
    if (trackid == 0) continue;
    if (track_indices_.count(trackid) == 0) {
      std::cout << "error trackid not found:" << trackid << std::endl;
      continue;
    }
    int index = track_indices_[trackid];
    float cur_x = (obj.box.x1 + obj.box.x2) / 2.0;
    float cur_y = obj.box.y1 * 1.2;
    //  counting num
    int pair_obj_id = obj.pair_obj_id;
    uint64_t pair_track_id = k_trackers[index].get_pair_trackid();
    int pair_tracker_idx =
        track_indices_.count(pair_track_id) == 0 ? -1 : track_indices_[pair_track_id];
    if (pair_tracker_idx != -1) {
      k_trackers[index].counting_gap =
          std::max(k_trackers[pair_tracker_idx].counting_gap, k_trackers[index].counting_gap);
      k_trackers[index].is_leave = k_trackers[pair_tracker_idx].is_leave;
      k_trackers[index].is_entry = k_trackers[pair_tracker_idx].is_entry;
    }
    if (pair_obj_id == -1) {
      if (k_trackers[index].is_entry && k_trackers[index].is_leave) {
        ;
      } else {
        consumer_counting_fun(obj, index, counting_line_t, rect);
      }
    }
    stRect rct(obj.box.x1, obj.box.y1, obj.box.x2 - obj.box.x1, obj.box.y2 - obj.box.y1);
    k_trackers[index].update(kf_, &rct, conf);
    k_trackers[index].counting_gap = std::max(0, k_trackers[index].counting_gap - 1);
    k_trackers[index].old_x = cur_x;
    k_trackers[index].old_y = cur_y;
  }

  update_tracks(conf, cls_objs);
  // consumer_counting_update_tracks(conf, cls_objs, counting_line_t, rect);
  head->entry_num = entry_num;
  head->miss_num = miss_num;
  track_indices_.clear();
  std::vector<int> head_tracks_inds;
  for (size_t i = 0; i < k_trackers.size(); i++) {
    track_indices_[k_trackers[i].id] = (int)i;
    if (k_trackers[i].class_id == head_label) {
      head_tracks_inds.push_back(i);
    }
  }
  for (auto &kv : cls_objs) {
    int label = kv.first;

    for (auto &obj : kv.second) {
      if (label == OBJ_HEAD) {
        head->info[obj.src_idx].unique_id = obj.track_id;
      } else {
        ped->info[obj.src_idx].unique_id = obj.track_id;
      }
    }
  }

  for (uint32_t i = 0; i < head->size; i++) {
    uint64_t trackid = head->info[i].unique_id;

    if (track_indices_.count(trackid) == 0) {
      LOGE("track not found in trackindst,:%d\n", (int)trackid);
      continue;
    }
    int index = track_indices_[trackid];
    if (index >= (int)k_trackers.size()) {
      std::cout << "error,index overflow,index:" << index << ",size:" << k_trackers.size()
                << ",trackid:" << trackid << std::endl;
      continue;
    }
    auto *p_track = &k_trackers[index];
    if (p_track->ages_ == 1) {
      head->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_NEW;
    } else if (p_track->tracker_state == k_tracker_state_e::PROBATION) {
      head->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_UNSTABLE;
    } else if (p_track->tracker_state == k_tracker_state_e::ACCREDITATION) {
      head->info[i].track_state = cvtdl_trk_state_type_t::CVI_TRACKER_STABLE;
    } else {
      LOGE("Tracker State Unknow.\n");
      printf("track unknown type error\n");
      continue;
    }
  }
  CVI_TDL_MemAlloc(head_tracks_inds.size(), tracker);
  for (uint32_t i = 0; i < tracker->size; i++) {
    memset(&tracker->info[i], 0, sizeof(tracker->info[i]));
    auto *p_track = &k_trackers[head_tracks_inds[i]];
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
  frame_id_ += 1;
#ifdef DEBUG_TRACK
  std::cout << "finish track,head num:" << head->size << std::endl;
  show_INFO_KalmanTrackers();
#endif
  return CVI_TDL_SUCCESS;
}
