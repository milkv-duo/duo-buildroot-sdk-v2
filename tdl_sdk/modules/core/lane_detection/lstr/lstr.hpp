#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"

namespace cvitdl {
class LSTR final : public Core {
 public:
  LSTR();
  virtual ~LSTR();
  float gen_x_by_y(float ys, std::vector<float> &point_line);
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_lane_t *lane_meta);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  // int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
  //                    VPSSConfig &vpss_config) override;

  int outputParser(cvtdl_lane_t *lane_meta);
};

}  // namespace cvitdl