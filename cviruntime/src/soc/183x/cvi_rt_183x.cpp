#include "cvi_rt_183x.h"
std::unique_ptr<CviRTSoc> cvi_chip(new CviRT183x());

CviRT183x::CviRT183x() {
  chip_name_ = "cv183x";
  submit_magic_ = 0x18325678;
  cvi_device = std::move(std::unique_ptr<CviDeviceMem>(new Cvi183xDeviceMem()));
}

CviRT183x::~CviRT183x() {}

CVI_RT_KHANDLE CviRT183x::GetKHandleBK(CVI_RT_HANDLE rt_handle) {
    bmctx_t ctx = (bmctx_t)rt_handle;
    return (CVI_RT_KHANDLE)(ctx->cvik_context.ctx183x);
}

CVI_RC CviRT183x::DeInitBK(CVI_RT_HANDLE rt_handle) {
    bmctx_t ctx = (bmctx_t)rt_handle;

    //deinit kernel related
    if (ctx->cvik_context.ctx183x) {
        bmk1880v2_cleanup(ctx->cvik_context.ctx183x);
    }

    if (ctx->cvik_cmdbuf) {
        free(ctx->cvik_cmdbuf);
    }

    //deinit basic context
    bm_exit(ctx);
    return CVI_SUCCESS;
}

CVI_RC CviRT183x::InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size) {
    bmctx_t *ctx = (bmctx_t *)rt_handle;

    //init basic context
    bm_init(DEVICE_INDEX_NUM, ctx);

    //init cvikernel related
    bmk1880v2_chip_info_t info      = bmk1880v2_chip_info();
    bmk1880v2_chip_info_t *dev_info = &info;

    bmk_info_t bmk_info;
    bmk_info.chip_version = dev_info->version;
    bmk_info.cmdbuf_size  = cmdbuf_size;
    bmk_info.cmdbuf       = (u8 *)malloc(bmk_info.cmdbuf_size);
    if (!bmk_info.cmdbuf) {
        TPU_ASSERT(bmk_info.cmdbuf, "malloc kernel buffer failed");
        return CVI_FAILURE;
    }

    (*ctx)->cvik_context.ctx183x = bmk1880v2_register(&bmk_info);
    (*ctx)->cvik_cmdbuf          = (void *)bmk_info.cmdbuf;

    return CVI_SUCCESS;
}

CVI_RC CviRT183x::LoadCmdbufTee(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                     size_t sz, uint64_t neuron_gaddr,
                     uint64_t weight_gaddr, uint32_t weight_len,
                     CVI_RT_MEM *cmdbuf_mem) {
    return (CVI_RC)cvi_device->load_cmdbuf_tee((bmctx_t)rt_handle, cmdbuf,
                                               sz, neuron_gaddr, weight_gaddr,
                                               weight_len, (bmmem_device_t *)cmdbuf_mem);
}

CVI_RC CviRT183x::RunCmdbufTee(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    CVI_RT_ARRAYBASE *p_array_base)
{
  CVI_RC ret;
  uint16_t seq_no;
  bm_memory_t *mem = (bm_memory_t *)cmdbuf_mem;

  ret = (CVI_RC)cvi_device->run_cmdbuf_tee(
                    (bmctx_t)rt_handle,
                    &seq_no, mem->p_addr, (cvi_array_base *)p_array_base);
  if (ret != 0)
    return ret;

  return (CVI_RC)cvi_device->wait_cmdbuf_done((bmctx_t)rt_handle, seq_no);
}