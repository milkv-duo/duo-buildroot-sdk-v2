#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "object_utils.hpp"
#include "pose_detection.hpp"

namespace cvitdl {

typedef std::tuple<int, int, int> TUPLE_INT;

class YoloV8Pose final : public PoseDetectionBase {
 public:
  YoloV8Pose();
  YoloV8Pose(TUPLE_INT pose_pair);

  ~YoloV8Pose(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  int onModelOpened() override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj_meta);

  void decode_bbox_feature_map(int stride, int anchor_idx, std::vector<float> &decode_box);
  void decode_keypoints_feature_map(int stride, int anchor_idx, std::vector<float> &decode_kpts);

  void postProcess(Detections &dets, int frame_width, int frame_height, cvtdl_object_t *obj,
                   std::vector<std::pair<int, int>> &valild_pairs);

  std::vector<int> strides;
  std::map<int, std::string> bbox_out_names;
  std::map<int, std::string> class_out_names;
  std::map<int, std::string> keypoints_out_names;
  int m_box_channel_ = 0;
  int m_kpts_channel_ = 0;
  int m_cls_channel_ = 0;
};
}  // namespace cvitdl
