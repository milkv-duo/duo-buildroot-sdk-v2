#pragma once
#include <bitset>
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"

namespace cvitdl {

class BezierLaneNet final : public Core {
 public:
  BezierLaneNet();

  virtual ~BezierLaneNet();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_lane_t *lane_meta);
  virtual bool allowExportChannelAttribute() const override { return true; }

 private:
  // virtual int onModelOpened() override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_lane_t *lane);

  float c_matrix[56][4];
};
}  // namespace cvitdl