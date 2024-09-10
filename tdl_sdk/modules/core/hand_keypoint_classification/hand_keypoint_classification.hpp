#pragma once

#include <vector>
#include "Eigen/Core"
#include "core/object/cvtdl_object_types.h"
#include "core_internel.hpp"
typedef Eigen::Matrix<float, 1, Eigen::Dynamic, Eigen::RowMajor> Vectorf;

namespace cvitdl {

class HandKeypointClassification final : public Core {
 public:
  HandKeypointClassification();
  virtual ~HandKeypointClassification();
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_handpose21_meta_t *handpose);

 private:
  Vectorf _data;
};
}  // namespace cvitdl