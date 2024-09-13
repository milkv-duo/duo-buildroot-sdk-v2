#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"

namespace cvitdl {

class RawImageClassification final : public Core {
 public:
  RawImageClassification();
  virtual ~RawImageClassification();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_class_meta_t *meta);
  virtual bool allowExportChannelAttribute() const override { return true; }
  std::vector<int> TopKIndex(std::vector<float> &vec, int topk);

 private:
  virtual int onModelOpened() override;
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  void outputParser(cvtdl_class_meta_t *meta);
};
}  // namespace cvitdl
