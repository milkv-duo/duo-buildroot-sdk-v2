#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core_internel.hpp"
#include "opencv2/core.hpp"

namespace cvitdl {

class EyeClassification final : public Core {
 public:
  EyeClassification();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta);

 private:
  void prepareInputTensor(cv::Mat &input_mat);
};
}  // namespace cvitdl
