#include <iostream>
#include <runtime/cpu_function.hpp>
#include <runtime/neuron.hpp>
namespace cvi {
namespace runtime {
class GatherNDFunc : public ICpuFunction {

public:
  GatherNDFunc() {}
  ~GatherNDFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new GatherNDFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;
  int batch_dims;
  int indice_dims;
  uint64_t gather_offset(std::vector<int> input_shape,
                          std::vector<int> gather_index);
};

}
}