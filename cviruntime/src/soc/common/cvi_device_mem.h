#pragma once
#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include "bmruntime.h"
#include "linux/ion.h"
#include "linux/bm_npu_ioctl.h"
#include "bm_types.h"

#define TPU_DEV_NAME "/dev/cvi-tpu0"
#define ION_DEV_NAME "/dev/ion"
#define MAX_HEAP_COUNT ION_HEAP_TYPE_CUSTOM

typedef struct __tee_firewall_info {
  uint64_t address;
} tee_firewall_info;

#define CTX_MAX_CNT 100
#define SUBMIT_QUEUE_MAX 100  //kernel driver queue is 100 as well
#define TEE_FIREWALL_MAX 6

#define ROOTDAEMON_LOCK() pthread_mutex_lock(&root_daemon_lock)
#define ROOTDAEMON_UNLOCK() pthread_mutex_unlock(&root_daemon_lock)

#define MEMARRAY_DUMP 0

#if MEMARRAY_DUMP
  #define BMEMEM_DUMP() bmmem_dump_mem_array()
#else
  #define BMEMEM_DUMP()
#endif

class CviDeviceMem {
public:
  CviDeviceMem();
  virtual ~CviDeviceMem();
  // bmruntime_soc.cpp
  virtual bmmem_device_t mem_alloc_raw(bmctx_t ctx, size_t size);
  virtual bmmem_device_t mem_alloc_pagesize(bmctx_t ctx, size_t size);
  virtual bmmem_device_t mem_prealloc_raw(bmctx_t ctx, bmmem_device_t mem, uint64_t offset,
                                         size_t size); 
  virtual void mem_free_raw(bmctx_t ctx, bmmem_device_t mem) = 0;
  virtual void mem_free_ex(uint64_t p_addr);
  virtual size_t mem_size(bmmem_device_t mem);
  virtual uint64_t mem_p_addr(bmmem_device_t mem);
  virtual uint8_t *mem_v_addr(bmmem_device_t mem);
  virtual int32_t mem_inc_ref(bmmem_device_t mem);
  virtual int32_t mem_dec_ref(bmmem_device_t mem);
  virtual bmerr_t mem_memcpy_s2d(bmctx_t ctx, bmmem_device_t dst, uint8_t *src);
  virtual bmerr_t mem_memcpy_s2d_ex(bmctx_t ctx, bmmem_device_t dst, uint8_t * src,
                                    uint64_t offset, size_t size);
  virtual bmerr_t mem_memcpy_d2s(bmctx_t ctx, uint8_t *dst, bmmem_device_t src);
  virtual bmerr_t mem_memcpy_d2s_ex(bmctx_t ctx, uint8_t * dst, bmmem_device_t src,
                                    uint64_t offset, size_t size);
  virtual bmerr_t mem_device_flush(bmctx_t ctx, bmmem_device_t mem); 
  virtual bmerr_t mem_device_flush_len(bmctx_t ctx, bmmem_device_t mem, size_t len); 
  virtual bmerr_t mem_device_invld(bmctx_t ctx, bmmem_device_t mem); 
  virtual bmerr_t mem_device_invld_len(bmctx_t ctx, bmmem_device_t mem, size_t len); 

  virtual bmerr_t context_create(bmctx_t *ctx);
  virtual void context_destroy(bmctx_t ctx);
  virtual bmerr_t bind_device(bmctx_t ctx, bmdev_t dev);
  virtual void unbind_device(bmctx_t ctx);
  virtual bmdev_t get_device(bmctx_t ctx);
  virtual bmerr_t device_init(int index, bmctx_t *ctx);
  virtual void device_exit(bmctx_t ctx);
  virtual bmerr_t load_cmdbuf(bmctx_t ctx, uint8_t * cmdbuf, size_t sz,
                              uint64_t neuron_gaddr, uint64_t weight_gaddr,
                              bool enable_pmu, bmmem_device_t *cmdbuf_mem) = 0;
  virtual bmerr_t load_cmdbuf_tee(bmctx_t ctx, uint8_t * cmdbuf, size_t sz,
                                  uint64_t neuron_gaddr, uint64_t weight_gaddr,
                                  uint32_t weight_len, bmmem_device_t * cmdbuf_mem) = 0;
  virtual bmerr_t load_dmabuf(bmctx_t ctx, bmmem_device_t in_mem,
                              size_t sz, uint64_t neuron_gaddr, uint64_t weight_gaddr,
                              bool enable_pmu, bmmem_device_t *dmabuf_mem)          = 0;
  virtual bmerr_t run_cmdbuf_tee(bmctx_t ctx, uint16_t * seq_no, uint64_t dmabuf_addr,
                                 cvi_array_base * array_base);
  virtual bmerr_t run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no);
  virtual bmerr_t run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       uint64_t input_base_addr, uint64_t output_base_addr);
  virtual bmerr_t run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       cvi_array_base *p_array_base);
  virtual bmerr_t run_async(bmctx_t ctx, bmmem_device_t cmdbuf_mem);
  virtual bmerr_t send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no);
  virtual bmerr_t wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
  virtual bmerr_t wait_cmdbuf_all(bmctx_t ctx);
  virtual bmerr_t run_cmdbuf_pio(bmctx_t ctx, uint8_t *cmdbuf, size_t sz);
  virtual void set_base_reg(bmctx_t ctx, uint32_t inx, uint64_t addr);
  virtual uint64_t read_base_reg(bmctx_t ctx, u32 inx);
  virtual int get_chip_ver(bmdev_t dev) = 0;
  virtual bmerr_t parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len) = 0;
  virtual void cvikernel_create(bmctx_t ctx, void **p_bk_ctx) = 0;
  virtual void cvikernel_submit(bmctx_t ctx) = 0;
  virtual void cvikernel_destroy(bmctx_t ctx) = 0;

public:
  virtual void bmmem_dump_mem_array(void);
  bmerr_t ion_query_heap(bmdev_t dev);
#ifdef ION_CACHE_OPEN
  virtual bmerr_t mem_flush_fd(bmdev_t dev, int dma_fd);
  virtual bmerr_t mem_invld_fd(bmdev_t dev, int dma_fd);
#endif
  virtual bmerr_t mem_flush_ext(bmdev_t dev, int dma_fd, uint64_t paddr, size_t size);
  virtual bmerr_t mem_invld_ext(bmdev_t dev, int dma_fd, uint64_t paddr, size_t size);
  virtual bmerr_t submit_dmabuf(bmdev_t dev, int dma_fd, uint32_t seq_no);
  virtual bmerr_t wait_dmabuf(bmdev_t dev, uint32_t seq_no);
  virtual bmerr_t mem_alloc(bmdev_t dev, size_t size, uint64_t *paddr, uint8_t **vaddr, int *dma_fd);
  virtual bmerr_t mem_free(uint8_t *vaddr, size_t size, int dma_fd);
  virtual bmerr_t device_open(int index, bmdev_t *dev) = 0;
  virtual void device_close(bmdev_t dev) = 0;
  virtual bmerr_t load_tee(bmctx_t ctx, uint64_t cmdbuf_addr_ree, uint32_t cmdbuf_len_ree,
                           uint64_t weight_addr_ree, uint32_t weight_len_ree, uint64_t neuron_addr_ree);
  virtual bmerr_t unload_tee(bmctx_t ctx, uint64_t paddr, size_t size) = 0;
  virtual bmerr_t run_tee(bmctx_t ctx, uint32_t seq_no, uint64_t dmabuf_addr,
                  uint64_t array_base2, uint64_t array_base3,
                  uint64_t array_base4, uint64_t array_base5,
                  uint64_t array_base6, uint64_t array_base7);
  virtual bmerr_t reopen_dev(bmdev_t dev, int flag);
  bmerr_t ion_ioctl(int fd, unsigned int heap_id_mask, size_t* size, uint64_t *paddr, int *dma_fd);

 protected:
  uint64_t GLOBAL_MEM_START_ADDR;
  uint64_t g_gmem_size;
  uint16_t tpu_dmabuf_header_m;
  bool ion_legacy = false;
  bool protect = false; //if cmdbuf_mem protect
public:
  static bmctx_t root_ctx_array[CTX_MAX_CNT];
  static uint16_t root_submit_array[SUBMIT_QUEUE_MAX];
  static pthread_mutex_t root_daemon_lock;
  static tee_firewall_info root_tee_firewall_info[TEE_FIREWALL_MAX];
};
