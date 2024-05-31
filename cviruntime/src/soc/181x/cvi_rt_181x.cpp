#include "cvi_rt_181x.h"

std::unique_ptr<CviRTSoc> cvi_chip(new CviRT181x());

CviRT181x::CviRT181x() {
    chip_name_    = "cv181x";
    submit_magic_ = 0x18225678;
    cvi_device    = std::move(std::unique_ptr<CviDeviceMem>(new Cvi181xDeviceMem()));
}

CviRT181x::~CviRT181x() {}

CVI_RT_KHANDLE CviRT181x::GetKHandleBK(CVI_RT_HANDLE rt_handle) {
    bmctx_t ctx = (bmctx_t)rt_handle;
    return (CVI_RT_KHANDLE)(ctx->cvik_context.ctx182x);
}

CVI_RC CviRT181x::DeInitBK(CVI_RT_HANDLE rt_handle) {
    bmctx_t ctx = (bmctx_t)rt_handle;

    //deinit kernel related
    if (ctx->cvik_context.ctx182x) {
        bmk1822_cleanup(ctx->cvik_context.ctx182x);
    }

    if (ctx->cvik_cmdbuf) {
        free(ctx->cvik_cmdbuf);
    }

    //deinit basic context
    bm_exit(ctx);
    return CVI_SUCCESS;
}

CVI_RC CviRT181x::InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size) {
    bmctx_t *ctx = (bmctx_t *)rt_handle;

    //init basic context
    bm_init(DEVICE_INDEX_NUM, ctx);

    //init cvikernel related
    bmk1822_chip_info_t info      = bmk1822_chip_info();
    bmk1822_chip_info_t *dev_info = &info;

    bmk_info_t bmk_info;
    bmk_info.chip_version = dev_info->version;
    bmk_info.cmdbuf_size  = cmdbuf_size;
    bmk_info.cmdbuf       = (u8 *)malloc(bmk_info.cmdbuf_size);
    if (!bmk_info.cmdbuf) {
        TPU_ASSERT(bmk_info.cmdbuf, "malloc kernel buffer failed");
        return CVI_FAILURE;
    }

    (*ctx)->cvik_context.ctx182x = bmk1822_register(&bmk_info);
    (*ctx)->cvik_cmdbuf          = (void *)bmk_info.cmdbuf;

    return CVI_SUCCESS;
}

CVI_RC CviRT181x::LoadCmdbufTee(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                                size_t sz, uint64_t neuron_gaddr,
                                uint64_t weight_gaddr, uint32_t weight_len,
                                CVI_RT_MEM *cmdbuf_mem) {
    (void)rt_handle;
    (void)cmdbuf;
    (void)sz;
    (void)neuron_gaddr;
    (void)weight_gaddr;
    (void)weight_len;
    (void)cmdbuf_mem;
    TPU_ASSERT(0, NULL);  // not support
    return CVI_SUCCESS;
}

CVI_RC CviRT181x::RunCmdbufTee(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                              CVI_RT_ARRAYBASE *p_array_base) {
    (void)rt_handle;
    (void)p_array_base;
    (void)cmdbuf_mem;
    TPU_ASSERT(0, NULL); // not support
    return CVI_SUCCESS;
}
