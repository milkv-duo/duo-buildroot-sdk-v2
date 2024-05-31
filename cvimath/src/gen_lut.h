#ifndef GEN_LUT_1880v2_H
#define GEN_LUT_1880v2_H

#include <assert.h>
#include <cvimath_internal.h>
#include <test_cvikernel_util.h>

#define IN
#define OUT
#define ASSERT(x) assert(x)

static inline int cvm_exp_start() { return -62; }
static inline int cvm_exp_end() { return 63; }
static inline int cvm_table_h() { return 32; }
static inline int cvm_table_w() { return 8; }
static inline int cvm_table_hw() { return cvm_table_h() * cvm_table_w(); }
static inline int half_h_table() { return cvm_table_h() * cvm_table_w() / 2; }
static inline bool is_1880v2_tbl_shape(cvk_tl_shape_t *s) {
  // FIXME: h could be reduce less than 32
  assert(s->h == (uint32_t)cvm_table_h() && s->w == (uint32_t)cvm_table_w() &&
         "table h/w should be 32/8");

  return s->h == (uint32_t)cvm_table_h() && s->w == (uint32_t)cvm_table_w();
}

// copy cvk_tl_t structure
static inline void bmk1880v2_tensor_lmem_s_copy(cvk_tl_t *dst, cvk_tl_t *src) {
  dst->start_address = src->start_address;
  dst->fmt = src->fmt;
  dst->shape = src->shape;
  dst->stride = src->stride;
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

static inline void bmk1880v2_tensor_lmem_s_copy_bf16_8(cvk_context_t *ctx, cvk_tl_t *dst,
                                                       cvk_tl_t *src, cvk_fmt_t fmt) {
  assert(src->fmt == CVK_FMT_BF16 && (fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8) &&
         "only support bf16->i8/uint8_t, plz check fmt\n");

  dst->start_address = src->start_address;
  dst->fmt = fmt;
  dst->shape = src->shape;
  dst->shape.w *= 2;
  dst->stride = ctx->ops->tl_default_stride(ctx, dst->shape, fmt, CTRL_NULL);
  // dst->shape.h *= 2;
  // dst->stride = ctx->ops->tl_default_stride(ctx, dst->shape,
  //                                                        /*eu_align*/ 1,
  //                                                        fmt);
  // dst->shape.h = src->shape.h;
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

// l2l means we keep the same shape between bf16/(u)int8
static inline void bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(cvk_context_t *ctx, cvk_tl_t *dst,
                                                           cvk_tl_t *src, cvk_fmt_t fmt) {
  assert(src->fmt == CVK_FMT_BF16 && (fmt == CVK_FMT_I8 || fmt == CVK_FMT_U8) &&
         "only support bf16->i8/uint8_t, plz check fmt\n");

  dst->start_address = src->start_address;
  dst->fmt = fmt;
  dst->shape = src->shape;
  dst->stride = ctx->ops->tl_default_stride(ctx, dst->shape, fmt, CTRL_NULL);
  dst->int8_rnd_mode = src->int8_rnd_mode;
}

int cvm_emit_square(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *OUT tl_ofmap_bf16,
                    cvk_fmt_t fmt);

void cvm_table_check(cvk_tl_t *IN tl_ifmap, cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa,
                     cvk_tl_t *OUT tl_ofmap_bf16);

int cvm_lut_exp_mantissa(cvk_context_t *ctx, cvk_tl_t *IN tl_ifmap, cvk_tl_t *IN tl_buf,
                         cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa,
                         cvk_tl_t *OUT tl_ofmap_bf16);

void cvm_get_uint8_t_tbl_idx(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *OUT tl_ofmap_bf16);

void cvm_get_dec(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                 cvk_tl_t *OUT tl_ofmap_bf16);

void cvm_get_dec_fractions(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *OUT buf,
                           cvk_tl_t *OUT tl_ofmap_bf16);

int cvm_emit_abs(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *OUT tl_ofmap_bf16,
                 cvk_fmt_t fmt);

int _cvm_lut_exp_mantissa(cvk_context_t *ctx, cvk_tl_t *IN tl_ifmap, cvk_tl_t *IN tl_buf,
                          cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa,
                          cvk_tl_t *OUT tl_ofmap_bf16, bool is_dirty_ifmap);

int _cvm_atan_fast_emit(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                        cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf,
                        cvk_tl_t *tl_table_answer, cvk_tl_t *tl_table_answer_mantissa,
                        cvk_tl_t *OUT tl_ofmap_bf16, cvk_fmt_t fmt, float b, bool is_dirty_ifmap);

int cvm_emit_x_over_y(cvk_context_t *ctx, cvk_tl_t *IN x, cvk_tl_t *IN y, cvk_tl_t *IN tl_buf,
                      cvk_tl_t *OUT tl_ofmap_bf16, cvk_tl_t *tl_table_answer,
                      cvk_tl_t *tl_table_answer_mantissa, cvk_fmt_t fmt, bool is_dirty_ifmap);

int _cvm_emit_mask(cvk_context_t *ctx, cvk_tl_t *IN tl_ifmap, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                   cvk_tl_t *tl_buf3, cvk_tl_t *tl_pos_neg_table, cvk_tl_t *tl_0_idx_table,
                   cvk_tl_t *OUT tl_ofmap_bf16, cvk_fmt_t fmt, enum CVM_MASK_TYPE mask,
                   bool is_dirty_ifmap);

void _cvm_get_tbl_idx(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *OUT tl_ofmap_bf16,
                      cvk_fmt_t src_fmt, int int8_rnd_mode);
int __cvm_atan_fast_emit(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                         cvk_tl_t *tl_buf2, cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_invert_buf,
                         cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                         cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *OUT tl_ofmap_bf16,
                         cvk_fmt_t fmt);

// not need to export to user
// mask please refer \CVM_MASK_TYPE for supported case
int cvm_emit_mask_gt0(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                      cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_pos_neg_buf,
                      cvk_tl_t *tl_0_idx_buf, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_mask_ge0(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                      cvk_tl_t *tl_pos_neg_table, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_mask_le0(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                      cvk_tl_t *tl_pos_neg_table, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_mask_eq0(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                      cvk_tl_t *tl_0_idx_table, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_mask_lt0(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                      cvk_tl_t *tl_pos_neg_table, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int _cvm_atan_emit(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                   cvk_tl_t *tl_buf3, cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_slope_buf,
                   cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                   cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt,
                   float b);

void cvm_emit_mask_ge0_lt0(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *index_i8,
                           cvk_tl_t *tl_buf3, cvk_fmt_t fmt);

void cvm_emit_mask_eq_0(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *tl_buf, cvk_tl_t *index_i8,
                        cvk_tl_t *tl_buf3, cvk_fmt_t fmt);

int cvm_lut_exp_mantissa(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                         cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa,
                         cvk_tl_t *tl_ofmap_bf16);

int cvm_emit_pythagoras(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *x, cvk_tl_t *tl_buf,
                        cvk_tl_t *tl_buf2, cvk_tl_t *tl_sqrt_table_answer,
                        cvk_tl_t *tl_sqrt_table_answer_mantissa, cvk_tl_t *tl_ofmap_bf16,
                        cvk_fmt_t fmt);

int cvm_emit_max_const(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ofmap_bf16,
                       cvk_fmt_t fmt, float b);

int cvm_emit_min_const(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ofmap_bf16,
                       cvk_fmt_t fmt, float b);

int cvm_emit_0_1_revert(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                        cvk_tl_t *tbl_answer, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_mul(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                 cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_add(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                 cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_emit_add_const(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ofmap_bf16,
                       cvk_fmt_t fmt, float b);

int cvm_emit_mul_const(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ofmap_bf16,
                       cvk_fmt_t fmt, float b);
// not release yet

void cvm_atan2_emit(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *x, cvk_tl_t *tl_buf,
                    cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_buf4, cvk_tl_t *tl_buf5,
                    cvk_tl_t *tl_buf6, cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_slope_buf,
                    cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                    cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *tl_sqrt_table_answer,
                    cvk_tl_t *tl_sqrt_table_answer_mantissa, cvk_tl_t *tl_0_idx_table,
                    cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

int cvm_atan_slope_multipilier(cvk_context_t *cvk_ctx, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                               cvk_tl_t *tl_buf3, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ofmap_bf16,
                               cvk_fmt_t fmt);

int cvm_atan_fast_emit(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                       cvk_tl_t *tl_buf2, cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_invert_buf,
                       cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                       cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt,
                       bool is_dirty_ifmap);

void cvm_atan2_fast_emit(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *x, cvk_tl_t *tl_buf,
                         cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_buf4,
                         cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_slope_buf, cvk_tl_t *tl_invert_buf,
                         cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                         cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *tl_0_idx_table,
                         cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

// conv used
int cvm_reshape_channel_same_pad(
    cvk_context_t *cvk_ctx, int ic, int ih, int iw, int kh, int kw, int pad_right, int pad_left,
    int stride_h, int stride_w, cvk_tl_shape_t *tl_load_shape, cvk_tl_stride_t *new_tl_ifmap_stride,
    cvk_tg_shape_t *new_tg_ifmap_shape, cvk_tg_stride_t *new_tg_ifmap_stride,
    cvk_tl_shape_t *new_tl_weight_shape, cvk_tl_shape_t *new_tl_bias_shape,
    cvk_tl_shape_t *new_tl_ofmap_shape, cvk_fmt_t fmt, int eu_align);

#endif /* GEN_LUT_1880v2_H */
