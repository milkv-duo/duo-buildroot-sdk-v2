#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/roi_pooling.hpp>

namespace cvi {
namespace runtime {

ROIPoolingFunc::~ROIPoolingFunc() {}

void ROIPoolingFunc::setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param) {
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

void ROIPoolingFunc::run() {
  auto top_data = _tops[0]->cpu_data<float>();
  memset(top_data, 0, _tops[0]->size());

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
    auto batch_top_data = top_data + _tops[0]->offset(b * num_rois);
    for (int n = 0; n < num_rois; ++n) {
      int roi_batch_ind = batch_rois[0];
      int roi_start_w = std::round(batch_rois[1] * spatial_scale);
      int roi_start_h = std::round(batch_rois[2] * spatial_scale);
      int roi_end_w = std::round(batch_rois[3] * spatial_scale);
      int roi_end_h = std::round(batch_rois[4] * spatial_scale);
      assert(roi_batch_ind < batch);

      int roi_height = std::max(roi_end_h - roi_start_h + 1, 1);
      int roi_width = std::max(roi_end_w - roi_start_w + 1, 1);
      const float bin_size_h = static_cast<float>(roi_height)
                                  / static_cast<float>(pooled_h);
      const float bin_size_w = static_cast<float>(roi_width)
                                  / static_cast<float>(pooled_w);

      float* batch_data = data + roi_batch_ind * channel * height * width;

      for (int c = 0; c < channel; ++c) {
        for (int ph = 0; ph < pooled_h; ++ph) {
          for (int pw = 0; pw < pooled_w; ++pw) {
            // Compute pooling region for this output unit:
            //  start (included) = floor(ph * roi_height / pooled_height_)
            //  end (excluded) = ceil((ph + 1) * roi_height / pooled_height_)
            int hstart = static_cast<int>(std::floor(static_cast<float>(ph)
                                          * bin_size_h));
            int wstart = static_cast<int>(std::floor(static_cast<float>(pw)
                                          * bin_size_w));
            int hend = static_cast<int>(std::ceil(static_cast<float>(ph + 1)
                                          * bin_size_h));
            int wend = static_cast<int>(std::ceil(static_cast<float>(pw + 1)
                                          * bin_size_w));

            hstart = std::min(std::max(hstart + roi_start_h, 0), height);
            hend = std::min(std::max(hend + roi_start_h, 0), height);
            wstart = std::min(std::max(wstart + roi_start_w, 0), width);
            wend = std::min(std::max(wend + roi_start_w, 0), width);

            bool is_empty = (hend <= hstart) || (wend <= wstart);

            const int pool_index = ph * pooled_w + pw;
            if (is_empty) {
              batch_top_data[pool_index] = 0;
            }

            for (int h = hstart; h < hend; ++h) {
              for (int w = wstart; w < wend; ++w) {
                const int index = h * width + w;
                if (batch_data[index] > batch_top_data[pool_index]) {
                  batch_top_data[pool_index] = batch_data[index];
                }
              }
            }
          }
        }
        batch_data += height * width;
        batch_top_data += pooled_h * pooled_w;
      }
      batch_rois += 5;
    }
  }
}

}
}