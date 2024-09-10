#pragma once
#include "core/face/cvtdl_face_types.h"
#include "face_detection.hpp"

namespace cvitdl {

class RetinafaceYolox final : public FaceDetectionBase {
 public:
  RetinafaceYolox();
  ~RetinafaceYolox(){};
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *face_meta) override;
  bool allowExportChannelAttribute() const override { return true; }

 private:
  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_face_t *face_meta);
};
}  // namespace cvitdl