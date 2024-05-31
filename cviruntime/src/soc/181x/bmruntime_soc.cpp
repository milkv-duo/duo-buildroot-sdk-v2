#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cstdlib>
#include <runtime/debug.h>
#include <bmruntime.h>
#include <mmpool.h>
#include "bmruntime_internal.h"
#include "cvi181x_device_mem.h"

Cvi181xDeviceMem cvi_device;

bmmem_device_t bmmem_device_alloc_raw(bmctx_t ctx, size_t size) {
  return cvi_device.mem_alloc_raw(ctx, size);
}

bmmem_device_t bmmem_device_prealloc_raw(bmctx_t ctx, bmmem_device_t mem, uint64_t offset,
                                         size_t size) {
  return cvi_device.mem_prealloc_raw(ctx, mem, offset, size);
}

void bmmem_device_free(bmctx_t ctx, bmmem_device_t mem) {
  cvi_device.mem_free_raw(ctx, mem);
}

void bmmem_device_free_ex(uint64_t p_addr) {
  cvi_device.mem_free_ex(p_addr);
}

size_t bmmem_device_size(bmmem_device_t mem) {
  return cvi_device.mem_size(mem);
}

uint64_t bmmem_device_addr(bmmem_device_t mem) {
  return cvi_device.mem_p_addr(mem);
}

uint8_t *bmmem_device_v_addr(bmmem_device_t mem) {
  return cvi_device.mem_v_addr(mem);
}

int32_t bmmem_device_inc_ref(bmmem_device_t mem) {
  return cvi_device.mem_inc_ref(mem);
}

int32_t bmmem_device_dec_ref(bmmem_device_t mem) {
  return cvi_device.mem_dec_ref(mem);
}

bmerr_t bm_memcpy_s2d(bmctx_t ctx, bmmem_device_t dst, uint8_t *src) {
  return cvi_device.mem_memcpy_s2d(ctx, dst, src);
}

bmerr_t bm_memcpy_s2d_ex(bmctx_t ctx, bmmem_device_t dst, uint8_t *src, uint64_t offset,
                         size_t size) {
  return cvi_device.mem_memcpy_s2d_ex(ctx, dst, src, offset, size);
}

bmerr_t bm_memcpy_d2s(bmctx_t ctx, uint8_t *dst, bmmem_device_t src) {
  return cvi_device.mem_memcpy_d2s(ctx, dst, src);
}

bmerr_t bm_memcpy_d2s_ex(bmctx_t ctx, uint8_t *dst, bmmem_device_t src, uint64_t offset,
                         size_t size) {
  return cvi_device.mem_memcpy_d2s_ex(ctx, dst, src, offset, size);
}

bmerr_t bm_context_create(bmctx_t *ctx) {
  return cvi_device.context_create(ctx);
}

bmerr_t bm_bind_device(bmctx_t ctx, bmdev_t dev) {
  return cvi_device.bind_device(ctx, dev);
}

void bm_unbind_device(bmctx_t ctx) {
  return cvi_device.unbind_device(ctx);
}

bmdev_t bm_get_device(bmctx_t ctx) {
  return cvi_device.get_device(ctx);
}

bmerr_t bm_init(int index, bmctx_t *ctx) {
  return cvi_device.device_init(index, ctx);
}

void bm_exit(bmctx_t ctx) {
  cvi_device.device_exit(ctx);
}

bmerr_t bm_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem) {
  return cvi_device.load_cmdbuf(ctx, cmdbuf, sz, neuron_gaddr,
                                 weight_gaddr, enable_pmu, cmdbuf_mem);
}

bmerr_t cvi_load_cmdbuf_tee(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr, uint32_t weight_len, bmmem_device_t *cmdbuf_mem)
{
  return cvi_device.load_cmdbuf_tee(ctx, cmdbuf, sz, neuron_gaddr,
                                     weight_gaddr, weight_len, cmdbuf_mem);
}

bmerr_t cvi_run_cmdbuf_tee(bmctx_t ctx, uint16_t *seq_no, uint64_t dmabuf_addr, cvi_array_base *array_base)
{
  return cvi_device.run_cmdbuf_tee(ctx, seq_no, dmabuf_addr, array_base);
}

bmerr_t bm_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no) {
  return cvi_device.run_cmdbuf(ctx, cmdbuf_mem, seq_no);
}

bmerr_t bm_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       uint64_t input_base_addr, uint64_t output_base_addr) {
  return cvi_device.run_cmdbuf_ex(ctx, cmdbuf_mem, seq_no, input_base_addr, output_base_addr);
}

bmerr_t bm_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem, uint16_t *seq_no,
                       cvi_array_base *p_array_base) {
  return cvi_device.run_cmdbuf_ex2(ctx, cmdbuf_mem, seq_no, p_array_base);
}

bmerr_t cvi_run_async(bmctx_t ctx, bmmem_device_t cmdbuf_mem)
{
  return cvi_device.run_async(ctx, cmdbuf_mem);
}

bmerr_t bm_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz, uint16_t *seq_no) {
  return cvi_device.send_cmdbuf(ctx, cmdbuf, sz, seq_no);
}

bmerr_t bm_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no) {
  return cvi_device.wait_cmdbuf_done(ctx, seq_no);
}

bmerr_t cvi_wait_cmdbuf_all(bmctx_t ctx) {
  return cvi_device.wait_cmdbuf_all(ctx);
}

bmerr_t bm_run_cmdbuf_pio(bmctx_t ctx, uint8_t *cmdbuf, size_t sz) {
  return cvi_device.run_cmdbuf_pio(ctx, cmdbuf, sz);
}

void bm_device_set_base_reg(bmctx_t ctx, uint32_t inx, uint64_t addr) {
  cvi_device.set_base_reg(ctx, inx, addr);
}

uint64_t bm_device_read_base_reg(bmctx_t ctx, u32 inx) {
  return cvi_device.read_base_reg(ctx, inx);
}

int bm_device_get_chip_ver(bmdev_t dev) {
  return cvi_device.get_chip_ver(dev);
}

bmerr_t bm_parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len) {
  return cvi_device.parse_pmubuf(cmdbuf_mem, buf_start, buf_len);
}

void cviruntime_cvikernel_create(bmctx_t ctx, void **p_bk_ctx) {
  cvi_device.cvikernel_create(ctx, p_bk_ctx);
}

void cviruntime_cvikernel_submit(bmctx_t ctx) {
  cvi_device.cvikernel_submit(ctx);
}

void cviruntime_cvikernel_destroy(bmctx_t ctx) {
  cvi_device.cvikernel_destroy(ctx);
}
