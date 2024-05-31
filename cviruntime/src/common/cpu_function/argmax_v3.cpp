#include <cmath>
#include <cpu_function/argmax_v3.hpp>
#include <iostream>
#include <runtime/neuron.hpp>
#include <string.h>
#include <vector>

namespace cvi {
namespace runtime {

static inline float BF16(const uint16_t &data) {
  float data_f32 = 0.0f;
  uint16_t *p_data_bf16 = (uint16_t *)(&data_f32);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  p_data_bf16[0] = data;
#else
  p_data_bf16[1] = data;
#endif
  return data_f32;
}

void ArgMaxV3Func::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                         std::vector<std::shared_ptr<Neuron>> &outputs,
                         OpParam &param) {
  (void)param;
  _bottom = inputs[0];
  _max_map = inputs[1];
  _indices = outputs[0];
  _values = nullptr;
  if (outputs.size() > 1) {
    _values = outputs[1];
  }
  auto axis = param.get<int32_t>("axis");
  if (param.has("scale")) {
    scale = param.get<float>("scale");
  }
  for (int i = 0; i < axis; i++) {
    _outer_dim *= _bottom->shape[i];
  }
  _inner_dim = _bottom->shape[axis];
  _tile_num = (_inner_dim + 256 - 1) / 256;
}

void ArgMaxV3Func::run() {
  if (_bottom->fmt == CVI_FMT_INT8) {
    argmax<int8_t>();
  } else {
    argmax<int16_t>();
  }
}

template <typename T>
void ArgMaxV3Func::argmax() {
  auto data = _bottom->cpu_data<T>();
  auto map = _max_map->cpu_data<T>();
  auto indices = _indices->cpu_data<float>();
  auto values = _values ? _values->cpu_data<float>() : nullptr;
  float max_val_fp32 = 0;
  for (int i = 0; i < _outer_dim; ++i) {
    auto map_ptr = map + i * _tile_num;
    T max_val = map_ptr[0];
    int idx = 0;
    // find max_val
    for (int j = 1; j < _tile_num; j++) {
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
    indices[i] = (float)(idx + offset);
    if (values) {
      if (std::is_same<T, int16_t>::value) {
        max_val_fp32 = BF16(max_val);
      } else {
        max_val_fp32 = (int)max_val * scale;
      }
      values[i] = max_val_fp32;
    }
  }
}

} // namespace runtime
} // namespace cvi
