#include <string.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/neuron.hpp>
#include <cpu_function/gatherelements_pt.hpp>

namespace cvi {
namespace runtime {

GatherElementsPtFunc::~GatherElementsPtFunc() {}

void GatherElementsPtFunc::setup(std::vector<std::shared_ptr<Neuron>> &inputs,
                       std::vector<std::shared_ptr<Neuron>> &outputs,
                       OpParam &param) {
  _bottoms = inputs;
  _tops = outputs;
  _axis = param.get<int32_t>("axis");

}

void gather_dim1_0(
    float *dst, const float *src,  const int *idx, int *shape) {
    for (int i = 0; i < shape[0]; ++i) {
        *dst = src[*idx];
        ++dst;
        ++idx;
    }
}

void gather_dim2_0(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    for (int i = 0; i < shape[0]; ++i) {
        for (int j = 0; j < shape[1]; ++j) {
            *dst = src[*idx * org_shape[1] + j];
            ++dst;
            ++idx;
        }
    }
}

void gather_dim2_1(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * org_shape[1];
        for (int j = 0; j < shape[1]; ++j) {
            *dst = src[idx_i + *idx];
            ++dst;
            ++idx;
        }
    }
}

void gather_dim3_0(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2 = org_shape[1] * org_shape[2];
    for (int i = 0; i < shape[0]; ++i) {
        for (int j = 0; j < shape[1]; ++j) {
            int idx_j = j * org_shape[2];
            for (int k = 0; k < shape[2]; ++k) {
                *dst = src[*idx * shape_1_2 + idx_j + k];
                ++dst;
                ++idx;
            }
        }
    }
}

void gather_dim3_1(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2 = org_shape[1] * org_shape[2];
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * shape_1_2;
        for (int j = 0; j < shape[1]; ++j) {
            for (int k = 0; k < shape[2]; ++k) {
                *dst = src[idx_i + *idx * org_shape[2] + k];
                ++dst;
                ++idx;
            }
        }
    }
}

void gather_dim3_2(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2 = org_shape[1] * org_shape[2];
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * shape_1_2;
        for (int j = 0; j < shape[1]; ++j) {
            int idx_j = idx_i + j * org_shape[2];
            for (int k = 0; k < shape[2]; ++k) {
                *dst = src[idx_j + *idx];
                ++dst;
                ++idx;
            }
        }
    }
}

void gather_dim4_0(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2_3 = org_shape[1] * org_shape[2] * org_shape[3];
    int shape_2_3 = org_shape[2] * org_shape[3];
    for (int i = 0; i < shape[0]; ++i) {
        for (int j = 0; j < shape[1]; ++j) {
            int idx_j = j * shape_2_3;
            for (int k = 0; k < shape[2]; ++k) {
                int idx_k = idx_j + k * org_shape[3];
                for (int g = 0; g < shape[3]; ++g) {
                    *dst = src[*idx * shape_1_2_3 + idx_k + g];
                    ++dst;
                    ++idx;
                }
            }
        }
    }
}

void gather_dim4_1(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2_3 = org_shape[1] * org_shape[2] * org_shape[3];
    int shape_2_3 = org_shape[2] * org_shape[3];
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * shape_1_2_3;
        for (int j = 0; j < shape[1]; ++j) {
            for (int k = 0; k < shape[2]; ++k) {
                int idx_k = k * org_shape[3];
                for (int g = 0; g < shape[3]; ++g) {
                    *dst = src[idx_i + *idx * shape_2_3 + idx_k + g];
                    ++dst;
                    ++idx;
                }
            }
        }
    }
}
void gather_dim4_2(
    float *dst, const float *src, const int *idx, int *shape, int *org_shape) {
    int shape_1_2_3 = org_shape[1] * org_shape[2] * org_shape[3];
    int shape_2_3 = org_shape[2] * org_shape[3];
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * shape_1_2_3;
        for (int j = 0; j < shape[1]; ++j) {
            int idx_j = idx_i + j * shape_2_3;
            for (int k = 0; k < shape[2]; ++k) {
                for (int g = 0; g < shape[3]; ++g) {
                    *dst = src[idx_j + *idx * org_shape[3] + g];
                    ++dst;
                    ++idx;
                }
            }
        }
    }
}
void gather_dim4_3(
    float *dst, const float *src, const int *idx, int  *shape, int *org_shape) {
    int shape_1_2_3 = org_shape[1] * org_shape[2] * org_shape[3];
    int shape_2_3 = org_shape[2] * org_shape[3];
    for (int i = 0; i < shape[0]; ++i) {
        int idx_i = i * shape_1_2_3;
        for (int j = 0; j < shape[1]; ++j) {
            int idx_j = idx_i + j * shape_2_3;
            for (int k = 0; k < shape[2]; ++k) {
                int idx_k = idx_j + k * org_shape[3];
                for (int g = 0; g < shape[3]; ++g) {
                    *dst = src[idx_k + *idx];
                    ++dst;
                    ++idx;
                }
            }
        }
    }
}

void GatherElementsPtFunc::run() {
  auto src_data = _bottoms[0]->cpu_data<float>();
  auto indices_data = _bottoms[1]->cpu_data<int>();
  auto dst_data = _tops[0]->cpu_data<float>();

  int src_dim = _bottoms[0]->shape.size();
  std::vector<int> input_shape = _bottoms[0]->shape; 
  std::vector<int> indices_shape = _bottoms[1]->shape;
  std::vector<int> output_shape = _tops[0]->shape;

  switch (src_dim)
  {
  case 1:
      gather_dim1_0(dst_data, src_data, indices_data, indices_shape.data());
      break;
  case 2:
      if (_axis == 0)
          gather_dim2_0(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 1)
          gather_dim2_1(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      break;
  case 3:
      if (_axis == 0)
          gather_dim3_0(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 1)
          gather_dim3_1(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 2)
          gather_dim3_2(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      break;
  case 4:
      if (_axis == 0)
          gather_dim4_0(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 1)
          gather_dim4_1(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 2)
          gather_dim4_2(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      else if (_axis == 3)
          gather_dim4_3(dst_data, src_data, indices_data, indices_shape.data(), input_shape.data());
      break;
  default:
      printf("error: %s: %d: invalid input dimension: %d. \n",
              __FILE__, __LINE__, static_cast<int>(input_shape.size()));
      exit(-1);
  }
}
}
}