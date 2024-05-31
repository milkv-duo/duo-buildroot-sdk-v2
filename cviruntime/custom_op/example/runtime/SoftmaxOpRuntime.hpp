/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*/
#include <iostream>
#include <vector>
#include <string>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>
#include <runtime/op_param.hpp>

class SoftmaxOpRuntime : public cvi::runtime::ICpuFunction {

public:
  SoftmaxOpRuntime() = default;
  ~SoftmaxOpRuntime();

private:
  std::shared_ptr<cvi::runtime::Neuron> _bottom;
  std::shared_ptr<cvi::runtime::Neuron> _top;
  int _axis;
  int _inner_dim;
  int _dim;
  int _c;
  int _n;
  float *_max = nullptr;
  float *_sum = nullptr;

public:
  static ICpuFunction *open() { return new SoftmaxOpRuntime(); }

  void setup(std::vector<std::shared_ptr<cvi::runtime::Neuron>> &inputs,
             std::vector<std::shared_ptr<cvi::runtime::Neuron>> &outputs,
             cvi::OpParam &param);
  void run();

};
