#pragma once
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "cvi_comm.h"

namespace cvitdl {

class FaceQuality final : public Core {
 public:
  FaceQuality();
  virtual ~FaceQuality();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *meta, bool *skip);

 private:
  virtual int onModelOpened() override;
  virtual int onModelClosed() override;
  CVI_S32 allocateION();
  void releaseION();

  VIDEO_FRAME_INFO_S m_wrap_frame;
};
}  // namespace cvitdl
