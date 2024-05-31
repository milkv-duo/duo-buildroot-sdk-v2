#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <cpu_function/transpose.hpp>

namespace cvi {
namespace runtime {

void TransposeFunc::setup(tensor_list_t &inputs,
                           tensor_list_t &outputs,
                           OpParam &param) {
  (void)param;
  _bottom = inputs[0];
  _top = outputs[0];
}

void TransposeFunc::run() {
  int channel = _top->shape[1];
  int channel_size = _top->shape[2] * _top->shape[3];
  int image_size = channel * channel_size;
  auto bottom_data = _bottom->cpu_data<float>();
  auto top_data = _top->cpu_data<float>();

  for (int i = 0; i < _top->shape[0]; i++) {
    for (int j = 0; j < image_size; j++) {
      int x_idx = i * image_size + j;
      int y_idx = i * image_size + (j % channel) * channel_size + j / channel;
      top_data[y_idx] = bottom_data[x_idx];
    }
  }
}

} // namespace runtime
} // namespace cvi