#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core_internel.hpp"
#include "cvi_comm.h"
#include "opencv2/core.hpp"

namespace cvitdl {

class SmokeClassification final : public Core {
 public:
  SmokeClassification();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta);

 private:
  void prepareInputTensor(cv::Mat &input_mat);
};
}  // namespace cvitdl
