#include <cstdlib>
#include <memory>
#include <cstring>
#include "cvi183x_device_mem.h"

Cvi183xDeviceMem::Cvi183xDeviceMem() {
  GLOBAL_MEM_START_ADDR = 0x100000000;
  g_gmem_size = 1ULL << 30; // 1GB
  tpu_dmabuf_header_m = 0xB5B5;
}

Cvi183xDeviceMem::~Cvi183xDeviceMem() {}


bmerr_t Cvi183xDeviceMem::device_open(int index, bmdev_t *dev)
{
  bm_device_t *pdev = new bm_device_t;

  BMDEV_LOCK_INIT(pdev);
  pdev->index = index;
  pdev->info.info183x = bmk1880v2_chip_info();
  pdev->gmem_size = g_gmem_size;

  const char* tpu_dev_name_defalut = TPU_DEV_NAME;
  const char* tpu_dev_name_env = std::getenv("TPU_DEV");
  const char *tpu_dev_name = tpu_dev_name_defalut;
  if (tpu_dev_name_env) {
    tpu_dev_name = tpu_dev_name_env;
  }

  pdev->dev_fd = open(tpu_dev_name, O_RDWR);
  if (pdev->dev_fd <= 0) {
    TPU_LOG_WARNING("open %s failed\n", tpu_dev_name);
    return BM_ERR_FAILURE;
  }

  pdev->ion_fd = open(ION_DEV_NAME, O_RDWR);
  if (pdev->ion_fd <= 0) {
    TPU_LOG_WARNING("open %s failed\n", ION_DEV_NAME);
    return BM_ERR_FAILURE;
  }

  int ret = ion_query_heap(pdev);
  TPU_ASSERT(ret == BM_SUCCESS, NULL);

  *dev = pdev;

  return BM_SUCCESS;
}

void Cvi183xDeviceMem::device_close(bmdev_t dev)
{
  close(dev->ion_fd);
  close(dev->dev_fd);

  // TPU_LOG_WARNING("device[%d] closed\n", dev->index);

  BMDEV_LOCK_DEINIT(dev);
  delete dev;
}

int Cvi183xDeviceMem::get_chip_ver(bmdev_t dev) {
  return dev->info.info183x.version;
}

void Cvi183xDeviceMem::mem_free_raw(bmctx_t ctx, bmmem_device_t mem) {
  char array_got = 0;
  bm_memory_t *device_mem = (bm_memory_t *)mem;
  TPU_ASSERT(device_mem->flags.u.type == BMMEM_TYPE_DEVICE, NULL);

  if (!device_mem->flags.u.is_prealloc) {
    //checking if it needs to unload firstly
    for (uint32_t i = 0; i < TEE_FIREWALL_MAX; i ++) {
      if (root_tee_firewall_info[i].address == device_mem->p_addr) {
        unload_tee(ctx, device_mem->p_addr, device_mem->size);
        memset(&root_tee_firewall_info[i], 0, sizeof(tee_firewall_info));
        break;
      }
    }

    mem_free(device_mem->v_addr, device_mem->size, device_mem->dma_fd);

    for (int i = 0; i < MEMARRAY_MAX_CNT; i ++) {
      if (ctx->root_mem_array[i].p_addr == device_mem->p_addr) {
        ctx->root_mem_array[i].p_addr = 0;
        ctx->root_mem_array[i].mem = NULL;
        array_got = 1;
        break;
      }
    }

    if (!array_got)
      TPU_LOG_WARNING("bmmem_device_free() can not find match\n");
  }

  BMEMEM_DUMP();
  delete device_mem;
}


bmerr_t Cvi183xDeviceMem::load_cmdbuf(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                       uint64_t neuron_gaddr, uint64_t weight_gaddr,
                       bool enable_pmu, bmmem_device_t *cmdbuf_mem) {
  bmerr_t ret;
  size_t dmabuf_size = 0;
  size_t pmubuf_size = 0;
  bmmem_device_t dmabuf_mem;
  ret = cvi183x_dmabuf_size(cmdbuf, sz, &dmabuf_size, &pmubuf_size);

  //calculate pmu size
  pmubuf_size = enable_pmu ? pmubuf_size : 0;
  //TPU_LOG_DEBUG("pmubuf_size = 0x%lx\n", pmubuf_size);
  if (protect) {
    dmabuf_mem = mem_alloc_pagesize(ctx, dmabuf_size + pmubuf_size);
  } else {
    dmabuf_mem = mem_alloc_raw(ctx, dmabuf_size + pmubuf_size);
  }
  if (!dmabuf_mem) {
    return BM_ERR_NOMEM;
  }
  uint64_t dmabuf_devaddr = mem_p_addr(dmabuf_mem);

  ret = cvi183x_dmabuf_convert(cmdbuf, sz, dmabuf_mem->v_addr);
  set_base_reg(ctx, 0, neuron_gaddr);
  set_base_reg(ctx, 1, weight_gaddr);
  cvi183x_arraybase_set(dmabuf_mem->v_addr, (u32)neuron_gaddr, (u32)weight_gaddr, 0, 0);

  cvi183x_dmabuf_relocate(dmabuf_mem->v_addr, dmabuf_devaddr + GLOBAL_MEM_START_ADDR, dmabuf_size,
                            pmubuf_size);
  TPU_ASSERT(mem_flush_ext(ctx->dev, dmabuf_mem->dma_fd,
        dmabuf_mem->p_addr, dmabuf_size) == BM_SUCCESS, NULL);
  // record dmabuf crc32
  // dmabuf_mem->crc32 = bm_crc32(dmabuf, dmabuf_size);

  *cmdbuf_mem = dmabuf_mem;

  // if (0) {
    //cvi183x_dmabuf_dump(dmabuf);
  //}
  return ret;
}

bmerr_t Cvi183xDeviceMem::load_dmabuf(bmctx_t ctx, bmmem_device_t dmabuf,
                                      size_t sz, uint64_t neuron_gaddr, uint64_t weight_gaddr,
                                      bool enable_pmu, bmmem_device_t *dmabuf_mem) {
  size_t pmubuf_size = 0;
  if (enable_pmu) {
    pmubuf_size = cvi183x_get_pmusize(dmabuf->v_addr);
    *dmabuf_mem = mem_alloc_raw(ctx, sz + pmubuf_size);
    if (*dmabuf_mem == nullptr) {
        TPU_LOG_ERROR("alloc dmabuf mem fail!\n");
        return BM_ERR_NOMEM;
    }
    std::memcpy((*dmabuf_mem)->v_addr, dmabuf->v_addr, sz);
  } else {
    *dmabuf_mem = dmabuf;
  }
  uint64_t dmabuf_devaddr = mem_p_addr(*dmabuf_mem);


  //set_base_reg(ctx, 0, neuron_gaddr);
  //set_base_reg(ctx, 1, weight_gaddr);
  cvi183x_arraybase_set((*dmabuf_mem)->v_addr, (u32)neuron_gaddr, (u32)weight_gaddr, 0, 0);

  cvi183x_dmabuf_relocate((*dmabuf_mem)->v_addr, dmabuf_devaddr + GLOBAL_MEM_START_ADDR, sz,
                          pmubuf_size);
  TPU_ASSERT(mem_flush_ext(ctx->dev, (*dmabuf_mem)->dma_fd,
                           (*dmabuf_mem)->p_addr, sz) == BM_SUCCESS,
             NULL);
  // if (0) {
  //cvi183x_dmabuf_dump(dmabuf);
  //}
  return BM_SUCCESS;
}

bmerr_t Cvi183xDeviceMem::load_cmdbuf_tee(bmctx_t ctx, uint8_t *cmdbuf, size_t sz,
                            uint64_t neuron_gaddr, uint64_t weight_gaddr,
                            uint32_t weight_len, bmmem_device_t *cmdbuf_mem)
{
  bmmem_device_t dmabuf_mem;
  uint8_t i = 0, firewall_hit = 0;

  //malloc double size buffer, because TEE needs 2nd space to calculate dmabuf
  if (protect) {
    dmabuf_mem = mem_alloc_pagesize(ctx, sz + sz);
  } else {
    dmabuf_mem = mem_alloc_raw(ctx, sz + sz);
  }

  if (!dmabuf_mem) {
      TPU_LOG_ERROR("alloc dmabuf mem fail!\n");
      return BM_ERR_NOMEM;
  }

  //transfer encrypted cmdbuf to TEE
  memcpy(dmabuf_mem->v_addr, cmdbuf, sz);
  TPU_ASSERT((int)mem_flush_ext(ctx->dev, dmabuf_mem->dma_fd,
        dmabuf_mem->p_addr, sz) == BM_SUCCESS, NULL);

  //firewall setting is in driver, only record info
  for (i = 0; i < TEE_FIREWALL_MAX; i ++) {
    if (root_tee_firewall_info[i].address == weight_gaddr) {
      firewall_hit = 1;
      break;
    }
  }

  if (!firewall_hit) {
    for (i = 0; i < TEE_FIREWALL_MAX; i ++) {
      if (!root_tee_firewall_info[i].address) {
        root_tee_firewall_info[i].address = weight_gaddr;
        firewall_hit = 1;
        break;
      }
    }
  }

  if (!firewall_hit) {
    TPU_LOG_ERROR("tee only supported %d firewall, not enough\n", TEE_FIREWALL_MAX);
    return BM_ERR_FAILURE;
  }

  //ioctl to get secure dma buffer
  load_tee(ctx, dmabuf_mem->p_addr, sz, weight_gaddr, weight_len, neuron_gaddr);

  //this region should be protected, can't touch in REE
  *cmdbuf_mem = dmabuf_mem;
  return BM_SUCCESS;
}


bmerr_t Cvi183xDeviceMem::unload_tee(bmctx_t ctx, uint64_t paddr, size_t size)
{
  int ret;
  struct cvi_unload_tee_arg unload_arg;
  unload_arg.paddr = paddr + GLOBAL_MEM_START_ADDR;
  unload_arg.size = size;

  ret = ioctl(ctx->dev->dev_fd, CVITPU_UNLOAD_TEE, &unload_arg);
  if (ret != 0) {
    TPU_LOG_WARNING("unload firewall failed, ret=%x\n", ret);
    return BM_ERR_FAILURE;
  }
  return BM_SUCCESS;
}

bmerr_t Cvi183xDeviceMem::parse_pmubuf(bmmem_device_t cmdbuf_mem, uint8_t **buf_start, uint32_t *buf_len) {
  dma_hdr_t *header = (dma_hdr_t *)(cmdbuf_mem->v_addr);
  //TPU_LOG_DEBUG("header->arraybase_0_L = 0x%x\n", header->arraybase_0_L);
  //TPU_LOG_DEBUG("header->arraybase_1_L = 0x%x\n", header->arraybase_1_L);
  //TPU_LOG_DEBUG("header->arraybase_0_H = 0x%x\n", header->arraybase_0_H);
  //TPU_LOG_DEBUG("header->arraybase_1_H = 0x%x\n", header->arraybase_1_H);
  //TPU_LOG_DEBUG("header->pmubuf_offset = 0x%x\n", header->pmubuf_offset);
  //TPU_LOG_DEBUG("header->pmubuf_size = 0x%x\n", header->pmubuf_size);
  if (header->pmubuf_size && header->pmubuf_offset) {
    tpu_pmu_dump_main(cmdbuf_mem->v_addr, cmdbuf_mem->p_addr);
  }
  *buf_start = cmdbuf_mem->v_addr;
  *buf_len = cmdbuf_mem->size;
  return BM_SUCCESS;
}

void Cvi183xDeviceMem::cvikernel_create(bmctx_t ctx, void **p_bk_ctx) {
  TPU_ASSERT(ctx != nullptr, nullptr);
  TPU_ASSERT(ctx->dev != nullptr, nullptr);

  bmk1880v2_chip_info_t info = bmk1880v2_chip_info();
  bmk1880v2_chip_info_t *dev_info = &info;

  bmk_info_t bmk_info;
  bmk_info.chip_version = dev_info->version;
  bmk_info.cmdbuf_size = 0x100000;
  bmk_info.cmdbuf = (u8 *)malloc(bmk_info.cmdbuf_size);
  TPU_ASSERT(bmk_info.cmdbuf, "create cvikernel, malloc failed\n");

  ctx->cvik_context.ctx183x = bmk1880v2_register(&bmk_info);
  ctx->cvik_cmdbuf = (void *)bmk_info.cmdbuf;

  *p_bk_ctx = ctx->cvik_context.ctx183x;
}

void Cvi183xDeviceMem::cvikernel_submit(bmctx_t ctx) {
  u32 len;
  u8 *cmdbuf = bmk1880v2_acquire_cmdbuf(ctx->cvik_context.ctx183x, &len);

  uint16_t seq_no;
  bmerr_t ret = send_cmdbuf(ctx, cmdbuf, (size_t)len, &seq_no);
  TPU_ASSERT(ret == BM_SUCCESS, NULL);
  bmk1880v2_reset(ctx->cvik_context.ctx183x);
}

void Cvi183xDeviceMem::cvikernel_destroy(bmctx_t ctx) {
  TPU_ASSERT(ctx->cvik_context.ctx183x, NULL);
  TPU_ASSERT(ctx->cvik_cmdbuf, NULL);

  bmk1880v2_cleanup(ctx->cvik_context.ctx183x);
  free(ctx->cvik_cmdbuf);
}