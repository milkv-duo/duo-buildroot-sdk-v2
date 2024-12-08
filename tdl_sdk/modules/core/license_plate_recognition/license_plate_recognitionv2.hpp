#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"
#include "lp_recognition_base.hpp"
namespace cvitdl {

/* WPODNet */
class LicensePlateRecognitionV2 final : public LicensePlateRecognitionBase {
 public:
  LicensePlateRecognitionV2();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta);
  void greedy_decode(float *prebs,cvtdl_vehicle_meta *v_meta);
  bool allowExportChannelAttribute() const override { return true; }
};
}  // namespace cvitdl
