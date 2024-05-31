#include "cmodel_cmdbuf.h"
#include <bmkernel/reg_tiu.h>

CModelCmdbuf::~CModelCmdbuf() {}

void CModelCmdbuf::reorder_tiu_cmdbuf_reg(uint8_t *cmdbuf) {
  int total_bits = TIU_DESC_REG_BYTES * 8;

  for (int i = 0; i < total_bits; i += 128)
    cmdbuf[(i + 128 - 8) / 8] |= (i / 128) << 4;

  uint8_t tmp[128 / 8];
  uint8_t *last = &cmdbuf[(total_bits - 128) / 8];
  memcpy(tmp, last, sizeof(tmp));
  memcpy(last, cmdbuf, sizeof(tmp));
  memcpy(cmdbuf, tmp, sizeof(tmp));
}

void CModelCmdbuf::reorder_tiu_cmdbuf(uint8_t *cmdbuf, size_t sz) {
  cmd_hdr_t *hdr = NULL;
  for (uint32_t i = 0; i < sz; i += sizeof(*hdr) + hdr->len) {
    hdr = (typeof(hdr))(&cmdbuf[i]);
    if (hdr->engine_id == CVI_TPU_TIU)
      reorder_tiu_cmdbuf_reg(hdr->cmd);
  }
}

void CModelCmdbuf::enable_tdma_cmdbuf_barrier(uint8_t *cmdbuf, size_t sz) {
  cmd_hdr_t *hdr = NULL;
  for (uint32_t i = 0; i < sz; i += sizeof(*hdr) + hdr->len) {
    hdr = (typeof(hdr))(&cmdbuf[i]);
    if (hdr->engine_id == CVI_TPU_TDMA) {
      uint32_t *buf = (uint32_t *)hdr->cmd;
      buf[0] |= (1 << 4);
    }
  }
}

void CModelCmdbuf::adjust_cmdbuf(uint8_t *cmdbuf, size_t sz) {

  set_eod(cmdbuf, sz);
  enable_interrupt(cmdbuf, sz);
  enable_tdma_cmdbuf_barrier(cmdbuf, sz);

  /*
   * Must come after all tiu cmdbuf adjustion
   */
  reorder_tiu_cmdbuf(cmdbuf, sz);
}

int CModelCmdbuf::extract_cmdbuf(int engine_id, uint8_t *cmdbuf, uint8_t *found_cmdbuf,
                          size_t sz) {
  int found_sz = 0;
  cmd_hdr_t *hdr = NULL;
  for (uint32_t i = 0; i < sz; i += sizeof(*hdr) + hdr->len) {
    hdr = (typeof(hdr))(&cmdbuf[i]);

    if (hdr->magic != cmdbuf_hdr_magic) {
      TPU_LOG_WARNING("env cv182x/cv183x might set incorrect, cmdbuf 0x%x, 0x%x\n", hdr->magic, cmdbuf_hdr_magic);
      return -1;
    }

    if (hdr->engine_id == engine_id) {
      memcpy(&found_cmdbuf[found_sz], hdr->cmd, hdr->len);
      found_sz += hdr->len;
    }
  }
  return found_sz;
}

int CModelCmdbuf::extract_dmabuf(int engine_id, uint8_t *dmabuf, uint8_t *found_dmabuf,
                          size_t sz) {
  (void)(sz);
  int found_sz = 0;
  dma_hdr_t *dma_hdr = reinterpret_cast<dma_hdr_t *>(dmabuf);
  cvi_cpu_desc_t *dma_desc = reinterpret_cast<cvi_cpu_desc_t *>(dmabuf + sizeof(dma_hdr_t));

  if (dma_hdr->dmabuf_magic_s != dmabuf_hdr_magic) {
    TPU_LOG_WARNING("env cv182x/cv183x might set incorrect, dmabuf 0x%x, 0x%x\n", dma_hdr->dmabuf_magic_s, dmabuf_hdr_magic);
    return -1;
  }

  for (uint32_t i = 0; i < dma_hdr->cpu_desc_count; ++i) {
    uint32_t engine_num = 0;
    uint64_t engine_offset = 0;
    uint32_t engine_step = 0;

    switch (engine_id) {
      case CVI_TPU_TIU: {   // TIU
        engine_num = dma_desc->num_tiu & 0xFFFF;
        engine_offset = dma_desc->offset_tiu;
        engine_step = BD_REG_BYTES;
        break;
      }
      case CVI_TPU_TDMA: {
        engine_num = dma_desc->num_tdma & 0xFFFF;
        engine_offset = dma_desc->offset_tdma;
        engine_offset = ALIGN(engine_offset, GDMA_DESC_ALIGN_SIZE);
        engine_step = GDMA_DESC_ALIGN_SIZE;
        break;
      }
      default:
        TPU_LOG_ERROR("engine id error!\n");
        assert(0);
        break;
    }
    int cur_sz = engine_num * engine_step;
    memcpy(found_dmabuf + found_sz, dmabuf + engine_offset, cur_sz);
    found_sz += cur_sz;
    ++dma_desc;
  }
  return found_sz;
}

bmerr_t CModelCmdbuf::rt_send_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint16_t *seq_no) {
  uint8_t *cmdbuf2 = new uint8_t[sz];
  memcpy(cmdbuf2, cmdbuf, sz);
  adjust_cmdbuf(cmdbuf2, sz);

  uint8_t *tiu_cmdbuf = new uint8_t[sz];
  int tiu_sz = extract_cmdbuf(CVI_TPU_TIU, cmdbuf2, tiu_cmdbuf, sz);
  assert(tiu_sz <= (int)g_tiu_cmdbuf_reserved_size);

  uint8_t *tdma_cmdbuf = new uint8_t[sz];
  int tdma_sz = extract_cmdbuf(CVI_TPU_TDMA, cmdbuf2, tdma_cmdbuf, sz);
  assert(tdma_sz <= (int)g_tdma_cmdbuf_reserved_size);

  BMDEV_LOCK(ctx->dev);
  bmmod_t model = ctx->dev->model;
  bm_cmodel_write_gmem(model, g_tiu_cmdbuf_gaddr, tiu_cmdbuf, tiu_sz);
  bm_cmodel_write_gmem(model, g_tdma_cmdbuf_gaddr, tdma_cmdbuf, tdma_sz);
  bm_cmodel_run_gmem_cmdbuf(model, g_tiu_cmdbuf_gaddr, tiu_sz,
                            g_tdma_cmdbuf_gaddr, tdma_sz);

  *seq_no = ctx->seq_no++;
  bm_wait_cmdbuf_done(ctx, *seq_no);

  delete[] tiu_cmdbuf;
  delete[] tdma_cmdbuf;
  delete[] cmdbuf2;
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_wait_cmdbuf_done(bmctx_t ctx, uint16_t seq_no) {
  (void)seq_no;
  BMDEV_UNLOCK(ctx->dev);
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem) {
  TPU_LOG_DEBUG("Cmodel: bm_load_cmdbuf\n");
  assert(enable_pmu == false);
  uint8_t *cmdbuf2 = new uint8_t[sz];
  memcpy(cmdbuf2, cmdbuf, sz);
  adjust_cmdbuf(cmdbuf2, sz);

  int cmdbuf3_sz = sizeof(CMDBUF_HEADER_T) + sz;
  uint8_t *cmdbuf3 = new uint8_t[cmdbuf3_sz];
  uint8_t *tiu_cmdbuf = cmdbuf3 + sizeof(CMDBUF_HEADER_T);
  int tiu_sz = extract_cmdbuf(CVI_TPU_TIU, cmdbuf2, tiu_cmdbuf, sz);
  assert(tiu_sz <= (int)sz && tiu_sz <= (int)g_tiu_cmdbuf_reserved_size);

  uint8_t *tdma_cmdbuf = tiu_cmdbuf + tiu_sz;
  int tdma_sz = extract_cmdbuf(CVI_TPU_TDMA, cmdbuf2, tdma_cmdbuf, sz);
  assert(tdma_sz + tiu_sz <= (int)sz && tdma_sz <= (int)g_tdma_cmdbuf_reserved_size);

  CMDBUF_HEADER_T *hdr = (typeof(hdr))cmdbuf3;
  hdr->neuron_gaddr = neuron_gaddr;
  hdr->weight_gaddr = weight_gaddr;
  hdr->tiu_cmdbuf_sz = tiu_sz;
  hdr->tdma_cmdbuf_sz = tdma_sz;

  *cmdbuf_mem = bmmem_device_alloc_raw(ctx, cmdbuf3_sz);
  bmerr_t ret = bm_memcpy_s2d(ctx, *cmdbuf_mem, cmdbuf3);
  TPU_ASSERT(ret == BM_SUCCESS, nullptr);
  delete[] cmdbuf2;
  delete[] cmdbuf3;
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_load_dmabuf(bmctx_t ctx, bmmem_device_t dmabuf, size_t sz,
                       unsigned long long neuron_gaddr, unsigned long long weight_gaddr,
                       bool enable_pmu, bmmem_device_t *dmabuf_mem) {
  TPU_LOG_DEBUG("Cmodel: bm_load_dmabuf\n");
  assert(enable_pmu == false);

  int dmabuf2_sz = sizeof(CMDBUF_HEADER_T) + sz;
  uint8_t *dmabuf2 = new uint8_t[dmabuf2_sz];
  uint8_t *tiu_dmabuf = dmabuf2 + sizeof(CMDBUF_HEADER_T);
  int tiu_sz = extract_dmabuf(CVI_TPU_TIU, dmabuf->v_addr, tiu_dmabuf, sz);
  assert(tiu_sz <= (int)sz && tiu_sz <= (int)g_tiu_cmdbuf_reserved_size);

  uint8_t *tdma_dmabuf = tiu_dmabuf + tiu_sz;
  int tdma_sz = extract_dmabuf(CVI_TPU_TDMA, dmabuf->v_addr, tdma_dmabuf, sz);
  assert(tdma_sz + tiu_sz <= (int)sz && tdma_sz <= (int)g_tdma_cmdbuf_reserved_size);

  CMDBUF_HEADER_T *hdr = (typeof(hdr))dmabuf2;
  hdr->neuron_gaddr = neuron_gaddr;
  hdr->weight_gaddr = weight_gaddr;
  hdr->tiu_cmdbuf_sz = tiu_sz;
  hdr->tdma_cmdbuf_sz = tdma_sz;

  *dmabuf_mem = bmmem_device_alloc_raw(ctx, dmabuf2_sz);
  bmerr_t ret = bm_memcpy_s2d(ctx, *dmabuf_mem, dmabuf2);
  TPU_ASSERT(ret == BM_SUCCESS, nullptr);
  delete[] dmabuf2;
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_run_cmdbuf(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no) {
  TPU_LOG_DEBUG("Cmodel: bm_run_cmdbuf\n");
  bm_memory_t *mem = (bm_memory_t *)cmdbuf_mem;
  size_t sz = mem->size;
  uint8_t *cmdbuf = new uint8_t[sz];
  bm_memcpy_d2s(ctx, cmdbuf, cmdbuf_mem);
  CMDBUF_HEADER_T *hdr = (typeof(hdr))cmdbuf;
  uint8_t *tiu_cmdbuf = cmdbuf + sizeof(CMDBUF_HEADER_T);
  uint8_t *tdma_cmdbuf = tiu_cmdbuf + hdr->tiu_cmdbuf_sz;

  BMDEV_LOCK(ctx->dev);
  bmmod_t model = ctx->dev->model;
  bm_device_set_base_reg(ctx, 0, hdr->neuron_gaddr);
  bm_device_set_base_reg(ctx, 1, hdr->weight_gaddr);
  bm_cmodel_write_gmem(model, g_tiu_cmdbuf_gaddr, tiu_cmdbuf, hdr->tiu_cmdbuf_sz);
  bm_cmodel_write_gmem(model, g_tdma_cmdbuf_gaddr, tdma_cmdbuf, hdr->tdma_cmdbuf_sz);
  bm_cmodel_run_gmem_cmdbuf(model, g_tiu_cmdbuf_gaddr, hdr->tiu_cmdbuf_sz,
                            g_tdma_cmdbuf_gaddr, hdr->tdma_cmdbuf_sz);

  *seq_no = ctx->seq_no++;

  delete[] cmdbuf;
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_run_cmdbuf_ex(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, uint64_t input_base_addr, uint64_t output_base_addr) {
  TPU_LOG_DEBUG("Cmodel: bm_run_cmdbuf\n");
  bm_memory_t *mem = (bm_memory_t *)cmdbuf_mem;
  size_t sz = mem->size;
  uint8_t *cmdbuf = new uint8_t[sz];
  bm_memcpy_d2s(ctx, cmdbuf, cmdbuf_mem);
  CMDBUF_HEADER_T *hdr = (typeof(hdr))cmdbuf;
  uint8_t *tiu_cmdbuf = cmdbuf + sizeof(CMDBUF_HEADER_T);
  uint8_t *tdma_cmdbuf = tiu_cmdbuf + hdr->tiu_cmdbuf_sz;

  BMDEV_LOCK(ctx->dev);
  bmmod_t model = ctx->dev->model;
  bm_device_set_base_reg(ctx, 0, hdr->neuron_gaddr);
  bm_device_set_base_reg(ctx, 1, hdr->weight_gaddr);
  bm_device_set_base_reg(ctx, 2, input_base_addr);
  bm_device_set_base_reg(ctx, 3, output_base_addr);
  bm_cmodel_write_gmem(model, g_tiu_cmdbuf_gaddr, tiu_cmdbuf, hdr->tiu_cmdbuf_sz);
  bm_cmodel_write_gmem(model, g_tdma_cmdbuf_gaddr, tdma_cmdbuf, hdr->tdma_cmdbuf_sz);
  bm_cmodel_run_gmem_cmdbuf(model, g_tiu_cmdbuf_gaddr, hdr->tiu_cmdbuf_sz,
                            g_tdma_cmdbuf_gaddr, hdr->tdma_cmdbuf_sz);

  *seq_no = ctx->seq_no++;

  delete[] cmdbuf;
  return BM_SUCCESS;
}

bmerr_t CModelCmdbuf::rt_run_cmdbuf_ex2(bmctx_t ctx, bmmem_device_t cmdbuf_mem,
                      uint16_t *seq_no, cvi_array_base *p_array_base) {
  TPU_LOG_DEBUG("Cmodel: bm_run_cmdbuf\n");
  bm_memory_t *mem = (bm_memory_t *)cmdbuf_mem;
  size_t sz = mem->size;
  uint8_t *cmdbuf = new uint8_t[sz];
  bm_memcpy_d2s(ctx, cmdbuf, cmdbuf_mem);
  CMDBUF_HEADER_T *hdr = (typeof(hdr))cmdbuf;
  uint8_t *tiu_cmdbuf = cmdbuf + sizeof(CMDBUF_HEADER_T);
  uint8_t *tdma_cmdbuf = tiu_cmdbuf + hdr->tiu_cmdbuf_sz;

  BMDEV_LOCK(ctx->dev);
  bmmod_t model = ctx->dev->model;
  bm_device_set_base_reg(ctx, 0, p_array_base->gaddr_base0);
  bm_device_set_base_reg(ctx, 1, p_array_base->gaddr_base1);
  bm_device_set_base_reg(ctx, 2, p_array_base->gaddr_base2);
  bm_device_set_base_reg(ctx, 3, p_array_base->gaddr_base3);
  bm_device_set_base_reg(ctx, 4, p_array_base->gaddr_base4);
  bm_device_set_base_reg(ctx, 5, p_array_base->gaddr_base5);
  bm_device_set_base_reg(ctx, 6, p_array_base->gaddr_base6);
  bm_device_set_base_reg(ctx, 7, p_array_base->gaddr_base7);
  bm_cmodel_write_gmem(model, g_tiu_cmdbuf_gaddr, tiu_cmdbuf, hdr->tiu_cmdbuf_sz);
  bm_cmodel_write_gmem(model, g_tdma_cmdbuf_gaddr, tdma_cmdbuf, hdr->tdma_cmdbuf_sz);
  bm_cmodel_run_gmem_cmdbuf(model, g_tiu_cmdbuf_gaddr, hdr->tiu_cmdbuf_sz,
                            g_tdma_cmdbuf_gaddr, hdr->tdma_cmdbuf_sz);

  *seq_no = ctx->seq_no++;

  delete[] cmdbuf;
  return BM_SUCCESS;
}

bmmem_device_t CModelCmdbuf::rt_device_alloc_raw(bmctx_t ctx, size_t size) {
  size_t axi_alignment = 16;
  size_t pool_size = ALIGN(size, axi_alignment);
  uint64_t addr = mem_pool_alloc(ctx->dev->device_mem_pool, pool_size);
  addr += g_gmem_reserved_size;

  // TPU_LOG_DEBUG("mmpool alloc, size=%lu, addr=%lx\n", size, addr);

  bm_memory_t *device_mem = new bm_memory_t();
  device_mem->flags.u.is_prealloc = 0;
  device_mem->flags.u.type = BMMEM_TYPE_DEVICE;
  device_mem->p_addr = addr;

  //device_mem->v_addr = NULL;
  //device_mem->v_addr = (void*)(device_mem->p_addr);
  device_mem->v_addr = (uint8_t*)((device_mem->p_addr) + ((unsigned long long)(bm_cmodel_get_chipGmem(ctx->dev->model))));
  device_mem->size = size;
  return (bmmem_device_t)device_mem;
}

void CModelCmdbuf::rt_device_free(bmctx_t ctx, bmmem_device_t mem) {
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, nullptr);
  if (!device_mem->flags.u.is_prealloc) {
    unsigned long long addr = device_mem->p_addr;
    // size_t size = device_mem->size;
    // TPU_LOG_DEBUG("mmpool free, size=%lu, addr=%lx\n", size, addr);

    addr -= g_gmem_reserved_size;
    mem_pool_free(ctx->dev->device_mem_pool, addr);
  }
  delete device_mem;
}
