#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/reducemax.hpp>

namespace cvi {
namespace runtime {

inline int count(std::vector<int> &shape, int start_axis, int end_axis) {
    int64_t count = 1;
    for (int i = start_axis; i < end_axis; ++i) {
      count *= shape[i];
    }
    return count;
}

template <typename T>
int my_reduce_max(T *input, T *output,
                     std::vector<int> &input_shape,
                     std::vector<int> &axes) {
  assert(axes.size() > 0);
  int axis = axes[0];
  // only support one axis, if has two axis, should be continous
  int total = count(input_shape, 0, input_shape.size());
  int n = count(input_shape, 0, axis);
  int c = input_shape[axis];
  int hw = total / (n*c);

  for (int nidx = 0; nidx < n; nidx++) {
    for (int inner_idx = 0; inner_idx < hw; inner_idx++) {
      for (int cidx = 0; cidx < c; cidx++) {
        T tmp = input[nidx * c * hw + cidx * hw + inner_idx];
        if (cidx == 0)
          output[nidx * hw + inner_idx] = tmp;
        output[nidx * hw + inner_idx] = std::max(tmp, output[nidx * hw + inner_idx]);
      }
    }
  }
  return 0;
}


ReduceMaxFunc::~ReduceMaxFunc() {}

void ReduceMaxFunc::setup(tensor_list_t &inputs,
            tensor_list_t &outputs,
            OpParam &param) {

  _top = outputs[0];
  _bottom = inputs[0];
  _axes = param.get<std::vector<int32_t>>("axes");
  assert(_bottom->fmt == _top->fmt && "in/out dtype should be equal");
}

void ReduceMaxFunc::run() {
  auto input_shape = _bottom->shape;
  if (CVI_FMT_INT8 == _bottom->fmt || CVI_FMT_UINT8 == _bottom->fmt) {
    auto input = _bottom->cpu_data<uint8_t>();
    auto output = _top->cpu_data<uint8_t>();
    my_reduce_max<uint8_t>(input, output, input_shape, _axes);
  }
  else if (CVI_FMT_FP32 == _bottom->fmt) {
    auto input = _bottom->cpu_data<float>();
    auto output = _top->cpu_data<float>();
    my_reduce_max<float>(input, output, input_shape, _axes);
  }
  else {
    assert(0 && "not support dtype");
  }
}

} // namespace runtime
} // namespace cvi
