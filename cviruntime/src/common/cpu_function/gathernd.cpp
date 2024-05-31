#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/gathernd.hpp>

namespace cvi {
namespace runtime {
  GatherNDFunc::~GatherNDFunc() {}
  
  void GatherNDFunc::setup(tensor_list_t &inputs, tensor_list_t &outputs, OpParam &param) {
    _bottoms = inputs;
    _tops = outputs;
    batch_dims = param.get<int32_t>("batch_dims");
    indice_dims = param.get<int32_t>("indice_dims");
  }

  uint64_t GatherNDFunc::gather_offset(
    std::vector<int> input_shape, std::vector<int> gather_index) {
    uint64_t offset = 0;
    int dim_size = gather_index.size();
    int gap = 1;
    for (int i = dim_size - 1; i >= 0; i--) {
        offset += gather_index[i] * gap;
        gap *= input_shape[i];
    }
    return offset;
}

  void GatherNDFunc::run() {
    int batch_dims_size = 1;
    auto input_info = _bottoms[0];
    auto indices_info = _bottoms[1];
    auto indices_shape = indices_info->shape;
    auto input_shape = input_info->shape;
    const float *input = input_info->cpu_data<float>();
    const int *indices = indices_info->cpu_data<int>();
    std::vector<int> indices_v(indices_info->count());
    for (size_t i = 0; i < indices_info->count(); ++i) {
        indices_v[i] = indices[i];
    }
    float *out = _tops[0]->cpu_data<float>();

    for (int i = 0; i < batch_dims; ++i) {
        batch_dims_size *= indices_shape[i];
    }

    int channel = (indices_info->count() / batch_dims_size) /
                    indices_shape[indice_dims - 1];
    assert(channel * indices_shape[indice_dims - 1] * batch_dims_size ==
            (int)indices_info->count());
    std::vector<int> indices_new_shape = {
        batch_dims_size, channel, indices_shape[indice_dims - 1]};
    std::vector<int> input_new_shape = {batch_dims_size};
    for (size_t i = batch_dims; i < input_shape.size(); ++i) {
        input_new_shape.push_back(input_shape[i]);
    }

    uint64_t gather_eltment =
        _tops[0]->count() / (indices_new_shape[0] * indices_new_shape[1]);
    assert(gather_eltment * indices_new_shape[0] * indices_new_shape[1] ==
            _tops[0]->count());  
    for (int b = 0; b < indices_new_shape[0]; ++b) {
        int index1 = b * indices_new_shape[1] * indices_new_shape[2];
        int indices_new_shape2_size = indices_new_shape[2] * sizeof(int);
        int index2 = b * indices_new_shape[1];
        int gather_eltment_size = gather_eltment * sizeof(float);
        for (int c = 0; c < indices_new_shape[1]; ++c) {
        std::vector<int> gather_index(indices_new_shape[2]);
        memcpy(gather_index.data(),
                (int *)indices_v.data() +
                    index1 + c * indices_new_shape[2],
                indices_new_shape2_size);
        gather_index.insert(gather_index.begin(), b);
        uint64_t offset = gather_offset(input_new_shape, gather_index);
        memcpy(out + (index2 + c) * gather_eltment,
                input + offset * gather_eltment, gather_eltment_size);
        }
    }    
  }
}
}