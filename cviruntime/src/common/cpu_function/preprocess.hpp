#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class PreprocessFunc : public ICpuFunction {
public:
  void setup(std::vector<std::shared_ptr<Neuron> > &inputs,
             std::vector<std::shared_ptr<Neuron> > &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new PreprocessFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;
  std::vector<int> _color_order;
  std::vector<float> _mean;
  float _scale = 1.0f;
  float _raw_scale = 1.0f;
};

}
}