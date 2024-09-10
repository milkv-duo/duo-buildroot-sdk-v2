#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core/utils/vpss_helper.h"

#include <string>
#include <vector>

namespace cvitdl {
namespace evaluation {

class cityscapesEval {
 public:
  cityscapesEval(const char *image_dir, const char *output_dir);
  void writeResult(VIDEO_FRAME_INFO_S *label_frame, const int index);
  void getImage(int index, std::string &image_path);
  void getImageNum(uint32_t *num);

 private:
  std::string m_output_dir;
  std::vector<std::string> m_images;
};
}  // namespace evaluation
}  // namespace cvitdl