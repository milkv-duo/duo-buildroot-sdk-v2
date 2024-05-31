#include <string.h>
#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <cpu_function/preprocess.hpp>

namespace cvi {
namespace runtime {

void PreprocessFunc::setup(tensor_list_t &inputs,
                           tensor_list_t &outputs,
                           OpParam &param) {
  _bottom = inputs[0];
  _top = outputs[0];
  _scale = param.get<float>("scale");
  _raw_scale = param.get<float>("raw_scale");
  _mean = param.get<std::vector<float>>("mean");
  _color_order = param.get<std::vector<int32_t>>("color_order");
}

void PreprocessFunc::run() {
  int n = _bottom->shape[0];
  int c = _bottom->shape[1];
  int csz = _bottom->shape[2] * _bottom->shape[3];
  int isz = c * csz;
  int count = n * isz;
  auto bottom_data = _bottom->cpu_data<float>();
  auto top_data = _top->cpu_data<float>();
  float *p = bottom_data;
  float *q = top_data;
  if (_color_order.size()) {
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < c; j++) {
        memcpy(q + _color_order[j] * csz,
               p + j * csz, csz * sizeof(float));
      }
      p += isz;
      q += isz;
    }
    p = q = top_data;
  }

  for (int i = 0; i < count; i++) {
    float val = *p++;
    if (_raw_scale != 0) {
      val *= _raw_scale;
    }
    if (_mean.size()) {
      val -= _mean[(i / csz) % c];
    }
    if (_scale != 1.0f) {
      val *= _scale;
    }
    *q++ = val;
  }
}

} // namespace runtime
} // namespace cvi