#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class FrcnDetectionFunc : public ICpuFunction {

public:
  FrcnDetectionFunc() {}

  ~FrcnDetectionFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new FrcnDetectionFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  float nms_threshold;
  float obj_threshold;
  int keep_topk;
  int class_num;
};

}
}
