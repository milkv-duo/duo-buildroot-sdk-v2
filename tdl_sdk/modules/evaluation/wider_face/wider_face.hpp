#pragma once
#include "core/face/cvtdl_face_types.h"

#include <string>
#include <vector>

namespace cvitdl {
namespace evaluation {

typedef struct {
  std::string group_path;
  std::string result_group_path;
  std::string group_entry_d_name;
} widerpair;

class widerFaceEval {
 public:
  widerFaceEval(const char *dataset_dir, const char *result_dir);
  int getEvalData(const char *dataset_dir, const char *result_dir);
  uint32_t getTotalImage();
  void getImageFilePath(const int index, std::string *filepath);
  int saveFaceData(const int index, const VIDEO_FRAME_INFO_S *frame, const cvtdl_face_t *face);
  void resetData();

 private:
  std::string m_dataset_dir;
  std::string m_result_dir;
  std::vector<widerpair> m_data;
  std::string m_last_result_group_path;
};
}  // namespace evaluation
}  // namespace cvitdl