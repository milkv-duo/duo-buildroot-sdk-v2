#ifndef __BM_KERNEL_H__
#define __BM_KERNEL_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <cvikernel/cvikernel.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#ifdef __cplusplus
extern "C" {
#endif

typedef int bmerr_t;
#define BM_SUCCESS 0               // The operation was successful
#define BM_ERR_AGAIN 1             // Not ready yet
#define BM_ERR_FAILURE 2           // General failure
#define BM_ERR_TIMEOUT 3           // Timeout
#define BM_ERR_UNINITIALIZED 4     // Uninitialzed
#define BM_ERR_INVALID_ARGUMENT 5  // Arguments invalid
#define BM_ERR_NOMEM 6             // Not enough memory
#define BM_ERR_DATA 7              // Data error
#define BM_ERR_BUSY 8              // Busy
#define BM_ERR_NOT_SUPPORTED 9     // Not supported yet

#define CVI_TPU_TIU        0  // Tensor Instruction Unit
#define CVI_TPU_CPU        1  // CPU, Reserved for common cpu op
#define CVI_TPU_TDMA       2  // TPU DMA
#define CVI_TPU_ENGINE_NUM 3  // Number of Engines

typedef cvk_fmt_t fmt_t;
#define FMT_F32     CVK_FMT_F32
#define FMT_F16     CVK_FMT_F16
#define FMT_I32     CVK_FMT_I32
#define FMT_I16     CVK_FMT_I16
#define FMT_I8      CVK_FMT_I8
#define FMT_I4      CVK_FMT_I4
#define FMT_I2      CVK_FMT_I2
#define FMT_I1      CVK_FMT_I1
#define FMT_U32     CVK_FMT_U32
#define FMT_U16     CVK_FMT_U16
#define FMT_U8      CVK_FMT_U8
#define FMT_BF16    CVK_FMT_BF16
#define FMT_INVALID CVK_FMT_INVALID

typedef enum _Cmdbuf_Head_Magic {
  CMDBUF_HDR_MAGIC_1880v2   = 0xA5,
  CMDBUF_HDR_MAGIC_1822     = 0xA6,
  CMDBUF_HDR_MAGIC_181X     = 0xA7,
  CMDBUF_HDR_MAGIC_180X     = 0xA8,
} Cmdbuf_Head_Magic;

#define BM_CMB_HDR_FLAG_NEURON    (0x1)
#define BM_CMB_HDR_FLAG_WEIGHT    (0x2)

typedef struct __cmd_hdr_s {
  uint8_t magic;              // 0xA5
  uint8_t len;                // lens in bytes
  uint8_t engine_id: 4;       // TPU, GDMA, CDMA
  uint8_t __deprecated: 4;
  uint8_t flags;              // CMD_ID, sync flags, etc. TBD
  uint32_t mask;              // bit mask for which register need to write
  uint8_t cmd[0];
} __attribute__((packed)) cmd_hdr_t;

typedef struct {
  uint32_t chip_version;
  uint32_t cmdbuf_size;
  uint8_t *cmdbuf;
} bmk_info_t;

cvk_chip_info_t bmk1880v2_chip_info(void);
cvk_chip_info_t bmk1822_chip_info(void);

static inline int ceiling_func(int numerator, int denominator)
{
  return (numerator + denominator - 1) / denominator;
}

static inline int ceiling_func_shift(int numerator, int shift)
{
  return (numerator + (1 << shift) - 1) >> shift;
}

static inline uint64_t align_up(uint64_t x, uint64_t n)
{
  return (x + n - 1) / n * n;
}

// len max number is 255, sometimes cmd larger than 255
static inline uint32_t cmd_hdr_len(cmd_hdr_t * hdr) {
  if (hdr->len == 0) {
    return hdr->mask;
  }
  return hdr->len;
}

#ifdef __cplusplus
}
#endif

#endif /* __BM_KERNEL_H__ */
