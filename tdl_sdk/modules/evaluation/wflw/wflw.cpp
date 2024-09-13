#include "wflw.hpp"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "core/core/cvtdl_errno.h"
#include "cvi_comm.h"
#include "cvi_tdl_log.hpp"

#define LEFT_EYE 96 * 2
#define RIGHT_EYE 97 * 2
#define NOSE 54 * 2
#define LEFT_MOUTH 76 * 2
#define RIGHT_MOUTH 82 * 2

namespace cvitdl {
namespace evaluation {

wflwEval::wflwEval(const char *filepath) { getEvalData(filepath); }

int wflwEval::getEvalData(const char *filepath) {
  FILE *fp;
  char fullPath[1024] = "\0";
  strcat(fullPath, filepath);
  strcat(fullPath, "/list.txt");

  if ((fp = fopen(fullPath, "r")) == NULL) {
    LOGE("file open error: %s!\n", filepath);
    return CVI_TDL_FAILURE;
  }

  char line[4096];
  while (fscanf(fp, "%[^\n]", line) != EOF) {
    fgetc(fp);

    std::string delimiter = " ";
    std::string strLine = line;

    size_t pos = strLine.find(delimiter);
    std::string img_name = strLine.substr(0, pos);
    strLine.erase(0, pos + delimiter.length());

    std::string token = "";
    std::vector<float> points;
    while ((pos = strLine.find(delimiter)) != std::string::npos) {
      token = strLine.substr(0, pos);
      strLine.erase(0, pos + delimiter.length());
      points.push_back(std::stof(token));
    }
    points.push_back(std::stof(strLine));

    m_img_name.push_back(img_name);
    m_img_points.push_back(points);
  }
  fclose(fp);

  m_result_points.resize(m_img_points.size());

  return CVI_TDL_SUCCESS;
}

uint32_t wflwEval::getTotalImage() { return m_img_name.size(); }

void wflwEval::getImage(const int index, std::string *name) { *name = m_img_name[index]; }

void wflwEval::insertPoints(const int index, const cvtdl_pts_t points, const int width,
                            const int height) {
  std::vector<float> temp(m_img_points[0].size(), -1);

  if (points.size == 5) {
    temp[LEFT_EYE] = points.x[0] / width;
    temp[LEFT_EYE + 1] = points.y[0] / height;
    temp[RIGHT_EYE] = points.x[1] / width;
    temp[RIGHT_EYE + 1] = points.y[1] / height;
    temp[NOSE] = points.x[2] / width;
    temp[NOSE + 1] = points.y[2] / height;
    temp[LEFT_MOUTH] = points.x[3] / width;
    temp[LEFT_MOUTH + 1] = points.y[3] / height;
    temp[RIGHT_MOUTH] = points.x[4] / width;
    temp[RIGHT_MOUTH + 1] = points.y[4] / height;
  } else {
    // TODO:: Not implement
  }

  m_result_points[index] = temp;
}

void wflwEval::distance() {
  int img_num = m_img_points.size();

  float total_distance = 0.0;
  int valid_num = 0;
  for (int i = 0; i < img_num; i++) {
    if (m_result_points[i].size() == 0) continue;

    float diff = 0;
    for (uint32_t j = 0; j < m_result_points[i].size(); j++) {
      if (m_result_points[i][j] != -1) {
        diff += std::pow(m_img_points[i][j] - m_result_points[i][j], 2);
      }
    }
    diff = std::sqrt(diff);
    total_distance += diff;
    valid_num++;
  }

  printf("total distance: %f, average distance: %f\n", total_distance, total_distance / valid_num);
}
}  // namespace evaluation
}  // namespace cvitdl
