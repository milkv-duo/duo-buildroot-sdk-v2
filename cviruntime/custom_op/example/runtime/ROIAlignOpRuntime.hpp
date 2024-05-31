#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/op_param.hpp>
#include <runtime/cpu_function.hpp>

class ROIAlignOpRuntime : public cvi::runtime::ICpuFunction {

public:
  ROIAlignOpRuntime() {}

  ~ROIAlignOpRuntime();
  void setup(std::vector<std::shared_ptr<cvi::runtime::Neuron>> &inputs,
             std::vector<std::shared_ptr<cvi::runtime::Neuron>> &outputs,
             cvi::OpParam &param);
  void run();

  static ICpuFunction *open() { return new ROIAlignOpRuntime(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::vector<std::shared_ptr<cvi::runtime::Neuron>> _bottoms;
  std::vector<std::shared_ptr<cvi::runtime::Neuron>> _tops;

  int pooled_h;
  int pooled_w;
  float spatial_scale;
};
