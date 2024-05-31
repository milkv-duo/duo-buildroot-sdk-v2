#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class QuantFunc : public ICpuFunction {
public:
  ~QuantFunc() {
    #if __arm__
    if (work_buf)
      free(work_buf);
    #endif
  }
  void setup(std::vector<std::shared_ptr<Neuron> > &inputs,
             std::vector<std::shared_ptr<Neuron> > &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new QuantFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  void quantFromFp32();
  void dequantToFp32();

  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;
  float _scale = 1.0f;
  bool _dequant = false;
  #if __arm__
  int32_t *work_buf = nullptr;
  #endif
};

}
}