#ifndef __BM1822_TPU_CFG__
#define __BM1822_TPU_CFG__

#define BM1822_VER                                       1822
#define BM1822_HW_NPU_SHIFT                              3
#define BM1822_HW_EU_SHIFT                               4
#define BM1822_HW_LMEM_SHIFT                             15
#define BM1822_HW_LMEM_BANKS                             8
#define BM1822_HW_LMEM_BANK_SIZE                         0x1000
#define BM1822_HW_NODE_CHIP_SHIFT                        0
#define BM1822_HW_NPU_NUM                                (1 << BM1822_HW_NPU_SHIFT)
#define BM1822_HW_EU_NUM                                 (1 << BM1822_HW_EU_SHIFT)
#define BM1822_HW_LMEM_SIZE                              (1 << BM1822_HW_LMEM_SHIFT)
#define BM1822_HW_LMEM_START_ADDR                        0x0C000000
#define BM1822_HW_NODE_CHIP_NUM                          (1 << BM1822_HW_NODE_CHIP_SHIFT)

#if (BM1822_HW_LMEM_SIZE != (BM1822_HW_LMEM_BANK_SIZE * BM1822_HW_LMEM_BANKS))
#error "Set wrong TPU configuraiton."
#endif

#define BM1822_GLOBAL_MEM_START_ADDR                     0x0
#define BM1822_GLOBAL_MEM_SIZE                           0x100000000

#define BM1822_GLOBAL_TIU_CMDBUF_ADDR                    0x00000000
#define BM1822_GLOBAL_TDMA_CMDBUF_ADDR                   0x10000000
#define BM1822_GLOBAL_TIU_CMDBUF_RESERVED_SIZE           0x10000000
#define BM1822_GLOBAL_TDMA_CMDBUF_RESERVED_SIZE          0x10000000
#define BM1822_GLOBAL_POOL_RESERVED_SIZE                (BM1822_GLOBAL_MEM_SIZE - BM1822_GLOBAL_TIU_CMDBUF_RESERVED_SIZE - BM1822_GLOBAL_TDMA_CMDBUF_RESERVED_SIZE)

#define BM1822_UART_CTLR_BASE_ADDR                       0x04140000

#define BM1822_TDMA_ENGINE_BASE_ADDR                     0x0C100000
#define BM1822_TDMA_ENGINE_END_ADDR                      (BM1822_TDMA_ENGINE_BASE_ADDR + 0x1000)

#define BM1822_TIU_ENGINE_BASE_ADDR                      0x0C101000 //"NPS Register" in memory map?
#define BM1822_TIU_ENGINE_END_ADDR                       (BM1822_TIU_ENGINE_BASE_ADDR + 0x1000)

#endif
