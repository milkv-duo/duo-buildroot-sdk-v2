#pragma once
#include <bitset>
#include "core.hpp"
#include "core/face/cvtdl_face_types.h"
#include "cvi_comm.h"

namespace cvitdl {
class FaceAttribute_cls final : public Core {
 public:
  FaceAttribute_cls();
  virtual ~FaceAttribute_cls();
  int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_face_t *face_attribute_cls_meta);

 private:
  // int vpssPreprocess(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *dstFrame,
  //                    VPSSConfig &vpss_config) override;

  int outputParser(cvtdl_face_t *face_attribute_cls_meta);
};

}  // namespace cvitdl