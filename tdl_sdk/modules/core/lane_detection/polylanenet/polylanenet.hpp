#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {
class Polylanenet final : public Core {
 public:
  Polylanenet();
  virtual ~Polylanenet();
  void set_lower(float th);
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_lane_t *lane_meta);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  // int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
  //                    VPSSConfig &vpss_config) override;

  int outputParser(cvtdl_lane_t *lane_meta);
  float ld_sigmoid(float x);
  int NUM_POINTS = 56;
  float LOWERE = 0.4;
};

}  // namespace cvitdl