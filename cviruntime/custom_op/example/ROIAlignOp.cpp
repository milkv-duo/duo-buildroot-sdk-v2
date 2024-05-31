/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <numeric>
#include <algorithm>
#include "ROIAlignOp.h"

namespace cvi {

void ROIAlignOp::interpretFp32(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  const int32_t pooled_h = param.get<int32_t>("pooled_h");
  const int32_t pooled_w = param.get<int32_t>("pooled_w");
  const float spatial_scale = param.get<float>("spatial_scale");

  auto data_shape = operand_shapes[0];
  auto roi_shape = operand_shapes[1];

  const int batch = (int)data_shape[0];
  const int channel = (int)data_shape[1];
  const int height = (int)data_shape[2];
  const int width = (int)data_shape[3];

  const int rois_num = roi_shape[2];
  assert(batch * rois_num == result_shape[0]);
  assert(channel == result_shape[1]);

  float* data = operand_tensors[0]->data();
  float* rois = operand_tensors[1]->data();
  float* result = result_tensor->data();

  const int one_batch_output_size = rois_num * channel * pooled_h * pooled_w;

  for (int b = 0; b < batch; ++b) {
    float* batch_rois = rois + b * rois_num * 5;
    float* batch_output = result + b * one_batch_output_size;
    for (int roi_idx = 0; roi_idx < rois_num; ++roi_idx) {
      const int roi_batch_idx = batch_rois[roi_idx * 5];
      assert(roi_batch_idx == b);

      const float roi_start_x = batch_rois[roi_idx * 5 + 1] * spatial_scale;
      const float roi_start_y = batch_rois[roi_idx * 5 + 2] * spatial_scale;
      const float roi_end_x = batch_rois[roi_idx * 5 + 3] * spatial_scale;
      const float roi_end_y = batch_rois[roi_idx * 5 + 4] * spatial_scale;

      const float roi_w = std::max(roi_end_x - roi_start_x + 1, 1.0f);
      const float roi_h = std::max(roi_end_y - roi_start_y + 1, 1.0f);

      float bin_size_w = roi_w / (float)pooled_w;
      float bin_size_h = roi_h / (float)pooled_h;

      float* batch_data = data + b * channel * height * width;

      for (int c = 0; c < channel; ++c) {
        for (int ph = 0; ph < pooled_h; ++ph) {
          for (int pw = 0; pw < pooled_w; ++pw) {
            const float region_start_x = std::min(pw * bin_size_w + roi_start_x, (float)(width));
            const float region_start_y = std::min(ph * bin_size_h + roi_start_y, (float)(height));
            const float region_end_x = std::min((pw+1) * bin_size_w + roi_start_x, (float)(width));
            const float region_end_y = std::min((ph+1) * bin_size_h + roi_start_y, (float)(height));

            const int region_grid_w = int(std::ceil(bin_size_w));
            const int region_grid_h = int(std::ceil(bin_size_h));

            const int output_idx = ph * pooled_w + pw;
            if (region_start_x >= region_end_x || region_start_y >= region_end_y) {
              batch_output[output_idx] = 0;
              continue;
            }

            float value = 0;
            float fmax = std::numeric_limits<float>::min();
            for (int gh = 0; gh < region_grid_h; ++gh) {
              for (int gw = 0; gw < region_grid_w; ++gw) {
                float x = roi_start_x + gw;
                float y = roi_start_y + gh;

                const int x_low = x;
                const int y_low = y;

                const int x_high = x_low + 1;
                const int y_high = y_low + 1;

                const float x_ratio = x - x_low;
                const float y_ratio = y - y_low;

                const float w1 = (1 - y_ratio) * (1 - x_ratio);
                const float w2 = (1 - y_ratio) * x_ratio;
                const float w3 = y_ratio * (1 - x_ratio);
                const float w4 = y_ratio * x_ratio;

                const float data1 = batch_data[y_low * height + x_low];
                const float data2 = batch_data[y_low * height + x_high];
                const float data3 = batch_data[y_high * height + x_low];
                const float data4 = batch_data[y_high * height + x_high];
                value = w1 * data1 + w2 * data2 + w3 * data3 + w4 * data4;
                if (value > fmax) {
                  fmax = value;
                }
              }
            }
            batch_output[output_idx] = fmax;
          }
        }

        batch_data += height * width;
        batch_output += pooled_h * pooled_w;
      }
    }
  }
}

RegisterCustomOp(roialign, ROIAlignOp);

} // namespace cvi
