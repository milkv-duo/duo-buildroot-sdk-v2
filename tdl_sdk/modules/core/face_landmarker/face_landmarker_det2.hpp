#pragma once
#include <bitset>
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class FaceLandmarkerDet2 final : public Core {
 public:
  FaceLandmarkerDet2();
  virtual ~FaceLandmarkerDet2();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta);

 private:
  virtual int onModelOpened() override;
  int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
                     VPSSConfig &vpss_config) override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_face_t *meta);

  std::map<std::string, std::string> out_names_;
};
}  // namespace cvitdl
