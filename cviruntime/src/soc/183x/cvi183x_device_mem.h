#pragma once

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "cvi_device_mem.h"
#include "bmruntime_internal.h"

class Cvi183xDeviceMem : public CviDeviceMem {
public:
  Cvi183xDeviceMem();
  ~Cvi183xDeviceMem() override;
  virtual bmerr_t device_open(int index, bmdev_t *dev) override;
  virtual void device_close(bmdev_t dev) override;
  virtual int get_chip_ver(bmdev_t dev) override;
  virtual void mem_free_raw(bmctx_t ctx, bmmem_device_t mem);
  virtual bmerr_t load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem) override;
  virtual bmerr_t load_dmabuf(bmctx_t ctx, bmmem_device_t dmabuf,
                              size_t sz, uint64_t neuron_gaddr,
                              uint64_t weight_gaddr, bool enable_pmu,
                              bmmem_device_t *dmabuf_mem) override;
  virtual bmerr_t load_cmdbuf_tee(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                                          uint64_t neuron_gaddr, uint64_t weight_gaddr,
                                          uint32_t weight_len,
                                          bmmem_device_t *cmdbuf_mem);
  virtual bmerr_t unload_tee(bmctx_t ctx, uint64_t paddr, size_t size);
  virtual bmerr_t parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len);
  virtual void cvikernel_create(bmctx_t ctx, void **p_bk_ctx) override;
  virtual void cvikernel_submit(bmctx_t ctx) override;
  virtual void cvikernel_destroy(bmctx_t ctx) override;
};
