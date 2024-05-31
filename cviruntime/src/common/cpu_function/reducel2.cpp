#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/reducel2.hpp>

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
int my_reduce_l2(T *input, T *output,
                     std::vector<int> &org_input_shape,
                     std::vector<int> &axes) {
  assert(axes.size() > 0);
  auto input_shape = org_input_shape;
  int size = count(input_shape, 0, input_shape.size());
  std::vector<T> tmp (size, 0);
  T* _output = tmp.data();

  for (int i = 0; i < (int)axes.size(); i++) {
    int dim = input_shape.size();
    int axis = axes[i];
    assert(dim > axis);

    int inner = count(input_shape, axis + 1, input_shape.size());
    int next_inner = inner * input_shape[axis];
    int outer = count(input_shape, 0, axis);

    for (int i = 0; i < outer; i++) { 
      std::vector<T> inner_sum (inner, 0);
      for (int s = 0; s < input_shape[axis]; s++) {
        for (int j = 0; j < inner; j++) {
          inner_sum[j] += std::pow(input[i * next_inner + s * inner + j], 2);
        }
      }

      // l2
      for (int j = 0; j < inner; j++) {
        _output[i * inner + j] = std::sqrt(inner_sum[j]);
      }
    }

    input_shape[axis] = 1;
    input = _output;
  }

  // export
  size = count(input_shape, 0, input_shape.size());
  std::copy(_output, _output + size, output);

  return 0;
}

ReduceL2Func::~ReduceL2Func() {}

void ReduceL2Func::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
            std::vector<std::shared_ptr<Neuron>> &outputs,
            OpParam &param) {

  _top = outputs[0];
  _bottom = inputs[0];
  _axes = param.get<std::vector<int32_t>>("axes");
  assert(_bottom->fmt == _top->fmt && "in/out dtype should be equal");
}
  
void ReduceL2Func::run() {
  auto input_shape = _bottom->shape;
  if (CVI_FMT_INT8 == _bottom->fmt || CVI_FMT_UINT8 == _bottom->fmt) {
    auto input = _bottom->cpu_data<uint8_t>();
    auto output = _top->cpu_data<uint8_t>();
    my_reduce_l2<uint8_t>(input, output, input_shape, _axes);
  }
  else if (CVI_FMT_FP32 == _bottom->fmt) {
    auto input = _bottom->cpu_data<float>();
    auto output = _top->cpu_data<float>();
    my_reduce_l2<float>(input, output, input_shape, _axes);
  }
  else {
    assert(0 && "not support dtype");
  }
}

} // namespace runtime
} // namespace cvi
