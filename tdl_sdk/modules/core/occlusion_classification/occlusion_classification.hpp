#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {
class OcclusionClassification final : public Core {
 public:
  OcclusionClassification();
  virtual ~OcclusionClassification();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_class_meta_t *occlusion_classification_meta);

};
}  // namespace cvitdl