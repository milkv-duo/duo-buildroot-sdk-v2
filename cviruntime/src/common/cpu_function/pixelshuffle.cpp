#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/pixelshuffle.hpp>

namespace cvi {
namespace runtime {

void PixelShuffleFunc::setup(tensor_list_t &inputs,
                           tensor_list_t &outputs,
                           OpParam &param) {
  (void)param;
  _bottom = inputs[0];
  _top = outputs[0];
  upscale_factor = param.get<int32_t>("upscale_factor");
  mode = param.get<std::string>("mode");
}

void PixelShuffleFunc::run() {
  int batch_size = _bottom->shape[0];
  int in_channel = _bottom->shape[1];
  int in_height = _bottom->shape[2];
  int in_width = _bottom->shape[3];
  int out_channel = _top->shape[1];
  int out_height = _top->shape[2];
  int out_width = _top->shape[3];
  int i_index = 0, o_index = 0, new_c = 0, new_h = 0, new_w = 0,
      r = upscale_factor;

  auto bottom_data = _bottom->cpu_data<int8_t>();
  auto top_data = _top->cpu_data<int8_t>();
  if (mode == "DCR"){
    for (int n = 0; n < batch_size; n++) {
      for (int c = 0; c < in_channel; c++) {
        for (int h = 0; h < in_height; h++) {
          for (int w = 0; w < in_width; w++) {
            new_c = c % out_channel;
            new_h = h * r + static_cast<int>(floor((c / out_channel) / r));
            new_w = w * r + ((c / out_channel) % r);
            o_index = n * (out_channel * out_height * out_width) +
                      new_c * (out_height * out_width) +
                      new_h * out_width +
                      new_w;
            top_data[o_index] = bottom_data[i_index];
            i_index++;
          }
        }
      }
    }
  }else{
    assert(0 && "only DCR mode use cpu");
  }

}

} // namespace runtime
} // namespace cvi