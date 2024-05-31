#include <string.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/argmax_v2.hpp>

namespace cvi {
namespace runtime {

static inline float BF16(const uint16_t & data) {
  float data_f32 = 0.0f;
  uint16_t *p_data_bf16 = (uint16_t*)(&data_f32);
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  p_data_bf16[0] = data;
#else
  p_data_bf16[1] = data;
#endif
  return data_f32;
}

void ArgMaxV2Func::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                        std::vector<std::shared_ptr<Neuron>> &outputs,
                        OpParam &param) {
  (void)param;
  _bottom = inputs[0];
  _max_map = inputs[1];
  _top = outputs[0];
  auto axis = param.get<int32_t>("axis");
  if (_bottom->fmt == CVI_FMT_INT8) {
    scale = param.get<float>("scale");
  }
  for (int i = 0; i < axis; i++) {
    _outer_dim *= _bottom->shape[i];
  }
  _inner_dim = _bottom->shape[axis];
  _tile_num = (_inner_dim + 256 - 1) / 256;
}

void ArgMaxV2Func::run() {
  if (_bottom->fmt == CVI_FMT_INT8) {
    argmax<int8_t>();
  } else {
    argmax<int16_t>();
  }
}

template <typename T>
void ArgMaxV2Func::argmax() {
  auto data = _bottom->cpu_data<T>();
  auto map = _max_map->cpu_data<T>();
  auto top = _top->cpu_data<float>();
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
    if (std::is_same<T, int16_t>::value) {
      max_val_fp32 = BF16(max_val);
    } else {
      max_val_fp32 = (int)max_val;
    }
    top[2 * i] = max_val_fp32 * scale;
    top[2 * i + 1] = (float)(idx + offset);
  }
}

}
}
