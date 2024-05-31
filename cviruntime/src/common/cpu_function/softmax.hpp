#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>


namespace cvi {
namespace runtime {

class SoftmaxFunc : public ICpuFunction {

public:
  SoftmaxFunc() {};
  ~SoftmaxFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new SoftmaxFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;
  int _axis;
  int _inner_dim;
  int _dim;
  int _c;
  int _n;
  float *_max = nullptr;
  float *_sum = nullptr;

};

}
}
