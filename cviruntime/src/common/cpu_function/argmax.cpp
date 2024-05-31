#include <string.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/argmax.hpp>

namespace cvi {
namespace runtime {

void ArgMaxFunc::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                        std::vector<std::shared_ptr<Neuron>> &outputs,
                        OpParam &param) {
  (void)param;
  _bottom = inputs[0];
  _max_map = inputs[1];
  _top = outputs[0];
  auto axis = param.get<int32_t>("axis");
  for (int i = 0; i < axis; i++) {
    _outer_dim *= _bottom->shape[i];
  }
  _inner_dim = _bottom->shape[axis];
  _tile_num = (_inner_dim + 256 - 1) / 256;
}

void ArgMaxFunc::run() {
  if (_bottom->fmt == CVI_FMT_INT8) {
    argmax<int8_t>();
  } else {
    argmax<int16_t>();
  }
}

template <typename T>
void ArgMaxFunc::argmax() {
  auto data = _bottom->cpu_data<T>();
  auto map = _max_map->cpu_data<T>();
  auto top = _top->cpu_data<float>();

  for (int i = 0; i < _outer_dim; ++i) {
    T max_val = 0;
    int idx = 0;
    auto map_ptr = map + i * _tile_num;
    // find max_val
    for (int j = 0; j < _tile_num; j++) {
      if (map_ptr[j] < 0) {
        continue;
      }
      if (map_ptr[j] > max_val) {
        max_val = map_ptr[j];
        idx = j;
      }
    }
    int offset = idx * 256;
    int len = std::min(_inner_dim - offset, 256);
    auto ptr = data + i * _inner_dim + offset;
    idx = 0;
    for (int j = 0; j < len; ++j) {
      if (ptr[j] == max_val) {
        idx = j;
        break;
      }
    }
    top[i] = (float)(idx + offset);
  }
}

}
}
