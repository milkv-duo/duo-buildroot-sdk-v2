#ifndef __BM_KERNEL_LEGACY_H__
#define __BM_KERNEL_LEGACY_H__

#include <bmkernel/bm_kernel.h>

typedef uint32_t laddr_t;
typedef uint64_t gaddr_t;

#ifdef __cplusplus
extern "C" {
#endif

#define LADDR_INVALID          (0xFFFFFFFF)
#define GADDR_INVALID          (0x000000FFFFFFFFFFULL)

#define FMT_U8_to_F32 0xFF

#define ENGINE_BD    0     // Broadcast Engine
#define ENGINE_CPU   1     // CPU, Reserved
#define ENGINE_GDMA  2     // GDMA Engine
#define ENGINE_CDMA  3     // CDMA Engine
#define ENGINE_END   4     // Invalid

typedef struct __dma_hdr_t {
  uint16_t dmabuf_magic_m;
  uint16_t dmabuf_magic_s;
  uint32_t dmabuf_size;
  uint32_t cpu_desc_count;
  uint32_t bd_desc_count; //16bytes
  uint32_t tdma_desc_count;
  uint32_t tpu_clk_rate;
  uint32_t pmubuf_size;
  uint32_t pmubuf_offset; //32bytes
  uint32_t arraybase_0_L;
  uint32_t arraybase_0_H;
  uint32_t arraybase_1_L;
  uint32_t arraybase_1_H; //48bytes
  uint32_t arraybase_2_L;
  uint32_t arraybase_2_H;
  uint32_t arraybase_3_L;
  uint32_t arraybase_3_H; //64bytes

  uint32_t arraybase_4_L;
  uint32_t arraybase_4_H;
  uint32_t arraybase_5_L;
  uint32_t arraybase_5_H;
  uint32_t arraybase_6_L;
  uint32_t arraybase_6_H;
  uint32_t arraybase_7_L;
  uint32_t arraybase_7_H;
  uint32_t reserve[8];   //128bytes, 128bytes align
} dma_hdr_t;

typedef struct {
  uint32_t version;
  uint32_t npu_num;
  uint32_t eu_num;
  uint32_t lmem_size;
  uint32_t lmem_banks;
  uint32_t lmem_bank_size;
} bmk_chip_info_t;

#define FLOAT_SIZE 4
#define INT8_SIZE 1
#define BF16_SIZE 2

#define UNUSED(x)               (void)(x)

#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a)              __ALIGN_MASK(x,(__typeof__(x))(a)-1)
#define ALIGN_DOWN(x, a)        ((x) / (a) * (a))

#define math_min(x, y)          ((x) < (y) ? (x) : (y))
#define math_max(x, y)          ((x) > (y) ? (x) : (y))

static inline int get_num_shift(uint64_t num)
{
  int n = 0;
  while (!(num & 1)) {
    n++;
    num >>= 1;
  }
  return n;
}

typedef struct {
  uint32_t dim;
  uint32_t n;
  uint32_t c;
  union {
    uint32_t h;
    uint32_t row;
  };
  union {
    uint32_t w;
    uint32_t col;
  };
} shape_t;

shape_t shape_t4(int n, int c, int h, int w);
shape_t shape_t3(int d3, int d2, int d1);
shape_t shape_t2(int row, int col);
shape_t shape_t1(int len);

uint8_t shape_equal(shape_t s1, shape_t s2);

typedef struct {
  uint32_t n;
  uint32_t c;
  union {
    uint32_t h;
    uint32_t row;
  };
  union {
    uint32_t w;
    uint32_t col;
  };
} stride_t;

static inline stride_t stride_st4(int n, int c, int h, int w)
{
  stride_t st;
  st.n = n;
  st.c = c;
  st.h = h;
  st.w = w;
  return st;
}

typedef uint32_t ctrl_t;
#define CTRL_NULL                   0
#define CTRL_AL                     (1 << 0) // alloc aligned with EU_NUM
#define CTRL_RA                     (1 << 2)  // result add
#define CTRL_BN                     (1 << 3)  // B_N_is_1 broadcast to A
#define CTRL_TP                     (1 << 5)  // transpose
#define CTRL_ADDR_ALIGN             (1 << 7)
#define CTRL_RELU                   (1 << 8)
#define CTRL_KFLIP                  (1 << 9)  // kernel flip
#define CTRL_WEIGHT                 (1 << 10) // mark weight address in GDMA
#define CTRL_NEURON                 (1 << 11) // mark neuron address in GDMA
#define CTRL_WINOGRAD               (1 << 12) // GDMA reshap winograd kernel
#define CTRL_WINOGRAD_SCALE_FACTOR  (1 << 28) // GDMA reshap winograd kernel

typedef uint32_t tl_type;
#define TL_TYPE_TENSOR 0
#define TL_TYPE_TENSOR_PREALLOC 1
#define TL_TYPE_CONSTANT 2
#define TL_TYPE_SLICE 3
#define TL_TYPE_CLONE 4

typedef union {
  uint32_t reg_val;
  float fp32_val;
} const_fp32_t;

typedef union {
  laddr_t laddr;
  const_fp32_t const_fp32;
} opd_t;

typedef struct {
  tl_type type;
  opd_t operand;
  shape_t shape;
  stride_t *stride;
  uint8_t aligned;
  fmt_t fmt;
  uint32_t bank_id;
  int reserved_size;
} tensor_lmem;

static inline laddr_t tl_address(tensor_lmem *tlp)
{
  if (tlp->type == TL_TYPE_CONSTANT) {
    return LADDR_INVALID;
  }
  return tlp->operand.laddr;
}

void tl_reshape(tensor_lmem * tlp, shape_t shape);

typedef struct {
  uint64_t addr;
  shape_t shape;
  stride_t stride;
} tensor_gmem;

static inline int tg_is_matrix(tensor_gmem *t)
{
  return t->shape.dim == 2;
}

static inline int tg_matrix_row(tensor_gmem *t)
{
  return t->shape.row;
}

static inline int tg_matrix_col(tensor_gmem *t)
{
  return t->shape.col;
}

static inline int tl_is_const(tensor_lmem *tlp)
{
  return tlp->type == TL_TYPE_CONSTANT;
}

static inline int tl_is_prealloc(tensor_lmem *tlp)
{
  return tlp->type == TL_TYPE_TENSOR_PREALLOC;
}

static inline int tl_is_matrix(tensor_lmem *tlp)
{
  int dim = tlp->shape.dim;
  return dim == 1 || dim == 2;
}

#ifdef __cplusplus
}
#endif

#endif /* __BM_KERNEL_LEGACY_H__ */
