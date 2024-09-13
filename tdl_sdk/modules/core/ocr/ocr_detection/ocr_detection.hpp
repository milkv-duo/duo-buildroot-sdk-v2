#pragma once
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"
#include "cvi_comm.h"

namespace cvitdl {

class OCRDetection final : public Core {
 public:
  OCRDetection();
  virtual ~OCRDetection();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *obj_meta);

 private:
  void outputParser(float thresh, float boxThresh, cvtdl_object_t *obj_meta);
};
}  // namespace cvitdl
