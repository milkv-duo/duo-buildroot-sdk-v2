#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

typedef std::pair<int, int> PAIR_INT;

class YoloV10Detection final : public DetectionBase {
 public:
  YoloV10Detection();
  YoloV10Detection(PAIR_INT yolov8_pair);

  ~YoloV10Detection(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  int onModelOpened() override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);

  void parseDecodeBranch(const int image_width, const int image_height, const int frame_width,
                         const int frame_height, cvtdl_object_t *obj_meta);

  void decode_bbox_feature_map(int stride, int anchor_idx, std::vector<float> &decode_box);
  void postProcess(Detections &dets, int frame_width, int frame_height, cvtdl_object_t *obj_meta);
  std::map<std::string, std::string> out_names_;

  // if output seperate featuremap
  std::vector<int> strides;
  std::map<int, std::string> class_out_names;
  std::map<int, std::string> bbox_out_names;
  std::map<int, std::string> bbox_class_out_names;
  int m_box_channel_ = 0;
  int m_cls_channel_ = 0;
};
}  // namespace cvitdl
