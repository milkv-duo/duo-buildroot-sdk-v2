#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "cvi_comm.h"
#include "opencv2/core.hpp"

namespace cvitdl {

class YawnClassification final : public Core {
 public:
  YawnClassification();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta);

 private:
  void prepareInputTensor(cv::Mat &input_mat);
};
}  // namespace cvitdl
