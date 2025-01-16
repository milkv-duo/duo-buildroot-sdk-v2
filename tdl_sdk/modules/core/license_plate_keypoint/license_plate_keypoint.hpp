#pragma once
#include "cvi_comm.h"
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class LicensePlateKeypoint final : public Core {
 public:
  LicensePlateKeypoint();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *vehicle_plate_meta);
  int outputParser(cvtdl_vehicle_meta *v_meta);
  virtual bool allowExportChannelAttribute() const override { return true; }
  int frame_h;
  int frame_w;
  cvtdl_object_info_t cur_obj_info;
};
}  // namespace cviai
