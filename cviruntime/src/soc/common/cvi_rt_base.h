#pragma once
#include <memory>
#include "bmruntime.h"
#include "cvi_device_mem.h"
#include "cviruntime_context.h"
#include "bm_types.h"
#include <string>

#define DEVICE_INDEX_NUM 0
typedef struct _cvi_rt_submit {
  cvk_context_t  cvk_ctx;
  bmctx_t        rt_ctx;
  uint8_t        *cmdbuf;
  bmmem_device_t dmabuf;
  uint32_t       magic;
} cvi_rt_submit;

class CviRTBase {
public:
  CviRTBase();
  CviRTBase(const CviRTBase&) = delete;
  CviRTBase(CviRTBase&&)  = delete;
  CviRTBase& operator=(const CviRTBase&) = delete;
  CviRTBase& operator=(CviRTBase&&) = delete;
  virtual ~CviRTBase()                                                                 = 0;
  virtual CVI_RC DeInitBK(CVI_RT_HANDLE rt_handle)                                     = 0;
  virtual CVI_RC InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size)      = 0;
  virtual CVI_RC SubmitBK(CVI_RT_HANDLE rt_handle)                                     = 0;
  virtual CVI_RT_KHANDLE GetKHandleBK(CVI_RT_HANDLE rt_handle)                         = 0;
  virtual CVI_RC SubmitPio(CVI_RT_HANDLE rt_handle)                                    = 0;
  virtual CVI_RC Init(CVI_RT_HANDLE *rt_handle)                                        = 0;
  virtual CVI_RC DeInit(CVI_RT_HANDLE rt_handle)                                       = 0;
  virtual CVI_RT_KHANDLE RegisterKernel(CVI_RT_HANDLE rt_handle, uint32_t cmdbuf_size) = 0;
  virtual CVI_RC UnRegisterKernel(CVI_RT_KHANDLE rt_khandle)                           = 0;
  virtual CVI_RC SubmitAsync(CVI_RT_KHANDLE rt_khandle, uint8_t submit_previous)       = 0;
  virtual CVI_RC WaitForAsync(CVI_RT_KHANDLE rt_khandle)                               = 0;
  virtual CVI_RC Submit(CVI_RT_KHANDLE rt_khandle)                                     = 0;
  virtual CVI_RC LoadCmdbuf(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                            uint64_t cmdbuf_sz, uint64_t gaddr_base0,
                            uint64_t gaddr_base1, bool enable_pmu,
                            CVI_RT_MEM *cmdbuf_mem)                                    = 0;
  virtual CVI_RC LoadDmabuf(
      CVI_RT_HANDLE rt_handle, CVI_RT_MEM dmabuf,
      uint64_t dmabuf_sz, uint64_t gaddr_base0,
      uint64_t gaddr_base1, bool enable_pmu, CVI_RT_MEM *dmabuf_mem)                   = 0;
  virtual CVI_RC RunCmdbuf(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                           uint64_t gaddr_base2, uint64_t gaddr_base3)                 = 0;
  virtual CVI_RC RunCmdbufEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                             CVI_RT_ARRAYBASE *p_array_base)                           = 0;
  virtual CVI_RC LoadCmdbufTee(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                               size_t sz, uint64_t neuron_gaddr,
                               uint64_t weight_gaddr, uint32_t weight_len,
                               CVI_RT_MEM *cmdbuf_mem)                                 = 0;
  virtual CVI_RC RunCmdbufTee(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                              CVI_RT_ARRAYBASE *p_array_base)                          = 0;
  virtual CVI_RT_MEM MemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size)                  = 0;
  virtual CVI_RT_MEM MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size)       = 0;
  virtual void MemFree(CVI_RT_HANDLE rt_hanlde, CVI_RT_MEM mem)                        = 0;
  virtual void MemFreeEx(uint64_t p_addr)                                              = 0;
  virtual uint64_t MemGetSize(CVI_RT_MEM mem)                                          = 0;
  virtual uint64_t MemGetPAddr(CVI_RT_MEM mem)                                         = 0;
  virtual uint8_t *MemGetVAddr(CVI_RT_MEM mem)                                         = 0;
  virtual int32_t MemIncRef(CVI_RT_MEM mem)                                            = 0;
  virtual int32_t MemDecRef(CVI_RT_MEM mem)                                            = 0;
  virtual CVI_RC MemFlush(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)                     = 0;
  virtual CVI_RC MemInvld(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem)                     = 0;
  virtual CVI_RC MemFlushEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)     = 0;
  virtual CVI_RC MemInvldEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len)     = 0;
  virtual CVI_RC MemCopyS2D(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst, uint8_t *src)     = 0;
  virtual CVI_RC MemCopyS2DEx(CVI_RT_HANDLE rt_handle,
                              CVI_RT_MEM dst, uint64_t offset,
                              uint64_t len, uint8_t *src)                              = 0;
  virtual CVI_RC MemCopyD2S(CVI_RT_HANDLE rt_handle, uint8_t *dst, CVI_RT_MEM src)     = 0;
  virtual CVI_RC ParsePmuBuf(CVI_RT_MEM cmdbuf_mem, uint8_t **buf_start,
                             uint32_t *buf_len)                                        = 0;
  virtual CVI_RC SetBaseReg(CVI_RT_HANDLE rt_handle, uint32_t inx,
                            uint64_t base_addr)                                        = 0;

 public:
  std::string chip_name_;
  std::unique_ptr<CviDeviceMem> cvi_device;
};


class CviRTSoc : public CviRTBase {
public:
  ~CviRTSoc() override;
  virtual CVI_RC SubmitBK(CVI_RT_HANDLE rt_handle) override;
  virtual CVI_RC SubmitPio(CVI_RT_HANDLE rt_handle) override;
  virtual CVI_RC Init(CVI_RT_HANDLE *rt_handle) override;
  virtual CVI_RC DeInit(CVI_RT_HANDLE rt_handle) override;
  virtual CVI_RT_KHANDLE RegisterKernel(CVI_RT_HANDLE rt_handle, uint32_t cmdbuf_size) override;
  virtual CVI_RC UnRegisterKernel(CVI_RT_KHANDLE rt_khandle) override;
  virtual CVI_RC SubmitAsync(CVI_RT_KHANDLE rt_khandle, uint8_t submit_previous) override;
  virtual CVI_RC WaitForAsync(CVI_RT_KHANDLE rt_khandle) override;
  virtual CVI_RC Submit(CVI_RT_KHANDLE rt_khandle) override;
  virtual CVI_RC LoadCmdbuf(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                    uint64_t cmdbuf_sz, uint64_t gaddr_base0,
                    uint64_t gaddr_base1, bool enable_pmu,
                    CVI_RT_MEM *cmdbuf_mem) override;
  virtual CVI_RC LoadDmabuf(
      CVI_RT_HANDLE rt_handle, CVI_RT_MEM dmabuf,
      uint64_t dmabuf_sz, uint64_t gaddr_base0,
      uint64_t gaddr_base1, bool enable_pmu, CVI_RT_MEM *dmabuf_mem) override;
  virtual CVI_RC RunCmdbuf(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                   uint64_t gaddr_base2, uint64_t gaddr_base3) override;
  virtual CVI_RC RunCmdbufEx(
      CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
      CVI_RT_ARRAYBASE *p_array_base);
  virtual CVI_RT_MEM MemAlloc(CVI_RT_HANDLE rt_handle, uint64_t size) override;
  virtual CVI_RT_MEM MemPreAlloc(CVI_RT_MEM mem, uint64_t offset, uint64_t size) override;
  virtual void MemFree(CVI_RT_HANDLE rt_hanlde, CVI_RT_MEM mem) override;
  virtual void MemFreeEx(uint64_t p_addr) override;
  virtual uint64_t MemGetSize(CVI_RT_MEM mem) override;
  virtual uint64_t MemGetPAddr(CVI_RT_MEM mem) override;
  virtual uint8_t* MemGetVAddr(CVI_RT_MEM mem) override;
  virtual int32_t MemIncRef(CVI_RT_MEM mem) override;
  virtual int32_t MemDecRef(CVI_RT_MEM mem) override;
  virtual CVI_RC MemFlush(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) override;
  virtual CVI_RC MemInvld(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem) override;
  virtual CVI_RC MemFlushEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len) override;
  virtual CVI_RC MemInvldEx(CVI_RT_HANDLE rt_handle, CVI_RT_MEM mem, uint64_t len) override;
  virtual CVI_RC MemCopyS2D(CVI_RT_HANDLE rt_handle, CVI_RT_MEM dst, uint8_t *src) override;
  virtual CVI_RC MemCopyS2DEx(CVI_RT_HANDLE rt_handle,
                      CVI_RT_MEM dst, uint64_t offset,
                      uint64_t len, uint8_t *src) override;
  virtual CVI_RC MemCopyD2S(CVI_RT_HANDLE rt_handle, uint8_t* dst, CVI_RT_MEM src) override;
  virtual CVI_RC ParsePmuBuf(CVI_RT_MEM cmdbuf_mem, uint8_t **buf_start,
                     uint32_t *buf_len) override;
  virtual CVI_RC SetBaseReg(CVI_RT_HANDLE rt_handle, uint32_t inx,
                           uint64_t base_addr) override;
public:
  uint32_t submit_magic_;
};
