#ifndef REG_GDMA_H
#define REG_GDMA_H

#define TDMA_DESC_REG_BYTES (0x40)
#define TDMA_ENGINE_DESCRIPTOR_NUM      (TDMA_DESC_REG_BYTES >> 2)
#define TDMA_NUM_BASE_REGS (0x8)

//backward compatible?
#define GDMA_TYPE_f32           0
#define GDMA_TYPE_f16           1
#define GDMA_TYPE_i32           2
#define GDMA_TYPE_i16           3
#define GDMA_TYPE_i8            4
#define GDMA_TYPE_i4            5
#define GDMA_TYPE_i2            6
#define GDMA_TYPE_i1            7
#define LAST_GDMA_TYPE_i1       8


//tdma descriptor define
#define TDMA_DESCRIPTOR_ALIGNED_BIT    6

#define TDMA_CMD_ACCP0          0
#define TDMA_CMD_ACCP1          4
#define TDMA_CMD_ACCP2          8
#define TDMA_CMD_ACCP3          12
#define TDMA_CMD_ACCP4          16
#define TDMA_CMD_ACCP5          20
#define TDMA_CMD_ACCP6          24
#define TDMA_CMD_ACCP7          28
#define TDMA_CMD_ACCP8          32
#define TDMA_CMD_ACCP9          36
#define TDMA_CMD_ACCP10         40
#define TDMA_CMD_ACCP11         44
#define TDMA_CMD_ACCP12         48
#define TDMA_CMD_ACCP13         52
#define TDMA_CMD_ACCP14         56

#define TDMA_ACCPI0_CMD_VALID_BIT      		0
#define TDMA_ACCPI0_EOD_BIT            		2
#define TDMA_ACCPI0_INTERRUPT_BIT      		3
#define TDMA_ACCPI0_BARRIER_ENABLE_BIT		4


//tdma control define
#define TDMA_CTRL				        (TDMA_ENGINE_BASE_ADDR + 0x0)
#define TDMA_DES_BASE           (TDMA_ENGINE_BASE_ADDR + 0x4)
#define TDMA_INT_MASK           (TDMA_ENGINE_BASE_ADDR + 0x8)
#define TDMA_SYNC_STATUS 		    (TDMA_ENGINE_BASE_ADDR + 0xC)
#define TDMA_ARRAYBASE0_L       (TDMA_ENGINE_BASE_ADDR + 0x70)
#define TDMA_ARRAYBASE1_L       (TDMA_ENGINE_BASE_ADDR + 0x74)
#define TDMA_ARRAYBASE2_L       (TDMA_ENGINE_BASE_ADDR + 0x78)
#define TDMA_ARRAYBASE3_L       (TDMA_ENGINE_BASE_ADDR + 0x7C)
#define TDMA_ARRAYBASE4_L       (TDMA_ENGINE_BASE_ADDR + 0x80)
#define TDMA_ARRAYBASE5_L       (TDMA_ENGINE_BASE_ADDR + 0x84)
#define TDMA_ARRAYBASE6_L       (TDMA_ENGINE_BASE_ADDR + 0x88)
#define TDMA_ARRAYBASE7_L       (TDMA_ENGINE_BASE_ADDR + 0x8C)
#define TDMA_ARRAYBASE0_H       (TDMA_ENGINE_BASE_ADDR + 0x90)
#define TDMA_ARRAYBASE1_H       (TDMA_ENGINE_BASE_ADDR + 0x94)
#define TDMA_DEBUG_MODE  				(TDMA_ENGINE_BASE_ADDR + 0xA0)



#define TDMA_CTRL_ENABLE_BIT				0
#define TDMA_CTRL_MODESEL_BIT				1
#define TDMA_CTRL_RESET_SYNCID_BIT	2
#define TDMA_CTRL_FORCE_1ARRAY			5
#define TDMA_CTRL_FORCE_2ARRAY			6
#define TDMA_CTRL_BURSTLEN_BIT			8
#define TDMA_CTRL_64BYTE_ALIGN_EN		10
#define TDMA_CTRL_DESNUM_BIT				16




//This function only supports the following condition
//localmem2tensor or tensor2localmem
//The source and dst shares the the same format
//Data is 32 bit
//no stride
//We use it in the forward_cpu backward_cpu
static inline int get_index_data_format(int size)
{
  if (size == 1) {
    return GDMA_TYPE_i1;
  } else if (size <= 16) {
    return GDMA_TYPE_i4;
  } else if (size <= 256){
    return GDMA_TYPE_i8;
  } else {
	return GDMA_TYPE_i16;
  }
}
#define LRN_LEFT_SHIFT  0
#define LRN_RIGHT_SHIFT 1

#endif /* REG_GDMA_H */

