#include "cvi_lpdr.hpp"

#include <string>

namespace cvitdl {
namespace evaluation {

LPDREval::LPDREval(const char *path_prefix, const char *json_path) {
  getEvalData(path_prefix, json_path);
}

LPDREval::~LPDREval() {}

void LPDREval::getEvalData(const char *path_prefix, const char *json_path) {
  m_path_prefix = path_prefix;
  m_json_read.clear();
  std::ifstream filestr(json_path);
  filestr >> m_json_read;
  filestr.close();
}

void LPDREval::getImageIdPair(const int index, std::string *path, int *id) {
  *path = m_path_prefix + std::string("/") + std::string(m_json_read["images"][index]["file_name"]);
  *id = m_json_read["images"][index]["id"];
}

uint32_t LPDREval::getTotalImage() { return m_json_read["images"].size(); }

}  // namespace evaluation
}  // namespace cvitdl