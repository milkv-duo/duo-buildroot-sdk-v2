#include <iostream>
#include <vector>
#include <runtime/neuron.hpp>
#include <runtime/cpu_function.hpp>


namespace cvi {
namespace runtime {

class ArgMaxV2Func : public ICpuFunction {

public:
  void setup(std::vector<std::shared_ptr<Neuron>> &inputs,
             std::vector<std::shared_ptr<Neuron>> &outputs,
             OpParam &param);
  void run();

  static ICpuFunction *open() { return new ArgMaxV2Func(); }
  static void close(ICpuFunction *func) { delete func; }

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;
  std::shared_ptr<Neuron> _max_map;

  template <typename T>
  void argmax();

  int _outer_dim = 1;
  int _inner_dim = 1;
  int _tile_num = 1;
  float scale = 1.;
};

}
}
