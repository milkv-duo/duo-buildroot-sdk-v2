#pragma once
#include "core/object/cvtdl_object_types.h"
#include "obj_detection.hpp"

namespace cvitdl {

class ThermalPerson final : public DetectionBase {
 public:
  ThermalPerson();
  ~ThermalPerson();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_object_t *obj) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  //   // @attention 原本该类缺少onModelOpened方法，即模型打开时不做任何处理
  int onModelOpened() override { return CVI_TDL_SUCCESS; }
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj);
};
}  // namespace cvitdl