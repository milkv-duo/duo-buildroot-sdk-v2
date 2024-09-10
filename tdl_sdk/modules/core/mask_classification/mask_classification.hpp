#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core_internel.hpp"

namespace cvitdl {

class MaskClassification final : public Core {
 public:
  MaskClassification();
  virtual ~MaskClassification();
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta);

 private:
};
}  // namespace cvitdl