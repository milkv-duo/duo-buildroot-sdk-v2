#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include <bmkernel/bm1822/bmkernel_1822.h>
#include <bmkernel/bm1822/bm1822_tiu_reg.h>
#include <bmkernel/bm1822/bm1822_tdma_reg.h>
#include <bmkernel/reg_tiu.h>
#include <bmkernel/reg_tdma.h>
#include <bmkernel/reg_bdcast.h>
#include <bmkernel/bm_regcpu.h>
#include "bmruntime_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)

#define BD_DESC_ALIGN_SIZE (1 << BDC_ENGINE_CMD_ALIGNED_BIT)
#define GDMA_DESC_ALIGN_SIZE (1 << TDMA_DESCRIPTOR_ALIGNED_BIT)
#define BD_EOD_PADDING_BYTES (128)

typedef struct {
  cmd_hdr_t hdr;
  uint32_t body[0];
} DESC;

static DESC *traverse_start(uint8_t *cmdbuf)
{
  TPU_ASSERT(cmdbuf, NULL);
  DESC *desc = (DESC *)cmdbuf;
  TPU_ASSERT(desc->hdr.magic == CMDBUF_HDR_MAGIC_1822, NULL);
  return desc;
}

static DESC *traverse_next(DESC *desc, uint8_t *cmdbuf, size_t size)
{
  DESC *next_desc = (DESC *)((uint8_t *)desc + cmd_hdr_len(&desc->hdr) + sizeof(cmd_hdr_t));
  if ((uint8_t *)next_desc >= cmdbuf + size)
    return NULL;
  TPU_ASSERT(next_desc->hdr.magic == CMDBUF_HDR_MAGIC_1822, NULL);
  return next_desc;
}

static bool is_last_desc(DESC *desc, uint8_t *cmdbuf, size_t size)
{
  DESC *next_desc = traverse_next(desc, cmdbuf, size);
  return next_desc ? false : true;
}

static void reorder_bd_cmdbuf_reg(uint8_t *cmdbuf)
{
  int total_bits = BD_REG_BYTES * 8;

  for (int i = 0; i < total_bits; i += 128)
    cmdbuf[(i + 128 - 8) / 8] |= (i / 128) << 4;

  uint8_t tmp[128 / 8];
  uint8_t *last = &cmdbuf[(total_bits - 128) / 8];
  memcpy(tmp, last, sizeof(tmp));
  memcpy(last, cmdbuf, sizeof(tmp));
  memcpy(cmdbuf, tmp, sizeof(tmp));
}

static void adjust_desc_tdma(uint32_t *body, bool eod)
{
  if (eod) {
    body[0] |= (1 << TDMA_ACCPI0_EOD_BIT);
    body[0] |= (1 << TDMA_ACCPI0_INTERRUPT_BIT); // interrupt
  }
  body[0] |= (1 << TDMA_ACCPI0_BARRIER_ENABLE_BIT);
}

static void adjust_desc_bd(uint32_t *body, bool eod)
{
  if (eod) {
    tiu_reg_t reg;
    parse_tiu_reg(&reg, body);
    reg.cmd_end = 1;
    reg.cmd_intr_en = 1;
    emit_tiu_reg(&reg, body);
  }
  reorder_bd_cmdbuf_reg((uint8_t *)body);
}

bmerr_t cvi182x_dmabuf_relocate(uint8_t *dmabuf, uint64_t dmabuf_devaddr, uint32_t original_size, uint32_t pmubuf_size)
{
  dma_hdr_t *header = (dma_hdr_t *)dmabuf;
  uint64_t tmpAddress = 0;

  TPU_ASSERT(header->dmabuf_magic_m == TPU_DMABUF_HEADER_M, NULL);
  cvi_cpu_desc_t *desc = (cvi_cpu_desc_t *)(dmabuf + sizeof(dma_hdr_t));

  for (uint32_t i = 0; i < header->cpu_desc_count; i++, desc++) {
    uint32_t tiu_num = desc->num_tiu & 0xFFFF;
    uint32_t tdma_num = desc->num_tdma & 0xFFFF;

    if (tiu_num) {
      tmpAddress = dmabuf_devaddr + desc->offset_tiu;
      //TPU_LOG_DEBUG("bd tmpAddress = 0x%lu\n", tmpAddress);
      desc->offset_tiu_ori_bk = desc->offset_tiu;
      desc->offset_tiu = tmpAddress >> BDC_ENGINE_CMD_ALIGNED_BIT;
    }

    if (tdma_num) {
      tmpAddress = dmabuf_devaddr + desc->offset_tdma;
      //TPU_LOG_DEBUG("tdma tmpAddress = 0x%lu\n", tmpAddress);
      desc->offset_tdma_ori_bk = desc->offset_tdma;
      desc->offset_tdma = tmpAddress >> TDMA_DESCRIPTOR_ALIGNED_BIT;
    }

    //set pmubuf_addr_p to enable pmu kick
    header->pmubuf_size = pmubuf_size;
    header->pmubuf_offset = original_size;
  }
  return BM_SUCCESS;
}

static uint32_t desc_sync_id(DESC *desc)
{
  switch (desc->hdr.engine_id) {
    case BMK1822_TIU: {
      tiu_reg_t reg;
      parse_tiu_reg(&reg, desc->body);
      return reg.cmd_id_tpu;
    }
    case BMK1822_TDMA: {
      tdma_reg_t reg;
      parse_tdma_reg(&reg, desc->body);
      return reg.cmd_id;
    }
    default:
      TPU_ASSERT(0, NULL);
      return 1;
  }
}

static void fill_header_and_arm(uint8_t *cmdbuf, size_t sz, uint8_t *dmabuf, uint64_t *tiu_offset, uint64_t *tdma_offset)
{
  dma_hdr_t header = {0};
  header.dmabuf_magic_m = TPU_DMABUF_HEADER_M;
  header.dmabuf_magic_s = 0x1822;

  cvi_cpu_desc_t *segments = (cvi_cpu_desc_t *)(dmabuf + sizeof(dma_hdr_t));
  DESC *desc = NULL;
  size_t desc_nums[BMK1822_ENGINE_NUM] = {0};
  size_t counters[BMK1822_ENGINE_NUM] = {0};
  size_t desc_size[BMK1822_ENGINE_NUM] = {0};

  TPU_ASSERT(segments, NULL);
  // fill arm descs
  desc = traverse_start(cmdbuf);

  while (desc != NULL) {
    uint32_t engine_id = (uint32_t)desc->hdr.engine_id;
    counters[engine_id]++;
    desc_nums[engine_id]++;
    if (engine_id != BMK1822_CPU) {
      // a new arm desc inserted to do sync operation
      if (desc_sync_id(desc) == 0xFFFF || is_last_desc(desc, cmdbuf, sz)) {
        desc_nums[BMK1822_CPU]++;
        cvi_cpu_desc_t *arm = segments + desc_nums[BMK1822_CPU] - 1;
        memset(arm, 0, sizeof(cvi_cpu_desc_t));
        arm->op_type = CPU_OP_SYNC;
        arm->num_tiu = counters[BMK1822_TIU];
        arm->num_tdma = counters[BMK1822_TDMA];
        strncpy(arm->str, "layer_end", sizeof(arm->str) - 1);
        if (counters[BMK1822_TIU] != 0) {
          desc_size[BMK1822_TIU] =
              ALIGN(desc_size[BMK1822_TIU] + counters[BMK1822_TIU] * BD_REG_BYTES + BD_EOD_PADDING_BYTES,
                    BD_DESC_ALIGN_SIZE);
        }
        counters[BMK1822_TIU] = 0;
        counters[BMK1822_TDMA] = 0;
      }
    } else {
      cvi_cpu_desc_t *arm = segments + desc_nums[BMK1822_CPU] - 1;
      memcpy(arm, &(desc->body), sizeof(cvi_cpu_desc_t));
      arm->num_tiu = counters[BMK1822_TIU];
      arm->num_tdma = counters[BMK1822_TDMA];
      if (counters[BMK1822_TIU] != 0) {
        desc_size[BMK1822_TIU] =
            ALIGN(desc_size[BMK1822_TIU] + counters[BMK1822_TIU] * BD_REG_BYTES + BD_EOD_PADDING_BYTES,
                  BD_DESC_ALIGN_SIZE);
      }
      counters[BMK1822_TIU] = 0;
      counters[BMK1822_TDMA] = 0;
    }
    desc = traverse_next(desc, cmdbuf, sz);
  }
  desc_size[BMK1822_CPU] = desc_nums[BMK1822_CPU] * CPU_ENGINE_BYTES;
  desc_size[BMK1822_TDMA] = desc_nums[BMK1822_TDMA] * GDMA_DESC_ALIGN_SIZE;

  (*tiu_offset) = ALIGN(sizeof(header) + desc_size[BMK1822_CPU], BD_DESC_ALIGN_SIZE);
  (*tdma_offset) = ALIGN((*tiu_offset) + desc_size[BMK1822_TIU], GDMA_DESC_ALIGN_SIZE);

  // dma hdr + arm descs + bd descs + tdma descs
  header.dmabuf_size = (*tdma_offset) + desc_size[BMK1822_TDMA];
  header.cpu_desc_count = desc_nums[BMK1822_CPU];
  header.bd_desc_count = desc_nums[BMK1822_TIU];
  header.tdma_desc_count = desc_nums[BMK1822_TDMA];

  //TPU_LOG_DEBUG("header.dmabuf_size = %d\n", header.dmabuf_size);
  //TPU_LOG_DEBUG("header.cpu_desc_count = %d\n", header.cpu_desc_count);
  //TPU_LOG_DEBUG("header.bd_desc_count = %d\n", header.bd_desc_count);
  //TPU_LOG_DEBUG("header.tdma_desc_count = %d\n", header.tdma_desc_count);

  memcpy(dmabuf, &header, sizeof(header));
}

static void fill_bd_and_tdma(uint8_t *cmdbuf, size_t sz, uint8_t *dmabuf, uint64_t tiu_offset, uint64_t tdma_offset)
{
  dma_hdr_t *p_header = (dma_hdr_t *)dmabuf;
  cvi_cpu_desc_t *segments = (cvi_cpu_desc_t *)(dmabuf + sizeof(dma_hdr_t));
  DESC *desc = traverse_start(cmdbuf);
  //uint64_t address_max = 0x0;
  
  for (uint32_t i = 0; i < p_header->cpu_desc_count; i++) {

    cvi_cpu_desc_t *arm = segments + i;
    
    uint32_t tiu_num = arm->num_tiu & 0xFFFF;
    uint32_t tdma_num = arm->num_tdma & 0xFFFF;

    if (tiu_num) {
      tiu_offset = ALIGN(tiu_offset, 1 << BDC_ENGINE_CMD_ALIGNED_BIT);
      arm->offset_tiu = tiu_offset;
      //TPU_LOG_DEBUG("arm->offset_tiu = 0x%x \n", arm->offset_tiu);
    }

    if (tdma_num) {
      tdma_offset = ALIGN(tdma_offset, 1 << TDMA_DESCRIPTOR_ALIGNED_BIT);
      arm->offset_tdma = tdma_offset;
      //TPU_LOG_DEBUG("arm->offset_tdma = 0x%x \n", arm->offset_tdma);
    }

    while (tiu_num || tdma_num) {
      uint32_t engine_id = (uint32_t)desc->hdr.engine_id;
      void *p_body = NULL;

      switch (engine_id) {
        case BMK1822_TIU:
          tiu_num--;
          p_body = (void *)(dmabuf + tiu_offset);
          tiu_offset += BD_REG_BYTES;
          memcpy(p_body, desc->body, desc->hdr.len);
          adjust_desc_bd((uint32_t *)p_body, tiu_num == 0);
          break;
        case BMK1822_TDMA:
          tdma_num--;
          tdma_offset = ALIGN(tdma_offset, GDMA_DESC_ALIGN_SIZE);
          p_body = (void *)(dmabuf + tdma_offset);
          tdma_offset += GDMA_DESC_ALIGN_SIZE;
          memcpy(p_body, desc->body, desc->hdr.len);

#if 0 //debug feature, for checking if neuron overshoot
{
          tdma_reg_t reg_tdma = {0};
          uint64_t tdma_address = 0, tdma_address2 = 0;

          parse_tdma_reg(&reg_tdma, p_body);

          if (reg_tdma.src_base_reg_sel == 0) {
            //  reg.trans_dir = 2; // 0:tg2l, 1:l2tg, 2:g2g, 3:l2l
            if (reg_tdma.trans_dir == 0) {
              TPU_LOG_DEBUG ("src_base_addr_high=%x, src_base_addr_low=%x\n", reg_tdma.src_base_addr_high, reg_tdma.src_base_addr_low);
              tdma_address = ((uint64_t)reg_tdma.src_base_addr_high) << 32 | (uint64_t)reg_tdma.src_base_addr_low;
            } else if (reg_tdma.trans_dir == 1) {
              TPU_LOG_DEBUG ("dst_base_addr_high=%x, dst_base_addr_low=%x\n", reg_tdma.dst_base_addr_high, reg_tdma.dst_base_addr_low);
              tdma_address = ((uint64_t)reg_tdma.dst_base_addr_high) << 32 | (uint64_t)reg_tdma.dst_base_addr_low;
            } else if (reg_tdma.trans_dir == 2) {
              TPU_LOG_DEBUG ("dst_base_addr_high=%x, dst_base_addr_low=%x\n", reg_tdma.dst_base_addr_high, reg_tdma.dst_base_addr_low);
              tdma_address = ((uint64_t)reg_tdma.src_base_addr_high) << 32 | (uint64_t)reg_tdma.src_base_addr_low;
              tdma_address2 = ((uint64_t)reg_tdma.dst_base_addr_high) << 32 | (uint64_t)reg_tdma.dst_base_addr_low;

              if (tdma_address2 > tdma_address) {
                tdma_address = tdma_address2;
              }
            }
            
            if (tdma_address > address_max) {
              address_max = tdma_address;
              TPU_LOG_DEBUG("address_max=%llx\n", address_max);
            }
          }
}
#endif
          adjust_desc_tdma((uint32_t *)p_body, tdma_num == 0);
          break;
        default:
          break;
      }
      desc = traverse_next(desc, cmdbuf, sz);
    }

    // padding zero after eod to workaroud hardware bug
    if (arm->num_tiu & 0xFFFF) {
      void *buf = (void *)(dmabuf + tiu_offset);
      memset(buf, 0, BD_EOD_PADDING_BYTES);
      tiu_offset += BD_EOD_PADDING_BYTES;
    }
  }

}

bmerr_t cvi182x_dmabuf_convert(uint8_t *cmdbuf, size_t sz, uint8_t *dmabuf)
{
  uint64_t tiu_offset = 0;
  uint64_t tdma_offset = 0;
  fill_header_and_arm(cmdbuf, sz, dmabuf, &tiu_offset, &tdma_offset);
  fill_bd_and_tdma(cmdbuf, sz, dmabuf, tiu_offset, tdma_offset);
  return BM_SUCCESS;
}

#define PER_DES_SIZE 16
#define PADDING_SIZE (1024 * 1024)
bmerr_t cvi182x_dmabuf_size(uint8_t *cmdbuf, size_t sz, size_t *psize, size_t *pmu_size)
{
  size_t tdma_desc_num = {0};
  size_t counters[BMK1822_ENGINE_NUM] = {0};
  size_t bd_size = 0;
  size_t dmabuf_size = 0;

  uint32_t tiu_cnt = 0;
  uint32_t tdma_cnt = 0;

  // calculate desc numbers
  DESC *desc = traverse_start(cmdbuf);

  while (desc != NULL) {
    uint32_t engine_id = (uint32_t)desc->hdr.engine_id;
    counters[engine_id]++;
    if (engine_id != BMK1822_CPU) {
      // a new arm desc inserted to do sync operation
      if (desc_sync_id(desc) == 0xFFFF || is_last_desc(desc, cmdbuf, sz)) {
        counters[BMK1822_CPU]++;
        tdma_desc_num += counters[BMK1822_TDMA];
        if (counters[BMK1822_TIU] != 0) {
          bd_size = ALIGN(bd_size + counters[BMK1822_TIU] * BD_REG_BYTES + BD_EOD_PADDING_BYTES,
                          BD_DESC_ALIGN_SIZE);
        }
        tiu_cnt += counters[BMK1822_TIU] & 0xFFFF;
        tdma_cnt += counters[BMK1822_TDMA] & 0xFFFF;
        counters[BMK1822_TIU] = 0;
        counters[BMK1822_TDMA] = 0;
      }
    } else {
      tdma_desc_num += counters[BMK1822_TDMA];
      if (counters[BMK1822_TIU] != 0) {
        bd_size = ALIGN(bd_size + counters[BMK1822_TIU] * BD_REG_BYTES + BD_EOD_PADDING_BYTES,
                        BD_DESC_ALIGN_SIZE);
      }
      tiu_cnt += counters[BMK1822_TIU] & 0xFFFF;
      tdma_cnt += counters[BMK1822_TDMA] & 0xFFFF;
      counters[BMK1822_TIU] = 0;
      counters[BMK1822_TDMA] = 0;
    }
    desc = traverse_next(desc, cmdbuf, sz);
  }
  // dma hdr + arm descs + bd descs + tdma descs
  dmabuf_size = sizeof(dma_hdr_t) + counters[BMK1822_CPU] * CPU_ENGINE_BYTES;
  dmabuf_size = ALIGN(dmabuf_size, BD_DESC_ALIGN_SIZE) + bd_size;
  dmabuf_size = ALIGN(dmabuf_size, GDMA_DESC_ALIGN_SIZE) + tdma_desc_num * GDMA_DESC_ALIGN_SIZE;

  *psize = dmabuf_size;

  *pmu_size = ALIGN((tiu_cnt + tdma_cnt) * PER_DES_SIZE + PADDING_SIZE, 0x1000);
  return BM_SUCCESS;
}

void cvi182x_arraybase_set(uint8_t *dmabuf, uint32_t arraybase0L, uint32_t arraybase1L, uint32_t arraybase0H, uint32_t arraybase1H)
{
  TPU_ASSERT(dmabuf, NULL);
  dma_hdr_t *header = (dma_hdr_t *)dmabuf;

  TPU_ASSERT(header->dmabuf_magic_m == TPU_DMABUF_HEADER_M, NULL);
  header->arraybase_0_L = arraybase0L;
  header->arraybase_1_L = arraybase1L;
  header->arraybase_0_H = arraybase0H;
  header->arraybase_1_H = arraybase1H;
  return;
}

uint64_t cvi182x_get_pmusize(uint8_t * dmabuf)
{
  uint32_t tiu_cnt = 0, tdma_cnt = 0;
  uint64_t pmu_size = 0;

  TPU_ASSERT(dmabuf, NULL);
  dma_hdr_t *header = (dma_hdr_t *)dmabuf;
  TPU_ASSERT(header->dmabuf_magic_m == TPU_DMABUF_HEADER_M, NULL);

  cvi_cpu_desc_t *desc = (cvi_cpu_desc_t *)(dmabuf + sizeof(dma_hdr_t));

  for (uint32_t i = 0; i < header->cpu_desc_count; i++, desc++) {
    tiu_cnt += (desc->num_tiu & 0xFFFF);
    tdma_cnt += (desc->num_tdma & 0xFFFF);
  }

  pmu_size = ALIGN((tiu_cnt + tdma_cnt) * PER_DES_SIZE + PADDING_SIZE, 0x1000);
  //TPU_LOG_DEBUG("cvi182x_get_pmusize pmusize= %" PRIu64 " \n", pmu_size);
  return pmu_size;
}
void cvi182x_dmabuf_dump(uint8_t *dmabuf)
{
  TPU_ASSERT(dmabuf, NULL);
  dma_hdr_t *header = (dma_hdr_t *)dmabuf;
  //TPU_LOG_DEBUG("bmk1822_dmabuf_dump header->arraybase_0_L = 0x%x\n", header->arraybase_0_L);
  //TPU_LOG_DEBUG("bmk1822_dmabuf_dump header->arraybase_1_L = 0x%x\n", header->arraybase_1_L);
  //TPU_LOG_DEBUG("bmk1822_dmabuf_dump header->arraybase_0_H = 0x%x\n", header->arraybase_0_H);
  //TPU_LOG_DEBUG("bmk1822_dmabuf_dump header->arraybase_1_H = 0x%x\n", header->arraybase_1_H);
  //TPU_LOG_DEBUG("bmk1822_dmabuf_dump header->pmubuf_offset = 0x%x\n", header->pmubuf_offset);

  TPU_ASSERT(header->dmabuf_magic_m == TPU_DMABUF_HEADER_M, NULL);
  cvi_cpu_desc_t *desc = (cvi_cpu_desc_t *)(dmabuf + sizeof(dma_hdr_t));

  for (u32 i = 0; i < header->cpu_desc_count; i++, desc++) {
    int bd_num = desc->num_tiu & 0xFFFF;
    int tdma_num = desc->num_tdma & 0xFFFF;
    u32 bd_offset = desc->offset_tiu;
    u32 tdma_offset = desc->offset_tdma;
    TPU_LOG_DEBUG("cvi182x_dmabuf_dump num<bd:%d, tdma:%d>, offset<0x%08x, 0x%08x>\n", bd_num, tdma_num, bd_offset, tdma_offset);
  }
}

#ifdef __cplusplus
}
#endif

