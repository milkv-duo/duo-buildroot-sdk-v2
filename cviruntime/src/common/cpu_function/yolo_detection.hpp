#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>


namespace cvi {
namespace runtime {


class YoloDetectionFunc : public ICpuFunction {

public:
  YoloDetectionFunc() {}

  ~YoloDetectionFunc();
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new YoloDetectionFunc(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;

  int _net_input_h;
  int _net_input_w;
  float _nms_threshold;
  float _obj_threshold;
  int _keep_topk;
  bool _tiny = false;
  bool _yolo_v4 = false;
  bool _spp_net = false;
  int _class_num = 80;
  std::vector<float> _anchors;
};

}
}
