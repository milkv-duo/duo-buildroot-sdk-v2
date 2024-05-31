#include <iostream>
#include <vector>
#include <cmath>
#include <numeric>      // std::accumulate
#include <runtime/debug.h>
#include <runtime/neuron.hpp>
#include <cpu_function/deformableconv.hpp>

#include <chrono>

//#include <ATen/ATen.h>
//#include <ATen/cuda/CUDAContext.h>

//#include <TH/TH.h>
//#include <THC/THCAtomics.cuh>
//#include <THC/THCDeviceUtils.cuh>

//extern THCState *state;

// author: Charles Shang
// https://github.com/torch/cunn/blob/master/lib/THCUNN/generic/SpatialConvolutionMM.cu
// modified from the CUDA version for CPU use by Daniel K. Suhendro
//
// come from https://github.com/CharlesShang/DCNv2

namespace cvi {
namespace runtime {

#define X(i) x[ (i)*incx ]
static void AddDot( int k, float *x, int incx,  float *y, float *gamma,
    float alpha, float beta)
{
  /* compute gamma := x' * y + gamma with vectors x and y of length n.
     Here x starts at location x with increment (stride) incx and y starts at location y and has (implicit) stride of 1.
  */

  int p;

  float _gamma = 0;
  for ( p=0; p<k; p++ ){
    //*gamma += X( p ) * y[ p ];
    _gamma += X( p ) * y[ p ];
  }
  *gamma = alpha * _gamma + beta * (*gamma);
}
#define A(i,j) a[ (j)*lda + (i) ]
#define B(i,j) b[ (j)*ldb + (i) ]
#define C(i,j) c[ (j)*ldc + (i) ]
// the defination comes from https://docs.rs/torch/0.1.0/src/torch/lib.rs.html#8865-8869
// and we implemented by https://github.com/flame/how-to-optimize-gemm/blob/master/src/MMult1.c
static void THFloatBlas_gemm(char transa,
                          char transb,
                          int m,
                          int n,
                          int k, float alpha,
                          float* a, int lda,
                          float* b, int ldb,
                          float beta, float* c,
                          int ldc) {
  (void)transa;
  (void)transb;
  int i, j;

  for ( j=0; j<n; j+=1 ){        /* Loop over the columns of C */
    for ( i=0; i<m; i+=1 ){        /* Loop over the rows of C */
      /* Update the C( i,j ) with the inner product of the ith row of A
         and the jth column of B */

      AddDot( k, &A( i,0 ), lda, &B( 0,j ), &C( i,j ), alpha, beta);
    }
  }
}

// deconstructor
DeformableConvFunc::~DeformableConvFunc() {}

void DeformableConvFunc::setup(tensor_list_t &inputs,
            tensor_list_t &outputs,
            OpParam &param) {

  // init in/out
  top_ = outputs[0];
  bottom_ = inputs[0];
  weight_ = inputs[1];
  bias_ = inputs[2];
  offset_ = inputs[3];
  mask_ = inputs[4];

  // init parameter
  kernel_h_ = weight_->shape[2];
  kernel_w_ = weight_->shape[3];
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

void DeformableConvFunc::run () {

  auto shape = bottom_->shape;
  const int batch = shape[0];
  const int channels = shape[1];
  const int height = shape[2];
  const int width = shape[3];

  shape = weight_->shape;
  const int channels_out = shape[0];

  const int height_out = (height + 2 * pad_h_ - (dilation_h_ * (kernel_h_ - 1) + 1)) / stride_h_ + 1;
  const int width_out = (width + 2 * pad_w_ - (dilation_w_ * (kernel_w_ - 1) + 1)) / stride_w_ + 1;

  printf("input shape: (%d, %d, %d, %d)\n", batch, channels, height, width);
  printf("output shape: (%d, %d, %d, %d)\n", batch, channels_out, height_out, width_out);

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

    // Do Bias first:
    // M,N,K are dims of matrix A and B
    // (see http://docs.nvidia.com/cuda/cublas/#cublas-lt-t-gt-gemm)
    // (N x 1) (1 x M)
    long m_ = channels_out;
    long n_ = height_out * width_out;
    long k_ = 1;

    auto t1 = std::chrono::system_clock::now();
    // C = alpha * A * B + beta * C
    THFloatBlas_gemm('t', 'n', n_, m_, k_, 1.0f,
        ones.data(), k_,
        bias_->cpu_data<float>(), k_, 0.0f,
        output_n, n_);

    auto t2 = std::chrono::system_clock::now();

    std::chrono::duration<double, std::milli> duration = t2 - t1;
    std::cout << "step1: " << duration.count() << "(ms)\n";
    t1 = t2;

    modulated_deformable_im2col_cpu(input_n,
        offset_n,
        mask_n,
        1, channels, height, width,
        height_out, width_out, kernel_h_, kernel_w_,
        pad_h_, pad_w_, stride_h_, stride_w_, dilation_h_, dilation_w_,
        deformable_group_,
        columns.data());

    t2 = std::chrono::system_clock::now();
    duration = t2 - t1;
    std::cout << "step2: " << duration.count() << "(ms)\n";

    t1 = t2;
    //(k * m)  x  (m * n)
    // Y = WC
    long m = channels_out;
    long n = height_out * width_out;
    long k = channels * kernel_h_ * kernel_w_;
    printf("m: %ld, n: %ld, k: %ld\n", m, n, k);
    THFloatBlas_gemm('n', 'n', n, m, k, 1.0f,
        columns.data(), n,
        weight_->cpu_data<float>(), k, 1.0f,
        output_n, n);

    t2 = std::chrono::system_clock::now();
    duration = t2 - t1;
    std::cout << "step3: " << duration.count() << "(ms)\n";
  }
}

} // namespace runtime
} // namespace cvi
