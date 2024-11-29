#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

class Yolov6 final : public DetectionBase {
 public:
  Yolov6();
  ~Yolov6();

  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj_meta) override;

 private:
  int onModelOpened() override;

  void decode_bbox_feature_map(int stride, int anchor_idx, std::vector<float> &decode_box);
  void clip_bbox(int frame_width, int frame_height, cvtdl_bbox_t *bbox);
  cvtdl_bbox_t boxRescale(int frame_width, int frame_height, int width, int height,
                          cvtdl_bbox_t bbox);
  void postProcess(Detections &dets, int frame_width, int frame_height, cvtdl_object_t *obj_meta);
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_hegiht, cvtdl_object_t *obj_meta);
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  std::map<int, std::string> class_out_names_;
  std::map<int, std::string> box_out_names_;
  std::vector<int> strides_;
};
}  // namespace cvitdl