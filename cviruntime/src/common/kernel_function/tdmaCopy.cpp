#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <iostream>
#include <runtime/debug.h>
#include <runtime/kernel_function.hpp>
#include <inttypes.h>

namespace cvi {
namespace runtime {

static CVI_RT_MEM runtimeJitCompile(CVI_RT_HANDLE ctx, void *cvk) {
  uint32_t sz;
  CVI_RT_MEM cmdbuf_mem;
  auto cvkernel = (cvk_context_t *)cvk;
  auto cmdbuf = cvkernel->ops->acquire_cmdbuf(cvkernel, &sz);
  int ret = CVI_RT_LoadCmdbuf(ctx, cmdbuf, sz, 0, 0, false, &cmdbuf_mem);
  cvkernel->ops->reset(cvkernel);
  if (ret != 0) {
      TPU_LOG_WARNING("runtimeJitCompile failed ret[%d]\n", ret);
      return nullptr;
  }
  return cmdbuf_mem;
}

CVI_RC runtimeExecuteKernelFunction(CVI_RT_HANDLE ctx, CVI_RT_MEM codeBuf,
                                  uint64_t gaddrSrc, uint64_t gaddrDst) {
  //TPU_LOG_DEBUG("runtimeExecuteKernelFunction, src:%" PRIu64 " dst:%" PRIu64 "\n", gaddrSrc, gaddrDst);
  CVI_RC ret = CVI_RT_RunCmdbuf(ctx, codeBuf, gaddrSrc, gaddrDst);
  if (ret != 0) {
    TPU_LOG_WARNING("runtimeExecuteKernelFunction failed ret[%d]\n", ret);
  }
  return ret;
}

CVI_RT_MEM runtimeJitTdmaStrideCopy(CVI_RT_HANDLE ctx, void *cvk, CVI_FMT fmt,
                                    cvk_tg_shape_t *shapeDst,
                                    cvk_tg_stride_t *strideDst,
                                    cvk_tg_shape_t *shapeSrc,
                                    cvk_tg_stride_t *strideSrc) {
  auto cvkernel = (cvk_context_t *)cvk;
  // programing with cvikernel intrinsic functions
  cvk_fmt_t cvk_fmt;
  if (fmt == CVI_FMT_INT8) {
    cvk_fmt = CVK_FMT_I8;
  } else if (fmt == CVI_FMT_UINT8) {
    cvk_fmt = CVK_FMT_U8;
  } else {
    cvk_fmt = CVK_FMT_BF16;
  }

  cvk_tg_t src;
  src.base_reg_index = 2;
  src.start_address = 0;
  src.shape = *shapeSrc;
  if (strideSrc) {
    src.stride = *strideSrc;
  } else {
    src.stride = cvkernel->ops->tg_default_stride(cvkernel, src.shape, cvk_fmt);
  }
  src.fmt = CVK_FMT_I8;

  cvk_tg_t dst;
  dst.base_reg_index = 3;
  dst.start_address = 0;
  dst.shape = *shapeDst;
  if (strideDst) {
    dst.stride = *strideDst;
  } else {
    dst.stride = cvkernel->ops->tg_default_stride(cvkernel, dst.shape, cvk_fmt);
  }
  dst.fmt = CVK_FMT_I8;

  cvk_tdma_g2g_tensor_copy_param_t p;
  p.src = &src;
  p.dst = &dst;

  cvkernel->ops->tdma_g2g_tensor_copy(cvkernel, &p);

  // do jit complilation
  return runtimeJitCompile(ctx, cvk);
}

} // namespace runtime
} // namespace cvi