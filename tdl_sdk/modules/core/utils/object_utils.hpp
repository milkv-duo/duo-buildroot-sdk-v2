#pragma once
#include <memory>
#include <vector>

namespace cvitdl {

struct object_detect_rect_t {
  float x1;
  float y1;
  float x2;
  float y2;
  float score;
  int label;
};

typedef std::shared_ptr<object_detect_rect_t> PtrDectRect;
typedef std::vector<PtrDectRect> Detections;

Detections topk_dets(const Detections &dets, uint32_t max_det);
Detections nms_multi_class(const Detections &dets, float iou_threshold);
Detections nms_multi_class_with_ids(const Detections &dets, float iou_threshold,
                                    std::vector<int> &keep);

std::vector<std::vector<float>> generate_mmdet_base_anchors(float base_size, float center_offset,
                                                            const std::vector<float> &ratios,
                                                            const std::vector<int> &scales);
std::vector<std::vector<float>> generate_mmdet_grid_anchors(
    int feat_w, int feat_h, int stride, std::vector<std::vector<float>> &base_anchors);

void clip_bbox(const size_t image_width, const size_t image_height, const PtrDectRect &box);

}  // namespace cvitdl
