#ifndef REG_TIU_H
#define REG_TIU_H

#define TIU_DESC_REG_BYTES (0x70)
#define TIU_ENGINE_DESCRIPTOR_NUM       (TIU_DESC_REG_BYTES >> 2)

// TIU operation data type
#define DCR_TYPE_CONV_FIX8B           0
#define DCR_TYPE_DEPTHWISE_POOL_FIX8B 1
#define DCR_TYPE_FC_FIX8B             2
#define DCR_TYPE_TENSOR_ARITH_FIX8B   3
#define NR_DCR_TYPES                  4

// BD control bits base on BD_CTRL_BASE_ADDR
#define BD_TPU_EN                     0    // TPU Enable bit
#define BD_LANE_NUM                   22   // Lane number bit[29:22]
#define BD_DES_ADDR_VLD               30   // enable descriptor mode
#define BD_INTR_ENABLE                31   // TIU interrupt global enable

#endif /* REG_TIU_H */
