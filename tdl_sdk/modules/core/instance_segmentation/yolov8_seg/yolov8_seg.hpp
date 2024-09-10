#pragma once

#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"
namespace cvitdl {

typedef std::pair<int, int> PAIR_INT;

class YoloV8Seg final : public DetectionBase {
 public:
  YoloV8Seg();
  YoloV8Seg(PAIR_INT yolov8_pair);
  ~YoloV8Seg();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta);

 private:
  int onModelOpened() override;
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);
  void decode_bbox_feature_map(int stride, int anchor_idx, std::vector<float> &decode_box);
  void detPostProcess(Detections &dets, cvtdl_object_t *obj_meta,
                      std::vector<std::pair<int, int>> &final_dets_id);

  std::vector<int> strides;
  std::map<int, std::string> class_out_names;
  std::map<int, std::string> bbox_out_names;
  std::map<int, std::string> mask_out_names;
  std::map<int, std::string> proto_out_names;
  std::map<int, std::string> bbox_class_out_names;
  int m_box_channel_ = 64;
  int m_mask_channel_ = 32;
};
}  // namespace cvitdl