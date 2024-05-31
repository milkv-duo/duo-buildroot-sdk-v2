/*
 * Copyright Bitmain Technologies Inc.
 *
 * Written by:
 *   Wanwei CAI <wanwei.cai@bitmain.com>
 * Created Time: 2017-06-29 15:33
 */

#ifndef _BM_REG_CPU_H
#define _BM_REG_CPU_H

#include <bmkernel/bm_kernel.h>

#define CPU_ENGINE_DESCRIPTOR_NUM     56
#define CPU_ENGINE_DESCRIPTOR_DMA_NUM CPU_ENGINE_DESCRIPTOR_NUM
#define CPU_ENGINE_BYTES              (CPU_ENGINE_DESCRIPTOR_NUM*sizeof(uint32_t))
#define CPU_ENGINE_STR_LIMIT_BYTE     (CPU_ENGINE_DESCRIPTOR_NUM - 7) * sizeof(uint32_t)

#define CPU_CMD_ACCPI0                0
#define CPU_CMD_ACCPI1                1
#define CPU_CMD_ACCPI2                2
#define CPU_CMD_ACCPI3                3
#define CPU_CMD_ACCPI4                4
/* CPU_CMD_ACCPI5 ~ CPU_CMD_ACCPI63
defined here if needed */

#define CPU_ACCPI0_OP_BIT             0
#define CPU_ACCPI1_BD_CMDID_BIT       0
#define CPU_ACCPI1_CPU_CMDID_BIT      16
#define CPU_ACCPI2_GDMA_CMDID_BIT     0
#define CPU_ACCPI2_CDMA_CMDID_BIT     16
#define CPU_ACCPI3_NEXT_BD_ADDR_BIT   0
#define CPU_ACCPI4_NEXT_GDMA_ADDR_BIT 0
#define CPU_ACCPI5_NEXT_CDMA_ADDR_BIT 0

typedef enum {
  CPU_OP_SYNC = 2,
  CPU_OP_INST = 3,
  CPU_OP_END
} CPU_OP;

// CPU common structure
typedef struct {
  uint32_t regs[CPU_ENGINE_DESCRIPTOR_NUM];
} bmk_cpu_desc_t;

// CPU_OP_SYNC structure
typedef struct {
  uint32_t op_type;  // CPU_CMD_ACCPI0
  uint32_t num_bd;   // CPU_CMD_ACCPI1
  uint32_t num_gdma; // CPU_CMD_ACCPI2
  uint32_t offset_bd; // CPU_CMD_ACCPI3
  uint32_t offset_gdma; // CPU_CMD_ACCPI4
  uint32_t reserved[2]; // CPU_CMD_ACCPI5-CPU_CMD_ACCPI6
  char str[CPU_ENGINE_STR_LIMIT_BYTE];
} __attribute__((packed)) bmk_cpu_sync_desc_t;

// CPU_OP_INST structure
#define CPU_INST_HEADER_COUNT 12
typedef struct {
  uint32_t op_type;  // CPU_CMD_ACCPI0
  uint32_t num_bd;   // CPU_CMD_ACCPI1
  uint32_t num_gdma; // CPU_CMD_ACCPI2
  uint32_t offset_bd; // CPU_CMD_ACCPI3
  uint32_t offset_gdma; // CPU_CMD_ACCPI4
  uint32_t reserved[2]; // CPU_CMD_ACCPI5-CPU_CMD_ACCPI6
  char lib_name[4*sizeof(uint32_t)]; // CPU_CMD_ACCPI7~CPU_CMD_ACCPI10
  uint32_t param_size;  //CPU_CMD_ACCPI11
  uint8_t  param[0];
} __attribute__((packed)) bmk_cpu_inst_desc_t;

#endif
