#include <string.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/cumsum.hpp>

namespace cvi {
namespace runtime {

CumSumFunc::~CumSumFunc() {}

void CumSumFunc::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                       std::vector<std::shared_ptr<Neuron>> &outputs,
                       OpParam &param) {
  _bottoms = inputs;
  _tops = outputs;
  _axis = param.get<int32_t>("axis");

}

void CumSumFunc::run() {
  auto bottom_data = _bottoms[0]->cpu_data<float>();
  auto top_data = _tops[0]->cpu_data<float>();
  std::vector<int> shape = _bottoms[0]->shape; 

  int length = shape[_axis];
  int stride = 1;

  for (size_t i = _axis + 1; i < shape.size(); i++) { 
    stride *= shape[i];
  }
  int numelement = 1;
  for (size_t i = 0; i < shape.size(); i++) { 
    numelement *= shape[i];
  }

  // int num_elements = _tops[0]->size();
  int cur_index = 0;
  while (cur_index < numelement) {
    for (int l = 0; l < length; l++) {
      int start =  cur_index + l * stride;
      for (int s = 0; s < stride; s++) {
        if (l == 0) {
          top_data[start + s] = bottom_data[start + s];
        } else {
          top_data[start + s] = bottom_data[start + s] + 
                                   top_data[start + s - stride];
        }
      }

    }
    cur_index += length * stride;
  }


}
}
}