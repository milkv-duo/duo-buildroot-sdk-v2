/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include "SoftmaxOp.h"

namespace cvi {

void SoftmaxOp::interpretFp32(
    std::vector<std::shared_ptr<std::vector<float>>> &operand_tensors,
    std::vector<std::vector<int64_t>> &operand_shapes,
    std::shared_ptr<std::vector<float>> &result_tensor,
    std::vector<int64_t> &result_shape) {
  (void)result_shape;
  auto axis = param.get<int32_t>("axis");
  auto& shape = operand_shapes[0];
  axis = axis % shape.size();

  int32_t n = 1, inner_dim = 1;
  for(int i = 0; i < axis; ++i) {
    n *= shape[i];
  }

  for(size_t i = axis + 1; i < shape.size(); ++i) {
    inner_dim *= shape[i];
  }

  int32_t c = shape[axis];
  int32_t dim = c * inner_dim;

  float *max = new float[inner_dim];
  float *sum = new float[inner_dim];
  float *p = operand_tensors[0]->data();
  float *q = result_tensor->data();

  for (int i = 0; i < n; ++i) {
    memcpy(max, p, inner_dim * sizeof(float));
    memset(sum, 0, inner_dim * sizeof(float));
    // find max value accross channel
    int c_offset = i *dim;
    for (int j = 0; j <c; ++j, c_offset +=inner_dim) {
      for (int k = 0; k <inner_dim; k++) {
        if (max[k] < p[c_offset + k])
          max[k] = p[c_offset + k];
      }
    }

    // calculate exp(x)
    c_offset = i *dim;
    for (int j = 0; j <c; ++j, c_offset +=inner_dim) {
      for (int k = 0; k <inner_dim; k++) {
        q[c_offset + k] = std::exp(p[c_offset + k] - max[k]);
        sum[k] += q[c_offset + k];
      }
    }

    c_offset = i *dim;
    for (int j = 0; j <c; ++j, c_offset +=inner_dim) {
      for (int k = 0; k <inner_dim; k++) {
        q[c_offset + k] /= sum[k];
      }
    }
  }

  delete[] max;
  delete[] sum;
}

RegisterCustomOp(mysoftmax, SoftmaxOp);

} // namespace cvi