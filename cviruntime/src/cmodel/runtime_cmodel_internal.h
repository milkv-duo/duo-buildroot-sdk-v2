#ifndef _RUNTIME_CMODEL_INTERNAL_H_
#define _RUNTIME_CMODEL_INTERNAL_H_

#include <stdint.h>
#include <cmodel/bm_cmodel.h>
#include <pthread.h>
#include <runtime/debug.h>
#include "mmpool.h"
#include <cvikernel/cvikernel.h>
#include <bmkernel/bm1822/bmkernel_1822.h>
#include <bmkernel/bm1880v2/bmkernel_1880v2.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)

typedef struct bm_context {
  bmdev_t dev;
  uint16_t seq_no;
  void *bk_ctx;
  u8 *cmdbuf;

  struct bmk_context *cvik_context;
  cvk_context_t *cvk_context;

  void *cvik_cmdbuf;

  unsigned long long neuron_paddr_nor;
  unsigned long long weight_paddr_nor;
  u32 weight_size_nor;

  unsigned long long dmabuf_addr_sec;
  unsigned long long dmabuf_len_sec;
} bm_context_t;

#define BMDEV_LOCK_INIT(dev) pthread_mutex_init(&dev->lock, NULL)
#define BMDEV_LOCK_DEINIT(dev) pthread_mutex_destroy(&dev->lock)
#define BMDEV_LOCK(dev) pthread_mutex_lock(&dev->lock)
#define BMDEV_UNLOCK(dev) pthread_mutex_unlock(&dev->lock)

typedef struct bm_device {
  int index;
  bmmod_t model;
  struct mem_pool *device_mem_pool;
  cvk_chip_info_t cvk_chip_info;
  unsigned long long gmem_size;

  pthread_mutex_t lock;
} bm_device_t;

typedef enum {
  BMMEM_TYPE_DEVICE = 0,
  BMMEM_TYPE_DEVICE_NEURON = 1, // obsolete
  BMMEM_TYPE_DEVICE_COEFF = 2,  // obsolete
  BMMEM_TYPE_HOST = 3,
  BMMEM_TYPE_SYSTEM = 4, // obsolete
  BMMEM_TYPE_INVALID = 5
} bmmem_type_t;

typedef union {
  struct {
    bmmem_type_t type : 3;
    int is_prealloc : 1;
    unsigned long long reserved : 60;
  } u;
  unsigned long long rawflags;
} bmmem_flags_t;

typedef struct bm_memory {
  uint8_t       *v_addr; // for host, or mapped device in soc mode
  unsigned long long p_addr;
  size_t        size;
  int32_t       user_ref_cnt;
  bmmem_flags_t flags;
} bm_memory_t;

void bm1880v2_enable_interrupt(uint8_t *cmdbuf, uint64_t sz);
void bm1880v2_set_eod(uint8_t *cmdbuf, uint64_t sz);
void bm1822_enable_interrupt(uint8_t *cmdbuf, uint64_t sz);
void bm1822_set_eod(uint8_t *cmdbuf, uint64_t sz);
void cv181x_enable_interrupt(uint8_t *cmdbuf, uint64_t sz);
void cv181x_set_eod(uint8_t *cmdbuf, uint64_t sz);
void cv180x_enable_interrupt(uint8_t *cmdbuf, uint64_t sz);
void cv180x_set_eod(uint8_t *cmdbuf, uint64_t sz);

bmerr_t rt_1880v2_device_open(int index, bmdev_t *dev);
bmerr_t rt_1880v2_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no);
bmerr_t rt_1880v2_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
bmerr_t rt_1880v2_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);
bmerr_t rt_1880v2_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no);
bmerr_t rt_1880v2_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, uint64_t input_base_addr, uint64_t output_base_addr);
bmerr_t rt_1880v2_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, cvi_array_base *p_array_base);

void rt_1880v2_device_free(bmctx_t ctx, bmmem_device_t mem);
bmmem_device_t rt_1880v2_device_alloc_raw(bmctx_t ctx, size_t size);


bmerr_t rt_1822_device_open(int index, bmdev_t *dev);
bmerr_t rt_1822_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no);
bmerr_t rt_1822_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
bmerr_t rt_1822_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);
bmerr_t rt_1822_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no);
bmerr_t rt_1822_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, uint64_t input_base_addr, uint64_t output_base_addr);
bmerr_t rt_1822_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, cvi_array_base *p_array_base);

void rt_1822_device_free(bmctx_t ctx, bmmem_device_t mem);
bmmem_device_t rt_1822_device_alloc_raw(bmctx_t ctx, size_t size);

bmerr_t rt_cv181x_device_open(int index, bmdev_t *dev);
bmerr_t rt_cv181x_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no);
bmerr_t rt_cv181x_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
bmerr_t rt_cv181x_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);
bmerr_t rt_cv181x_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no);
bmerr_t rt_cv181x_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, uint64_t input_base_addr, uint64_t output_base_addr);
bmerr_t rt_cv181x_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, cvi_array_base *p_array_base);

void rt_cv181x_device_free(bmctx_t ctx, bmmem_device_t mem);
bmmem_device_t rt_cv181x_device_alloc_raw(bmctx_t ctx, size_t size);

bmerr_t rt_cv180x_device_open(int index, bmdev_t *dev);
bmerr_t rt_cv180x_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no);
bmerr_t rt_cv180x_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
bmerr_t rt_cv180x_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);
bmerr_t rt_cv180x_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no);
bmerr_t rt_cv180x_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, uint64_t input_base_addr, uint64_t output_base_addr);
bmerr_t rt_cv180x_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, cvi_array_base *p_array_base);

void rt_cv180x_device_free(bmctx_t ctx, bmmem_device_t mem);
bmmem_device_t rt_cv180x_device_alloc_raw(bmctx_t ctx, size_t size);

#ifdef __cplusplus
}
#endif

#endif

