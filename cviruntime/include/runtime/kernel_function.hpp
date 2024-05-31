#ifndef RUNTIME_TDMA_COPY_HPP
#define RUNTIME_TDMA_COPY_HPP

#include "cviruntime_context.h"
#include "cviruntime.h"
#include "cvikernel/cvikernel.h"

namespace cvi {
namespace runtime {

CVI_RC runtimeExecuteKernelFunction(
    CVI_RT_HANDLE ctx, CVI_RT_MEM codeBuf,
    uint64_t gaddrSrc, uint64_t gaddrDst);

CVI_RT_MEM runtimeJitTdmaStrideCopy(
    CVI_RT_HANDLE ctx, void *cvk, CVI_FMT fmt,
    cvk_tg_shape_t *shapeDst, cvk_tg_stride_t *strideDst,
    cvk_tg_shape_t *shapeSrc, cvk_tg_stride_t *strideSrc);

CVI_RT_MEM runtimeJitMatrixMul(
    CVI_RT_HANDLE ctx, void* cvk_ctx, CVI_FMT fmt,
    uint32_t m, uint32_t k, uint32_t n);

CVI_RT_MEM runtimeJitEuclideanDistance(
    CVI_RT_HANDLE ctx, void* cvk_ctx,
    uint32_t records, uint32_t feature_size);

CVI_RT_MEM runtimeJitGrayImageLight(
    CVI_RT_HANDLE ctx, void* cvk_ctx,
    int32_t ih, int32_t iw, int32_t kernel_sz);
}

}
#endif