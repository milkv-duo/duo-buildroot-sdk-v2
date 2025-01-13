#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class OSNet final : public Core {
 public:
  explicit OSNet();
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *meta, int obj_idx = -1);

 private:
};
}  // namespace cvitdl