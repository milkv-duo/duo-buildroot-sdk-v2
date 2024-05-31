#include <cmath>
#include <cpu_function/embedding.hpp>
#include <iostream>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <vector>

namespace cvi {
namespace runtime {

EmbeddingFunc::~EmbeddingFunc() {}

void EmbeddingFunc::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                          std::vector<std::shared_ptr<Neuron>> &outputs,
                          OpParam &param) {
  (void)param;
  _top = outputs[0];
  _bottoms = inputs;
  assert(_bottoms[1]->fmt == _top->fmt && "in/out dtype should be equal");
  _feature_len = _bottoms[1]->shape[1];
  _table_len = _bottoms[1]->count() / _feature_len;
  _search_num = _bottoms[0]->count();
}

void EmbeddingFunc::run() {
  switch (_bottoms[0]->fmt) {
  case CVI_FMT_INT16:
    lookup<int16_t>();
    break;
  case CVI_FMT_UINT16:
    lookup<uint16_t>();
    break;
  case CVI_FMT_INT32:
    lookup<int32_t>();
    break;
  case CVI_FMT_UINT32:
    lookup<uint32_t>();
    break;
  default:
    assert(0 && "input fmt error");
    break;
  }
}

template <typename T1, typename T2>
void EmbeddingFunc::lookup() {
  auto indices = _bottoms[0]->cpu_data<T1>();
  auto table = _bottoms[1]->cpu_data<T2>();
  auto out = _top->cpu_data<T2>();

  for (int i = 0; i < _search_num; i++) {
    size_t in_offset = indices[i] * _feature_len;
    memcpy(out, table + in_offset, _feature_len * sizeof(T2));
    out += _feature_len;
  }
}

template <typename T1>
void EmbeddingFunc::lookup() {
  if (_top->fmt == CVI_FMT_INT8) {
    lookup<T1, int8_t>();
  } else {
    lookup<T1, int16_t>();
  }
}

} // namespace runtime
} // namespace cvi
