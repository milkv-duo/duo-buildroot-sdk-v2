#include <iostream>
#include <runtime/cpu_function.hpp>
#include <runtime/neuron.hpp>
#include <unordered_map>
#include <vector>

namespace cvi {
namespace runtime {

class InterpolationFunc : public ICpuFunction {

public:
  InterpolationFunc() {}

  ~InterpolationFunc();
  void setup(tensor_list_t &inputs, tensor_list_t &outputs, OpParam &param);
  void run();

  static ICpuFunction *open() { return new InterpolationFunc(); }
  static void close(ICpuFunction *func) { delete func; }

protected:
  template <typename T>
  void interp_nearest_inner(int n, int c, int ih, int iw, int oh, int ow, bool half_pixel);
  void interp_nearest();

private:
  std::shared_ptr<Neuron> _bottom;
  std::shared_ptr<Neuron> _top;

  int shrink_factor;
  int zoom_factor;
  int pad_end_;
  int pad_beg_;
  int height;
  int width;
  std::string coordinate_transformation_mode;

  int height_in_eff_;
  int width_in_eff_;
  int height_out_ = -1;
  int width_out_ = -1;
  int height_in_;
  int width_in_;
  int num_;
  int channels_;
};

} // namespace runtime
} // namespace cvi
