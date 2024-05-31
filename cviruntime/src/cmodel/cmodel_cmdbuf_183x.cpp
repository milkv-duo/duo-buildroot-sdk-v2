#include "cmodel_cmdbuf_183x.h"
#include <bmkernel/bm1880v2/bmkernel_1880v2.h>
#include <bmkernel/bm1880v2/bm1880v2_tiu_reg.h>
#include <bmkernel/bm1880v2/bm1880v2_tdma_reg.h>
#include <bmkernel/bm1880v2/bm1880v2_tpu_cfg.h>

CModelCmdbuf183x::CModelCmdbuf183x() {
  g_tiu_cmdbuf_gaddr = 0;
  g_tdma_cmdbuf_gaddr = 0;
  g_tiu_cmdbuf_reserved_size = 0;
  g_tdma_cmdbuf_reserved_size = 0;
  g_gmem_reserved_size = 0;
  g_gmem_size = 0;
  cmdbuf_hdr_magic = CMDBUF_HDR_MAGIC_1880v2;
  dmabuf_hdr_magic = 0x1835;
}

CModelCmdbuf183x::~CModelCmdbuf183x() {}

bmerr_t CModelCmdbuf183x::rt_device_open(int index, bmdev_t *dev) {
  bm_device_t *pdev = new bm_device_t;

  BMDEV_LOCK_INIT(pdev);
  pdev->index = index;
  pdev->cvk_chip_info = bmk1880v2_chip_info();
  g_tiu_cmdbuf_gaddr = BM1880V2_GLOBAL_TIU_CMDBUF_ADDR;
  g_tdma_cmdbuf_gaddr = BM1880V2_GLOBAL_TDMA_CMDBUF_ADDR;
  g_tiu_cmdbuf_reserved_size = BM1880V2_GLOBAL_TIU_CMDBUF_RESERVED_SIZE;
  g_tdma_cmdbuf_reserved_size = BM1880V2_GLOBAL_TDMA_CMDBUF_RESERVED_SIZE;
  g_gmem_reserved_size =
      g_tiu_cmdbuf_reserved_size + g_tdma_cmdbuf_reserved_size;

  g_gmem_size = pdev->cvk_chip_info.gmem_size;
  pdev->gmem_size = g_gmem_size;
  bm_cmodel_init(&pdev->model, &pdev->cvk_chip_info);

  assert(g_gmem_size > g_gmem_reserved_size);
  unsigned long long pool_size = g_gmem_size - g_gmem_reserved_size;
  mem_pool_create(&pdev->device_mem_pool, pool_size);

  TPU_LOG_DEBUG("device[%d] opened, %" PRIu64 "\n", index, g_gmem_size);

  *dev = pdev;

  return BM_SUCCESS;
}

void CModelCmdbuf183x::enable_interrupt(uint8_t *cmdbuf, size_t sz) {
  cmd_hdr_t *hdr = NULL, *last_tiu = NULL, *last_tdma = NULL;
  for (uint32_t i = 0; i < sz; i += sizeof(*hdr) + hdr->len) {
    hdr = (typeof(hdr))(&cmdbuf[i]);
    if (hdr->engine_id == CVI_TPU_TDMA)
      last_tdma = hdr;
    else if (hdr->engine_id == CVI_TPU_TIU)
      last_tiu = hdr;
  }

  if (last_tiu) {
    tiu_reg_t reg;
    parse_tiu_reg(&reg, (u32 *)last_tiu->cmd);
    reg.cmd_intr_en = 1;
    emit_tiu_reg(&reg, (u32 *)last_tiu->cmd);
  }

  if (last_tdma) {
    u32 *p = (u32 *)last_tdma->cmd;
    p[0] |= (1 << 3);
  }
}

void CModelCmdbuf183x::set_eod(uint8_t *cmdbuf, uint64_t sz) {
  cmd_hdr_t *hdr = NULL;
  cmd_hdr_t *last_tiu = NULL;
  cmd_hdr_t *last_tdma = NULL;

  for (uint32_t i = 0; i < sz; i += sizeof(*hdr) + hdr->len) {
    hdr = (typeof(hdr))(&cmdbuf[i]);
    if (hdr->engine_id == CVI_TPU_TDMA)
      last_tdma = hdr;
    else if (hdr->engine_id == CVI_TPU_TIU)
      last_tiu = hdr;
    else if (hdr->engine_id == CVI_TPU_CPU)
      continue;
    else {
      TPU_LOG_ERROR("unknown engine_id:%d\n", (int)(hdr->engine_id));
      while (1)
        ;
      assert(0);
    }
  }

  if (last_tiu) {
    tiu_reg_t reg;
    parse_tiu_reg(&reg, (u32 *)last_tiu->cmd);
    reg.cmd_end = 1;
    emit_tiu_reg(&reg, (u32 *)last_tiu->cmd);
  }

  if (last_tdma) {
    tdma_reg_t reg;
    parse_tdma_reg(&reg, (u32 *)last_tdma->cmd);
    reg.eod = 1;
    emit_tdma_reg(&reg, (u32 *)last_tdma->cmd);
  }
}