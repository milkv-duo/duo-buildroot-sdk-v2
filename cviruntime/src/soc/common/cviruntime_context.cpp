#include <stdlib.h>
#include <cstdlib>
#include <memory>
#include "cviruntime_context.h"
#include "bmruntime.h"
#include "bmruntime_internal.h"

#include <runtime/debug.h>
#include <mmpool.h>
#include "cvi_rt_base.h"

#define DEVICE_INDEX_NUM 0

extern std::unique_ptr<CviRTSoc> cvi_chip;

CVI_RC CVI_RT_DeInitBK(CVI_RT_HANDLE rt_handle)
{
  return cvi_chip->DeInitBK(rt_handle);
}

CVI_RC CVI_RT_InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size)
{
  return cvi_chip->InitWithKernelBK(rt_handle, cmdbuf_size);
}

CVI_RC CVI_RT_SubmitBK(CVI_RT_HANDLE rt_handle)
{
  return cvi_chip->SubmitBK(rt_handle);
}

CVI_RT_KHANDLE CVI_RT_GetKHandleBK(CVI_RT_HANDLE rt_handle)
{
  return cvi_chip->GetKHandleBK(rt_handle);
}

CVI_RC CVI_RT_SubmitPio(CVI_RT_HANDLE rt_handle)
{
  return cvi_chip->SubmitPio(rt_handle);
}

CVI_RC CVI_RT_Init(CVI_RT_HANDLE *rt_handle)
{
  return cvi_chip->Init(rt_handle);
}

CVI_RC CVI_RT_DeInit(CVI_RT_HANDLE rt_handle)
{
  return cvi_chip->DeInit(rt_handle);
}

CVI_RT_KHANDLE CVI_RT_RegisterKernel(CVI_RT_HANDLE rt_handle, uint32_t cmdbuf_size)
{
   return cvi_chip->RegisterKernel(rt_handle, cmdbuf_size);
}

CVI_RC CVI_RT_UnRegisterKernel(CVI_RT_KHANDLE rt_khandle)
{
  return cvi_chip->UnRegisterKernel(rt_khandle);
}

CVI_RC CVI_RT_SubmitAsync(CVI_RT_KHANDLE rt_khandle, uint8_t submit_previous)
{
  return cvi_chip->SubmitAsync(rt_khandle, submit_previous);
}

CVI_RC CVI_RT_WaitForAsync(CVI_RT_KHANDLE rt_khandle)
{
  return cvi_chip->WaitForAsync(rt_khandle);
}

CVI_RC CVI_RT_Submit(CVI_RT_KHANDLE rt_khandle)
{
  return cvi_chip->Submit(rt_khandle);
}

CVI_RC CVI_RT_LoadCmdbuf(
    CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
    uint64_t cmdbuf_sz, uint64_t gaddr_base0,
    uint64_t gaddr_base1, bool enable_pmu,
    CVI_RT_MEM *cmdbuf_mem)
{
  return cvi_chip->LoadCmdbuf(rt_handle, cmdbuf, cmdbuf_sz, gaddr_base0,
                             gaddr_base1, enable_pmu, cmdbuf_mem);
}

CVI_RC CVI_RT_LoadDmabuf(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM dmabuf,
    uint64_t dmabuf_sz, uint64_t gaddr_base0, 
    uint64_t gaddr_base1, bool enable_pmu, CVI_RT_MEM *dmabuf_mem)
{
  return cvi_chip->LoadDmabuf(rt_handle, dmabuf, dmabuf_sz, gaddr_base0,
                             gaddr_base1, enable_pmu, dmabuf_mem);
}

CVI_RC CVI_RT_RunCmdbuf(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    uint64_t gaddr_base2, uint64_t gaddr_base3)
{
  return cvi_chip->RunCmdbuf(rt_handle, cmdbuf_mem, gaddr_base2, gaddr_base3);
}

CVI_RC CVI_RT_RunCmdbufEx(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    CVI_RT_ARRAYBASE *p_array_base)
{
  return cvi_chip->RunCmdbufEx(rt_handle, cmdbuf_mem, p_array_base);
}

CVI_RC CVI_RT_LoadCmdbufTee(
    CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
    size_t sz, uint64_t neuron_gaddr, uint64_t weight_gaddr, uint32_t weight_len, CVI_RT_MEM *cmdbuf_mem)
{
  return cvi_chip->LoadCmdbufTee(rt_handle, cmdbuf, sz, neuron_gaddr, weight_gaddr, weight_len, cmdbuf_mem);
}

CVI_RC CVI_RT_RunCmdbufTee(
    CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
    CVI_RT_ARRAYBASE *p_array_base)
{
  return cvi_chip->RunCmdbufTee(rt_handle, cmdbuf_mem, p_array_base);
}

CVI_RT_MEM CVI_RT_MemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size)
{
  return cvi_chip->MemAlloc(rt_handle, size);
}

CVI_RT_MEM CVI_RT_MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size)
{
  return cvi_chip->MemPreAlloc(mem, offset, size);
}

void CVI_RT_MemFree(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  return cvi_chip->MemFree(rt_handle, mem);
}

void CVI_RT_MemFreeEx(uint64_t p_addr)
{
  return cvi_chip->MemFreeEx(p_addr);
}

uint64_t CVI_RT_MemGetSize(CVI_RT_MEM mem)
{
  return cvi_chip->MemGetSize(mem);
}

uint64_t CVI_RT_MemGetPAddr(CVI_RT_MEM mem)
{
  return cvi_chip->MemGetPAddr(mem);
}

uint8_t* CVI_RT_MemGetVAddr(CVI_RT_MEM mem)
{
  return cvi_chip->MemGetVAddr(mem);
}

int32_t CVI_RT_MemIncRef(CVI_RT_MEM mem) {
  return cvi_chip->MemIncRef(mem);
}

int32_t CVI_RT_MemDecRef(CVI_RT_MEM mem) {
  return cvi_chip->MemDecRef(mem);
}

CVI_RC CVI_RT_MemFlush(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  return cvi_chip->MemFlush(rt_handle, mem);
}

CVI_RC CVI_RT_MemInvld(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)
{
  return cvi_chip->MemInvld(rt_handle, mem);
}

CVI_RC CVI_RT_MemFlushEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)
{
  return cvi_chip->MemFlushEx(rt_handle, mem, len);
}

CVI_RC CVI_RT_MemInvldEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)
{
  return cvi_chip->MemInvldEx(rt_handle, mem, len);
}

CVI_RC CVI_RT_MemCopyS2D(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst, uint8_t* src)
{
  return cvi_chip->MemCopyS2D(rt_handle, dst, src);
}

CVI_RC CVI_RT_MemCopyS2DEx(
    CVI_RT_HANDLE rt_handle,
    CVI_RT_MEM dst, uint64_t offset,
    uint64_t len, uint8_t* src)
{
  return cvi_chip->MemCopyS2DEx(rt_handle, dst, offset, len, src);
}

CVI_RC CVI_RT_MemCopyD2S(CVI_RT_HANDLE rt_handle, uint8_t* dst, CVI_RT_MEM src)
{
  return cvi_chip->MemCopyD2S(rt_handle, dst, src);
}

CVI_RC CVI_RT_ParsePmuBuf(CVI_RT_MEM cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len)
{
  return cvi_chip->ParsePmuBuf(cmdbuf_mem, buf_start, buf_len);
}

CVI_RC CVI_RT_SetBaseReg(CVI_RT_HANDLE rt_handle, uint32_t inx, uint64_t base_addr)
{
  return cvi_chip->SetBaseReg(rt_handle, inx, base_addr);
}
