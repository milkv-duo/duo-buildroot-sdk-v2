#include "cmodel_cmdbuf_181x.h"
#include <cvikernel/cv181x/cv181x_tiu_reg.h>
#include <cvikernel/cv181x/cv181x_tdma_reg.h>
#include <cvikernel/cv181x/cv181x_tpu_cfg.h>

CModelCmdbuf181x::CModelCmdbuf181x() {
    g_tiu_cmdbuf_gaddr = 0;
    g_tdma_cmdbuf_gaddr = 0;
    g_tiu_cmdbuf_reserved_size = 0;
    g_tdma_cmdbuf_reserved_size = 0;
    g_gmem_reserved_size = 0;
    g_gmem_size = 0;
    cmdbuf_hdr_magic = CMDBUF_HDR_MAGIC_181X;
    dmabuf_hdr_magic = 0x182202;
}

CModelCmdbuf181x::~CModelCmdbuf181x() {

}

bmerr_t CModelCmdbuf181x::rt_device_open(int index, bmdev_t *dev) {
  cvk_reg_info_t req_info;
  cvk_context_t *cvk_ctx;
  uint8_t tmp_buf[32];
  bm_device_t *pdev = new bm_device_t;

  memset(&req_info, 0, sizeof(cvk_reg_info_t));
  strncpy(req_info.chip_ver_str, CVI_TPU_VERSION_181X, sizeof(req_info.chip_ver_str) - 1);
  req_info.cmdbuf = tmp_buf;
  req_info.cmdbuf_size = sizeof(tmp_buf);
  cvk_ctx = cvikernel_register(&req_info);
  if (!cvk_ctx) {
    delete pdev;
    return BM_ERR_FAILURE;
  }

  BMDEV_LOCK_INIT(pdev);
  pdev->index = index;
  pdev->cvk_chip_info = cvk_ctx->info;
  g_tiu_cmdbuf_gaddr = CV181X_GLOBAL_TIU_CMDBUF_ADDR;
  g_tdma_cmdbuf_gaddr = CV181X_GLOBAL_TDMA_CMDBUF_ADDR;
  g_tiu_cmdbuf_reserved_size = CV181X_GLOBAL_TIU_CMDBUF_RESERVED_SIZE;
  g_tdma_cmdbuf_reserved_size = CV181X_GLOBAL_TDMA_CMDBUF_RESERVED_SIZE;
  g_gmem_reserved_size =
      g_tiu_cmdbuf_reserved_size + g_tdma_cmdbuf_reserved_size;

  g_gmem_size = pdev->cvk_chip_info.gmem_size;
  pdev->gmem_size = g_gmem_size;
  bm_cmodel_init(&pdev->model, &pdev->cvk_chip_info);

  assert(g_gmem_size > g_gmem_reserved_size);
  unsigned long long pool_size = g_gmem_size - g_gmem_reserved_size;
  mem_pool_create(&pdev->device_mem_pool, pool_size);

  cvk_ctx->ops->cleanup(cvk_ctx);
  if (cvk_ctx->priv_data)
    free(cvk_ctx->priv_data);
  free(cvk_ctx);

  TPU_LOG_DEBUG("device[%d] opened, %" PRIu64 "\n", index, g_gmem_size);

  *dev = pdev;

  return BM_SUCCESS;
}

void CModelCmdbuf181x::enable_interrupt(uint8_t *cmdbuf, size_t sz) {
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

void CModelCmdbuf181x::set_eod(uint8_t *cmdbuf, uint64_t sz) {
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
