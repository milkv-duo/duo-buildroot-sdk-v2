#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>      // std::accumulate
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/instancenorm.hpp>

namespace cvi {
namespace runtime {

// Y = (X-mean(X))/(sqrt(var(X)+variance_epsilon))
static int my_bn(float *input, float *mean, float *variance, float *scale, float variance_epsilon,
    float *output, int n, int c, int h, int w, float *bias) {
  float scale_factor = 1 / scale[0];
  for (int i = 0; i < c; ++i) {
    mean[i] = mean[i] * scale_factor;
    variance[i] = variance[i] * scale_factor;
  }
  for (int ni = 0; ni < n; ++ni) {
    for (int ci = 0; ci < c; ++ci) {
      float b = 0;
      if (bias) {
        b = bias[ci];
      }

      for (int i = 0; i < h * w; ++i) {
        auto x = input[ni * c * h * w + ci * h * w + i] - mean[ci];
        auto d = sqrt(variance[ci] + variance_epsilon);
        output[ni * c * h * w + ci * h * w + i] = x / d + b;
        if (fabs(variance[ci]) <= variance_epsilon && fabs(mean[ci]) <= 1e-8
            && fabs(input[ni * c * h * w + ci * h * w + i]) >= 1.0e-4
            && fabs(output[ni * c * h * w + ci * h * w + i]) >= 1.0e-2) {
          //assert(0);
        }
      }
    }
  }
  for (int i = 0; i < c; ++i) {
    mean[i] = mean[i] * scale[0];
    variance[i] = variance[i] * scale[0];
  }
  return 0;
}

static int my_in(float *input, float* gamma_value,
    float* beta_value,
    float *output, float variance_epsilon, int n, int c, int h, int w) {

  std::vector<float> mean(c);
  std::vector<float> variance(c);
  int hw = h * w;

  for (int ni = 0; ni < n; ++ni) {
    for (int ci = 0; ci < c; ++ci) {
      //int channel_shift = ni * c * h * w + ci * h * w;
      //auto start = input + channel_shift * sizeof(float);
      //mean[ci] = std::accumulate(start, start + hw, static_cast<float>(0.0)) / hw;
      {
        // TODO: leverage std::accumulate
        float m = 0;
        for (int i = 0; i < hw; i++) {
          m += input[ni * c * h * w + ci * h * w + i];
        }

        mean[ci] = m / hw;
      }

      float var = 0;

      for (int i = 0; i < hw; ++i) {
        var += pow((input)[ni * c * h * w + ci * h * w + i] - mean[ci], 2);
      }
      var = (var) / hw;
      variance[ci] = var;
    }
  }

  return my_bn(input, mean.data(), variance.data(),
      gamma_value, variance_epsilon, output, n, c, h, w,
      beta_value);
}

InstanceNormFunc::~InstanceNormFunc() {}

void InstanceNormFunc::setup(tensor_list_t &inputs,
            tensor_list_t &outputs,
            OpParam &param) {

  top_ = outputs[0];
  bottom_ = inputs[0];
  scale_ = inputs[1];
  bias_ = inputs[2];

  auto shape = bottom_->shape;
  assert((CVI_FMT_FP32 == bottom_->fmt && bottom_->fmt == top_->fmt) &&
         "ONLY support fp32 now");

  variance_epsilon_ = param.get<float>("variance_epsilon");
  num_ = shape[0];
  channels_ = shape[1];
  h_ = shape[2];
  w_ = shape[3];
}

void InstanceNormFunc::run() {
  auto input = bottom_->cpu_data<float>();
  auto output = top_->cpu_data<float>();
  auto scale = scale_->cpu_data<float>();
  auto bias = bias_->cpu_data<float>();

  int in = num_;
  int ic = channels_;

  my_in(input, scale, bias, output, variance_epsilon_, in, ic, h_, w_);
}

} // namespace runtime
} // namespace cvi
