#include <iostream>
#include <runtime/cpu_function.hpp>
#include <runtime/neuron.hpp>
namespace cvi {
namespace runtime {
#define INT(val) (static_cast<int>(val))
enum GridSamplerInterpolation {
    GridSamplerBilinear = 0,
    GridSamplerNearest = 1
};
enum GridSamplerPaddingMode {
    GridSamplerZeros = 0,
    GridSamplerBorder = 1,
    GridSamplerReflection = 2
};
class GridSamplerFunc : public ICpuFunction {
public:
  GridSamplerFunc() {};
  ~GridSamplerFunc() {};
  void setup(tensor_list_t &inputs,
             tensor_list_t &outputs,
             OpParam &param);
  void run();
  static ICpuFunction *open() { return new GridSamplerFunc(); }
  static void close(ICpuFunction *func) { delete func; }  

private:
  tensor_list_t _bottoms;
  tensor_list_t _tops;
  int mode;
  int padding_mode;
  bool align_corners;
  
  float computeIndex(float coord, int size, int paddingMode, bool alignCorners);

  template <typename scalar_t>
  scalar_t reflect_coordinates(scalar_t in, int64_t twice_low, int64_t twice_high);

  template <typename scalar_t>
  scalar_t clip_coordinates(scalar_t in, int64_t clip_limit);  

};

}
}