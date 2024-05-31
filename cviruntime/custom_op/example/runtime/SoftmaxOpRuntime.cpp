/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include <cmath>
#include <string.h>
#include "SoftmaxOpRuntime.hpp"

SoftmaxOpRuntime::~SoftmaxOpRuntime() {
  if (_max)
    delete[] _max;
  if (_sum)
    delete[] _sum;
}

void SoftmaxOpRuntime::setup(std::vector<std::shared_ptr<cvi::runtime::Neuron>> &inputs,
                             std::vector<std::shared_ptr<cvi::runtime::Neuron>> &outputs,
                             cvi::OpParam &param) {
  _bottom = inputs[0];
  _top = outputs[0];
  _axis = param.get<int32_t>("axis");
  assert(_axis >= 0);
  auto shape = _bottom->shape;
  _axis = _axis % shape.size();

  _n = 1;
  for(int i = 0; i < _axis; ++i) {
    _n *= shape[i];
  }

  _inner_dim = 1;
  for(size_t i = _axis+1; i < shape.size(); ++i) {
    _inner_dim *= shape[i];
  }

  _c = shape[_axis];
  _dim = _c * _inner_dim;

  _max = new float[_inner_dim];
  _sum = new float[_inner_dim];
}

void SoftmaxOpRuntime::run() {
  auto bottom_data = _bottom->cpu_data<float>();
  auto top_data = _top->cpu_data<float>();

  for (int i = 0; i < _n; ++i) {
    memcpy(_max, bottom_data, _inner_dim * sizeof(float));
    memset(_sum, 0, _inner_dim * sizeof(float));
    // find max value accross channel
    int c_offset = i * _dim;
    for (int j = 0; j < _c; ++j, c_offset += _inner_dim) {
      for (int k = 0; k < _inner_dim; k++) {
        if (_max[k] < bottom_data[c_offset + k])
          _max[k] = bottom_data[c_offset + k];
      }
    }

    // calculate exp(x)
    c_offset = i * _dim;
    for (int j = 0; j < _c; ++j, c_offset += _inner_dim) {
      for (int k = 0; k < _inner_dim; k++) {
        top_data[c_offset + k] = std::exp(bottom_data[c_offset + k] - _max[k]);
        _sum[k] += top_data[c_offset + k];
      }
    }

    c_offset = i * _dim;
    for (int j = 0; j < _c; ++j, c_offset += _inner_dim) {
      for (int k = 0; k < _inner_dim; k++) {
        top_data[c_offset + k] /= _sum[k];
      }
    }
  }
}