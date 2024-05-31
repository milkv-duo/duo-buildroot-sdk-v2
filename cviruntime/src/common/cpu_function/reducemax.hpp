#include <iostream>
#include <vector>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class ReduceMaxFunc : public ICpuFunction {

public:
  ReduceMaxFunc() {}

  ~ReduceMaxFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new ReduceMaxFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;

  std::vector<int> _axes;
};

}
}
