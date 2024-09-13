#pragma once
#include <fstream>
#include <vector>
#include "core/object/cvtdl_object_types.h"
#include "json.hpp"

namespace cvitdl {
namespace evaluation {
class CocoEval {
 public:
  CocoEval(const char *path_prefix, const char *json_path);
  void getEvalData(const char *path_prefix, const char *json_path);
  uint32_t getTotalImage();
  void getImageIdPair(const int index, std::string *path, int *id);
  void insertObjectData(const int id, const cvtdl_object_t *obj);
  void start_eval(const char *out_path);
  void end_eval();
  ~CocoEval();

 private:
  const char *m_path_prefix;
  std::vector<std::pair<std::string, int>> m_dataset;
  std::ofstream m_ofs_results;
  int m_det_count;
};
}  // namespace evaluation
}  // namespace cvitdl