#include "model_debugger.hpp"

#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <ctime>
#include <experimental/filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::experimental::filesystem;

namespace cvitdl {
namespace debug {

static bool isPathExist(const std::string &s) {
  struct stat buffer;
  return (stat(s.c_str(), &buffer) == 0);
}

static bool tryToMakeDir(const std::string &dirpath) {
  if (!isPathExist(dirpath)) {
    if (mkdir(dirpath.c_str(), 0777) == -1) {
      LOGE("failed to create \"%s\": %s\n", dirpath.c_str(), strerror(errno));
      return false;
    }
  }
  return true;
}

ModelDebugger::ModelDebugger() : m_dir_path("."), m_ready(false), m_enable(false) {}

bool ModelDebugger::checkFileNode() {
  std::string filenode = m_dir_path + std::string("/enable");
  if (isPathExist(filenode)) {
    std::string enable;
    std::ifstream file(filenode, std::ios::in);
    if (std::getline(file, enable)) {
      if (enable.compare("1") == 0) {
        return true;
      }
    }
    file.close();
  }
  return false;
}

void ModelDebugger::newSession(const std::string &classname) {
  m_ready = false;
  if (!m_enable) return;
  if (!checkFileNode()) return;

  if (!isPathExist(m_dir_path)) {
    LOGE("Directory isn't exists for model inference dump: %s\n", m_dir_path.c_str());
    return;
  }

  const auto now = std::chrono::system_clock::now();
  const auto nowAsTimeT = std::chrono::system_clock::to_time_t(now);
  const auto nowMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

  std::stringstream ss;
  auto tm = std::localtime(&nowAsTimeT);
  ss << std::put_time(tm, "%a %b %d %Y %T") << '.' << std::setfill('0') << std::setw(3)
     << nowMs.count();

  std::string timestamp = ss.str();

  ss.str("");
  ss << m_dir_path << "/" << classname;
  std::string dump_dir_path = ss.str();

  if (!tryToMakeDir(dump_dir_path)) {
    return;
  }

  ss.str("");
  ss << m_dir_path << "/" << classname << "/dump_" << tm->tm_year + 1900 << "-" << tm->tm_mon + 1
     << "-" << tm->tm_mday << "-" << tm->tm_hour << "-" << tm->tm_min << "-" << tm->tm_sec
     << std::setfill('0') << std::setw(3) << nowMs.count() << ".npz";

  m_current_file_path = ss.str();

  cnpy::npz_save(m_current_file_path, "timestamp", timestamp.c_str(), {timestamp.size()}, "w");
  LOGW("dump input for %s\n", classname.c_str());
  m_ready = true;
}

void ModelDebugger::setDirPath(const std::string &dir_path) {
  if (dir_path.at(dir_path.size() - 1) == '/') {
    m_dir_path = dir_path.substr(0, dir_path.size() - 1);
  } else {
    m_dir_path = dir_path;
  }

  m_ready = false;
}

void ModelDebugger::save_tensor(CVI_TENSOR *tensor, void *ptr) {
  if (m_enable && m_ready) {
    size_t count = CVI_NN_TensorCount(tensor);
    size_t bytes = CVI_NN_TensorSize(tensor);
    CVI_SHAPE shape = CVI_NN_TensorShape(tensor);

    size_t length = count / bytes;

    std::string tensor_name = CVI_NN_TensorName(tensor);
    std::string tensor_field = std::string("input_") + tensor_name;

    std::vector<size_t> save_shape = {(size_t)shape.dim[0], (size_t)shape.dim[1],
                                      (size_t)shape.dim[2], (size_t)shape.dim[3]};

    if (length == 1) {
      cnpy::npz_save(m_current_file_path, tensor_field, (int8_t *)ptr, save_shape, "a");
    } else if (length == 4) {
      cnpy::npz_save(m_current_file_path, tensor_field, (float *)ptr, save_shape, "a");
    }

    float quant_scale = CVI_NN_TensorQuantScale(tensor);
    std::string qscale_field = tensor_field + std::string("_qscale");
    cnpy::npz_save(m_current_file_path, qscale_field, &quant_scale, {1}, "a");
  }
}

void ModelDebugger::save_origin_frame(VIDEO_FRAME_INFO_S *frame, CVI_TENSOR *tensor) {
  if (m_enable && m_ready) {
    bool do_unmap = false;
    CVI_U32 u32MapSize =
        frame->stVFrame.u32Length[0] + frame->stVFrame.u32Length[1] + frame->stVFrame.u32Length[2];
    if (frame->stVFrame.pu8VirAddr[0] == NULL) {
      frame->stVFrame.pu8VirAddr[0] =
          (CVI_U8 *)CVI_SYS_Mmap(frame->stVFrame.u64PhyAddr[0], u32MapSize);
      ;
      frame->stVFrame.pu8VirAddr[1] = frame->stVFrame.pu8VirAddr[0] + frame->stVFrame.u32Length[0];
      frame->stVFrame.pu8VirAddr[2] = frame->stVFrame.pu8VirAddr[1] + frame->stVFrame.u32Length[1];
      do_unmap = true;
    }

    std::string tensor_name = CVI_NN_TensorName(tensor);
    std::string tensor_field = std::string("input_orig_") + tensor_name;
    cnpy::npz_save(m_current_file_path, tensor_field, (uint8_t *)frame->stVFrame.pu8VirAddr[0],
                   {(size_t)(frame->stVFrame.u32Height + frame->stVFrame.u32Height / 2),
                    (size_t)(frame->stVFrame.u32Width)},
                   "a");

    std::string frame_type_field = tensor_field + std::string("_frame_format");

    cnpy::npz_save(m_current_file_path, frame_type_field,
                   (uint32_t *)&frame->stVFrame.enPixelFormat, {1}, "a");

    if (do_unmap) {
      CVI_SYS_Munmap((void *)frame->stVFrame.pu8VirAddr[0], u32MapSize);
      frame->stVFrame.pu8VirAddr[0] = NULL;
    }
  }
}

void ModelDebugger::save_normalizer(CVI_TENSOR *tensor, const VPSS_NORMALIZE_S &normalize) {
  if (m_enable && m_ready) {
    std::string tensor_name = CVI_NN_TensorName(tensor);
    std::string tensor_field = std::string("input_") + tensor_name;

    std::string factor_field = tensor_field + std::string("_factor");
    cnpy::npz_save(m_current_file_path, factor_field, normalize.factor, {3}, "a");

    std::string mean_field = tensor_field + std::string("_mean");
    cnpy::npz_save(m_current_file_path, mean_field, normalize.mean, {3}, "a");
  }
}

}  // namespace debug
}  // namespace cvitdl
