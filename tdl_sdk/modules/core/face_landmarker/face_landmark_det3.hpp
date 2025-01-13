#pragma once
#include <bitset>
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"

namespace cvitdl {

class FaceLandmarkDet3 final : public Core {
 public:
  FaceLandmarkDet3();
  virtual ~FaceLandmarkDet3();
  int inference(VIDEO_FRAME_INFO_S *srcFrame, cvtdl_face_t *meta);

 private:
  virtual int onModelOpened() override;

  void outputParser(const int image_width, const int image_height, const int frame_width,
                    const int frame_height, cvtdl_face_t *meta);

  std::map<std::string, std::string> out_names_;
};
}  // namespace cvitdl
