#include <iostream>
#include <vector>
#include <cmath>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/grid_sampler.hpp>

namespace cvi {
namespace runtime {
  void GridSamplerFunc::setup(tensor_list_t &inputs, tensor_list_t &outputs, OpParam &param) {
    _bottoms = inputs;
    _tops = outputs;
    mode = param.get<int32_t>("mode");
    padding_mode = param.get<int32_t>("padding_mode");
    align_corners = param.get<bool>("align_corners");
  }

template <typename scalar_t>
scalar_t GridSamplerFunc::clip_coordinates(scalar_t in, int64_t clip_limit) {
  return std::min(static_cast<scalar_t>(clip_limit - 1),
                  std::max(in, static_cast<scalar_t>(0)));
}

// Reflects coordinates until they fall between low and high (inclusive).
// The bounds are passed as twice their value so that half-integer values
// can be represented as ints.
template <typename scalar_t>
scalar_t GridSamplerFunc::reflect_coordinates(scalar_t in, int64_t twice_low,
                                           int64_t twice_high) {
  if (twice_low == twice_high) {
    return static_cast<scalar_t>(0);
  }
  scalar_t min = static_cast<scalar_t>(twice_low) / 2;
  scalar_t span = static_cast<scalar_t>(twice_high - twice_low) / 2;
  in = std::fabs(in - min);
  // `fmod` returns same sign as `in`, which is positive after the `fabs` above.
  scalar_t extra = std::fmod(in, span);
  int flips = static_cast<int>(std::floor(in / span));
  if (flips % 2 == 0) {
    return extra + min;
  } else {
    return span - extra + min;
  }
}

float GridSamplerFunc::computeIndex(float coord, int size, int paddingMode,
                                    bool alignCorners) {
  float res = 0.f;

  // Unnormalize coordinate
  // From [-1, 1] to pixel index
  if (alignCorners)
    res = ((coord + 1.f) * .5f) * (size - 1);
  else
    res = ((coord + 1.f) * size - 1.f) * .5f;

  switch (paddingMode) {
  case GridSamplerZeros:
    break;
  case GridSamplerBorder:
    res = clip_coordinates(res, size);
    break;
  case GridSamplerReflection:
    if (alignCorners) {
      res = reflect_coordinates(res, 0, 2 * (size - 1));
    } else {
      res = reflect_coordinates(res, -1, 2 * size - 1);
    }
    res = clip_coordinates(res, size);
    break;
  default:
    assert(0);
  }
  return res;
} 

void GridSamplerFunc::run() {
  auto input_tensor = _bottoms[0];
  auto grid_tensor = _bottoms[1];
  auto output_tensor = _tops[0];
  std::vector<int> input_shapes = input_tensor->shape;
  std::vector<int> grid_shape = grid_tensor->shape;
  const float *input_ptr = input_tensor->cpu_data<float>();
  const float *grid_ptr = grid_tensor->cpu_data<float>();
  float *output_ptr = output_tensor->cpu_data<float>();
  assert((grid_shape.size() == 4 && grid_shape[3] == 2) ||
         (grid_shape.size() == 5 && grid_shape[4] == 3));
  const int N = input_shapes[0];
  const int C = input_shapes[1];
  const int IH = input_shapes[2];
  const int IW = input_shapes[3];
  const int OH = grid_shape[1];
  const int OW = grid_shape[2];
  
  int IHW = IH * IW;
  int OHW = OH * OW;
  int OHW2 = 2 * OHW;
  int ICHW = C * IHW;
  int OCHW = C * OHW;
  if (mode == GridSamplerBilinear) {
    for (int n = 0; n < N; ++n) {
        const float *input = input_ptr + n * ICHW;
        const float *grid = grid_ptr + n * OHW2;
        float *output = output_ptr + n * OCHW;
        for (int h = 0; h < OH; ++h) {
            for (int w = 0; w < OW; ++w) {
                auto fx =
                    computeIndex(*grid, IW, padding_mode, align_corners);
                ++grid;
                auto fy =
                    computeIndex(*grid, IH, padding_mode, align_corners);
                ++grid;
                int x = INT(std::floor(fx));
                int y = INT(std::floor(fy));
                float dx = fx - x;
                float dy = fy - y;
                float tx = 1.f - dx;
                float ty = 1.f - dy;
                float txty = tx * ty, dxty = dx * ty, txdy = tx * dy, dxdy = dx * dy;
                bool yBound_0 = y >= 0 && y < IH;
                bool yBound_1 = y + 1 >= 0 && y + 1 < IH;
                bool xBound_0 = x >= 0 && x < IW;
                bool xBound_1 = x + 1 >= 0 && x + 1 < IW;
                const float *iiter = input + y * IW + x;
                float *oiter = output;
                for (int c = 0; c < C; ++c) {
                    *oiter = 0.f;
                    if (yBound_0) {
                    if (xBound_0)
                        *oiter += iiter[0] * txty;
                    if (xBound_1)
                        *oiter += iiter[1] * dxty;
                    }
                    if (yBound_1) {
                    if (xBound_0)
                        *oiter += iiter[IW] * txdy;
                    if (xBound_1)
                        *oiter += iiter[IW + 1] * dxdy;
                    }
                    iiter += IHW;
                    oiter += OHW;
                }
                ++output;
            }
        }
    }
  } else if (mode == GridSamplerNearest) {
    for (int n = 0; n < N; ++n) {
        const float *input = input_ptr + n * ICHW;
        const float *grid = grid_ptr + n * OHW2;
        float *output = output_ptr + n * OCHW;
        for (int h = 0; h < OH; ++h) {
            for (int w = 0; w < OW; ++w) {
                auto fx =
                    computeIndex(*grid, IW, padding_mode, align_corners);
                ++grid;
                auto fy =
                    computeIndex(*grid, IH, padding_mode, align_corners);
                ++grid;
                int x = INT(std::round(fx));
                int y = INT(std::round(fy));
                const float *iiter = input + y * IW + x;
                float *oiter = output;
                for (int c = 0; c < C; ++c) {
                    *oiter = y >= 0 && y < IH && x >= 0 && x < IW ? *iiter : 0.f;
                    iiter += IHW;
                    oiter += OHW;
                }
                ++output;
            }
        }
    }
  } else {
    assert(0);
  }
  output_tensor->shape = {N, C, OH, OW};
  return;
}

}
}