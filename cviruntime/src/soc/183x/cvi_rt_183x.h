#pragma once
#include "cvi_rt_base.h"
#include "cvi183x_device_mem.h"

class CviRT183x : public CviRTSoc {
public:
  CviRT183x();
  virtual ~CviRT183x() override;

  virtual CVI_RT_KHANDLE GetKHandleBK(CVI_RT_HANDLE rt_handle) override;
  virtual CVI_RC DeInitBK(CVI_RT_HANDLE rt_handle) override;
  virtual CVI_RC InitWithKernelBK(CVI_RT_HANDLE *rt_handle, uint32_t cmdbuf_size) override;
  virtual CVI_RC LoadCmdbufTee(CVI_RT_HANDLE rt_handle, uint8_t *cmdbuf,
                       size_t sz, uint64_t neuron_gaddr,
                       uint64_t weight_gaddr, uint32_t weight_len,
                       CVI_RT_MEM *cmdbuf_mem) override;
  virtual CVI_RC RunCmdbufTee(CVI_RT_HANDLE rt_handle, CVI_RT_MEM cmdbuf_mem,
                              CVI_RT_ARRAYBASE *p_array_base);
};
