#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class IspImageClassification final : public Core {
 public:
  IspImageClassification();
  virtual ~IspImageClassification();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_class_meta_t *meta, cvtdl_isp_meta_t *isparg);
  virtual bool allowExportChannelAttribute() const override { return true; }
  std::vector<int> TopKIndex(std::vector<float> &vec, int topk);

 private:
#ifdef __CV186X__
  virtual int onModelOpened() override;
  virtual int onModelClosed() override;
#endif
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;
  void outputParser(cvtdl_class_meta_t *meta);
  bool use_input_num_check_ = false;
};
}  // namespace cvitdl
