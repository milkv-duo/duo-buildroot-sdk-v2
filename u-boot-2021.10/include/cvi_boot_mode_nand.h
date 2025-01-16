#ifndef __CVI_BOOT_MODE_NAND_H__
#define __CVI_BOOT_MODE_NAND_H__
#include <common.h>
#include <command.h>

typedef enum {
    AVB_AB_FLOW_RESULT_OK,
    AVB_AB_FLOW_RESULT_ERROR,
    AVB_AB_FLOW_RESULT_ERROR_IO,
    AVB_AB_FLOW_RESULT_ERROR_NO_BOOTABLE_SLOTS,
} AvbABFlowResult;

typedef enum {
    BOOT_MODE_A,
    BOOT_MODE_B,
    BOOT_MODE_INVALID
} boot_mode_t;

typedef struct AvbABSlotData {
    uint8_t priority;
    uint8_t tries_remaining;
    uint8_t successful_boot;
    uint8_t reserved[1];
} AvbABSlotData;

typedef struct AvbABData {
    uint8_t boot_slot;
    AvbABSlotData slots[2];
} AvbABData;

#endif /* __CVI_BOOT_MODE_NAND_H__ */
