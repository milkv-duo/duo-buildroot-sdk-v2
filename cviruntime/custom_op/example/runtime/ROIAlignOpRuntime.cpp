#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <runtime/neuron.hpp>
#include "ROIAlignOpRuntime.hpp"


ROIAlignOpRuntime::~ROIAlignOpRuntime() {}

void ROIAlignOpRuntime::setup(std::vector<std::shared_ptr<cvi::runtime::Neuron>> &inputs,
             std::vector<std::shared_ptr<cvi::runtime::Neuron>> &outputs,
             cvi::OpParam &param) {
  pooled_h = param.get<int32_t>("pooled_h");
  pooled_w = param.get<int32_t>("pooled_w");
  spatial_scale = param.get<float>("spatial_scale");

  auto on = outputs[0]->shape[0];
  auto oc = outputs[0]->shape[1];
  if (inputs[0]->shape[1] == oc && inputs[1]->shape[2] == on) {
    _bottoms = inputs;
  } else {
    std::swap(inputs[0], inputs[1]);
    _bottoms = inputs;
  }

  _tops = outputs;
}

void ROIAlignOpRuntime::run() {
  auto top_data = _tops[0]->cpu_data<float>();

  size_t bottom_count = _bottoms.size();
  assert(bottom_count == 2);

  float *data = (float *)_bottoms[0]->cpu_data<float>();
  float *rois = (float *)_bottoms[1]->cpu_data<float>();

  int num_rois = _bottoms[1]->shape[2];
  int batch = _bottoms[0]->shape[0];
  int channel = _bottoms[0]->shape[1];
  int height = _bottoms[0]->shape[2];
  int width = _bottoms[0]->shape[3];

  for (int b = 0; b < batch; ++b) {
    auto batch_rois = rois + _bottoms[1]->offset(b);
    auto batch_output = top_data + b * num_rois * channel * pooled_h * pooled_w;
    for (int roi_idx = 0; roi_idx < num_rois; ++roi_idx) {
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
