#pragma once
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"
#include "cvi_comm.h"
#include "lp_recognition_base.hpp"
namespace cvitdl {

/* WPODNet */
class LicensePlateRecognitionV2 final : public LicensePlateRecognitionBase {
 public:
  LicensePlateRecognitionV2();

  ~LicensePlateRecognitionV2(){};
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_meta) override;
  void greedy_decode(float *prebs);
  bool allowExportChannelAttribute() const override { return true; }
};
}  // namespace cvitdl
