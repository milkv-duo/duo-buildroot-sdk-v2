#pragma once
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"

namespace cvitdl {

class Stereo final : public Core {
 public:
  Stereo();
  virtual ~Stereo();
  int inference(VIDEO_FRAME_INFO_S *frame1, VIDEO_FRAME_INFO_S *frame2, cvtdl_depth_logits_t *depth_logist);

//  private:
//   virtual int setupInputPreprocess(std::vector<InputPreprecessSetup> *data) override;
};
}  // namespace cvitdl