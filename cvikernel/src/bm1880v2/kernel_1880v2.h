#ifndef KERNEL_1880v2_H
#define KERNEL_1880v2_H

#include "kernel_internal.h"

#include <bmkernel/bm1880v2/bmkernel_1880v2.h>
#include <bmkernel/bm1880v2/bm1880v2_tiu_reg.h>
#include <bmkernel/bm1880v2/bm1880v2_tdma_reg.h>
#include <bmkernel/bm1880v2/bm1880v2_tpu_cfg.h>
#include <bmkernel/reg_tiu.h>
#include <bmkernel/reg_bdcast.h>
#include <bmkernel/reg_tdma.h>
#include "bmkernel_standard.h"

#include <cvikernel/cvikernel.h>

#define TENSOR_MUL_FIX8B 0
#define TENSOR_MAC_FIX8B 1
#define TENSOR_ADD_FIX8B 2
#define TENSOR_SUB_FIX8B 3
#define TENSOR_MAX_FIX8B 4
#define TENSOR_MIN_FIX8B 5
#define TENSOR_SHIFT_FIX8B 6
#define TENSOR_AND_FIX8B 7
#define TENSOR_OR_FIX8B 8
#define TENSOR_XOR_FIX8B 9
#define TENSOR_COPY_FIX8B 10

typedef bmk1880v2_tensor_lmem_shape_t tl_shape_t;
typedef bmk1880v2_matrix_lmem_shape_t ml_shape_t;
typedef bmk1880v2_tensor_tgmem_shape_t tg_shape_t;
typedef bmk1880v2_matrix_tgmem_shape_t mg_shape_t;

typedef bmk1880v2_tensor_lmem_stride_t tl_stride_t;

typedef bmk1880v2_tensor_lmem_t tl_t;
typedef bmk1880v2_matrix_lmem_t ml_t;
typedef bmk1880v2_tensor_tgmem_t tg_t;
typedef bmk1880v2_matrix_tgmem_t mg_t;
typedef bmk1880v2_compressed_tensor_tgmem_t compressed_tg_t;
typedef bmk1880v2_compressed_matrix_tgmem_t compressed_mg_t;

desc_pair_t * bm1880v2_get_desc_pair(ctx_t *k, uint8_t eng_id);

static inline void assert_same_stride(const tl_t *a, const tl_t *b)
{
  ASSERT(a->stride.n == b->stride.n);
  ASSERT(a->stride.c == b->stride.c);
  ASSERT(a->stride.h == b->stride.h);
  ASSERT(a->stride.w == b->stride.w);
}

static inline void assert_same_shape(const tl_t *a, const tl_t *b)
{
  ASSERT(a->shape.n == b->shape.n);
  ASSERT(a->shape.c == b->shape.c);
  ASSERT(a->shape.h == b->shape.h);
  ASSERT(a->shape.w == b->shape.w);
}

static inline void assert_same_shape_3(
    const tl_t *a,
    const tl_t *b,
    const tl_t *c)
{
  assert_same_shape(a, b);
  assert_same_shape(a, c);
}

static inline void assert_same_shape_4(
    const tl_t *a,
    const tl_t *b,
    const tl_t *c,
    const tl_t *d)
{
  assert_same_shape_3(a, b, c);
  assert_same_shape(a, d);
}

static inline void assert_same_shape_5(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2,
    const tl_t *t3,
    const tl_t *t4)
{
  assert_same_shape_3(t0, t1, t2);
  assert_same_shape_3(t0, t3, t4);
}

static inline void assert_same_shape_6(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2,
    const tl_t *t3,
    const tl_t *t4,
    const tl_t *t5)
{
  assert_same_shape_5(t0, t1, t2, t3, t4);
  assert_same_shape(t0, t5);
}

static inline void assert_tiu_tensor_shape(const tl_t *t)
{
  ASSERT(t->shape.n > 0);
  ASSERT(t->shape.c > 0);
  ASSERT(t->shape.h > 0);
  ASSERT(t->shape.w > 0);

  ASSERT(t->shape.n < 0x1000);
  ASSERT(t->shape.c < 0x1000);
  ASSERT(t->shape.h <= (4095-32)); // 12bit, max 4095-32(lanes)
  ASSERT(t->shape.w <= (4095-32)); // 12bit, max 4095-32(lanes)
}

static inline void check_tiu_tensor(const tl_t *t)
{
  ASSERT(t);
  assert_tiu_tensor_shape(t);
  ASSERT(t->fmt == FMT_I8 || t->fmt == FMT_U8 || t->fmt == FMT_BF16);
}

static inline void check_tiu_tensor_2(
    const tl_t *t0,
    const tl_t *t1)
{
  check_tiu_tensor(t0);
  check_tiu_tensor(t1);
}

static inline void check_tiu_tensor_3(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2)
{
  check_tiu_tensor(t0);
  check_tiu_tensor_2(t1, t2);
}

static inline void check_tiu_tensor_4(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2,
    const tl_t *t3)
{
  check_tiu_tensor_3(t0, t1, t2);
  check_tiu_tensor(t3);
}

static inline void check_tiu_tensor_5(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2,
    const tl_t *t3,
    const tl_t *t4)
{
  check_tiu_tensor_3(t0, t1, t2);
  check_tiu_tensor_2(t3, t4);
}

static inline void check_tiu_tensor_6(
    const tl_t *t0,
    const tl_t *t1,
    const tl_t *t2,
    const tl_t *t3,
    const tl_t *t4,
    const tl_t *t5)
{
  check_tiu_tensor_3(t0, t1, t2);
  check_tiu_tensor_3(t3, t4, t5);
}

static inline void check_16bit_tiu_tensor(const tl_t *low, const tl_t *high)
{
  check_tiu_tensor_2(low, high);
  assert_same_shape(low, high);
  assert_same_stride(low, high);
  ASSERT(low->fmt == high->fmt);
  ASSERT(low->start_address < high->start_address);
}

static inline void assert_stride_type_0(ctx_t *ctx, const tl_t *t)
{
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t fmt = (t->fmt == FMT_BF16) ? 2 : 1;

  uint32_t h = t->shape.h;
  uint32_t w = t->shape.w * fmt;
  uint32_t c_stride = align_up(h * w, eu_num);

  ASSERT(t->stride.c == c_stride);
  ASSERT(t->stride.h == w);
  ASSERT(t->stride.w == fmt);
}

static inline void assert_bf16_stride_type_0(ctx_t *ctx, const tl_t *t)
{
  uint32_t eu_num = ctx->chip_info.eu_num;
  uint32_t fmt = (t->fmt == FMT_BF16) ? 2 : 1;

  ASSERT(t->stride.c % eu_num == 0);
  ASSERT(t->stride.w == fmt);
}


static inline void assert_stride_type_2(ctx_t *ctx, const tl_t *t)
{
  ASSERT(t->shape.h == 1);
  ASSERT(t->shape.w == 1);

  uint32_t fmt = (t->fmt == FMT_BF16) ? 2 : 1;
  uint32_t c = t->shape.c;
  uint32_t npu_num = ctx->chip_info.npu_num;

  ASSERT(t->stride.n == fmt * align_up(c, npu_num) / npu_num);
  ASSERT(t->stride.c == 1 * fmt);
  ASSERT(t->stride.h == 1 * fmt);
  ASSERT(t->stride.w == 1 * fmt);
}

static inline void assert_bf16_stride_type_2(ctx_t *ctx, const tl_t *t)
{
  ASSERT(t->shape.h == 1);
  ASSERT(t->shape.w == 1);

  uint32_t fmt = (t->fmt == FMT_BF16) ? 2 : 1;
  uint32_t c = t->shape.c;
  uint32_t npu_num = ctx->chip_info.npu_num;

  ASSERT(t->stride.n == fmt * align_up(c, npu_num) / npu_num);
  ASSERT(t->stride.c == 1 * fmt);
  ASSERT(t->stride.h == 1 * fmt);
  ASSERT(t->stride.w == 1 * fmt);
}

static inline int tensor_is_signed(const tl_t *t)
{
  switch (t->fmt) {
    case FMT_I8:
      return 1;
    case FMT_U8:
    case FMT_BF16: //does not matter, so set to default 0
      return 0;
    default:
      ASSERT(0);
  }
}

static inline int matrix_is_signed(const ml_t *t)
{
  switch (t->fmt) {
    case FMT_I8:
      return 1;
    case FMT_U8:
    case FMT_BF16: //does not matter, so set to default 0
      return 0;
    default:
      ASSERT(0);
  }
}

static inline void fill_same_tensor_shape(tiu_reg_t *r, tl_shape_t s)
{
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = s.w;

  r->opd0_n = n;
  r->opd0_c = c;
  r->opd0_h = h;
  r->opd0_w = w;

  r->opd1_n = n;
  r->opd1_c = c;
  r->opd1_h = h;
  r->opd1_w = w;

  r->opd2_n = n;
  r->opd2_c = c;
  r->opd2_h = h;
  r->opd2_w = w;

  r->res0_n = n;
  r->res0_c = c;
  r->res0_h = h;
  r->res0_w = w;
}

static inline void assert_stride_range(tl_stride_t s)
{
  ASSERT(s.n < 0x10000);
  ASSERT(s.c < 0x10000);
  ASSERT(s.h < 0x10000);
}

static inline void fill_same_tensor_stride(tiu_reg_t *r, tl_stride_t s)
{
  uint32_t n = s.n;
  uint32_t c = s.c;
  uint32_t h = s.h;
  uint32_t w = 1;

  r->opd0_n_str = n;
  r->opd0_c_str = c;
  r->opd0_h_str = h;
  r->opd0_w_str = w;

  r->opd1_n_str = n;
  r->opd1_c_str = c;
  r->opd1_h_str = h;
  r->opd1_w_str = w;

  r->opd2_n_str = n;
  r->opd2_c_str = c;
  r->opd2_h_str = h;
  r->opd2_w_str = w;

  r->res0_n_str = n;
  r->res0_c_str = c;
  r->res0_h_str = h;
  r->res0_w_str = w;
}

#define fill_stride_code(r, op, str)            \
  do {                                          \
    r->op##_n_str = str->n;                     \
    r->op##_c_str = str->c;                     \
    r->op##_h_str = str->h;                     \
    r->op##_w_str = str->w;                     \
  } while (0)

static inline void fill_opd0_stride(tiu_reg_t *r, const tl_stride_t *str)
{
  fill_stride_code(r, opd0, str);
}

static inline void fill_opd1_stride(tiu_reg_t *r, const tl_stride_t *str)
{
  fill_stride_code(r, opd1, str);
}

static inline void fill_opd2_stride(tiu_reg_t *r, const tl_stride_t *str)
{
  fill_stride_code(r, opd2, str);
}

static inline void fill_res0_stride(tiu_reg_t *r, const tl_stride_t *str)
{
  fill_stride_code(r, res0, str);
}

static inline void fill_same_tensor_stride_type(tiu_reg_t *r, int type)
{
  r->short_opd0_str = type & 0b11;
  r->short_opd1_str = type & 0b11;
  r->short_opd2_str = type & 0b11;
  r->short_res0_str = type & 0b11;
}

static inline ec_desc_t * emit_tiu_cmdbuf(ctx_t *k, tiu_reg_t *r)
{
  int engine_id = BMK1880v2_TIU;

  desc_pair_t *dp = bm1880v2_get_desc_pair(k, engine_id);
  uint32_t *cmdbuf = (uint32_t *)dp->cmd_hdr->cmd;
  emit_tiu_reg(r, cmdbuf);

  return dp->ec_desc;
}

#endif /* KERNEL_1880v2_H */
