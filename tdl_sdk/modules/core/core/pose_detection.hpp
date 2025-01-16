#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class PoseDetectionBase : public Core {
 public:
  PoseDetectionBase() : Core(CVI_MEM_DEVICE){};
  ~PoseDetectionBase(){};
  virtual int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *obj_meta) {
    LOGE("inference function not implement!\n");
    return 0;
  }
  virtual bool allowExportChannelAttribute() const override { return true; }
};
}  // namespace cvitdl