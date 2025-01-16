#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class Clip_Text final : public Core {
 public:
  Clip_Text();
  virtual ~Clip_Text();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_clip_feature *clip_feature);
};
}  // namespace cvitdl