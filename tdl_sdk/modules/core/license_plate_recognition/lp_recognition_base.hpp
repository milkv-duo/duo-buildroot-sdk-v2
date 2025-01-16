#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"
#include "decode_tool.hpp"

namespace cvitdl {

class LicensePlateRecognitionBase : public Core {
 public:
  LicensePlateRecognitionBase(CVI_MEM_TYPE_E CVI_MEM_SYSTEM) : Core(CVI_MEM_SYSTEM){};
  LicensePlateRecognitionBase(){};
  virtual ~LicensePlateRecognitionBase(){};
  virtual int inference(VIDEO_FRAME_INFO_S *frame, cvtdl_object_t *object_meta) = 0;
  virtual bool allowExportChannelAttribute() const override { return false; }
  int afterInference() { return 0; }
};

}  // namespace cvitdl
