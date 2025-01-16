#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"

#include "opencv2/core.hpp"

namespace cvitdl {

class Liveness final : public Core {
 public:
  Liveness();
  int inference(VIDEO_FRAME_INFO_S *rgbFrame, VIDEO_FRAME_INFO_S *irFrame, cvtdl_face_t *rgb_meta,
                cvtdl_face_t *ir_meta);

 private:
  void prepareInputTensor(std::vector<cv::Mat> &input_mat);
};
}  // namespace cvitdl