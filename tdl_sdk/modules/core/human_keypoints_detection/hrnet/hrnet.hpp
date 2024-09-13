#pragma once
#include "core/face/cvtdl_face_types.h"
#include "pose_detection.hpp"

namespace cvitdl {
class Hrnet final : public PoseDetectionBase {
 public:
  Hrnet();
  ~Hrnet(){};
  int inference(VIDEO_FRAME_INFO_S *stOutFrame, cvtdl_object_t *obj_meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  void outputParser(const float nn_width, const float nn_height, const int frame_width,
                    const int frame_height, cvtdl_object_t *obj, std::vector<float> &box,
                    int index);
  template <typename T>
  void getMaxPreds(int batch_size, int num_joints, int height, int width, T *ptr_data,
                   std::vector<float> &preds, std::vector<float> &maxvals, float qscale);

  std::vector<float> preds;
  std::vector<float> maxvals;
};
}  // namespace cvitdl