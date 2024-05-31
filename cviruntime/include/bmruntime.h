/*
* Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
*
* File Name: bmruntime.h
* Description:
*/

#ifndef _BM_RUNTIME_H_
#define _BM_RUNTIME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bm_context;
typedef struct bm_context *bmctx_t;
struct bm_device;
typedef struct bm_device *bmdev_t;
typedef int32_t bmerr_t;

struct bm_memory;
typedef struct bm_memory *bmmem_t;
typedef bmmem_t bmmem_device_t;

typedef enum bmfmt_e {
  BM_FMT_FP32   = 0,
  BM_FMT_FP16   = 1,
  BM_FMT_INT16  = 2,
  BM_FMT_INT8   = 3,
  BM_FMT_BF16   = 4,
  BM_FMT_MAX    = 5
} bmfmt_t;

#define BM_SHAPE_MAX_DIM       (4)
typedef struct bmshape_s {
  bmfmt_t fmt;
  int dim_size;
  int dim[BM_SHAPE_MAX_DIM];
} bmshape_t;

typedef struct _cvi_array_base {
  uint64_t gaddr_base0;
  uint64_t gaddr_base1;
  uint64_t gaddr_base2;
  uint64_t gaddr_base3;
  uint64_t gaddr_base4;
  uint64_t gaddr_base5;
  uint64_t gaddr_base6;
  uint64_t gaddr_base7;
} cvi_array_base;

bmerr_t bm_init(int index, bmctx_t *ctx);
void bm_exit(bmctx_t ctx);

bmmem_device_t bmmem_device_alloc_raw(bmctx_t ctx, size_t size);
bmmem_device_t bmmem_device_prealloc_raw(bmctx_t ctx, bmmem_device_t mem, uint64_t offset, size_t size);
void bmmem_device_free(bmctx_t ctx, bmmem_device_t mem);
void bmmem_device_free_ex(uint64_t p_addr);

size_t bmmem_device_size(bmmem_device_t mem);
uint64_t bmmem_device_addr(bmmem_device_t mem);
int32_t bmmem_device_inc_ref(bmmem_device_t mem);
int32_t bmmem_device_dec_ref(bmmem_device_t mem);
uint8_t* bmmem_device_v_addr(bmmem_device_t mem);

bmerr_t bm_memcpy_s2d(bmctx_t ctx, bmmem_device_t dst, uint8_t* src);
bmerr_t bm_memcpy_d2s(bmctx_t ctx, uint8_t* dst, bmmem_device_t src);
bmerr_t bm_memcpy_s2d_ex(bmctx_t ctx, bmmem_device_t dst, uint8_t* src, uint64_t offset, size_t size);
bmerr_t bm_memcpy_d2s_ex(bmctx_t ctx, uint8_t* dst, bmmem_device_t src, uint64_t offset, size_t size);

bmerr_t bm_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem);

bmerr_t bm_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       uint64_t input_base_addr, uint64_t output_base_addr);
bmerr_t bm_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       cvi_array_base *array_base);
bmerr_t cvi_run_async(bmctx_t ctx, bmmem_device_t cmdbuf_mem);
bmerr_t cvi_wait_cmdbuf_all(bmctx_t ctx);

bmerr_t bm_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no);
bmerr_t bm_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint16_t *seq_no);
bmerr_t bm_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no);
bmerr_t bm_parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len);
bmerr_t bm_run_cmdbuf_pio(bmctx_t ctx, uint8_t *cmdbuf, size_t sz);

bmerr_t cvi_load_cmdbuf_tee(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr, uint32_t weight_len, bmmem_device_t *cmdbuf_mem);
bmerr_t cvi_run_cmdbuf_tee(bmctx_t ctx, uint16_t *seq_no, uint64_t dmabuf_addr, cvi_array_base *array_base);

void bm_device_set_base_reg(bmctx_t ctx, uint32_t inx, uint64_t addr);
uint64_t bm_device_read_base_reg(bmctx_t ctx, unsigned int inx);

void cviruntime_cvikernel_create(bmctx_t ctx, void **p_bk_ctx);
void cviruntime_cvikernel_submit(bmctx_t ctx);
void cviruntime_cvikernel_destroy(bmctx_t ctx);

#ifdef __cplusplus
}
#endif

#endif /* _BM_RUNTIME_H_ */
