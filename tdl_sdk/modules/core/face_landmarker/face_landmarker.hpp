#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core_internel.hpp"
#include "cvi_comm.h"

namespace cvitdl {

class FaceLandmarker final : public Core {
 public:
  FaceLandmarker();
  virtual ~FaceLandmarker();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta);

 private:
  void Preprocessing(cvtdl_face_info_t *face_info, int *max_side, int img_width, int img_height);

  int landmark_num = 106;
};
}  // namespace cvitdl
