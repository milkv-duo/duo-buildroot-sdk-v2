#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class TransposeFunc : public ICpuFunction {
public:
  void setup(std::vector<std::shared_ptr<Neuron> > &inputs,
             std::vector<std::shared_ptr<Neuron> > &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new TransposeFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;
};

}
}