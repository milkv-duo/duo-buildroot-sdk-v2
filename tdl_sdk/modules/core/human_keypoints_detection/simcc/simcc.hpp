#pragma once
#include "core/face/cvtdl_face_types.h"
#include "pose_detection.hpp"

namespace cvitdl {

class Simcc final : public PoseDetectionBase {
 public:
  Simcc();
  ~Simcc(){};
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *obj_meta) override;

 private:
  void outputParser(const float nn_width, const float nn_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj, std::vector<float> &box,
                    int index);
};
}  // namespace cvitdl