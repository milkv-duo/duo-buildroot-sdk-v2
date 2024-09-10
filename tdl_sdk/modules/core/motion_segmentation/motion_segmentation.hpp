#pragma once

#include <vector>
#include "Eigen/Core"
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"

namespace cvitdl {

class MotionSegmentation final : public Core {
 public:
  MotionSegmentation();
  virtual ~MotionSegmentation();
  int inference(VIDEO_FRAME_INFO_S *input0, VIDEO_FRAME_INFO_S *input1,
                cvtdl_seg_logits_t *seg_logits);
  virtual bool allowExportChannelAttribute() const { return true; };

 private:
  // virtual int vpssPreprocess(VIDEO_FRAME_INFO_S* srcFrame, VIDEO_FRAME_INFO_S* dstFrame,
  //                             VPSSConfig& vpss_config) override;
};
}  // namespace cvitdl