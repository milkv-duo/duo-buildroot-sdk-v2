#include <iostream>
#include <vector>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class InstanceNormFunc : public ICpuFunction {

public:
  InstanceNormFunc() {}

  ~InstanceNormFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new InstanceNormFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> bottom_;
  std::shared_ptr<Neuron> top_;
  std::shared_ptr<Neuron> scale_;
  std::shared_ptr<Neuron> bias_;

  float variance_epsilon_;
  int num_;
  int channels_;
  int h_;
  int w_;
};

}
}
