#include <iostream>
#include <vector>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class ReduceL2Func : public ICpuFunction {

public:
  ReduceL2Func() {}

  ~ReduceL2Func();
  void setup(std::vector<std::shared_ptr<Neuron>> &inputs,
             std::vector<std::shared_ptr<Neuron>> &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new ReduceL2Func(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;

  std::vector<int> _axes;
};

}
}
