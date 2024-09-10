#include "lstr.hpp"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <error_msg.hpp>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include "core/core/cvtdl_core_types.h"
#include "core/core/cvtdl_errno.h"
#include "core/cvi_tdl_types_mem_internal.h"
#include "core/object/cvtdl_object_types.h"
#include "core/utils/vpss_helper.h"
#include "core_utils.hpp"
#include "cvi_comm.h"
#include "misc.hpp"
#include "object_utils.hpp"

namespace cvitdl {

LSTR::LSTR() : Core(CVI_MEM_DEVICE) {
  m_preprocess_param[0].factor[0] = 0.014598;
  m_preprocess_param[0].factor[1] = 0.0150078;
  m_preprocess_param[0].factor[2] = 0.0142201;

  m_preprocess_param[0].mean[0] = 1.79226;
  m_preprocess_param[0].mean[1] = 1.752097;
  m_preprocess_param[0].mean[2] = 1.48022;
  m_preprocess_param[0].format = PIXEL_FORMAT_RGB_888_PLANAR;
  m_preprocess_param[0].keep_aspect_ratio = false;
}
LSTR::~LSTR() {}

int LSTR::inference(VIDEO_FRAME_INFO_S *frame, cvtdl_lane_t *lane_meta) {
  lane_meta->height = frame->stVFrame.u32Height;
  lane_meta->width = frame->stVFrame.u32Width;
  std::vector<VIDEO_FRAME_INFO_S *> frames = {frame};
  int ret = run(frames);
  if (ret != CVI_TDL_SUCCESS) {
    return ret;
  }
  outputParser(lane_meta);
  model_timer_.TicToc("post");
  return CVI_TDL_SUCCESS;
}

float LSTR::gen_x_by_y(float ys, std::vector<float> &point_line) {
  return point_line[2] / ((ys - point_line[3]) * (ys - point_line[3])) +
         point_line[4] / (ys - point_line[3]) + point_line[5] + point_line[6] * ys - point_line[7];
}

int LSTR::outputParser(cvtdl_lane_t *lane_meta) {
  float *out_conf = getOutputRawPtr<float>(1);
  float *out_feature = getOutputRawPtr<float>(0);
  CVI_SHAPE output_shape = getOutputShape(0);
  std::vector<std::vector<float>> point_map;
  std::vector<float> lane_dis;
  //(5 lanes) * (1 conf + 2 (upper & lower) + 4 poly coeffs)
  for (int i = 0; i < 7; i++) {
    int cls = 0;
    if (out_conf[i * 2 + 1] > out_conf[i * 2]) {
      cls = 1;
    }
    if (cls == 1) {
      std::vector<float> line_info(out_feature + i * 8, out_feature + i * 8 + 8);
      point_map.push_back(line_info);
      float cur_dis = gen_x_by_y(1.0, point_map.back());
      cur_dis = (cur_dis - 0.5) * lane_meta->width;
      lane_dis.push_back(cur_dis);
    }
  }
  std::vector<int> sort_index(point_map.size(), 0);
  for (int i = 0; i != point_map.size(); i++) {
    sort_index[i] = i;
  }
  std::sort(sort_index.begin(), sort_index.end(),
            [&](const int &a, const int &b) { return (lane_dis[a] < lane_dis[b]); });
  std::vector<int> final_index;
  for (int i = 0; i != sort_index.size(); i++) {
    if (lane_dis[sort_index[i]] < 0) {
      if (i == sort_index.size() - 1 || lane_dis[sort_index[i + 1]] > 0) {
        if (point_map[sort_index[i]][1] - point_map[sort_index[i]][0] > 0.2)
          final_index.push_back(sort_index[i]);
      }

    } else {
      if (point_map[sort_index[i]][1] - point_map[sort_index[i]][0] > 0.2) {
        final_index.push_back(sort_index[i]);
        break;
      }
    }
  }
  lane_meta->size = final_index.size();
  if (lane_meta->size > 0) {
    lane_meta->lane = (cvtdl_lane_point_t *)malloc(sizeof(cvtdl_lane_point_t) * lane_meta->size);
  } else {
    return CVI_TDL_SUCCESS;
  }
  for (int i = 0; i < lane_meta->size; i++) {
    float upper = std::min(1.0f, point_map[final_index[i]][1]);
    float lower = std::max(0.0f, point_map[final_index[i]][0]);
    float slice = (upper - lower) / 100;
    float true_y1 = lower + 25 * slice;
    float true_x1 = gen_x_by_y(true_y1, point_map[final_index[i]]);
    float true_y2 = lower + 75 * slice;
    float true_x2 = gen_x_by_y(true_y2, point_map[final_index[i]]);

    lane_meta->lane[i].y[0] = 0.6 * lane_meta->height;
    lane_meta->lane[i].x[0] =
        (true_x1 + (0.6 - true_y1) * (true_x2 - true_x1) / (true_y2 - true_y1)) * lane_meta->width;
    lane_meta->lane[i].y[1] = 0.8 * lane_meta->height;
    lane_meta->lane[i].x[1] =
        (true_x1 + (0.8 - true_y1) * (true_x2 - true_x1) / (true_y2 - true_y1)) * lane_meta->width;
  }
  return CVI_TDL_SUCCESS;
}

}  // namespace cvitdl
