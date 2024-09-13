#pragma once
#include <cnpy.h>
#include <cviruntime.h>
#include <memory>
#include <string>
#include <vector>
#include "cvi_comm.h"
#include "cvi_tdl_log.hpp"

namespace cvitdl {
namespace debug {

class ModelDebugger {
 public:
  ModelDebugger();
  ModelDebugger(const ModelDebugger &) = delete;
  ModelDebugger &operator=(const ModelDebugger &) = delete;

  void setDirPath(const std::string &dir_path);

  void newSession(const std::string &classname);

  void setEnable(bool enable) { m_enable = enable; }

  void save_origin_frame(VIDEO_FRAME_INFO_S *frame, CVI_TENSOR *tensor);

  void save_tensor(CVI_TENSOR *tensor, void *ptr);

  void save_normalizer(CVI_TENSOR *tensor, const VPSS_NORMALIZE_S &normalize);

  bool isEnable() const { return m_enable; }

  std::string getDirPath() const { return m_dir_path; }

  template <typename T>
  void save_field(std::string field_name, const T *data, const std::vector<size_t> &shape);

  template <typename T>
  void save_field(std::string field_name, const T data);

 private:
  bool checkFileNode();

  std::string m_dir_path;
  std::string m_current_file_path;
  std::string m_filenode;
  bool m_ready;
  bool m_enable;
};

template <typename T>
void ModelDebugger::save_field(std::string field_name, const T *data,
                               const std::vector<size_t> &shape) {
  if (m_enable && m_ready) {
    cnpy::npz_save(m_current_file_path, field_name, data, shape, "a");
  }
}

template <typename T>
void ModelDebugger::save_field(std::string field_name, const T data) {
  if (m_enable && m_ready) {
    cnpy::npz_save(m_current_file_path, field_name, &data, {1}, "a");
  }
}
}  // namespace debug
}  // namespace cvitdl
