#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/deform_im2col.hpp>

#include <chrono>

namespace cvi {
namespace runtime {

// deconstructor
DeformableIm2ColFunc::~DeformableIm2ColFunc() {}

void DeformableIm2ColFunc::setup(tensor_list_t &inputs,
            tensor_list_t &outputs,
            OpParam &param) {

  // init in/out
  top_ = outputs[0];
  bottom_ = inputs[0];
  offset_ = inputs[1];
  mask_ = inputs[2];

  // init parameter
  kernel_h_ = param.get<int>("kernel_h");
  kernel_w_ = param.get<int>("kernel_w");
  stride_h_ = param.get<int>("stride_h");
  stride_w_ = param.get<int>("stride_w");
  pad_h_ = param.get<int>("padding_t");
  pad_w_ = param.get<int>("padding_l");
  dilation_h_ = param.get<int>("dilation_h");
  dilation_w_ = param.get<int>("dilation_w");
  deformable_group_ = param.get<int>("deform_group");

  // sanity check
  assert((CVI_FMT_FP32 == bottom_->fmt && bottom_->fmt == top_->fmt) &&
      "ONLY support fp32 now");
}

void DeformableIm2ColFunc::run () {

  auto shape = bottom_->shape;
  const int batch = shape[0];
  const int channels = shape[1];
  const int height = shape[2];
  const int width = shape[3];

  const int height_out = (height + 2 * pad_h_ - (dilation_h_ * (kernel_h_ - 1) + 1)) / stride_h_ + 1;
  const int width_out = (width + 2 * pad_w_ - (dilation_w_ * (kernel_w_ - 1) + 1)) / stride_w_ + 1;

  printf("input shape: (%d, %d, %d, %d)\n", batch, channels, height, width);
  printf("output shape: (%d, %d)\n", channels*kernel_h_*kernel_w_, height_out * width_out);

  std::vector<float> ones(height_out*width_out, 1);
  std::vector<float> columns(channels * kernel_h_ * kernel_w_ * 1 * height_out * width_out);
  auto output_ = top_;
  int input_chw = channels * height * width * 1;
  int offset_chw = offset_->shape[1] * offset_->shape[2] * offset_->shape[3] * 1;
  int mask_chw = mask_->shape[1] * mask_->shape[2] * mask_->shape[3] * 1;
  int output_chw = output_->shape[1] * output_->shape[2] * output_->shape[3] * 1;

  for (int b = 0; b < batch; b++)
  {
    auto input_n = &bottom_->cpu_data<float>()[b * input_chw];
    auto offset_n = &offset_->cpu_data<float>()[b * offset_chw];
    auto mask_n = &mask_->cpu_data<float>()[b * mask_chw];
    auto output_n = &output_->cpu_data<float>()[b * output_chw];

    auto t1 = std::chrono::system_clock::now();

    modulated_deformable_im2col_cpu(input_n,
        offset_n,
        mask_n,
        1, channels, height, width,
        height_out, width_out, kernel_h_, kernel_w_,
        pad_h_, pad_w_, stride_h_, stride_w_, dilation_h_, dilation_w_,
        deformable_group_,
        output_n);

    auto t2 = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> duration = t2 - t1;
    std::cout << "duration: " << duration.count() << "(ms)\n";
  }
}

} // namespace runtime
} // namespace cvi
