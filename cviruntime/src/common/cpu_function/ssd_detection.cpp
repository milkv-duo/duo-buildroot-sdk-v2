#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/ssd_detection.hpp>
#include <time.h>
#include <sys/time.h>
#ifdef __ARM_NEON
#include <numeric>
#include <tuple>
#include "arm_neon.h"
#endif /* ifdef __ARM_NEON */


namespace cvi {
namespace runtime {

bool SortScoreCmp0(const std::pair<float, int> &pair1,
                   const std::pair<float, int> &pair2) {
  return pair1.first > pair2.first;
}

bool SortScoreCmp1(const std::pair<float, std::pair<int, int>> &pair1,
                   const std::pair<float, std::pair<int, int>> &pair2) {
  return pair1.first > pair2.first;
}
//#undef __ARM_NEON
#ifdef __ARM_NEON
typedef union {
  int ival;
  unsigned int uval;
  float fval;
} unitype;

const float __expf_rng[2] = {
  1.442695041f,
  0.693147180f
};

const float __expf_lut[8] = {
  0.9999999916728642,    //p0
  0.04165989275009526,   //p4
  0.5000006143673624,   //p2
  0.0014122663401803872,   //p6
  1.000000059694879,     //p1
  0.008336936973260111,   //p5
  0.16666570253074878,   //p3
  0.00019578093328483123  //p7
};

float expf_c(float x)
{
  float a, b, c, d, xx;
  int m;

  union {
    float   f;
    int   i;
  } r;

  //Range Reduction:
  m = (int) (x * __expf_rng[0]);
  x = x - ((float) m) * __expf_rng[1];

  //Taylor Polynomial (Estrins)
  a = (__expf_lut[4] * x) + (__expf_lut[0]);
  b = (__expf_lut[6] * x) + (__expf_lut[2]);
  c = (__expf_lut[5] * x) + (__expf_lut[1]);
  d = (__expf_lut[7] * x) + (__expf_lut[3]);
  xx = x * x;
  a = a + b * xx;
  c = c + d * xx;
  xx = xx* xx;
  r.f = a + c * xx;

  //multiply by 2 ^ m
  m = m << 23;
  r.i = r.i + m;

  return r.f;
}

// fast exp
float expf_neon_sfp(float x)
{
  return expf_c(x);
}

static inline bool is_background_cls(int cls, int background_label_id) {
  return cls == background_label_id;
}

static inline bool is_share_loc_background_cls(int cls, int background_label_id) {
  return false;
}

// get loc and decode bbox info in one for loop
void GetLocBBox_opt(
    std::vector<std::map<int, std::pair<std::vector<std::pair<float, int>>, std::vector<BBox_l>* >>> *all_conf_scores,
    const float *loc_data, const float *prior_data,
    const int num, const int num_priors, const int num_loc_classes,
    const bool share_location, const int num_classes,
    const int background_label_id, const CodeType code_type,
    const bool variance_encoded_in_target, int top_k,
    std::vector<LabelBBox_l> *all_decode_bboxes
    ) {

  assert(code_type == PriorBoxParameter_CodeType_CENTER_SIZE);
  bool (*check_background)(int, int) = is_background_cls;

  if (share_location) {
    assert(num_loc_classes == 1);
    check_background = is_share_loc_background_cls;
    // return const, it should be remove check background's branch
  }

  for (int i = 0; i < num; ++i) {
    // save valid decode
    std::map<int, std::vector<std::pair<float, int>>> indices;
    LabelBBox_l &decode_bboxes = (*all_decode_bboxes)[i];
    std::vector<int> decode_keep_index((*all_conf_scores)[i].size() * top_k);
    int decode_keep_index_cnt = 0;

    // collect all decode index
    for (auto it = (*all_conf_scores)[i].begin(); it != (*all_conf_scores)[i].end(); it++) {
      std::vector<std::pair<float, int>> &scores = it->second.first;
      auto c = it->first;

      if (check_background(c, background_label_id)) {
        // Ignore background class.
        continue;
      }

      // init for share position
      it->second.second = &(decode_bboxes[-1]);

      // sort by top_k, align cmodel
      if (top_k < (int)scores.size()) {
        std::partial_sort(scores.begin(), scores.begin() + top_k, scores.end(),
                          SortScoreCmp0);
      } else {
        std::sort(scores.begin(), scores.end(), SortScoreCmp0);
      }

      int length = std::min(top_k, (int)scores.size());

      for (int k = 0; k < length; ++k) {
        // later get index
        scores[k].second /= num_classes;
        decode_keep_index[decode_keep_index_cnt] = (scores[k].second);
        decode_keep_index_cnt++;
      }
    }

    for (int c = 0; c < num_loc_classes; ++c) {
      int label = c;
      std::vector<BBox_l> *p = &(decode_bboxes[label]);
      if (share_location) {
        label = -1;
        p = &(decode_bboxes[label]);
        assert (label != background_label_id);
      }
      else {
        if (label == background_label_id) {
          // Ignore background class.
          continue;
        }

        auto all_conf_score = (*all_conf_scores)[i].find(c);
        if (all_conf_score == (*all_conf_scores)[i].end()) {
          continue;
        }

        std::vector<BBox_l> *p = &(decode_bboxes[label]);
        all_conf_score->second.second = p;
      }

      p->resize(num_priors); // assing max
      int sz = decode_keep_index_cnt;

      auto init_bbox = [=] (int k, int idx, BBox_l* decode_bbox) mutable -> void {
        // prior_bboxes
        int start_idx = k * 4;
        const float *p0 = prior_data + start_idx;
        const float *p1 = prior_data + start_idx + 4 * num_priors;

        // get prior_width / prior_height / prior_center_x / prior_center_y
        float32x4_t _v1 = {p0[2], p0[3], p0[0], p0[1]};
        float32x4_t _v2 = {-p0[0], -p0[1], p0[2], p0[3]};
        float32x4_t _v3 = {1, 1, 0.5, 0.5};
        float32x4_t sum = vaddq_f32(_v1, _v2);
        float32x4_t prod = vmulq_f32(sum, _v3);

        assert(prod[0] > 0); // prior_width
        assert(prod[1] > 0); // prior_height

        float _p1[4];
        memcpy(_p1, p1, sizeof(float) * 4);

        // opt CENTER_SIZE
        if (variance_encoded_in_target) {
          // variance is encoded in target, we simply need to retore the offset
          // predictions.
          _p1[0] = _p1[1] = _p1[2] = _p1[3] = 1;
        }
        //else {
        //  // variance is encoded in bbox, we need to scale the offset accordingly.
        //}

        // get decode_bbox_center_x/decode_bbox_center_y/decode_bbox_width/decode_bbox_height
        int shift = k * num_loc_classes * 4 + c * 4;
        auto xmin = _p1[0] * loc_data[shift];
        auto ymin = _p1[1] * loc_data[shift + 1];
        auto xmax = loc_data[shift + 2];
        auto ymax = loc_data[shift + 3];


        float _decode_bbox_width, _decode_bbox_height;
        _decode_bbox_width = _p1[2] * xmax;
        _decode_bbox_height = _p1[3] * ymax;
        _decode_bbox_width = expf_neon_sfp(_decode_bbox_width);
        _decode_bbox_height = expf_neon_sfp(_decode_bbox_height);

        // decode bbox, please refer \DecodeBBoxesAll_opt
        float32x4_t v1 = {xmin, ymin, _decode_bbox_width, _decode_bbox_height};
        float32x4_t v2 = {prod[0], prod[1], prod[0], prod[1]};
        float32x4_t v3 = {prod[2], prod[3], 0, 0};
        float32x4_t acc = vmlaq_f32(v3, v1, v2);  // acc = v3 + v1 * v2

        float32x4_t v4 = {acc[0], acc[1], acc[0], acc[1]};
        float32x4_t v5 = {-acc[2], -acc[3], acc[2], acc[3]};
        float32_t s = 0.5;

        // directly store back to xmin/... info
        vst1q_f32(decode_bbox->xy.b, vmlaq_n_f32(v4, v5, s));

        decode_bbox->CalcSize();
      };

      // TODO: try to leverage openmp
      {
#define ADD_DECODE_BBOX(idx) \
          init_bbox (decode_keep_index[idx], idx, &((*p)[decode_keep_index[idx]]));

        for (int _k = 0; _k < sz; _k++) {
          ADD_DECODE_BBOX(_k);
        }
      }
    }
    loc_data += num_priors * num_loc_classes * 4;
  }
}

void inline GetConfidenceScores(
    const float *conf_data, const int num, const int num_preds_per_class,
    const int num_classes, const float score_threshold,
    const int background_label_id, const bool share_location,
    std::vector<std::map<int, std::pair<std::vector<std::pair<float, int>>, std::vector<BBox_l>* >>> *conf_preds) {

  // we compare float as integer
  assert(score_threshold > 0);

  // exclude background 0: exclude, 1: include
  int skip_backgroud = !share_location;  // accord to normal implement background always skipped
  // assert (background_label_id == 0);

  auto all_classes = num_preds_per_class * num_classes;
  unitype t;
  t.fval = score_threshold;

  for (int i = 0; i < num; i++) {
    std::map<int, std::pair<std::vector<std::pair<float, int>>, std::vector<BBox_l>*>> &label_scores = (*conf_preds)[i];

    // later handle bbox idx
    unitype* _conf_data = (unitype*)conf_data;
    // measure neon/unroll performance
    //struct timeval net_fwd_time_t0;
    //gettimeofday(&net_fwd_time_t0, NULL);
#if 0
    unitype v;
    t.fval = score_threshold;
    float32x4_t thes = vmovq_n_f32(score_threshold);
    for (int i = 0; i < all_classes; i+=4) {
      // vector compare less than
      float32x4_t v1 = vld1q_f32(conf_data + i);

      if (vmaxvq_f32(v1) < t.fval) {
        continue;
      }
      uint32x4_t vcon = vcgtq_f32(v1, thes);
#define SELECT_SCORE(idx) \
      do {\
        if (vcon[idx] == 0xffffffff) label_scores[(i+idx) % num_classes].first.emplace_back(std::make_pair(conf_data[i+idx], (i+idx))); \
      } while (0);

      SELECT_SCORE(0);
      SELECT_SCORE(1);
      SELECT_SCORE(2);
      SELECT_SCORE(3);
    }
#else
    // unroll for not stall once cache miss
    // 8 is magic number that we try the best unroll loops
    int unroll_cnt = 8;
#define SELECT_SCORE(idx) \
    do {\
      unitype v = _conf_data[(j+idx)];\
      if (v.ival > t.ival) { \
        int c = (j+idx) % num_classes;\
        if ((c - background_label_id) || skip_backgroud) label_scores[c].first.emplace_back(std::make_pair(v.fval, (j+idx) )); \
      } \
    } while (0);

    int j = 0;
    for (; j < all_classes - unroll_cnt; j+=unroll_cnt) {
      // uint32 with less compare instruction
      //if (score > score_threshold)
      SELECT_SCORE(0);
      SELECT_SCORE(1);
      SELECT_SCORE(2);
      SELECT_SCORE(3);
      SELECT_SCORE(4);
      SELECT_SCORE(5);
      SELECT_SCORE(6);
      SELECT_SCORE(7);
    }

    // deal with residule
    for (; j < all_classes ; j++) {
      SELECT_SCORE(0);
    }
#endif
    // next batch
    conf_data += num_preds_per_class * num_classes;
  }
}

static void ApplyNMSFast(std::vector<BBox_l> *bboxes,
                      const std::vector<std::pair<float, int>> &conf_score,
                      const float nms_threshold, const float eta, int top_k,
                      int label,
                      std::vector<std::pair<float, std::tuple<int, int, std::vector<BBox_l>*>>> &score_index_pairs,
                      int* det_num) {
  // Do nms.
  float adaptive_threshold = nms_threshold;
  int i = 0;
  int indices_sz = 0;
  int offset = score_index_pairs.size();
  int length = (top_k < (int)conf_score.size()) ? top_k : conf_score.size();
  while (length != i) {
    bool keep = true;
    for (int k = 0; k < indices_sz; ++k) {
      if (keep) {
        int kept_idx = std::get<1>(score_index_pairs[k + offset].second);
        const BBox_l &b1 = (*bboxes)[conf_score[i].second];
        const BBox_l &b2 = (*bboxes)[kept_idx];

        if (b2.xy.s.xmin > b1.xy.s.xmax || b2.xy.s.xmax < b1.xy.s.xmin || b2.xy.s.ymin > b1.xy.s.ymax ||
            b2.xy.s.ymax < b1.xy.s.ymin) {
          keep = true;
        } else {
          const float inter_xmin = std::max(b1.xy.s.xmin, b2.xy.s.xmin);
          const float inter_ymin = std::max(b1.xy.s.ymin, b2.xy.s.ymin);
          const float inter_xmax = std::min(b1.xy.s.xmax, b2.xy.s.xmax);
          const float inter_ymax = std::min(b1.xy.s.ymax, b2.xy.s.ymax);
          const float inter_width = inter_xmax - inter_xmin;
          const float inter_height = inter_ymax - inter_ymin;
          const float inter_size = inter_width * inter_height;
          const float total_size = b1.size + b2.size;
          keep =
              (inter_size * (adaptive_threshold + 1) <= total_size * adaptive_threshold)
                  ? true
                  : false;
        }
      } else {
        break;
      }
    }

    if (keep) {
      // preserve
      score_index_pairs.emplace_back(std::make_pair(
            conf_score[i].first, std::make_tuple(label, conf_score[i].second, bboxes)));
      indices_sz++;
    }

    if (keep && eta < 1 && adaptive_threshold > 0.5) {
      adaptive_threshold *= eta;
    }
    i++;
  }
  (*det_num) = indices_sz;
}



void SSDDetectionFunc::neon_run(float* top_data, bool variance_encoded_in_target_,
      int num_loc_classes, float eta_, Decode_CodeType code_type_) {
  int num = _bottoms[0]->shape[0];
  int num_priors_ = _bottoms[2]->shape[2] / 4;

  float *loc_data = _bottoms[0]->cpu_data<float>();
  float *conf_data = _bottoms[1]->cpu_data<float>();
  float *prior_data = reinterpret_cast<float *>(_bottoms[2]->cpu_data<uint8_t>());

  std::vector<std::map<int, std::pair<std::vector<std::pair<float, int>>, std::vector<BBox_l>* >>> all_conf_scores;
  all_conf_scores.resize(num);
  // select score
  GetConfidenceScores(conf_data, num, num_priors_, _num_classes, _obj_threshold,
                          _background_label_id, _share_location, &all_conf_scores);

  std::vector<LabelBBox_l> all_decode_bboxes(num);

  // get box location
  GetLocBBox_opt(&all_conf_scores, loc_data, prior_data, num, num_priors_,
      num_loc_classes, _share_location, _num_classes,
      _background_label_id, code_type_, variance_encoded_in_target_,
      _top_k, &all_decode_bboxes);

  int num_shift = 0;
  for (int i = 0; i < num; ++i) {
    std::map<int, std::pair<std::vector<std::pair<float, int>>, std::vector<BBox_l>* >> &conf_scores =
        all_conf_scores[i];
    int num_det = 0;

    // we keep bbox point for reduce search
    std::vector<std::pair<float, std::tuple<int, int, std::vector<BBox_l>*>>> score_index_pairs;
    // init and collect count in each label
    // pair store <each lable count, each data offset in lable>
    std::map<int, std::pair<int, int>> new_indices_cnt;

    bool (*check_background)(int, int) = is_background_cls;
    if (_share_location) {
      check_background = is_share_loc_background_cls;
      // return const, it should be remove check background's branch
    }

    for (auto it = conf_scores.begin(); it != conf_scores.end(); it++) {
      int c = it->first;

      if (check_background(c, _background_label_id)) {
        // Ignore background class.
        continue;
      }

      std::vector<BBox_l> *bboxes = it->second.second;
      const std::vector<std::pair<float, int>> &aa = it->second.first;

      // get property nms
      ApplyNMSFast(bboxes, aa, _nms_threshold, eta_, _top_k, c, score_index_pairs,
          &new_indices_cnt[c].first);
    }

    num_det = score_index_pairs.size();
    int sz = num_det;

    if (_keep_topk > -1 && num_det > _keep_topk) {
      // Keep top k results per image.
      std::sort(score_index_pairs.begin(), score_index_pairs.end(),
          [](const std::pair<float, std::tuple<int, int, std::vector<BBox_l>*>> &pair1,
          const std::pair<float, std::tuple<int, int, std::vector<BBox_l>*>> &pair2) {
          return pair1.first > pair2.first;
          });

      sz = _keep_topk;

      // reset it
      for (auto it = new_indices_cnt.begin(); it != new_indices_cnt.end(); it++) {
        it->second.first = 0;
      }

      for (int j = 0; j < sz; ++j) {
        int label = std::get<0>(score_index_pairs[j].second);
        new_indices_cnt[label].first++;
      }
    }

    auto it = new_indices_cnt.begin();
    if (it != new_indices_cnt.end()) {
      int first_cnt = it->second.first;
      it->second.first = 0; // start with

      // calculate each label's distance
      for (++it; it != new_indices_cnt.end(); ++it) {
        int curr = it->second.first;
        it->second.first = first_cnt;
        first_cnt += curr;
      }
    }

    // try to continuous write
    for (int j = 0; j < sz; ++j) {

      int label = std::get<0>(score_index_pairs[j].second);
      int idx = std::get<1>(score_index_pairs[j].second);
      float s = score_index_pairs[j].first;
      std::vector<BBox_l>* bboxes = std::get<2>(score_index_pairs[j].second);

      int _cnt = (new_indices_cnt[label].first + new_indices_cnt[label].second + num_shift) * 7;
      (*bboxes)[idx].num = i;
      (*bboxes)[idx].label = label;
      (*bboxes)[idx].score = s;
      // try to sequential write cus num/lable/score/boxs are continuously
      memcpy(&top_data[_cnt], &((*bboxes)[idx]), sizeof(float) * 7);

      // add shift in its label
      new_indices_cnt[label].second++;
    }

    num_shift += sz;
  }

  int output_size = num * _keep_topk * 1 * 1 * 7;

  // fill dummy to end for align cmodel
  for (int i = num_shift * 7; i < output_size; ++i) {
    top_data[i] = -1;
  }
}
#else
#endif
void GetConfidenceScores_opt(
    const float *conf_data, const int num, const int num_preds_per_class,
    const int num_classes, const float score_threshold,
    std::vector<std::map<int, std::vector<std::pair<float, int>>>> *conf_preds) {
  conf_preds->clear();
  conf_preds->resize(num);

  for (int i = 0; i < num; i++) {
    std::map<int, std::vector<std::pair<float, int>>> &label_scores = (*conf_preds)[i];
    for (int p = 0; p < num_preds_per_class; ++p) {
      int start_idx = p * num_classes;
      for (int c = 0; c < num_classes; ++c) {
        if (conf_data[start_idx + c] > score_threshold) {
          label_scores[c].push_back(std::make_pair(conf_data[start_idx + c], p));
        }
      }
    }
    conf_data += num_preds_per_class * num_classes;
  }
}

void GetLocPredictions_opt(const float *loc_data, const int num,
                           const int num_preds_per_class, const int num_loc_classes,
                           const bool share_location, float *decode_index,
                           std::vector<LabelBBox_l> *loc_preds) {
  loc_preds->clear();
  if (share_location) {
    assert(num_loc_classes == 1);
  }
  loc_preds->resize(num);
  float *decode_pos = decode_index;
  for (int i = 0; i < num; ++i) {
    if (share_location) {
      decode_pos = decode_index + i * num_preds_per_class;
    }
    LabelBBox_l &label_bbox = (*loc_preds)[i];
    for (int p = 0; p < num_preds_per_class; ++p) {
      int start_idx = p * num_loc_classes * 4;
      for (int c = 0; c < num_loc_classes; ++c) {
        if (!share_location) {
          decode_pos = decode_index + num_preds_per_class * num_loc_classes * i +
                       num_preds_per_class * c;
        }
        int label = share_location ? -1 : c;
        if (label_bbox.find(label) == label_bbox.end()) {
          label_bbox[label].resize(num_preds_per_class);
        }
        if (decode_pos[p] != 1) {
          continue;
        }
        label_bbox[label][p].xy.s.xmin = loc_data[start_idx + c * 4];
        label_bbox[label][p].xy.s.ymin = loc_data[start_idx + c * 4 + 1];
        label_bbox[label][p].xy.s.xmax = loc_data[start_idx + c * 4 + 2];
        label_bbox[label][p].xy.s.ymax = loc_data[start_idx + c * 4 + 3];
      }
    }
    loc_data += num_preds_per_class * num_loc_classes * 4;
  }
}

void DecodeBBoxesAll_opt(const std::vector<LabelBBox_l> &all_loc_preds, int num_priors,
                         const float *prior_data, const int num,
                         const bool share_location, const int num_loc_classes,
                         const int background_label_id, const CodeType code_type,
                         const bool variance_encoded_in_target, float *decode_index,
                         std::vector<LabelBBox_l> *all_decode_bboxes) {
  assert(all_loc_preds.size() == (size_t)num);
  all_decode_bboxes->clear();
  all_decode_bboxes->resize(num);
  float *decode_pos = decode_index;
  for (int i = 0; i < num; ++i) {
    if (share_location) {
      decode_pos = decode_index + i * num_priors;
    }
    // Decode predictions into bboxes.
    for (int c = 0; c < num_loc_classes; ++c) {
      int label = share_location ? -1 : c;
      if (label == background_label_id) {
        // Ignore background class.
        continue;
      }
      if (all_loc_preds[i].find(label) == all_loc_preds[i].end()) {
        //TPU_LOG_DEBUG("Could not find location predictions for label %d\n", label);
      }
      const std::vector<BBox_l> &bboxes = all_loc_preds[i].find(label)->second;
      LabelBBox_l &decode_bboxes = (*all_decode_bboxes)[i];
      std::vector<BBox_l> *p = &(decode_bboxes[label]);
      p->clear();

      if (!share_location) {
        decode_pos = decode_index + num_priors * num_loc_classes * i + num_priors * c;
      }
      for (int k = 0; k < num_priors; ++k) {
        // NormalizedBBox decode_bbox;
        BBox_l decode_bbox;
        if (decode_pos[k] != 1) {
          p->push_back(decode_bbox);
          continue;
        }
        // opt CENTER_SIZE
        assert(code_type == PriorBoxParameter_CodeType_CENTER_SIZE);
        // prior_bboxes
        int start_idx = k * 4;
        const float *p0 = prior_data + start_idx;
        const float *p1 = prior_data + start_idx + 4 * num_priors;
        float prior_width = p0[2] - p0[0];
        assert(prior_width > 0);
        float prior_height = p0[3] - p0[1];
        assert(prior_height > 0);
        float prior_center_x = (p0[0] + p0[2]) * 0.5;
        float prior_center_y = (p0[1] + p0[3]) * 0.5;

        float decode_bbox_center_x, decode_bbox_center_y;
        float decode_bbox_width, decode_bbox_height;
        if (variance_encoded_in_target) {
          // variance is encoded in target, we simply need to retore the offset
          // predictions.
          decode_bbox_center_x = bboxes[k].xy.s.xmin * prior_width + prior_center_x;
          decode_bbox_center_y = bboxes[k].xy.s.ymin * prior_height + prior_center_y;
          decode_bbox_width = std::exp(bboxes[k].xy.s.xmax) * prior_width;
          decode_bbox_height = std::exp(bboxes[k].xy.s.ymax) * prior_height;
        } else {
          // variance is encoded in bbox, we need to scale the offset accordingly.
          decode_bbox_center_x = p1[0] * bboxes[k].xy.s.xmin * prior_width + prior_center_x;
          decode_bbox_center_y = p1[1] * bboxes[k].xy.s.ymin * prior_height + prior_center_y;
          decode_bbox_width = std::exp(p1[2] * bboxes[k].xy.s.xmax) * prior_width;
          decode_bbox_height = std::exp(p1[3] * bboxes[k].xy.s.ymax) * prior_height;
        }
        decode_bbox.xy.s.xmin = decode_bbox_center_x - decode_bbox_width * 0.5;
        decode_bbox.xy.s.ymin = decode_bbox_center_y - decode_bbox_height * 0.5;
        decode_bbox.xy.s.xmax = decode_bbox_center_x + decode_bbox_width * 0.5;
        decode_bbox.xy.s.ymax = decode_bbox_center_y + decode_bbox_height * 0.5;
        decode_bbox.CalcSize();
        p->push_back(decode_bbox);
      }
    }
  }
}

void SSDDetectionFunc::ApplyNMSFast_opt(const std::vector<BBox_l> &bboxes,
                      const std::vector<std::pair<float, int>> &conf_score,
                      const float nms_threshold, const float eta, int top_k,
                      std::vector<std::pair<float, int>> *indices) {
  // Do nms.
  float adaptive_threshold = nms_threshold;
  int i = 0;
  int length = (top_k < (int)conf_score.size()) ? top_k : conf_score.size();
  while (length != i) {
    bool keep = true;
    for (int k = 0; k < (int)indices->size(); ++k) {
      if (keep) {
        const int kept_idx = (*indices)[k].second;
        const BBox_l &b1 = bboxes[conf_score[i].second];
        const BBox_l &b2 = bboxes[kept_idx];

        if (b2.xy.s.xmin > b1.xy.s.xmax || b2.xy.s.xmax < b1.xy.s.xmin || b2.xy.s.ymin > b1.xy.s.ymax ||
            b2.xy.s.ymax < b1.xy.s.ymin) {
          keep = true;
        } else {
          const float inter_xmin = std::max(b1.xy.s.xmin, b2.xy.s.xmin);
          const float inter_ymin = std::max(b1.xy.s.ymin, b2.xy.s.ymin);
          const float inter_xmax = std::min(b1.xy.s.xmax, b2.xy.s.xmax);
          const float inter_ymax = std::min(b1.xy.s.ymax, b2.xy.s.ymax);
          const float inter_width = inter_xmax - inter_xmin;
          const float inter_height = inter_ymax - inter_ymin;
          const float inter_size = inter_width * inter_height;
          const float total_size = b1.size + b2.size;
          keep =
              (inter_size * (adaptive_threshold + 1) <= total_size * adaptive_threshold)
                  ? true
                  : false;
        }
      } else {
        break;
      }
    }
    if (keep) {
      indices->push_back(conf_score[i]);
    }
    if (keep && eta < 1 && adaptive_threshold > 0.5) {
      adaptive_threshold *= eta;
    }
    i++;
  }
}

SSDDetectionFunc::~SSDDetectionFunc() {}

void SSDDetectionFunc::setup(tensor_list_t &inputs,
                             tensor_list_t &outputs,
                             OpParam &param) {
  _num_classes = param.get<int32_t>("num_classes");
  _share_location = param.get<bool>("share_location");
  _background_label_id = param.get<int32_t>("background_label_id");
  _code_type = param.get<std::string>("code_type");
  _top_k = param.get<int32_t>("top_k");
  _nms_threshold = param.get<float>("nms_threshold");
  _obj_threshold = param.get<float>("confidence_threshold");
  _keep_topk = param.get<int32_t>("keep_top_k");

  // location : mbox_loc [1, prior_num * 4]
  // priorbox : mbox_priorbox [1, 2, prior_num * 4]
  // confidence: mbox_conf [1, prior_num * class_num]
  // can't decide input order by shape
  // so still use name to arrange
  std::vector<std::string> names = {"mbox_loc", "mbox_conf", "mbox_priorbox"};

  for (auto name : names) {
    for (auto input : inputs) {
      if (input->name.find(name) != std::string::npos) {
        _bottoms.emplace_back(input);
        break;
      }
    }
  }
  if (_bottoms.size() != 3){
    _bottoms.clear();
    for (auto input : inputs) {
      _bottoms.emplace_back(input);
    }
  }
  _tops = outputs;
}

void SSDDetectionFunc::run() {
  //struct timeval net_fwd_time_t0;
  //gettimeofday(&net_fwd_time_t0, NULL);
  auto top_data = _tops[0]->cpu_data<float>();

  size_t bottom_count = _bottoms.size();
  assert(bottom_count == 3);

  int num_loc_classes = _share_location ? 1 : _num_classes;
  float eta_ = 1.0;
  Decode_CodeType code_type_;
  if (_code_type == "CORNER") {
    code_type_ = PriorBoxParameter_CodeType_CORNER;
  } else if (_code_type == "CENTER_SIZE") {
    code_type_ = PriorBoxParameter_CodeType_CENTER_SIZE;
  } else if (_code_type == "CORNER_SIZE") {
    code_type_ = PriorBoxParameter_CodeType_CORNER_SIZE;
  } else {
    assert(0);
  }

  //TPU_LOG_DEBUG("priorbox_size: %zu\n", _bottoms[2]->shape.size());
  //TPU_LOG_DEBUG("n = %d, c = %d, h = %d, w = %d\n",
  //              _bottoms[2]->shape[0], _bottoms[2]->shape[1],
  //              _bottoms[2]->shape[2], _bottoms[2]->shape[3]);

  bool variance_encoded_in_target_ = false;

#ifdef __ARM_NEON
  neon_run(top_data, variance_encoded_in_target_, num_loc_classes, eta_, code_type_);

  return;
#else
#endif
  memset(top_data, 0, _tops[0]->size());

  int num = _bottoms[0]->shape[0];  // batch_size
  int num_priors_ = _bottoms[2]->shape[2] / 4;

  float *loc_data = _bottoms[0]->cpu_data<float>();
  float *conf_data = _bottoms[1]->cpu_data<float>();
  float *prior_data = reinterpret_cast<float *>(_bottoms[2]->cpu_data<uint8_t>());

  std::vector<std::map<int, std::vector<std::pair<float, int>>>> all_conf_scores;
  // filter result by score, retun batched vector [{label0: [(score, detcetion_idx), ...],  ...}, ...]
  GetConfidenceScores_opt(conf_data, num, num_priors_, _num_classes, _obj_threshold,
                          &all_conf_scores);
  for (int i = 0; i < num; ++i) {
    for (int c = 0; c < _num_classes; ++c) {
      if (all_conf_scores[i].find(c) == all_conf_scores[i].end()) {
        continue;
      }
      std::vector<std::pair<float, int>> &scores = all_conf_scores[i].find(c)->second;
      // get topK in eatch class
      if (_top_k < (int)scores.size()) {
        std::partial_sort(scores.begin(), scores.begin() + _top_k, scores.end(),
                          SortScoreCmp0);
      } else {
        std::sort(scores.begin(), scores.end(), SortScoreCmp0);
      }
    }   // nofclass
  }   // batch

  // build keep for decode ,recode vilad index
  float *decode_keep_index;
  int buf_length = 0;
  if (_share_location) {
    buf_length = num * num_priors_;
  } else {
    buf_length = num * num_priors_ * _num_classes;
  }
  decode_keep_index = new float[buf_length];
  memset(decode_keep_index, 0, buf_length * 4);
  float *p = decode_keep_index;
  for (int i = 0; i < num; ++i) {
    if (_share_location) {
      p = decode_keep_index + num_priors_ * i;
    }
    for (int c = 0; c < _num_classes; ++c) {
      if (!_share_location) {
        p = decode_keep_index + num_priors_ * _num_classes * i + num_priors_ * c;
      }
      if (c == _background_label_id) {
        // Ignore background class.
        continue;
      }

      if (all_conf_scores[i].find(c) == all_conf_scores[i].end())
        continue;
      std::vector<std::pair<float, int>> &scores = all_conf_scores[i].find(c)->second;
      int length = _top_k < (int)scores.size() ? _top_k : scores.size();
      for (int k = 0; k < length; ++k) {
        p[scores[k].second] = 1;
      }
    }
  }

  // Retrieve all location predictions.
  std::vector<LabelBBox_l> all_loc_preds;
  GetLocPredictions_opt(loc_data, num, num_priors_, num_loc_classes, _share_location,
                        decode_keep_index, &all_loc_preds);

  // Decode all loc predictions to bboxes.
  std::vector<LabelBBox_l> all_decode_bboxes;
  DecodeBBoxesAll_opt(all_loc_preds, num_priors_, prior_data, num, _share_location,
                      num_loc_classes, _background_label_id, code_type_,
                      variance_encoded_in_target_, decode_keep_index, &all_decode_bboxes);
  delete[] decode_keep_index;

  int num_kept = 0;
  std::vector<std::map<int, std::vector<std::pair<float, int>>>> all_indices;
  for (int i = 0; i < num; ++i) {
    const LabelBBox_l &decode_bboxes = all_decode_bboxes[i];
    const std::map<int, std::vector<std::pair<float, int>>> &conf_scores =
        all_conf_scores[i];
    std::map<int, std::vector<std::pair<float, int>>> indices;
    int num_det = 0;
    for (int c = 0; c < _num_classes; ++c) {
      if (c == _background_label_id) {
        // Ignore background class.
        continue;
      }
      if (conf_scores.find(c) == conf_scores.end())
        continue;
      int label = _share_location ? -1 : c;
      if (decode_bboxes.find(label) == decode_bboxes.end()) {
        // Something bad happened if there are no predictions for current label.
        continue;
      }
      const std::vector<BBox_l> &bboxes = decode_bboxes.find(label)->second;
      const std::vector<std::pair<float, int>> &aa = conf_scores.find(c)->second;
      ApplyNMSFast_opt(bboxes, aa, _nms_threshold, eta_, _top_k, &(indices[c]));

      num_det += indices[c].size();
    }

    if (_keep_topk > -1 && num_det > _keep_topk) {
      std::vector<std::pair<float, std::pair<int, int>>> score_index_pairs;
      for (auto it = indices.begin(); it != indices.end(); ++it) {
        int label = it->first;
        const std::vector<std::pair<float, int>> &label_indices = it->second;
        for (int j = 0; j < (int)label_indices.size(); ++j) {
          score_index_pairs.emplace_back(std::make_pair(
              label_indices[j].first, std::make_pair(label, label_indices[j].second)));
        }
      }
      // Keep top k results per image.
      std::sort(score_index_pairs.begin(), score_index_pairs.end(), SortScoreCmp1);
      score_index_pairs.resize(_keep_topk);
      // Store the new indices.
      std::map<int, std::vector<std::pair<float, int>>> new_indices;
      for (int j = 0; j < (int)score_index_pairs.size(); ++j) {

        int label = score_index_pairs[j].second.first;
        int idx = score_index_pairs[j].second.second;
        float s = score_index_pairs[j].first;

        new_indices[label].emplace_back(std::make_pair(s, idx));
      }
      all_indices.emplace_back(new_indices);
      num_kept += _keep_topk;
    } else {
      all_indices.emplace_back(indices);
      num_kept += num_det;
    }
  }

  int output_size = num * _keep_topk * 1 * 1 * 7;
  for (int i = 0; i < output_size; ++i) {
    top_data[i] = -1;
  }

  if (num_kept == 0) {
    // Generate fake results per image.
    for (int i = 0; i < num; ++i) {
      top_data[0] = i;
      top_data += 7;
    }
  } else {
    int count = 0;
    for (int i = 0; i < num; ++i) {
      const LabelBBox_l &decode_bboxes = all_decode_bboxes[i];
      for (auto it = all_indices[i].begin(); it != all_indices[i].end(); ++it) {
        int label = it->first;
        int loc_label = _share_location ? -1 : label;
        if (decode_bboxes.find(loc_label) == decode_bboxes.end()) {
          // Something bad happened if there are no predictions for current label.
          continue;
        }
        const std::vector<BBox_l> &bboxes = decode_bboxes.find(loc_label)->second;
        std::vector<std::pair<float, int>> &indices = it->second;
        for (int j = 0; j < (int)indices.size(); ++j) {
          int idx = indices[j].second;
          top_data[count * 7] = i;
          top_data[count * 7 + 1] = label;
          top_data[count * 7 + 2] = indices[j].first;
          const BBox_l &bbox = bboxes[idx];
          top_data[count * 7 + 3] = bbox.xy.s.xmin;
          top_data[count * 7 + 4] = bbox.xy.s.ymin;
          top_data[count * 7 + 5] = bbox.xy.s.xmax;
          top_data[count * 7 + 6] = bbox.xy.s.ymax;
          ++count;
        }
      }
    }
  }
}

} // namespace runtime
} // namespace cvi
