#pragma once
#include "core/face/cvtdl_face_types.h"
#include "face_detection.hpp"

#include "anchor_generator.h"

namespace cvitdl {

class ScrFDFace : public FaceDetectionBase {
 public:
  ScrFDFace();
  ~ScrFDFace(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  int onModelOpened() override;
  void outputParser(int image_width, int image_height, int frame_width, int frame_height,
                    cvtdl_face_t *meta);
  std::vector<int> m_feat_stride_fpn;
  // std::map<std::string, std::vector<anchor_box>> m_anchors;
  // std::map<std::string, int> m_num_anchors;
  std::map<int, std::vector<std::vector<float>>> fpn_anchors_;
  std::map<int, std::map<std::string, std::string>>
      fpn_out_nodes_;  //{stride:{"box":"xxxx","score":"xxx","landmark":"xxxx"}}
  std::map<int, int> fpn_grid_anchor_num_;

  PROCESS process_;
};
}  // namespace cvitdl
