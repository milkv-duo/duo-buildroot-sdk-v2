#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"

namespace cvitdl {

class Deeplabv3 final : public Core {
 public:
  Deeplabv3();
  virtual ~Deeplabv3();
  int inference(VIDEO_FRAME_INFO_S *frame, VIDEO_FRAME_INFO_S *out_frame,
                cvtdl_class_filter_t *filter);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  virtual int onModelOpened() override;
  virtual int onModelClosed() override;
  CVI_S32 allocateION();
  void releaseION();
  int outputParser(cvtdl_class_filter_t *filter);
  VIDEO_FRAME_INFO_S m_label_frame;
};
}  // namespace cvitdl
