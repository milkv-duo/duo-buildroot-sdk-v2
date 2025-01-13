#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"

namespace cvitdl {

class IrLiveness final : public Core {
 public:
  IrLiveness();
  virtual ~IrLiveness();
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_face_t *meta);

 private:
};
}  // namespace cvitdl