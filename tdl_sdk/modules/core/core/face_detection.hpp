#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class FaceDetectionBase : public Core {
 public:
  FaceDetectionBase() : Core(CVI_MEM_DEVICE){};
  virtual ~FaceDetectionBase(){};
  virtual int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta) {
    LOGE("inference function not implement!\n");
    return 0;
  }

 private:
  cvtdl_det_algo_param_t alg_param_;
};
}  // namespace cvitdl