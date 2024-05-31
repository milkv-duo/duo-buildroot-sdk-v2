#ifndef GEN_LUT_1880v2_H
#define GEN_LUT_1880v2_H

#include "../kernel_1880v2.h"

#include <assert.h>

#define IN
#define OUT
static inline int bf16_exp_start()
{
  return -62;
}
static inline int bf16_exp_end()
{
  return 63;
}
static inline int bf16_table_h()
{
  return 32;
}
static inline int bf16_table_w()
{
  return 8;
}
static inline int bf16_table_hw()
{
  return bf16_table_h() * bf16_table_w();
}
static inline int half_h_table()
{
  return bf16_table_h() * bf16_table_w() / 2;
}
static inline uint8_t is_1880v2_tbl_shape(bmk1880v2_tensor_lmem_shape_t *s)
{
  // FIXME: h could be reduce less than 32
  assert(s->h == (uint32_t)bf16_table_h() && s->w == (uint32_t)bf16_table_w() &&
         "table h/w should be 32/8");

  return s->h == (uint32_t)bf16_table_h() && s->w == (uint32_t)bf16_table_w();
}

// <! end copy from bmkernel/src/kernel_internal.h
static inline int bytesize_of_fmt(fmt_t fmt)
{
  return bitsize_of_fmt(fmt) / 8;
}

// duplicate from 1880v2_test_util.h
static inline uint64_t tl_shape_size(const bmk1880v2_tensor_lmem_shape_t *s)
{
  return (uint64_t)s->n * s->c * s->h * s->w;
}

// copy bmk1880v2_tensor_lmem_t structure
static inline void bmk1880v2_tensor_lmem_s_copy(bmk1880v2_tensor_lmem_t *dst,
                                                bmk1880v2_tensor_lmem_t *src)
{

  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  dst->shape = src->shape;
  dst->stride = src->stride;
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static inline void
bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx_t *ctx, bmk1880v2_tensor_lmem_t *dst,
                                    bmk1880v2_tensor_lmem_t *src, fmt_t fmt)
{
  assert(src->fmt == FMT_BF16 && (fmt == FMT_I8 || fmt == FMT_U8) &&
         "only support bf16->i8/uint8_t, plz check fmt\n");

  dst->start_address = src->start_address;
  dst->fmt = fmt;
  dst->shape = src->shape;
  dst->shape.w *= 2;
  dst->stride = bmk1880v2_tensor_lmem_default_stride(ctx, dst->shape,
                                                     fmt, CTRL_NULL);
  // dst->shape.h *= 2;
  // dst->stride = bmk1880v2_tensor_lmem_default_stride(ctx, dst->shape,
  //                                                        /*eu_align*/ 1,
  //                                                        fmt);
  // dst->shape.h = src->shape.h;
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

// l2l means we keep the same shape between bf16/(u)int8
static inline void
bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx_t *ctx,
                                        bmk1880v2_tensor_lmem_t *dst,
                                        bmk1880v2_tensor_lmem_t *src, fmt_t fmt)
{
  assert(src->fmt == FMT_BF16 && (fmt == FMT_I8 || fmt == FMT_U8) &&
         "only support bf16->i8/uint8_t, plz check fmt\n");

  dst->start_address = src->start_address;
  dst->fmt = fmt;
  dst->shape = src->shape;
  dst->stride = bmk1880v2_tensor_lmem_default_stride(ctx, dst->shape,
                                                     fmt, CTRL_NULL);
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

int bf16_emit_square(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                     bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16, fmt_t fmt);

void bf16_table_check(bmk1880v2_tensor_lmem_t *IN tl_ifmap,
                      bmk1880v2_tensor_lmem_t *tbl_answer,
                      bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
                      bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16);

int bf16_lut_exp_mantissa(ctx_t *ctx, bmk1880v2_tensor_lmem_t *IN tl_ifmap,
                          bmk1880v2_tensor_lmem_t *IN tl_buf,
                          bmk1880v2_tensor_lmem_t *tbl_answer,
                          bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
                          bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16);

void bf16_get_u8_tbl_idx(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                         bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16);

void bf16_get_dec(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                  bmk1880v2_tensor_lmem_t *tl_buf,
                  bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16);

void bf16_get_dec_fractions(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                            bmk1880v2_tensor_lmem_t *OUT buf,
                            bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16);

int bf16_emit_abs(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                  bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16, fmt_t fmt);

int _bf16_lut_exp_mantissa(ctx_t *ctx, bmk1880v2_tensor_lmem_t *IN tl_ifmap,
                           bmk1880v2_tensor_lmem_t *IN tl_buf,
                           bmk1880v2_tensor_lmem_t *tbl_answer,
                           bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
                           bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16,
                           uint8_t is_dirty_ifmap);

int _bf16_atan_fast_emit(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                         bmk1880v2_tensor_lmem_t *tl_buf,
                         bmk1880v2_tensor_lmem_t *tl_buf2,
                         bmk1880v2_tensor_lmem_t *tl_y0_buf,
                         bmk1880v2_tensor_lmem_t *tl_invert_buf,
                         bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                         bmk1880v2_tensor_lmem_t *tl_table_answer,
                         bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                         bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16, fmt_t fmt,
                         float b, uint8_t is_dirty_ifmap);

int bf16_emit_x_over_y(ctx_t *ctx, bmk1880v2_tensor_lmem_t *IN x,
                       bmk1880v2_tensor_lmem_t *IN y,
                       bmk1880v2_tensor_lmem_t *IN tl_buf,
                       bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16,
                       bmk1880v2_tensor_lmem_t *tl_table_answer,
                       bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                       fmt_t fmt, uint8_t is_dirty_ifmap);

int _bf16_emit_mask(ctx_t *ctx, bmk1880v2_tensor_lmem_t *IN tl_ifmap,
                    bmk1880v2_tensor_lmem_t *tl_buf,
                    bmk1880v2_tensor_lmem_t *tl_buf2,
                    bmk1880v2_tensor_lmem_t *tl_buf3,
                    bmk1880v2_tensor_lmem_t *tl_pos_neg_table,
                    bmk1880v2_tensor_lmem_t *tl_0_idx_table,
                    bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16, fmt_t fmt,
                    enum BF16_MASK_TYPE mask, uint8_t is_dirty_ifmap);

void _bf16_get_tbl_idx(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16,
                       fmt_t src_fmt, int int8_rnd_mode);
int __bf16_atan_fast_emit(ctx_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                          bmk1880v2_tensor_lmem_t *tl_buf,
                          bmk1880v2_tensor_lmem_t *tl_buf2,
                          bmk1880v2_tensor_lmem_t *tl_y0_buf,
                          bmk1880v2_tensor_lmem_t *tl_invert_buf,
                          bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                          bmk1880v2_tensor_lmem_t *tl_table_answer,
                          bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                          bmk1880v2_tensor_lmem_t *OUT tl_ofmap_bf16,
                          fmt_t fmt);

#endif /* GEN_LUT_1880v2_H */
