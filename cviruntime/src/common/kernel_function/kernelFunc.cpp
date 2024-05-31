#include "cviruntime.h"
#include "IKernelFunc.hpp"
#include <runtime/kernel_function.hpp>

CVI_KFUNC_HANDLE CVI_NN_PrepareEuclideanDistanceKernelFunc(
    CVI_RT_HANDLE ctx, CVI_FMT fmt, uint32_t k, uint32_t n) {
  (void)fmt;
  auto cvk = CVI_RT_RegisterKernel(ctx, 200000);
  assert(cvk);

  auto kfun = new cvi::runtime::IKernelFunc(ctx);
  kfun->cmdbuf_mem = cvi::runtime::runtimeJitEuclideanDistance(ctx, cvk, n, k);
  CVI_RT_UnRegisterKernel(cvk);
  return (void *)kfun;
}

CVI_KFUNC_HANDLE CVI_NN_PrepareMatrixMulKernelFunc(
    CVI_RT_HANDLE ctx, CVI_FMT fmt, uint32_t m, uint32_t k, uint32_t n) {

  auto cvk = CVI_RT_RegisterKernel(ctx, 200000);
  assert(cvk);

  auto kfun = new cvi::runtime::IKernelFunc(ctx);
  kfun->cmdbuf_mem = cvi::runtime::runtimeJitMatrixMul(ctx, cvk, fmt, m, k, n);
  CVI_RT_UnRegisterKernel(cvk);
  return (void *)kfun;
}

CVI_KFUNC_HANDLE CVI_NN_PrepareGrayImageLightKernelFunc(
    CVI_RT_HANDLE ctx, uint32_t ih, uint32_t iw, uint32_t kernel_sz) {

  auto cvk = CVI_RT_RegisterKernel(ctx, 100000);
  assert(cvk);

  auto kfun = new cvi::runtime::IKernelFunc(ctx);
  kfun->cmdbuf_mem = cvi::runtime::runtimeJitGrayImageLight(ctx, cvk, ih, iw, kernel_sz);
  CVI_RT_UnRegisterKernel(cvk);
  return (void *)kfun;
}

CVI_RC CVI_NN_RunKernelFunc(CVI_KFUNC_HANDLE kfun, int32_t mem_num, ...) {
  assert(mem_num <= 6);
  uint64_t baseArray[mem_num + 2];

  baseArray[0] = 0;
  baseArray[1] = 0;

  va_list valist;
  va_start(valist, mem_num);
  for (int i = 2; i < mem_num + 2; i++) {
    baseArray[i] = va_arg(valist, uint64_t);
  }
  va_end(valist);
  auto kfn = static_cast<cvi::runtime::IKernelFunc *>(kfun);
  return CVI_RT_RunCmdbufEx(kfn->ctx, kfn->cmdbuf_mem, (CVI_RT_ARRAYBASE *)baseArray);
}

CVI_RC CVI_NN_DestroyKernelFunc(CVI_KFUNC_HANDLE kfun) {
  auto kfn = static_cast<cvi::runtime::IKernelFunc *>(kfun);
  delete kfn;
  return CVI_RC_SUCCESS;
}
