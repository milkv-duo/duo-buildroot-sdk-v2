
#include "object_utils.hpp"
#include "core/object/cvtdl_object_types.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
using namespace std;

namespace cvitdl {

static vector<size_t> sort_indexes(const Detections &v) {
  // initialize original index locations
  vector<size_t> idx(v.size());
  iota(idx.begin(), idx.end(), 0);

  // sort indexes based on comparing values in v
  // using std::stable_sort instead of std::sort
  // to avoid unnecessary index re-orderings
  // when v contains elements of equal values
  stable_sort(idx.begin(), idx.end(),
              [&v](size_t i1, size_t i2) { return v[i1]->score > v[i2]->score; });
  return idx;
}

static vector<size_t> calculate_area(const Detections &dets) {
  vector<size_t> areas(dets.size());
  for (size_t i = 0; i < dets.size(); i++) {
    areas[i] = (dets[i]->x2 - dets[i]->x1) * (dets[i]->y2 - dets[i]->y1);
  }
  return areas;
}

Detections topk_dets(const Detections &dets, uint32_t max_det) {
  vector<size_t> order = sort_indexes(dets);

  uint32_t num_to_keep = dets.size() > max_det ? max_det : dets.size();
  Detections final_dets(num_to_keep);
  for (size_t k = 0; k < num_to_keep; k++) {
    final_dets[k] = dets[k];
  }
  return final_dets;
}

Detections nms_multi_class(const Detections &dets, float iou_threshold) {
  vector<int> keep(dets.size(), 0);
  vector<int> suppressed(dets.size(), 0);

  size_t ndets = dets.size();
  size_t num_to_keep = 0;

  vector<size_t> order = sort_indexes(dets);
  vector<size_t> areas = calculate_area(dets);

  for (size_t _i = 0; _i < ndets; _i++) {
    auto i = order[_i];
    if (suppressed[i] == 1) continue;
    keep[num_to_keep++] = i;
    auto ix1 = dets[i]->x1;
    auto iy1 = dets[i]->y1;
    auto ix2 = dets[i]->x2;
    auto iy2 = dets[i]->y2;
    auto iarea = areas[i];

    for (size_t _j = _i + 1; _j < ndets; _j++) {
      auto j = order[_j];
      if (suppressed[j] == 1) continue;
      auto xx1 = std::max(ix1, dets[j]->x1);
      auto yy1 = std::max(iy1, dets[j]->y1);
      auto xx2 = std::min(ix2, dets[j]->x2);
      auto yy2 = std::min(iy2, dets[j]->y2);

      auto w = std::max(0.0f, xx2 - xx1);
      auto h = std::max(0.0f, yy2 - yy1);
      auto inter = w * h;
      float ovr = static_cast<float>(inter) / (iarea + areas[j] - inter);
      if (ovr > iou_threshold && dets[j]->label == dets[i]->label) suppressed[j] = 1;
    }
  }

  Detections final_dets(num_to_keep);
  for (size_t k = 0; k < num_to_keep; k++) {
    final_dets[k] = dets[keep[k]];
  }
  return final_dets;
}

Detections nms_multi_class_with_ids(const Detections &dets, float iou_threshold,
                                    vector<int> &keep) {
  vector<int> suppressed(dets.size(), 0);

  size_t ndets = dets.size();
  size_t num_to_keep = 0;

  vector<size_t> order = sort_indexes(dets);
  vector<size_t> areas = calculate_area(dets);

  for (size_t _i = 0; _i < ndets; _i++) {
    auto i = order[_i];
    if (suppressed[i] == 1) continue;
    keep[num_to_keep++] = i;
    auto ix1 = dets[i]->x1;
    auto iy1 = dets[i]->y1;
    auto ix2 = dets[i]->x2;
    auto iy2 = dets[i]->y2;
    auto iarea = areas[i];

    for (size_t _j = _i + 1; _j < ndets; _j++) {
      auto j = order[_j];
      if (suppressed[j] == 1) continue;
      auto xx1 = std::max(ix1, dets[j]->x1);
      auto yy1 = std::max(iy1, dets[j]->y1);
      auto xx2 = std::min(ix2, dets[j]->x2);
      auto yy2 = std::min(iy2, dets[j]->y2);

      auto w = std::max(0.0f, xx2 - xx1);
      auto h = std::max(0.0f, yy2 - yy1);
      auto inter = w * h;
      float ovr = static_cast<float>(inter) / (iarea + areas[j] - inter);
      if (ovr > iou_threshold && dets[j]->label == dets[i]->label) suppressed[j] = 1;
    }
  }

  Detections final_dets(num_to_keep);
  for (size_t k = 0; k < num_to_keep; k++) {
    final_dets[k] = dets[keep[k]];
  }
  return final_dets;
}

// x1,y1,x2,y2
std::vector<std::vector<float>> generate_mmdet_base_anchors(float base_size, float center_offset,
                                                            const std::vector<float> &ratios,
                                                            const std::vector<int> &scales) {
  std::vector<std::vector<float>> base_anchors;
  float x_center = base_size * center_offset;
  float y_center = base_size * center_offset;

  for (size_t i = 0; i < ratios.size(); i++) {
    float h_ratio = sqrt(ratios[i]);
    float w_ratio = 1 / h_ratio;
    for (size_t j = 0; j < scales.size(); j++) {
      float halfw = base_size * w_ratio * scales[j] / 2;
      float halfh = base_size * h_ratio * scales[j] / 2;
      // x1,y1,x2,y2
      std::vector<float> base_anchor = {x_center - halfw, y_center - halfh, x_center + halfw,
                                        y_center + halfh};
      std::cout << "anchor:" << base_anchor[0] << "," << base_anchor[1] << "," << base_anchor[2]
                << "," << base_anchor[3] << std::endl;
      base_anchors.emplace_back(base_anchor);
    }
  }
  return base_anchors;
}
// x1,y1,x2,y2
std::vector<std::vector<float>> generate_mmdet_grid_anchors(
    int feat_w, int feat_h, int stride, std::vector<std::vector<float>> &base_anchors) {
  std::vector<std::vector<float>> grid_anchors;
  for (size_t k = 0; k < base_anchors.size(); k++) {
    auto &base_anchor = base_anchors[k];
    for (int ih = 0; ih < feat_h; ih++) {
      int sh = ih * stride;
      for (int iw = 0; iw < feat_w; iw++) {
        int sw = iw * stride;
        std::vector<float> grid_anchor = {base_anchor[0] + sw, base_anchor[1] + sh,
                                          base_anchor[2] + sw, base_anchor[3] + sh};
        // if (grid_anchors.size() < 10)
        //   std::cout << "gridanchor:" << grid_anchor[0] << "," << grid_anchor[1] << ","
        //             << grid_anchor[2] << "," << grid_anchor[3] << std::endl;
        grid_anchors.emplace_back(grid_anchor);
      }
    }
  }
  return grid_anchors;
}

void clip_bbox(const size_t image_width, const size_t image_height, const PtrDectRect &box) {
  if (box->x1 < 0) box->x1 = 0;
  if (box->y1 < 0) box->y1 = 0;
  if (box->x2 < 0) box->x2 = 0;
  if (box->y2 < 0) box->y2 = 0;

  if (box->x1 >= image_width) box->x1 = image_width - 1;
  if (box->y1 >= image_height) box->y1 = image_height - 1;
  if (box->x2 >= image_width) box->x2 = image_width - 1;
  if (box->y2 >= image_height) box->y2 = image_height - 1;
}
}  // namespace cvitdl
