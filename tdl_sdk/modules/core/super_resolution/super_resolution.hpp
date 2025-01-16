#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class SuperResolution final : public Core {
 public:
  SuperResolution();
  virtual ~SuperResolution();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_sr_feature *srfeature);

 private:
  void outputParser(VIDEO_FRAME_INFO_S *dstFrame);
};
}  // namespace cvitdl
