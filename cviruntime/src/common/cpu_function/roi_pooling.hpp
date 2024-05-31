#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class ROIPoolingFunc : public ICpuFunction {

public:
  ROIPoolingFunc() {}

  ~ROIPoolingFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new ROIPoolingFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  int pooled_h;
  int pooled_w;
  float spatial_scale;
};

}
}
