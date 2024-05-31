#include <stdlib.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>

#include <mmpool.h>
#include "cvi_rt_base.h"
#include "bmruntime.h"

CviRTBase::CviRTBase() {}
CviRTBase::~CviRTBase() {}
CviRTSoc::~CviRTSoc() {}

CVI_RC CviRTSoc::SubmitBK(CVI_RT_HANDLE rt_handle) {
  cvi_device->cvikernel_submit((bmctx_t)rt_handle);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::SubmitPio(CVI_RT_HANDLE rt_handle) {
  (void)rt_handle;
  TPU_ASSERT(0, NULL);  // not support
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::Init(CVI_RT_HANDLE *rt_handle) {
  bmctx_t *ctx = (bmctx_t *)rt_handle;
  cvi_device->device_init(DEVICE_INDEX_NUM, ctx);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::DeInit(CVI_RT_HANDLE rt_handle) {
  bmctx_t ctx = (bmctx_t)rt_handle;

  //deinit basic context
  cvi_device->device_exit(ctx);
  return CVI_SUCCESS;
}

CVI_RT_KHANDLE CviRTSoc::RegisterKernel(CVI_RT_HANDLE rt_handle, uint32_t cmdbuf_size)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  cvk_reg_info_t req_info;
  cvk_context_t *tmp_cvk_context;
  cvi_rt_submit *submit_handle;

  //fill cvikernel request info
  memset(&req_info, 0, sizeof(cvk_reg_info_t));
  strncpy(req_info.chip_ver_str, chip_name_.c_str(), sizeof(req_info.chip_ver_str)-1);
  req_info.cmdbuf_size = cmdbuf_size;
  req_info.cmdbuf = (uint8_t *)malloc(req_info.cmdbuf_size);
  if (!req_info.cmdbuf) {
    TPU_ASSERT(req_info.cmdbuf, "Expect allocated cmdbuf");
    return NULL;
  }

  //register cvikernel
  tmp_cvk_context = cvikernel_register(&req_info);
  submit_handle = (cvi_rt_submit *)malloc(sizeof(cvi_rt_submit));
  if (!submit_handle) {
    TPU_ASSERT(req_info.cmdbuf, "Expect allocated kernel context");
    return NULL;
  }

  memset(submit_handle, 0, sizeof(cvi_rt_submit));

  //assign handle mapping related, and reassign cvikernel handle
  memcpy(submit_handle, tmp_cvk_context, sizeof(cvk_context_t));
  submit_handle->rt_ctx = ctx;
  submit_handle->cmdbuf = req_info.cmdbuf;
  submit_handle->magic = submit_magic_;
  free(tmp_cvk_context);

  return submit_handle;
}


CVI_RC CviRTSoc::UnRegisterKernel(CVI_RT_KHANDLE rt_khandle)
{
  cvk_context_t *cvk_context = (cvk_context_t *)rt_khandle;
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;

  if (!cvk_context) {
    TPU_ASSERT(0, "CVI_RT_UnRegisterKernel() NULL kernel handle");
    return CVI_FAILURE;
  }

  if (submit_handle->dmabuf)
    cvi_device->mem_free_raw(submit_handle->rt_ctx, submit_handle->dmabuf);

  if (cvk_context)
    cvk_context->ops->cleanup(cvk_context);

  if (cvk_context->priv_data)
    free(cvk_context->priv_data);

  if (submit_handle->cmdbuf) {
    free(submit_handle->cmdbuf);
  }

  free(rt_khandle);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::SubmitAsync(CVI_RT_KHANDLE rt_khandle, uint8_t submit_previous)
{
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;
  uint32_t len;
  bmctx_t ctx = submit_handle->rt_ctx;

  if (submit_handle->magic != submit_magic_) {
    TPU_LOG_WARNING("incorrect submit handle input\n");
    return CVI_FAILURE;
  }

  if (submit_previous) {
    if (submit_handle->dmabuf) {
      cvi_run_async(ctx, submit_handle->dmabuf);
    } else {
      TPU_LOG_WARNING("CVI_RT_SubmitAsync() previous cmdbuff NULL!\n");
      return CVI_FAILURE;
    }

  } else {
    cvk_context_t *cvk_context = &submit_handle->cvk_ctx;
    uint8_t *cmdbuf = cvk_context->ops->acquire_cmdbuf(cvk_context, &len);
    bmmem_device_t dmabuf_mem;

    //free last
    if (submit_handle->dmabuf)
      cvi_device->mem_free_raw(ctx, submit_handle->dmabuf);

    //load and run
    cvi_device->load_cmdbuf(ctx, cmdbuf, (size_t)len, 0, 0, false, &dmabuf_mem);
    cvi_device->run_async(ctx, dmabuf_mem);

    //record the last
    submit_handle->dmabuf = dmabuf_mem;
    cvk_context->ops->reset(cvk_context);
  }

  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::WaitForAsync(CVI_RT_KHANDLE rt_khandle)
{
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;
  return (CVI_RC)cvi_device->wait_cmdbuf_all(submit_handle->rt_ctx);
}

CVI_RC CviRTSoc::Submit(CVI_RT_KHANDLE rt_khandle)
{
  cvi_rt_submit *submit_handle = (cvi_rt_submit *)rt_khandle;
  uint32_t len;
  uint16_t seq_no;

  if (submit_handle->magic != submit_magic_) {
    TPU_LOG_WARNING("incorrect submit handle input\n");
    return CVI_FAILURE;
  }

  cvk_context_t *cvk_context = &submit_handle->cvk_ctx;
  uint8_t *cmdbuf = cvk_context->ops->acquire_cmdbuf(cvk_context, &len);

  int ret = cvi_device->send_cmdbuf(submit_handle->rt_ctx, cmdbuf, (size_t)len, &seq_no);
  if (ret != 0) {
    TPU_LOG_WARNING("send_cmdbuf failed\n");
  }
  cvk_context->ops->reset(cvk_context);
  return ret;
}

CVI_RC CviRTSoc::LoadCmdbuf(
    CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
    uint64_t cmdbuf_sz, uint64_t gaddr_base0,
    uint64_t gaddr_base1, bool enable_pmu,
    CVI_RT_MEM *cmdbuf_mem)
{
  return (CVI_RC)cvi_device->load_cmdbuf(
                    (bmctx_t)rt_handle, cmdbuf, (size_t)cmdbuf_sz,
                    (uint64_t)gaddr_base0,
                    (uint64_t)gaddr_base1,
                    enable_pmu, (bmmem_device_t *)cmdbuf_mem);
}

CVI_RC CviRTSoc::LoadDmabuf(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM dmabuf,
    uint64_t dmabuf_sz, uint64_t gaddr_base0,
    uint64_t gaddr_base1, bool enable_pmu, CVI_RT_MEM *dmabuf_mem)
{
  return (CVI_RC)cvi_device->load_dmabuf(
                    (bmctx_t)rt_handle, (bmmem_device_t)dmabuf, 
                    (size_t)dmabuf_sz,
                    (uint64_t)gaddr_base0,
                    (uint64_t)gaddr_base1,
                    enable_pmu,
                    (bmmem_device_t *)dmabuf_mem);
}

CVI_RC CviRTSoc::RunCmdbuf(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    uint64_t gaddr_base2, uint64_t gaddr_base3)
{
  CVI_RC ret;
  uint16_t seq_no;
  ret = (CVI_RC)cvi_device->run_cmdbuf_ex(
                    (bmctx_t)rt_handle, (bmmem_device_t)cmdbuf_mem,
                    &seq_no, gaddr_base2, gaddr_base3);
  if (ret != 0) {
    TPU_LOG_ERROR("RunCmdbuf fail!");
    return ret;
  }

  return (CVI_RC)cvi_device->wait_cmdbuf_done((bmctx_t)rt_handle, seq_no);
}

CVI_RC CviRTSoc::RunCmdbufEx(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    CVI_RT_ARRAYBASE *p_array_base)
{
  CVI_RC ret;
  uint16_t seq_no;

  ret = (CVI_RC)cvi_device->run_cmdbuf_ex2(
                    (bmctx_t)rt_handle, (bmmem_device_t)cmdbuf_mem,
                    &seq_no, (cvi_array_base *)p_array_base);
  if (ret != 0)
    return ret;

  return (CVI_RC)cvi_device->wait_cmdbuf_done((bmctx_t)rt_handle, seq_no);
}

CVI_RT_MEM CviRTSoc::MemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size)
{
  return (CVI_RT_MEM)cvi_device->mem_alloc_raw((bmctx_t)rt_handle, size);
}

CVI_RT_MEM CviRTSoc::MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size)
{
  bm_memory_t *dev_mem = (bm_memory_t*)mem;

  TPU_ASSERT(dev_mem != nullptr, nullptr);
  TPU_ASSERT(dev_mem->size >= size + offset, nullptr);
  bm_memory_t *preAlloc_mem = new bm_memory_t();
  preAlloc_mem->flags.u.is_prealloc = 1;
  preAlloc_mem->flags.u.type = BMMEM_TYPE_DEVICE;
  preAlloc_mem->p_addr = ((bm_memory_t *)dev_mem)->p_addr + offset;
  preAlloc_mem->v_addr = ((bm_memory_t *)dev_mem)->v_addr + offset;
  preAlloc_mem->dma_fd = ((bm_memory_t *)dev_mem)->dma_fd;
  preAlloc_mem->size = size;
  return (CVI_RT_MEM)preAlloc_mem;
}

void CviRTSoc::MemFree(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  cvi_device->mem_free_raw((bmctx_t)rt_handle, (bmmem_device_t)mem);
}

void CviRTSoc::MemFreeEx(uint64_t p_addr)
{
  cvi_device->mem_free_ex(p_addr);
}

uint64_t CviRTSoc::MemGetSize(CVI_RT_MEM mem)
{
  if (!mem)
    return 0;
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);
  return dev_mem->size;
}

uint64_t CviRTSoc::MemGetPAddr(CVI_RT_MEM mem)
{
  if (!mem)
    return 0;
  bm_memory_t *dev_mem = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);
  return dev_mem->p_addr;
}

uint8_t* CviRTSoc::MemGetVAddr(CVI_RT_MEM mem)
{
  if (!mem)
    return 0;
  bm_memory_t *dev_mem  = (bm_memory_t *)mem;
  TPU_ASSERT(dev_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);
  return dev_mem->v_addr;
}

int32_t CviRTSoc::MemIncRef(CVI_RT_MEM mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);
  return (++device_mem->user_ref_cnt);
}

int32_t CviRTSoc::MemDecRef(CVI_RT_MEM mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);
  return (--device_mem->user_ref_cnt);
}

CVI_RC CviRTSoc::MemFlush(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(cvi_device->mem_flush_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, device_mem->size) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemInvld(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(cvi_device->mem_invld_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, device_mem->size) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemFlushEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(cvi_device->mem_flush_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, len) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemInvldEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(cvi_device->mem_invld_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, len) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemCopyS2D(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst, uint8_t* src)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)dst;
  memcpy(device_mem->v_addr, src, device_mem->size);
  TPU_ASSERT((int)cvi_device->mem_flush_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, device_mem->size) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemCopyS2DEx(
    CVI_RT_HANDLE rt_handle,
    CVI_RT_MEM dst, uint64_t offset,
    uint64_t len, uint8_t* src)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)dst;
  TPU_ASSERT((size_t)(offset + len) <= device_mem->size, nullptr);
  memcpy(device_mem->v_addr + offset, src, len);
  TPU_ASSERT((int)cvi_device->mem_flush_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr + offset, len) == BM_SUCCESS, NULL);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::MemCopyD2S(CVI_RT_HANDLE rt_handle, uint8_t* dst, CVI_RT_MEM src)
{
  bmctx_t ctx = (bmctx_t)rt_handle;
  bm_memory_t *device_mem = (bm_memory_t *)src;
  TPU_ASSERT(cvi_device->mem_invld_ext(ctx->dev, device_mem->dma_fd,
        device_mem->p_addr, device_mem->size) == BM_SUCCESS, NULL);
  memcpy(dst, device_mem->v_addr, device_mem->size);
  return CVI_SUCCESS;
}

CVI_RC CviRTSoc::ParsePmuBuf(CVI_RT_MEM cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len)
{
  return (CVI_RC)cvi_device->parse_pmubuf((bmmem_device_t)cmdbuf_mem, buf_start, buf_len);
}

CVI_RC CviRTSoc::SetBaseReg(CVI_RT_HANDLE rt_handle, uint32_t inx, uint64_t base_addr)
{
  bmctx_t ctx = (bmctx_t)rt_handle;

  if (inx == 0)
    ctx->array_base0 = base_addr;
  else if (inx == 1)
    ctx->array_base1 = base_addr;
  else
    TPU_ASSERT(0, NULL); // not support

  return CVI_SUCCESS;
}