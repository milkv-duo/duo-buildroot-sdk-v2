#pragma once

#include "string.h"
#include <inttypes.h>
#include <bmruntime.h>
#include <bmkernel/reg_tiu.h>

#include <bmkernel/bm_kernel.h>
#include <bmkernel/bm_regcpu.h>
#include <bmkernel/reg_bdcast.h>
#include <bmkernel/reg_tdma.h>
#include "runtime_cmodel_internal.h"

#define GDMA_DESC_ALIGN_SIZE (1 << TDMA_DESCRIPTOR_ALIGNED_BIT)

typedef struct {
  unsigned long long neuron_gaddr;
  unsigned long long weight_gaddr;
  uint32_t tiu_cmdbuf_sz;
  uint32_t tdma_cmdbuf_sz;
} CMDBUF_HEADER_T;

typedef struct __dma_hdr_t {
  uint16_t dmabuf_magic_m;
  uint16_t dmabuf_magic_s;
  uint32_t dmabuf_size;
  uint32_t cpu_desc_count;
  uint32_t bd_desc_count; //16bytes
  uint32_t tdma_desc_count;
  uint32_t tpu_clk_rate;
  uint32_t pmubuf_size;
  uint32_t pmubuf_offset; //32bytes
  uint32_t arraybase_0_L;
  uint32_t arraybase_0_H;
  uint32_t arraybase_1_L;
  uint32_t arraybase_1_H; //48bytes
  uint32_t arraybase_2_L;
  uint32_t arraybase_2_H;
  uint32_t arraybase_3_L;
  uint32_t arraybase_3_H; //64bytes

  uint32_t arraybase_4_L;
  uint32_t arraybase_4_H;
  uint32_t arraybase_5_L;
  uint32_t arraybase_5_H;
  uint32_t arraybase_6_L;
  uint32_t arraybase_6_H;
  uint32_t arraybase_7_L;
  uint32_t arraybase_7_H;
  uint32_t reserve[8];   //128bytes, 128bytes align
} dma_hdr_t;

// CPU_OP_SYNC structure
typedef struct {
  uint32_t op_type;
  uint32_t num_tiu;
  uint32_t num_tdma;
  uint32_t offset_tiu;
  uint32_t offset_tdma;
  uint32_t offset_tiu_ori_bk;
	uint32_t offset_tdma_ori_bk;
  char str[CPU_ENGINE_STR_LIMIT_BYTE];
} __attribute__((packed)) cvi_cpu_desc_t;

class CModelCmdbuf {
public:
  virtual ~CModelCmdbuf() = 0;
  virtual bmerr_t rt_device_open(int index, bmdev_t *dev) = 0;
  virtual bmerr_t rt_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                              uint16_t *seq_no);
  virtual bmerr_t rt_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
  virtual bmerr_t rt_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);
  virtual bmerr_t rt_load_dmabuf(bmctx_t ctx, bmmem_device_t dmabuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *dmabuf_mem);
  virtual bmerr_t rt_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                                     uint16_t *seq_no);
  virtual bmerr_t rt_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                                        uint16_t *seq_no,
                                        uint64_t input_base_addr,
                                        uint64_t output_base_addr);
  virtual bmerr_t rt_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                                         uint16_t *seq_no,
                                         cvi_array_base *p_array_base);
  virtual void enable_interrupt(uint8_t *cmdbuf, uint64_t sz) = 0;
  virtual void set_eod(uint8_t *cmdbuf, uint64_t sz) = 0;
  virtual bmmem_device_t rt_device_alloc_raw(bmctx_t ctx, size_t size);
  virtual void rt_device_free(bmctx_t ctx, bmmem_device_t mem);

protected:
  virtual void reorder_tiu_cmdbuf_reg(uint8_t *cmdbuf);
  virtual void reorder_tiu_cmdbuf(uint8_t *cmdbuf, size_t sz);
  virtual void enable_tdma_cmdbuf_barrier(uint8_t *cmdbuf, size_t sz);
  virtual void adjust_cmdbuf(uint8_t *cmdbuf, size_t sz);
  virtual int extract_cmdbuf(int engine_id, uint8_t *cmdbuf,
                             uint8_t *found_cmdbuf, size_t sz);
  virtual int extract_dmabuf(int engine_id, uint8_t *dmabuf,
                             uint8_t *found_dmabuf, size_t sz);

public:
  u64 g_tiu_cmdbuf_gaddr;
  u64 g_tdma_cmdbuf_gaddr;
  u64 g_tiu_cmdbuf_reserved_size;
  u64 g_tdma_cmdbuf_reserved_size;
  u64 g_gmem_reserved_size;
  u64 g_gmem_size;
  uint32_t cmdbuf_hdr_magic;
  uint32_t dmabuf_hdr_magic;
};