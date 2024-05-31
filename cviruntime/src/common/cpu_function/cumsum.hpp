#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class CumSumFunc : public ICpuFunction {

public:
  CumSumFunc() {}

  ~CumSumFunc();

  void setup(std::vector<std::shared_ptr<Neuron> > &inputs,
             std::vector<std::shared_ptr<Neuron> > &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new CumSumFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::vector<std::shared_ptr<Neuron> > _bottoms;
  std::vector<std::shared_ptr<Neuron> > _tops;

  int _axis;
};
}
}