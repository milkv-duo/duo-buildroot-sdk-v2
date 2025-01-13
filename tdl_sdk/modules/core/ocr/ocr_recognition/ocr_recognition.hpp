#pragma once
#include "core.hpp"
#include "core/object/cvtdl_object_types.h"
#include "cvi_comm.h"

namespace cvitdl {

class OCRRecognition final : public Core {
 public:
  OCRRecognition();
  virtual ~OCRRecognition();
  int inference(VIDEO_FRAME_INFO_S* frame, cvtdl_object_t* obj_meta);

 private:
  void greedy_decode(float* prebs, std::vector<std::string>& chars);
  std::pair<std::string, float> decode(const std::vector<int>& text_index,
                                       const std::vector<float>& text_prob,
                                       std::vector<std::string>& chars, bool is_remove_duplicate);
};
}  // namespace cvitdl
