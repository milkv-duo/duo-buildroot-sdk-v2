#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>

namespace cvi {
namespace runtime {

class ProposalFunc : public ICpuFunction {

public:
  ProposalFunc() {}

  ~ProposalFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new ProposalFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  int feat_stride;
  int anchor_base_size;

  float rpn_obj_threshold;
  float rpn_nms_threshold;
  int rpn_nms_post_top_n;
  int net_input_w;
  int net_input_h;
};

}
}
