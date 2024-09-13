#pragma once

#include <fstream>
#include "json.hpp"

namespace cvitdl {
namespace evaluation {

class LPDREval {
 public:
  LPDREval(const char *path_prefix, const char *json_path);
  ~LPDREval();
  uint32_t getTotalImage();
  void getEvalData(const char *path_prefix, const char *json_path);
  void getImageIdPair(const int index, std::string *path, int *id);

 private:
  const char *m_path_prefix;
  nlohmann::json m_json_read;
  std::ofstream m_ofs_results;
};
}  // namespace evaluation
}  // namespace cvitdl