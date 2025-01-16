#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class HandKeypoint final : public Core {
 public:
  HandKeypoint();
  virtual ~HandKeypoint();
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_handpose21_meta_ts *meta);

 private:
};
}  // namespace cvitdl