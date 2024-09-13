#pragma once
#include <bitset>
#include <limits>
#include <memory>
#include <vector>

#include "anchors.hpp"
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"
#include "object_utils.hpp"

namespace cvitdl {

class MobileDetV2 final : public DetectionBase {
 public:
  enum class Category {
    coco80,          // COCO 80 classes
    vehicle,         // CAR, TRUCK, MOTOCYCLE
    pedestrian,      // Pedestrian
    person_pets,     // PERSON, DOG, and CAT
    person_vehicle,  // PERSON, CAR, BICYCLE, MOTOCYCLE, BUS, TRUCK
  };

  struct CvimodelInfo {
    MobileDetV2::Category category;
    size_t min_level;
    size_t max_level;
    size_t num_scales;
    std::vector<std::pair<float, float>> aspect_ratios;
    float anchor_scale;

    size_t image_width;
    size_t image_height;
    int num_classes;
    std::vector<int> strides;
    std::map<int, std::string> class_out_names;
    std::map<int, std::string> bbox_out_names;
    std::map<int, std::string> obj_max_names;
    std::map<int, float> class_dequant_thresh;
    std::map<int, float> bbox_dequant_thresh;
    float default_score_threshold;

    typedef int (*ClassMapFunc)(int);
    ClassMapFunc class_id_map;
    static CvimodelInfo create_config(MobileDetV2::Category model);
  };

  explicit MobileDetV2(MobileDetV2::Category category, float iou_thresh = 0.5);
  ~MobileDetV2(){};
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *meta) override;
  int onModelOpened() override;
  void select_classes(const std::vector<uint32_t> &selected_classes);
  void setModelThreshold(const float &threshold) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  void get_raw_outputs(std::vector<std::pair<int8_t *, size_t>> *cls_tensor_ptr,
                       std::vector<std::pair<int8_t *, size_t>> *objectness_tensor_ptr,
                       std::vector<std::pair<int8_t *, size_t>> *bbox_tensor_ptr);
  void generate_dets_for_each_stride(Detections *det_vec);
  void generate_dets_for_tensor(Detections *det_vec, float class_dequant_thresh,
                                float bbox_dequant_thresh, int8_t quant_thresh,
                                const int8_t *logits, const int8_t *objectness, int8_t *bboxes,
                                size_t class_tensor_size, const std::vector<AnchorBox> &anchors);
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  std::vector<std::vector<AnchorBox>> m_anchors;
  CvimodelInfo m_model_config;
  float m_iou_threshold;

  // score threshold for quantized inverse threshold
  std::vector<int8_t> m_quant_inverse_score_threshold;
  std::bitset<CVI_TDL_DET_TYPE_END> m_filter;
};

}  // namespace cvitdl
