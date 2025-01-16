#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class ImageClassification final : public Core {
 public:
  ImageClassification();
  virtual ~ImageClassification();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_class_meta_t *meta);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  virtual int onModelOpened() override;
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;
  void outputParser(cvtdl_class_meta_t *meta);
};
}  // namespace cvitdl
