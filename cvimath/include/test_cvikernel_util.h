#ifndef CVIMATH_TEST_UTIL_H
#define CVIMATH_TEST_UTIL_H

#include <cviruntime_context.h>
#include "cvikernel/cvikernel.h"

#include "bmruntime.h"
#include "bmruntime_bmkernel.h"

#include <assert.h>
#include <math.h>    // pow
#include <stdint.h>  // uint8_t / uint16_t
#include <stdio.h>   /* printf, scanf, NULL */
#include <stdlib.h>  /* malloc, free, rand */
#include <string.h>  // strncpy

// copy from lagency
// TODO: move to properly header files
#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a) __ALIGN_MASK(x, (__typeof__(x))(a)-1)
typedef uint32_t laddr_t;
typedef uint64_t gaddr_t;
typedef uint32_t ctrl_t;
#define CTRL_NULL 0
#define CTRL_AL (1 << 0)       // alloc aligned with EU_NUM
#define CTRL_TP (1 << 5)       // transpose
#define CTRL_NEURON (1 << 11)  // mark neuron address in GDMA

#define LADDR_INVALID (0xFFFFFFFF)
#define GADDR_INVALID (0x000000FFFFFFFFFFULL)
static inline int ceiling_func(int numerator, int denominator) {
  return (numerator + denominator - 1) / denominator;
}
static inline int ceiling_func_shift(int numerator, int shift) {
  return (numerator + (1 << shift) - 1) >> shift;
}
static inline int get_num_shift(uint64_t num) {
  int n = 0;
  while (!(num & 1)) {
    n++;
    num >>= 1;
  }
  return n;
}

#ifdef __cplusplus
extern "C" {
#endif

/*
 * bm runtime binds with bm kernel.
 * cvi kernel still needs bm runtime.
 *
 * Need to create the separate function to combine bm runtime and cvi kernel.
 * Function with postfix _comp (compatible) for such combination.
 */

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/**
 * @brief submit command buffer
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 */
void test_submit_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx);

/**
 * @brief alloc tensor from device memory
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 * @param shape tensor shape
 * @param fmt tensor format such as \CVK_FMT_U16 or \CVK_FMT_U8
 *
 * @return cvk_tg_t structure
 */
cvk_tg_t *test_alloc_tg_mem_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx,
                                 cvk_tg_shape_t shape, cvk_fmt_t fmt);

/**
 * @brief alloc matrix from device memory
 *
 * @param rt_ctx runtime structure
 * @param shape matrix shape
 * @param fmt tensor format such as \CVK_FMT_U16 or \CVK_FMT_U8
 *
 * @return cvk_mg_t structure
 */
cvk_mg_t *test_alloc_mg_mem_comp(CVI_RT_HANDLE *rt_ctx, cvk_mg_shape_t shape, cvk_fmt_t fmt);

/**
 * @brief free tensor from device memory
 *
 * @param rt_ctx runtime structure
 * @param tg pointer of tg
 */
void test_free_tg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_tg_t *tg);

/**
 * @brief free matrix from device memory
 *
 * @param rt_ctx runtime structure
 * @param mg pointer of mg
 */
void test_free_mg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_mg_t *mg);

/**
 * @brief put host data to alloced tensor device memory
 *
 * @param rt_ctx runtime structure
 * @param tg pointer of tg
 * @param data[] host data
 */
void test_put_tg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_tg_t *tg, uint8_t data[]);

/**
 * @brief put host data to alloced matrix device memory
 *
 * @param rt_ctx runtime structure
 * @param mg pointer of mg
 * @param data[] host data
 */
void test_put_mg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_mg_t *mg, uint8_t data[]);

/**
 * @brief syntactic sugar for \test_alloc_mg_mem_comp -> \test_put_mg_mem_comp
 *
 * @param rt_ctx runtime structure
 * @param mg_data_format mg format such as \CVK_FMT_U16 or \CVK_FMT_U8
 * @param data[] host data
 *
 * @return
 */
cvk_mg_t *test_put_matrix_g(CVI_RT_HANDLE *rt_ctx, const cvk_mg_shape_t shape,
                            cvk_fmt_t mg_data_format, uint8_t data[]);

/**
 * @brief get tensor data from device memory
 *
 * @param rt_ctx runtime structure
 * @param tg pointer of tg
 *
 * @return data in device memory
 */
uint8_t *test_get_tg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_tg_t *tg);

/**
 * @brief get matrix data from device memory
 *
 * @param rt_ctx runtime structure
 * @param mg pointer of mg
 *
 * @return data in device memory
 */
uint8_t *test_get_mg_mem_comp(CVI_RT_HANDLE *rt_ctx, const cvk_mg_t *mg);

/**
 * @brief get tensor data from tpu memory,
 * the data path should be tpu memory -> device memory -> host memory
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 * @param tl pointer of tl
 *
 * @return data in tpu memory
 */
uint8_t *test_get_tensor_l2g_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx,
                                  const cvk_tl_t *tl);

/**
 * @brief get matrix data from tpu memory,
 * the data path should be tpu memory -> device memory -> host memory
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 * @param ml pointer of ml
 *
 * @return data in tpu memory
 */
uint8_t *test_get_matrix_l2g_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx,
                                  const cvk_ml_t *ml);

/**
 * @brief put host data to tpu memory with tensor
 * the data path should be host memory -> device memory -> tpu memory
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 * @param tl pointer of tl
 * @param data[] data in host memory
 */
void test_put_tensor_g2l_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, const cvk_tl_t *tl,

                              uint8_t data[]);

/**
 * @brief put host data to tpu memory with matrix
 * the data path should be host memory -> device memory -> tpu memory
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 * @param ml pointer of ml
 * @param data[] data in host memory
 */
void test_put_matrix_g2l_comp(CVI_RT_HANDLE *rt_ctx, cvk_context_t *cvk_ctx, const cvk_ml_t *ml,
                              uint8_t data[]);

/**
 * @brief alloc tensor from tpu memory
 *
 * @param cvk_ctx kernel structure
 * @param shape shape of tensor
 * @param fmt tensor format such as \CVK_FMT_U16 or \CVK_FMT_U8
 * @param eu_align is align excution unit
 *
 * @return pointer of tl
 */
cvk_tl_t *test_alloc_tl(cvk_context_t *cvk_ctx, cvk_tl_shape_t shape, cvk_fmt_t fmt, int eu_align);

/**
 * @brief free tpu  memory with tensor
 *
 * @param cvk_ctx kernel structure
 * @param tl pointer of tl
 */
void test_free_tl(cvk_context_t *cvk_ctx, const cvk_tl_t *tl);


/**
 * @brief a small structure for getting RT memory information
 */
typedef struct _AddrInfo
{
  uint64_t phy_addr;
  uint64_t size_bytes;
  uint8_t *vir_addr;
  int mem;
}AddrInfo;

/**
 * @brief get tpu global memory and assign info to an structure
 *
 * @param[in]  bm_ctx runtime structure
 * @param[out] pAddrInfo a structure for physical, virtual address
 */
uint8_t *test_get_vp_addr(bmctx_t *ctx, AddrInfo *pAddrInfo);

/**
 * @brief free tpu global memory from an info structure
 *
 * @param[in] bm_ctx runtime structure
 * @param[in] pAddrInfo a structure for physical, virtual address
 */
void test_free_vp_addr(bmctx_t *ctx,  AddrInfo *pAddrInfo);


/**
 * @breif wrapper function
 */
// tensor in local functions
// get tl size
static inline uint64_t tl_shape_size(const cvk_tl_shape_t *s) {
  return (uint64_t)s->n * s->c * s->h * s->w;
}

static inline uint64_t tg_shape_size(const cvk_tg_shape_t *s) {
  return (uint64_t)s->n * s->c * s->h * s->w;
}

static inline uint64_t mg_shape_size(const cvk_mg_shape_t *s) { return (uint64_t)s->row * s->col; }

static inline void free_tl(cvk_context_t *cvk_ctx, const cvk_tl_t *t) {
  return cvk_ctx->ops->lmem_free_tensor(cvk_ctx, t);
}

typedef struct {
  cvk_fmt_t src_fmt;
  cvk_fmt_t dst_fmt;
} cvk_fmt_type;

static inline int bitsize_of_fmt(cvk_fmt_t fmt) {
  switch (fmt) {
    case CVK_FMT_F32:
    case CVK_FMT_I32:
      return 32;
    case CVK_FMT_F16:
    case CVK_FMT_I16:
    case CVK_FMT_U16:
    case CVK_FMT_BF16:
      return 16;
    case CVK_FMT_I8:
    case CVK_FMT_U8:
      return 8;
    case CVK_FMT_I4:
      return 4;
    case CVK_FMT_I2:
      return 2;
    case CVK_FMT_I1:
      return 1;
    default:
      assert(0);
      return -1;
  }
}
static inline int bytesize_of_fmt(cvk_fmt_t fmt) { return bitsize_of_fmt(fmt) / 8; }
static inline void tg_2_tl_shape(cvk_tl_shape_t *tl, cvk_tg_shape_t *tg) {
  tl->n = tg->n;
  tl->c = tg->c;
  tl->h = tg->h;
  tl->w = tg->w;
}

static inline void tl_2_tg_shape(cvk_tg_shape_t *tg, cvk_tl_shape_t *tl) {
  tg->n = tl->n;
  tg->c = tl->c;
  tg->h = tl->h;
  tg->w = tl->w;
}
/**
 * @brief init test case with runtime/kernel
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 */
// static inline void _test_init(CVI_RT_HANDLE ctx, cvk_context_t **cvk_ctx) {
//  CVI_RT_HANDLE _ctx = (CVI_RT_HANDLE)ctx;
//  int ret = CVI_RT_Init(&_ctx);
//  if (ret != CVI_SUCCESS) {
//    fprintf(stderr, "init failed, err %d\n", ret);
//    exit(-1);
//  }
//
//  int alloc_size = 0x10000;
//  *cvk_ctx = (cvk_context_t*) CVI_RT_RegisterKernel(_ctx, alloc_size);
//  printf("alloc command buffer %d bytes success\n", alloc_size);
//}
// static inline void _test_exit(CVI_RT_HANDLE ctx, cvk_context_t *cvk_ctx) {
//  CVI_RT_UnRegisterKernel(cvk_ctx);
//  CVI_RT_HANDLE _ctx = (CVI_RT_HANDLE)ctx;
//  CVI_RT_DeInit(_ctx);
//}

static inline void test_init(CVI_RT_HANDLE *ctx, cvk_context_t **cvk_ctx) {
  CVI_RT_HANDLE *_ctx = (CVI_RT_HANDLE *)ctx;
  int ret = CVI_RT_Init(_ctx);
  if (ret != CVI_SUCCESS) {
    fprintf(stderr, "init failed, err %d\n", ret);
    exit(-1);
  }

  int alloc_size = 0x100000;
  *cvk_ctx = (cvk_context_t *)CVI_RT_RegisterKernel(*_ctx, alloc_size);
  printf("alloc command buffer %d bytes success\n", alloc_size);
}

/**
 * @brief de-init with runtime/kernel
 *
 * @param rt_ctx runtime structure
 * @param cvk_ctx kernel structure
 */
static inline void test_exit(CVI_RT_HANDLE *ctx, cvk_context_t *cvk_ctx) {
  CVI_RT_UnRegisterKernel(cvk_ctx);
  CVI_RT_HANDLE *_ctx = (CVI_RT_HANDLE *)ctx;
  CVI_RT_DeInit(*_ctx);
}

// converter bf16<->int8
uint8_t convert_bf16_u8(uint16_t data);
int8_t convert_bf16_s8(uint16_t data);
uint16_t convert_int8_bf16(uint8_t data, uint8_t sign);
uint32_t convert_fp32_u32(float fp32);
float convert_hex_fp32(uint32_t hval);
uint32_t convert_fp32_hex(float val);
float convert_bf16_fp32(uint16_t bf16);
uint16_t convert_fp32_bf16(float fp32);
int set_store_feround();
void restore_feround(int round_mode);

static inline void *xmalloc(size_t size) {
  void *p = malloc(size);
  if (!p) {
    return NULL;
  }
  return p;
}

#ifdef __cplusplus
}
#endif

#endif  // CVIMATH_TEST_UTIL_H
