#include "market1501.hpp"

#include <dirent.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <algorithm>
#include <numeric>
#include <queue>
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_core.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "cvi_comm.h"

#define GALLERY_DIR "/bounding_box_test/"
#define QUERY_DIR "/query/"
#define MAX_RANK 50

static float cosine_distance(const cvtdl_feature_t *features1, const cvtdl_feature_t *features2) {
  int32_t value1 = 0, value2 = 0, value3 = 0;
  for (uint32_t i = 0; i < features1->size; i++) {
    value1 += (short)features1->ptr[i] * features1->ptr[i];
    value2 += (short)features2->ptr[i] * features2->ptr[i];
    value3 += (short)features1->ptr[i] * features2->ptr[i];
  }

  return 1 - ((float)value3 / (sqrt((double)value1) * sqrt((double)value2)));
}

namespace cvitdl {
namespace evaluation {

market1501Eval::market1501Eval(const char *img_dir) { getEvalData(img_dir); }

market1501Eval::~market1501Eval() { resetData(); }

int market1501Eval::getEvalData(const char *img_dir) {
  resetData();

  std::string root_dir = img_dir;
  std::string query_dir = root_dir + QUERY_DIR;
  std::string gallery_dir = root_dir + GALLERY_DIR;

  DIR *dirp;
  struct dirent *entry;
  dirp = opendir(query_dir.c_str());
  while ((entry = readdir(dirp)) != NULL) {
    if (entry->d_type != 8 && entry->d_type != 0) continue;
    if (0 == strcmp(strchr(entry->d_name, '.'), ".db")) continue;

    std::string img_name = entry->d_name;

    market_info info;
    info.cam_id = atoi(&img_name.substr(img_name.find("_") + 2)[0]) - 1;
    info.pid = std::stoi(img_name.substr(0, img_name.find("_")));
    info.img_path = query_dir + "/" + img_name;
    m_querys.push_back(info);
  }

  DIR *dirp_g;
  dirp_g = opendir(gallery_dir.c_str());
  while ((entry = readdir(dirp_g)) != NULL) {
    if (entry->d_type != 8 && entry->d_type != 0) continue;
    if (entry->d_name[0] == '-' && entry->d_name[1] == '1') continue;
    if (0 == strcmp(strchr(entry->d_name, '.'), ".db")) continue;

    std::string img_name = entry->d_name;

    market_info info;
    info.cam_id = atoi(&img_name.substr(img_name.find("_") + 2)[0]) - 1;
    info.pid = std::stoi(img_name.substr(0, img_name.find("_")));
    info.img_path = gallery_dir + "/" + img_name;
    m_gallerys.push_back(info);
  }

  closedir(dirp);
  closedir(dirp_g);

  return CVI_TDL_SUCCESS;
}

uint32_t market1501Eval::getImageNum(bool is_query) {
  return (is_query) ? m_querys.size() : m_gallerys.size();
}

void market1501Eval::getPathIdPair(const int index, bool is_query, std::string *path, int *cam_id,
                                   int *pid) {
  if (is_query) {
    *path = m_querys[index].img_path;
    *cam_id = m_querys[index].cam_id;
    *pid = m_querys[index].pid;
  } else {
    *path = m_gallerys[index].img_path;
    *cam_id = m_gallerys[index].cam_id;
    *pid = m_gallerys[index].pid;
  }
}

void market1501Eval::insertFeature(const int index, bool is_query, const cvtdl_feature_t *feature) {
  if (is_query) {
    memset(&m_querys[index].feature, 0, sizeof(cvtdl_feature_t));
    CVI_TDL_MemAlloc(sizeof(int8_t), feature->size, TYPE_INT8, &m_querys[index].feature);
    memcpy(m_querys[index].feature.ptr, feature->ptr, feature->size);
  } else {
    memset(&m_gallerys[index].feature, 0, sizeof(cvtdl_feature_t));
    CVI_TDL_MemAlloc(sizeof(int8_t), feature->size, TYPE_INT8, &m_gallerys[index].feature);
    memcpy(m_gallerys[index].feature.ptr, feature->ptr, feature->size);
  }
}

void market1501Eval::resetData() {
  for (auto info : m_querys) {
    CVI_TDL_Free(&info.feature);
  }

  for (auto info : m_gallerys) {
    CVI_TDL_Free(&info.feature);
  }

  m_querys.clear();
  m_gallerys.clear();
}

void market1501Eval::evalCMC() {
  std::vector<float> all_cmc(MAX_RANK, 0);
  float all_ap = 0;
  int num_valid_q = 0;

  for (uint32_t i = 0; i < m_querys.size(); i++) {
    market_info q_info = m_querys[i];
    std::vector<std::pair<float, int>> dist_mat;

    int sum_of_elems = 0;
    for (uint32_t j = 0; j < m_gallerys.size(); j++) {
      if ((q_info.pid == m_gallerys[j].pid) && (q_info.cam_id != m_gallerys[j].cam_id)) {
        sum_of_elems = 1;
        break;
      }
    }
    if (sum_of_elems == 0) continue;

    for (uint32_t j = 0; j < m_gallerys.size(); j++) {
      if ((q_info.pid == m_gallerys[j].pid) && (q_info.cam_id == m_gallerys[j].cam_id)) {
        continue;
      }
      dist_mat.push_back(
          std::make_pair(cosine_distance(&q_info.feature, &m_gallerys[j].feature), j));
    }

    std::sort(dist_mat.begin(), dist_mat.end());

    std::vector<int> raw_cmc(dist_mat.size(), 0);
    for (uint32_t j = 0; j < raw_cmc.size(); j++) {
      if (q_info.pid == m_gallerys[dist_mat[j].second].pid) {
        raw_cmc[j] = 1;
      }
    }

    num_valid_q++;

    int cum_sum = 0;
    for (uint32_t j = 0; j < MAX_RANK; j++) {
      cum_sum += raw_cmc[j];
      if (cum_sum >= 1) all_cmc[j]++;
    }

    int num_rel = 0;
    float ap = 0;
    for (uint32_t j = 0; j < raw_cmc.size(); j++) {
      num_rel += raw_cmc[j];
      ap += ((float)num_rel / (j + 1)) * raw_cmc[j];
    }

    ap = ap / num_rel;
    all_ap += ap;
  }

  for (uint32_t i = 0; i < MAX_RANK; i++) {
    all_cmc[i] = all_cmc[i] / num_valid_q;
  }

  printf("Rank [1]:%f, [5]:%f, [10]:%f, [20]:%f\n", all_cmc[0], all_cmc[4], all_cmc[9],
         all_cmc[19]);
  printf("mAP: %f\n", all_ap / num_valid_q);
}
}  // namespace evaluation
}  // namespace cvitdl
