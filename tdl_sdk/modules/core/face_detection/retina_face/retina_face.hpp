#pragma once
#include "core/face/cvtdl_face_types.h"
#include "face_detection.hpp"

#include "anchor_generator.h"

namespace cvitdl {

class RetinaFace final : public FaceDetectionBase {
 public:
  RetinaFace(PROCESS process);
  ~RetinaFace(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  int onModelOpened() override;
  void outputParser(int image_width, int image_height, int frame_width, int frame_height,
                    cvtdl_face_t *meta);
  std::vector<int> m_feat_stride_fpn;
  std::map<std::string, std::vector<anchor_box>> m_anchors;
  std::map<std::string, int> m_num_anchors;
  PROCESS process_;
};
}  // namespace cvitdl