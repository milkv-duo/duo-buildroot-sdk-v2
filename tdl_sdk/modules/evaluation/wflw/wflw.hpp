#pragma once
#include "core/face/cvtdl_face_types.h"
#include "core/utils/vpss_helper.h"

#include <string>
#include <vector>

namespace cvitdl {
namespace evaluation {

class wflwEval {
 public:
  wflwEval(const char *fiilepath);
  int getEvalData(const char *fiilepath);
  uint32_t getTotalImage();
  void getImage(const int index, std::string *name);
  void insertPoints(const int index, const cvtdl_pts_t points, const int width, const int height);
  void distance();

 private:
  std::vector<std::string> m_img_name;
  std::vector<std::vector<float>> m_img_points;
  std::vector<std::vector<float>> m_result_points;
};
}  // namespace evaluation
}  // namespace cvitdl