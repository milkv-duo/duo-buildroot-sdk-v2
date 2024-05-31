#ifndef __BMKERNEL_1880v2_NON_ATOMIC_H__
#define __BMKERNEL_1880v2_NON_ATOMIC_H__

#include "bmkernel_1880v2.h"

#ifdef __cplusplus
extern "C" {
#endif

// non atomic
void bf16_table_shape(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_shape_t *s);

int bf16_emit_sqrt(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                   bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tbl_answer,
                   bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
                   bmk1880v2_tensor_lmem_t *tl_ofmap_bf16);
void bf16_gen_sqrt(uint16_t *table_data, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_gen_sqrt_mantissa(uint16_t *table_mantissa, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_sqrt_tbl(uint16_t *sqrt_table_data, uint16_t *sqrt_table_data_mantissa,
                   bmk1880v2_tensor_lmem_shape_t *table_shape);

int bf16_emit_reciprocal(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                         bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tbl_answer,
                         bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
                         bmk1880v2_tensor_lmem_t *tl_ofmap_bf16);
void bf16_gen_reciprocal(uint16_t *table_data, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_gen_reciprocal_mantissa(uint16_t *table_mantissa, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_reciprocal_tbl(uint16_t *table_data, uint16_t *table_mantissa,
                         bmk1880v2_tensor_lmem_shape_t *table_shape);

void bf16_atan_y0(uint16_t *table_data_y0, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_atan_fast_degree_y0(uint16_t *table_data_y0, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_atan_slope(uint16_t *table_slope, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_atan_s_01(uint16_t *table_invert, bmk1880v2_tensor_lmem_shape_t *table_shape);
void bf16_atan_pos_neg(uint16_t *table_pos_neg, bmk1880v2_tensor_lmem_shape_t *table_shape);
int bf16_atan_slope_multipilier(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_buf,
                                bmk1880v2_tensor_lmem_t *tl_buf2, bmk1880v2_tensor_lmem_t *tl_buf3,
                                bmk1880v2_tensor_lmem_t *tl_ifmap,
                                bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_atan_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                   bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_buf2,
                   bmk1880v2_tensor_lmem_t *tl_buf3, bmk1880v2_tensor_lmem_t *tl_y0_buf,
                   bmk1880v2_tensor_lmem_t *tl_slope_buf, bmk1880v2_tensor_lmem_t *tl_invert_buf,
                   bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                   bmk1880v2_tensor_lmem_t *tl_table_answer,
                   bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                   bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_atan_fast_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                        bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_buf2,
                        bmk1880v2_tensor_lmem_t *tl_y0_buf,
                        bmk1880v2_tensor_lmem_t *tl_invert_buf,
                        bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                        bmk1880v2_tensor_lmem_t *tl_table_answer,
                        bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                        bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt,
                        uint8_t is_dirty_ifmap);

void bf16_atan2_fast_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *y,
                     bmk1880v2_tensor_lmem_t *x, bmk1880v2_tensor_lmem_t *tl_buf,
                     bmk1880v2_tensor_lmem_t *tl_buf2, bmk1880v2_tensor_lmem_t *tl_buf3,
                     bmk1880v2_tensor_lmem_t *tl_buf4, bmk1880v2_tensor_lmem_t *tl_y0_buf,
                     bmk1880v2_tensor_lmem_t *tl_slope_buf, bmk1880v2_tensor_lmem_t *tl_invert_buf,
                     bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                     bmk1880v2_tensor_lmem_t *tl_table_answer,
                     bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                     bmk1880v2_tensor_lmem_t *tl_0_idx_table,
                     bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

void bf16_atan2_fast_degree_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *y,
                     bmk1880v2_tensor_lmem_t *x, bmk1880v2_tensor_lmem_t *tl_buf,
                     bmk1880v2_tensor_lmem_t *tl_buf2, bmk1880v2_tensor_lmem_t *tl_buf3,
                     bmk1880v2_tensor_lmem_t *tl_y0_buf,
                     bmk1880v2_tensor_lmem_t *tl_invert_buf,
                     bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                     bmk1880v2_tensor_lmem_t *tl_table_answer,
                     bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                     bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

void bf16_atan2_merge_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *y,
                     bmk1880v2_tensor_lmem_t *x, bmk1880v2_tensor_lmem_t *tl_buf,
                     bmk1880v2_tensor_lmem_t *tl_buf2, bmk1880v2_tensor_lmem_t *tl_buf3,
                     bmk1880v2_tensor_lmem_t *tl_y0_buf,
                     bmk1880v2_tensor_lmem_t *tl_invert_buf,
                     bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                     bmk1880v2_tensor_lmem_t *tl_table_answer,
                     bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                     bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);


uint64_t bf16_lut_tbl_bytesize(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_shape_t *table_shape,
                          fmt_t fmt);

void bf16_atan_tbl(uint16_t *table_data_atan_y0, uint16_t *table_data_atan_slope, uint16_t *table_data_atan_invert,
                   uint16_t *table_data_atan_pos_neg, bmk1880v2_tensor_lmem_shape_t *table_shape);

void bf16_atan_fast_degree_tbl(uint16_t *table_data_atan_y0, uint16_t *table_data_atan_invert,
                   uint16_t *table_data_atan_pos_neg, bmk1880v2_tensor_lmem_shape_t *table_shape);

void bf16_gen_0_tbl(uint16_t *table_0, bmk1880v2_tensor_lmem_shape_t *table_shape);

int bf16_emit_0_idx(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                    bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tbl_answer,
                    bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_neg_idx(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                      bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                      bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_pos_idx(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                      bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                      bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_0_1_revert_input(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                               bmk1880v2_tensor_lmem_t *tl_buf,
                               bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

void bf16_atan2_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *y,
                     bmk1880v2_tensor_lmem_t *x, bmk1880v2_tensor_lmem_t *tl_buf,
                     bmk1880v2_tensor_lmem_t *tl_buf2, bmk1880v2_tensor_lmem_t *tl_buf3,
                     bmk1880v2_tensor_lmem_t *tl_buf4, bmk1880v2_tensor_lmem_t *tl_buf5,
                     bmk1880v2_tensor_lmem_t *tl_buf6, bmk1880v2_tensor_lmem_t *tl_y0_buf,
                     bmk1880v2_tensor_lmem_t *tl_slope_buf, bmk1880v2_tensor_lmem_t *tl_invert_buf,
                     bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                     bmk1880v2_tensor_lmem_t *tl_table_answer,
                     bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                     bmk1880v2_tensor_lmem_t *tl_sqrt_table_answer,
                     bmk1880v2_tensor_lmem_t *tl_sqrt_table_answer_mantissa,
                     bmk1880v2_tensor_lmem_t *tl_0_idx_table,
                     bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

// nn function
int bf16_emit_pythagoras(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *y,
                         bmk1880v2_tensor_lmem_t *x, bmk1880v2_tensor_lmem_t *tl_buf,
                         bmk1880v2_tensor_lmem_t *tl_buf2,
                         bmk1880v2_tensor_lmem_t *tl_sqrt_table_answer,
                         bmk1880v2_tensor_lmem_t *tl_sqrt_table_answer_mantissa,
                         bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_max_const(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                        bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt, float b);

int bf16_emit_min_const(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                        bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt, float b);

int bf16_emit_0_1_revert(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                         bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tbl_answer,
                         bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_mul(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                  bmk1880v2_tensor_lmem_t *tl_ifmap2, bmk1880v2_tensor_lmem_t *tl_ofmap_bf16,
                  fmt_t fmt);

int bf16_emit_add(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                  bmk1880v2_tensor_lmem_t *tl_ifmap2, bmk1880v2_tensor_lmem_t *tl_ofmap_bf16,
                  fmt_t fmt);

int bf16_emit_add_const(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                        bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt, float b);

int bf16_emit_mul_const(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                        bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt, float b);

// mask please refer \BF16_MASK_TYPE for supported case
int bf16_emit_mask_gt0(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_buf2,
                       bmk1880v2_tensor_lmem_t *tl_buf3, bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                       bmk1880v2_tensor_lmem_t *tl_0_idx_buf,
                       bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_mask_ge0(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_pos_neg_table,
                       bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_mask_le0(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_pos_neg_table,
                       bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int bf16_emit_mask_eq0(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_0_idx_table,
                       bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);
enum BF16_MASK_TYPE {
  BF16_MASK_TYPE_GT_0 = 0,  // remain >  0
  BF16_MASK_TYPE_GE_0,      // remain >= 0
  BF16_MASK_TYPE_EQ_0,      // remain  = 0
  BF16_MASK_TYPE_LT_0,      // remain <  0
  BF16_MASK_TYPE_LE_0,      // remain <= 0
  BF16_MASK_MAX
};

int bf16_emit_mask(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                   bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_buf2,
                   bmk1880v2_tensor_lmem_t *tl_buf3, bmk1880v2_tensor_lmem_t *tl_pos_neg_table,
                   bmk1880v2_tensor_lmem_t *tl_0_idx_table, bmk1880v2_tensor_lmem_t *tl_ofmap_bf16,
                   fmt_t fmt, enum BF16_MASK_TYPE mask);

int bf16_emit_mask_lt0(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                       bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_pos_neg_table,
                       bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt);

int _bf16_atan_emit(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
                    bmk1880v2_tensor_lmem_t *tl_buf, bmk1880v2_tensor_lmem_t *tl_buf2,
                    bmk1880v2_tensor_lmem_t *tl_buf3, bmk1880v2_tensor_lmem_t *tl_y0_buf,
                    bmk1880v2_tensor_lmem_t *tl_slope_buf, bmk1880v2_tensor_lmem_t *tl_invert_buf,
                    bmk1880v2_tensor_lmem_t *tl_pos_neg_buf,
                    bmk1880v2_tensor_lmem_t *tl_table_answer,
                    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
                    bmk1880v2_tensor_lmem_t *tl_ofmap_bf16, fmt_t fmt, float b);

uint32_t *bm1880v2_reshape_channel_bias(uint8_t *bias, int ni, int ci, int hi, int wi, int old_bias_c,
                                   fmt_t fmt);

int bm1880v2_reshape_channel_same(bmk1880v2_context_t *bk_ctx, int ic, int ih, int iw, int kh,
                                  int kw, int pad_right, int pad_left, int stride_h, int stride_w,
                                  bmk1880v2_tensor_lmem_shape_t *tl_load_shape,
                                  bmk1880v2_tensor_lmem_stride_t *new_tl_ifmap_stride,
                                  bmk1880v2_tensor_tgmem_shape_t *new_tg_ifmap_shape,
                                  bmk1880v2_tensor_tgmem_stride_t *new_tg_ifmap_stride,
                                  bmk1880v2_tensor_lmem_shape_t *new_tl_weight_shape,
                                  bmk1880v2_tensor_lmem_shape_t *new_tl_bias_shape,
                                  bmk1880v2_tensor_lmem_shape_t *new_tl_ofmap_shape, fmt_t fmt,
                                  int eu_align);

uint8_t *bm1880v2_reshape_channel_weight(uint8_t *weight, int ni, int ci, int hi, int wi, int old_weight_c,
                                    fmt_t fmt);


int bm1880v2_reshape_channel_same_pad(
    bmk1880v2_context_t *bk_ctx,
    int ic, int ih, int iw, int kh, int kw,
    int pad_right, int pad_left, int stride_h, int stride_w,
    bmk1880v2_tensor_lmem_shape_t* tl_load_shape,
    bmk1880v2_tensor_lmem_stride_t* new_tl_ifmap_stride,
    bmk1880v2_tensor_tgmem_shape_t* new_tg_ifmap_shape, 
    bmk1880v2_tensor_tgmem_stride_t* new_tg_ifmap_stride,
    bmk1880v2_tensor_lmem_shape_t* new_tl_weight_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_bias_shape,
    bmk1880v2_tensor_lmem_shape_t* new_tl_ofmap_shape,
    fmt_t fmt, int eu_align);

int bf16_emit_sigmoid(bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_slope,
    bmk1880v2_tensor_lmem_t* tl_ofmap_bf16,
    float scale);

void bf16_sigmoid_tbl(uint16_t *sigmoid_table_data, uint16_t* sigmoid_table_data_slope,
    bmk1880v2_tensor_lmem_shape_t* table_shape,
    int range_start, int range_end);

float bf16_sigmoid_scale(int range_start, int range_end);

void bf16_emit_mask_ge0_lt0(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_t* y,
    bmk1880v2_tensor_lmem_t* index_i8,
    bmk1880v2_tensor_lmem_t* tl_buf3,
    fmt_t fmt
    );

void bf16_emit_mask_eq_0(
    bmk1880v2_context_t *ctx,
    bmk1880v2_tensor_lmem_t* y,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* index_i8,
    bmk1880v2_tensor_lmem_t* tl_buf3,
    fmt_t fmt
    );

int bf16_lut_exp_mantissa(bmk1880v2_context_t *ctx, bmk1880v2_tensor_lmem_t *tl_ifmap,
    bmk1880v2_tensor_lmem_t *tl_buf,
    bmk1880v2_tensor_lmem_t *tbl_answer,
    bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
    bmk1880v2_tensor_lmem_t *tl_ofmap_bf16);

int bf16_s2s_fp32_bf16(bmk1880v2_context_t *ctx, uint64_t gaddr_fp32,
    bmk1880v2_tensor_tgmem_shape_t fp32_shape, uint64_t gaddr_bf16,
    bmk1880v2_tensor_tgmem_shape_t bf16_shape, fmt_t fmt);

/**
 * \gaddr_nc_image for temp gaddr, it could be the same as \gaddr_image
 * \re_order_gaddr_svm means we re-ordered weight by \unit_size and oc/ic transpose
 * \svm_shape as alias as weight of conv, record actually shape likes (oc, ic, kh, kw),
 * the passible shape is <oc, \unit_size, 15, 7>
 * \unit_size as vecotr size, it should be 36 in HOG
 */
int bf16_hists_svm(bmk1880v2_context_t *ctx, uint64_t gaddr_image, uint64_t gaddr_nc_image,
    bmk1880v2_tensor_tgmem_shape_t image_shape, uint64_t re_order_gaddr_svm,
    bmk1880v2_tensor_tgmem_shape_t svm_shape,  // (oc, ic, kh, kw)
    uint64_t gaddr_output, int unit_size, fmt_t fmt);

#ifdef __cplusplus
}
#endif

#endif /* __BMKERNEL_1880v2_NON_ATOMIC_H__ */

