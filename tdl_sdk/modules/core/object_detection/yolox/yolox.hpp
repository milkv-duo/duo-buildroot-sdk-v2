#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

class YoloX final : public DetectionBase {
 public:
  YoloX();
  ~YoloX();

  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  int onModelOpened() override;
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);
  void generate_yolox_proposals(Detections &detections);

  std::vector<int> strides_;
  std::map<int, std::string> class_out_names_;
  std::map<int, std::string> object_out_names_;
  std::map<int, std::string> box_out_names_;
};
}  // namespace cvitdl