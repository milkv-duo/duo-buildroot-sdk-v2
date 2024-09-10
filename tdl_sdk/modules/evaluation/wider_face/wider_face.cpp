#include "wider_face.hpp"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <experimental/filesystem>
#include "core/core/cvtdl_errno.h"
#include "core_utils.hpp"
#include "cvi_comm.h"
#include "cvi_tdl_log.hpp"

namespace cvitdl {
namespace evaluation {

widerFaceEval::widerFaceEval(const char* dataset_dir, const char* result_dir) {
  getEvalData(dataset_dir, result_dir);
}

int widerFaceEval::getEvalData(const char* dataset_dir, const char* result_dir) {
  m_dataset_dir = dataset_dir;
  m_result_dir = result_dir;
  if (std::experimental::filesystem::exists(m_result_dir)) {
    std::experimental::filesystem::remove_all(m_result_dir.c_str());
    m_last_result_group_path = "";
  }
  std::experimental::filesystem::create_directory(m_result_dir.c_str());
  m_data.clear();

  std::string dataset_path = m_dataset_dir + "/images";
  for (const auto& entry : std::experimental::filesystem::directory_iterator(dataset_path)) {
    if (!std::experimental::filesystem::is_directory(entry)) {
      continue;
    }
    std::string result_group_path = m_result_dir + "/" + entry.path().filename().c_str();
    for (const auto& group_entry : std::experimental::filesystem::directory_iterator(entry)) {
      if (!std::experimental::filesystem::is_regular_file(group_entry)) {
        continue;
      }
      m_data.push_back({entry.path(), result_group_path, group_entry.path().filename()});
    }
  }
  return CVI_TDL_SUCCESS;
}

uint32_t widerFaceEval::getTotalImage() { return m_data.size(); }

void widerFaceEval::getImageFilePath(const int index, std::string* filepath) {
  *filepath = m_data[index].group_path + "/" + m_data[index].group_entry_d_name;
}

int widerFaceEval::saveFaceData(const int index, const VIDEO_FRAME_INFO_S* frame,
                                const cvtdl_face_t* face) {
  auto& result_group_path = m_data[index].result_group_path;
  if (m_last_result_group_path != result_group_path) {
    if (!std::experimental::filesystem::exists(result_group_path)) {
      std::experimental::filesystem::create_directory(result_group_path);
    }
  }
  m_last_result_group_path = result_group_path;

  std::experimental::filesystem::path exfp = m_data[index].group_entry_d_name;
  std::string result_path = result_group_path + "/" + exfp.stem().c_str() + ".txt";

  FILE* fp;
  if ((fp = fopen(result_path.c_str(), "w+")) == NULL) {
    LOGE("Write file open error. %s!\n", result_path.c_str());
    return CVI_TDL_FAILURE;
  }

  fprintf(fp, "%s\n", exfp.stem().c_str());
  fprintf(fp, "%d\n", face->size);
  for (uint32_t i = 0; i < face->size; i++) {
    cvtdl_bbox_t bbox =
        box_rescale(frame->stVFrame.u32Width, frame->stVFrame.u32Height, face->width, face->height,
                    face->info[i].bbox, meta_rescale_type_e::RESCALE_CENTER);
    fprintf(fp, "%f %f %f %f %f\n", bbox.x1, bbox.y1, bbox.x2 - bbox.x1, bbox.y2 - bbox.y1,
            bbox.score);
  }
  fclose(fp);
  return CVI_TDL_SUCCESS;
}

void widerFaceEval::resetData() { m_data.clear(); }

}  // namespace evaluation
}  // namespace cvitdl
