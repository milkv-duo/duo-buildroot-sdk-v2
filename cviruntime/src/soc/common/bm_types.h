#pragma once
#include <pthread.h>
#include <bmruntime.h>
#include <cvikernel/cvikernel.h>
#include "cvitpu_debug.h"
#include <bmkernel/bm_regcpu.h>
#include <bmkernel/bm1880v2/bmkernel_1880v2.h>
#include <bmkernel/bm1822/bmkernel_1822.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define UNUSED(x) (void)(x)
#define MEMARRAY_MAX_CNT 1000

typedef struct __cvi_mem_pair {
  uint64_t p_addr;
  void *mem;
} cvi_mem_pair;

typedef struct bm_context {
  bmdev_t dev;
  u16 seq_no;
  union {
    bmk1880v2_context_t *ctx183x;
    bmk1822_context_t *ctx182x;
  } cvik_context;
  cvk_context_t *cvk_context;
  void *cvik_cmdbuf;

  uint64_t array_base0;
  uint64_t array_base1;
  cvi_mem_pair  root_mem_array[MEMARRAY_MAX_CNT];
} bm_context_t;

typedef struct bm_device {
  int index;
  int dev_fd;
  int ion_fd;
  u32 ion_heap_id;
  union {
    bmk1880v2_chip_info_t info183x;
    bmk1822_chip_info_t info182x;
  } info;
  unsigned long long gmem_size;
  pthread_mutex_t lock;
#define BMDEV_LOCK_INIT(dev)    pthread_mutex_init(&dev->lock, NULL)
#define BMDEV_LOCK_DEINIT(dev)  pthread_mutex_destroy(&dev->lock)
#define BMDEV_LOCK(dev)         pthread_mutex_lock(&dev->lock)
#define BMDEV_UNLOCK(dev)       pthread_mutex_unlock(&dev->lock)
} bm_device_t;

typedef enum {
  BMMEM_TYPE_DEVICE             = 0,
  BMMEM_TYPE_DEVICE_NEURON      = 1,  // obsolete
  BMMEM_TYPE_DEVICE_COEFF       = 2,  // obsolete
  BMMEM_TYPE_HOST               = 3,
  BMMEM_TYPE_SYSTEM             = 4,  // obsolete
  BMMEM_TYPE_INVALID            = 5
} bmmem_type_t;

typedef union {
  struct {
    bmmem_type_t        type : 3;
    int                 is_prealloc: 1;
    unsigned long long  reserved : 60;
  } u;
  unsigned long long    rawflags;
} bmmem_flags_t;

typedef struct bm_memory {
  uint8_t               *v_addr;  // for host, or mapped device in soc mode
  uint64_t              p_addr;
  size_t                size;
  bmmem_flags_t         flags;
  uint32_t              crc32;  // for data check if needed
  int                   dma_fd;
  int32_t               user_ref_cnt;
  uint64_t              offset;
} bm_memory_t;

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

#ifdef __cplusplus
}
#endif