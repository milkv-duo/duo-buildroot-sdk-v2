#include "coco.hpp"
#include "cvi_tdl_log.hpp"

#include <string>

namespace cvitdl {
namespace evaluation {
CocoEval::CocoEval(const char *path_prefix, const char *json_path) : m_det_count(0) {
  getEvalData(path_prefix, json_path);
}

CocoEval::~CocoEval() {}

void CocoEval::start_eval(const char *out_path) {
  if (m_ofs_results.is_open()) {
    m_ofs_results.close();
  }

  m_ofs_results.open(out_path);
  if (m_ofs_results.is_open()) {
    m_ofs_results << "[" << std::endl;
  }
  m_det_count = 0;
}

void CocoEval::getEvalData(const char *path_prefix, const char *json_path) {
  m_path_prefix = path_prefix;
  m_dataset.clear();
  std::ifstream filestr(json_path);
  std::string fname;
  int id;
  while (filestr >> fname >> id) {
    m_dataset.push_back({fname, id});
  }
  filestr.close();
}

uint32_t CocoEval::getTotalImage() { return m_dataset.size(); }

void CocoEval::getImageIdPair(const int index, std::string *path, int *id) {
  *path = m_path_prefix + std::string("/") + m_dataset[index].first;
  *id = m_dataset[index].second;
}

void CocoEval::insertObjectData(const int id, const cvtdl_object_t *obj) {
  LOGD("Image id %d insert object %d\n", id, obj->size);
  nlohmann::json jobj;
  for (uint32_t j = 0; j < obj->size; j++) {
    cvtdl_object_info_t &info = obj->info[j];
    float width = info.bbox.x2 - info.bbox.x1;
    float height = info.bbox.y2 - info.bbox.y1;

    jobj["image_id"] = id;
    jobj["category_id"] = info.classes;
    jobj["bbox"] = {info.bbox.x1, info.bbox.y1, width, height};
    jobj["score"] = info.bbox.score;

    if (m_det_count > 0) {
      m_ofs_results << "," << std::endl;
    }
    m_ofs_results << jobj;
    m_det_count++;
    jobj.clear();
  }
}

void CocoEval::end_eval() {
  m_ofs_results << "]" << std::endl;
  m_ofs_results.close();
}
}  // namespace evaluation
}  // namespace cvitdl