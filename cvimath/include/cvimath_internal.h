#ifndef CVIMATH_INTERNAL_H
#define CVIMATH_INTERNAL_H

#include <stdbool.h>  //bool
#include <stddef.h>   //size_t
#include "cvimath.h"

// copy from lagency
// TODO: move to properly header files
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

static inline uint64_t align_up(uint64_t x, uint64_t n) { return (x + n - 1) / n * n; }

/**
 * please refer @example for more details
 */
#include <cvikernel/cvikernel.h>

#define CVK_MULTIPLIER_BIAS_PACKED_DATA_SIZE 9
#define CVK_MULTIPLIER_ONLY_PACKED_DATA_SIZE 5

// public function
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief get lookup tabel shape
 *
 * @param cvk_ctx kernel structure
 * @param [out] shape the table shape
 */
void cvm_table_shape(cvk_context_t *cvk_ctx, cvk_tl_shape_t *shape);

/**
 * @brief generate sqrt look up table for bf16 exponent part
 *
 * @param [out] table_data bf16 exponent part lookup table in host
 * @param table_shape table shape
 */
void cvm_gen_sqrt(uint16_t *table_data, cvk_tl_shape_t *table_shape);

/**
 * @brief syntactic sugar for cvm_gen_sqrt/cvm_gen_sqrt_mantissa
 *
 * @param [out] sqrt_table_data bf16 exponent part lookup table in host
 * @param [out] sqrt_table_data_mantissa bf16 fraction part lookup table in host
 * @param table_shape table shape
 */
void cvm_sqrt_tbl(uint16_t *sqrt_table_data, uint16_t *sqrt_table_data_mantissa,
                  cvk_tl_shape_t *table_shape);

/**
 * @brief generate sqrt look up table for bf16 fraction part
 *
 * @param [out] table_mantissa bf16 fraction part lookup table in host
 * @param table_shape table shape
 */
void cvm_gen_sqrt_mantissa(uint16_t *table_mantissa, cvk_tl_shape_t *table_shape);

/**
 * @brief implement sqrt in tpu memory
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tbl_answer lookup table tensor for bf16 exponent part in tpu memory
 * @param tbl_answer_mantissa lookup table tensor for fraction part in tpu memory
 * @param [out] tl_ofmap_bf16 result in in memory
 *
 * @example
 *  // 1. alloc in tpu memory
 *  // 2. prepare table
 *  cvm_sqrt_tbl(table_data, table_data_mantissa, &table_shape);
 *  // 3. put host data to tpu memory
 *  // 4. prepare command buffer
 *  cvm_emit_sqrt(cvk_ctx, tl_ifmap, tl_buf, cvk_tl_table_answer, cvk_tl_table_answer_mantissa,
 *                tl_ofmap_bf16);
 *  // 5. submit it
 *  test_submit_comp(rt_ctx, cvk_ctx);
 *
 *  // 6. get result from tpu memory
 *  uint16_t* result = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_sqrt(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                  cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa, cvk_tl_t *tl_ofmap_bf16);

/**
 * @brief generate reciprocal look up table for bf16 exponent part
 *
 * @param [out] table_data bf16 exponent part lookup table in host
 * @param table_shape table shape
 */
void cvm_gen_reciprocal(uint16_t *table_data, cvk_tl_shape_t *table_shape);

/**
 * @brief generate reciprocal look up table for bf16 fraction part
 *
 * @param [out] table_mantissa bf16 fraction part lookup table in host
 * @param table_shape table shape
 */
void cvm_gen_reciprocal_mantissa(uint16_t *table_mantissa, cvk_tl_shape_t *table_shape);

/**
 * @brief syntactic sugar for cvm_gen_reciprocal/cvm_gen_reciprocal_mantissa
 *
 * @param [out] sqrt_table_data bf16 exponent part lookup table in host
 * @param [out] sqrt_table_data_mantissa bf16 fraction part lookup table in host
 * @param table_shape table shape
 */
void cvm_reciprocal_tbl(uint16_t *table_data, uint16_t *table_mantissa,
                        cvk_tl_shape_t *table_shape);

/**
 * @brief implement reciprocal in tpu memory
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tbl_answer lookup table tensor for bf16 exponent part in tpu memory
 * @param tbl_answer_mantissa lookup table tensor for fraction part in tpu memory
 * @param [out] tl_ofmap_bf16 result in in memory
 *
 * @example
 *  int align = 1; // align eu(excution unit)
 *  // 1. alloc in tpu memory
 *  // 2. prepare table
 *  cvm_reciprocal_tbl(table_data, table_data_mantissa, &table_shape);
 *  // 3. put host data to tpu memory
 *  // 4. prepare command buffer
 *  cvm_emit_reciprocal(cvk_ctx, tl_ifmap, tl_buf, cvk_tl_table_answer,
 * cvk_tl_table_answer_mantissa, tl_ofmap_bf16);
 *
 *  // 5. submit it
 *  test_submit_comp(rt_ctx, cvk_ctx);
 *
 *  // 6. get result from tpu memory
 *  uint16_t* result = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_reciprocal(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                        cvk_tl_t *tbl_answer, cvk_tl_t *tbl_answer_mantissa,
                        cvk_tl_t *tl_ofmap_bf16);

/**
 * @brief generate sigmoid lookup table in host,
 * we leverage Linear interpolation fairly close to the original
 * you can refer [wiki](https://en.wikipedia.org/wiki/Interpolation) for more details
 *
 * @param [out] sigmoid_table_data lookup table in host
 * @param [out] sigmoid_table_data_slope slope table in host
 * @param table_shape table shape
 * @param range_start quantize range from,
 * e.g: the original input range is -127 ~ 128, we quantize to -8 ~ 8
 * than -8 is our \range_start and 8 is \range_end
 * @param range_end quantize range end
 */
void cvm_sigmoid_tbl(uint16_t *sigmoid_table_data, uint16_t *sigmoid_table_data_slope,
                     cvk_tl_shape_t *table_shape, int range_start, int range_end);

/**
 * @brief get scale factor from \range_start and \range_end
 *
 * @param range_start quantize range from
 * @param range_end quantize range end
 *
 * @return scale factor
 */
float cvm_sigmoid_scale(int range_start, int range_end);

/**
 * @brief get sigmoid value by linear interpolation
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tl_table_answer sigmoid table in tpu memory generated by \cvm_sigmoid_tbl
 * @param tl_table_answer_slope sigmoid slope table in tpu memory generated by \cvm_sigmoid_tbl
 * @param [out] tl_ofmap_bf16 result in in memory
 * @param scale scale factor generated by \cvm_sigmoid_scale
 *
 * @example
 *  // 1. alloc in tpu memory
 *  // 2. prepare table
 *  cvm_sigmoid_tbl(table_data, table_data_slope, &table_shape, range_start, range_end);
 *  float scale = cvm_sigmoid_scale(range_start, range_end);
 *  // 3. put host data to tpu memory
 *  // 4. prepare command buffer
 *  cvm_emit_sigmoid(cvk_ctx, tl_ifmap, tl_buf, cvk_tl_table_answer, cvk_tl_table_answer_slope,
 *                   tl_ofmap_bf16, scale);
 *  // 5. submit it
 *  test_submit_comp(rt_ctx, cvk_ctx);
 *
 *  // 6. get result from tpu memory
 *  uint16_t* result = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_sigmoid(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                     cvk_tl_t *tl_table_answer, cvk_tl_t *tl_table_answer_slope,
                     cvk_tl_t *tl_ofmap_bf16, float scale);

/**
 * @brief General Matrix Multiplication
 * that equal \lhs_gaddr * \rhs_gaddr = \dest_gaddr
 *
 * @param cvk_ctx kernel structure
 * @param lhs_gaddr left hand side device memory address
 * @param rhs_gaddr right hand side device memory address
 * @param dest_gaddr destination device memory address
 * @param in_row \lhs_gaddr matrix row
 * @param in_col \lhs_gaddr matrix col
 * @param out_col \dest_gaddr matrix col
 * @param fmt the possible value is \CVK_FMT_BF16 or \CVK_FMT_I8 or \CVK_FMT_U8
 * @example
 *
 * // 1. alloc host memory and put it to device memory
 * // M=in_row K=in_col N=out_col
 * cvk_mg_t *mg_A = _test_put_matrix_g(&ctx, M, K, CVK_FMT_BF16, (uint8_t *)bf16_A);
 * cvk_mg_t *mg_B = _test_put_matrix_g(&ctx, K, N, CVK_FMT_BF16, (uint8_t *)bf16_B);
 * cvk_mg_t *mg_R = _test_put_matrix_g(&ctx, M * 2, N, CVK_FMT_BF16, (uint8_t *)bf16_R);
 *
 * // 2. get device address for gemm
 * gaddr_t gaddr_a = mg_A->start_address;
 * gaddr_t gaddr_b = mg_B->start_address;
 * gaddr_t gaddr_r = mg_R->start_address;
 *
 * // 3. prepare gemm descriptor
 * cvm_gemm(cvk_ctx, gaddr_a, gaddr_b, gaddr_r, M, K, N);
 *
 * // 4. submit descriptor
 * test_submit_comp(&ctx, cvk_ctx);
 *
 * // 5. get result from device to host
 * uint16_t *bf16_ref = (uint16_t *)test_get_mg_mem_comp(&ctx, mg_R);
 *
 * @ return slice_num array of {M, N, K}
 */
size_t *cvm_gemm(cvk_context_t *cvk_ctx, uint64_t lhs_gaddr, uint64_t rhs_gaddr,
                 uint64_t dest_gaddr, int in_row, int in_col, int out_col, cvk_fmt_t fmt);

/**
 * @brief combine \cvm_gemm int8 result to int32
 * the raw output is seperate 32bit result info 4 part with bstride
 * and we need to 'combine' it to human readable
 * for instance, the following is the raw result
 * lsb             31               msb
 * 0x1 0x2 0x3 0x4 0x5 0x6 0x7 0x8
 * 0x9 0xa 0xb 0xc 0xd 0xe 0xf 0x0
 * 0x11 0x12 0x13 0x14 0x15 0x16 0x17 0x18
 * 0x19 0x20 0x21 0x22 0x23 0x24 0x25 0x26
 *
 * the value by strategy could be column major:
 * 1. 0x19110901
 * 2. 0x20120a02
 * 3. 0x21130b03
 * and so on
 *
 * @param cvm_gemm_strategy return strategy value from \cvm_gemm
 * @param cvm_output raw result from \cvm_gemm
 * @param [out] i32_R int32 result
 * @param M row of output matrix
 * @param N column of output matrix
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_combin_gemm_i8(size_t *cvm_gemm_strategy, uint8_t *cvm_output, uint32_t *i32_R, int M,
                       int N);
/**
 * @brief fp32 to bf16 format int device memory
 *
 * @param cvk_ctx kernel structure
 * @param gaddr_fp32 fp32 data with device memory address
 * @param fp32_shape fp32 tensor shape
 * @param [out] gaddr_bf16 bf16 data with device memory address
 * @param bf16_shape bf16 tensor shape
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @example
 *
 * cvk_tl_shape_t s = {1, 2, 3, 4}
 * // 1. put fp32 to device memory
 * test_put_tg_mem_comp(rt_ctx, tg_with_fp32, data)
 * // 2. init bf16 tg
 * // 3. prepare command buffer
 * cvm_s2s_fp32_bf16(cvk_ctx, tg_with_fp32->start_address, tg_with_fp32->shape,
 * tg_with_bf16->start_address, tg_with_bf16->shape, CVK_FMT_BF16);
 * // 4. submit it
 * test_submit_comp(rt_ctx, cvk_ctx);
 * // 5. get result from device memory
 * uint16_t *dst_data = (uint16_t *)test_get_tg_mem_comp(rt_ctx, tg_with_bf16);
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_s2s_fp32_bf16(cvk_context_t *cvk_ctx, uint64_t gaddr_fp32, cvk_tg_shape_t fp32_shape,
                      uint64_t gaddr_bf16, cvk_tg_shape_t bf16_shape, cvk_fmt_t fmt);

/**
 * @brief generate lookup table for check input is 0 or not
 *
 * @param [out] table_0 lookup table for 0 or not
 * @param table_shape table shape
 */
void cvm_gen_0_tbl(uint16_t *table_0, cvk_tl_shape_t *table_shape);

// mask function
/**
 * @brief get mask value that seperate 0 or not
 * e.g: input = [0, 1, -1, 2] output [1, 0, 0, 0]
 * please see \cvm_emit_mask for more details
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input in tpu memory
 * @param tl_buf working buffer
 * @param tbl_answer lookup table for 0 or not in tpu memory, generate by \cvm_gen_0_tbl
 * @param [out] tl_ofmap_bf16 mask result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_0_idx(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                   cvk_tl_t *tbl_answer, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

/**
 * @brief get mask value that check < 0
 * e.g: input = [0, 10, 6, -1, 0] output [0, 0, 0, 1, 0]
 * please see \cvm_emit_mask for more details
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input in tpu memory
 * @param tl_buf working buffer
 * @param tl_pos_neg_buf lookup table generate from \cvm_pos_neg_tbl
 * @param [out] tl_ofmap_bf16 mask result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_neg_idx(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                     cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

/**
 * @brief get mask value that check >= 0
 * e.g: input = [0, 10, 6, -1, 0] output [0, 1, 1, 0, 0]
 * please see \cvm_emit_mask for more details
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input in tpu memory
 * @param tl_buf working buffer
 * @param tl_pos_neg_buf lookup table generate from \cvm_pos_neg_tbl
 * @param [out] tl_ofmap_bf16 mask result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_pos_idx(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                     cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

/**
 * @brief invert 0/1 input
 * e.g: input = [0, 1, 1, 1, 0] output [1, 0, 0, 0, 1]
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input in tpu memory
 * @param tl_buf working buffer
 * @param [out] tl_ofmap_bf16 mask result in tpu memory
 * @param fmt
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_0_1_revert_input(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                              cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

// mask enum define
enum CVM_MASK_TYPE {
  CVM_MASK_TYPE_GT_0 = 0,  // remain >  0
  CVM_MASK_TYPE_GE_0,      // remain >= 0
  CVM_MASK_TYPE_EQ_0,      // remain  = 0
  CVM_MASK_TYPE_LT_0,      // remain <  0
  CVM_MASK_TYPE_LE_0,      // remain <= 0
  CVM_MASK_MAX
};

/**
 * @brief get mask for \CVM_MASK_TYPE case
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input in tpu memory
 * @param tl_buf working buffer
 * @param tl_buf2 working buffer
 * @param tl_buf3 working buffer
 * @param tl_pos_neg_table lookup table generate from \cvm_pos_neg_tbl
 * @param tl_0_idx_table lookup table for 0 or not in tpu memory generated by \cvm_gen_0_tbl
 * @param [out] tl_ofmap_bf16 mask result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 * @param mask \CVM_MASK_TYPE
 *
 * @example
 *  // 1. alloc in tpu memory
 *  // 2. prepare table
 *  cvm_gen_0_tbl(idx_0_table_data, &table_shape);
 *  cvm_pos_neg_tbl(table_data_atan_pos_neg, &table_shape);
 *  // 3. put host data to tpu memory
 *  // 4. prepare command buffer
 *  cvm_emit_mask(cvk_ctx,
 *                tl_ifmap,                        // input
 *                tl_buf, tl_buf2, tl_buf4,        // tmp buffer
 *                tl_pos_neg_buf, tl_0_idx_table,  // lookup table
 *                tl_ofmap_bf16,                   // output
 *                fmt, mode);
 *
 *  // 5. submit it
 *  test_submit_comp(rt_ctx, cvk_ctx);
 *
 *  // 6. get result from tpu memory
 *  uint16_t* result = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 *
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_emit_mask(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                  cvk_tl_t *tl_buf3, cvk_tl_t *tl_pos_neg_table, cvk_tl_t *tl_0_idx_table,
                  cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt, enum CVM_MASK_TYPE mask);

/**
 * @brief generate lookup table for atan by degree
 *
 * @param [out] table_data_y0 atan by degree lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_fast_degree_y0(uint16_t *table_data_y0, cvk_tl_shape_t *table_shape);

/**
 * @brief generate lookup table for check value of absolute in [0,1] or > 1
 * atan2 used, [0-1] indicate 1, > 1 indicate with -1
 *
 * @param [out] table_invert lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_s_01(uint16_t *table_invert, cvk_tl_shape_t *table_shape);

/**
 * @brief generate table for check input value is positive(>=0) or negtive(<0)
 * by lookup table, 'pos_neg' means data is positive(>=0) is 1 or negtive(<0) is -1
 *
 * @param [out] table_pos_neg lookup table in host
 * @param table_shape table shape
 */
void cvm_pos_neg_tbl(uint16_t *table_pos_neg, cvk_tl_shape_t *table_shape);

// deprecated code from \cvm_pos_neg_tbl
void cvm_atan_pos_neg(uint16_t *table_pos_neg, cvk_tl_shape_t *table_shape);

/**
 * @brief generate atan answer by lookup table,
 * plz refer [git](https://github.com/xiezhq-hermann/atan_lookup) for more details
 *
 * @param [out] table_data_y0 atan answer lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_y0(uint16_t *table_data_y0, cvk_tl_shape_t *table_shape);

/**
 * @brief generate atan slope data, for more accuracy
 *
 * @param [out] table_slope atan slope lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_slope(uint16_t *table_slope, cvk_tl_shape_t *table_shape);

/**
 * @brief syntactic sugar for cvm_atan_y0/cvm_atan_slope/cvm_atan_s_01/cvm_pos_neg_tbl
 *
 * @param [out] table_data_atan_y0 atan answer lookup table in host
 * @param [out] table_data_atan_slope atan slope lookup table in host
 * @param [out] table_data_atan_invert lookup table in host
 * @param [out] table_data_atan_pos_neg lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_tbl(uint16_t *table_data_atan_y0, uint16_t *table_data_atan_slope,
                  uint16_t *table_data_atan_invert, uint16_t *table_data_atan_pos_neg,
                  cvk_tl_shape_t *table_shape);

/**
 * @brief implement atan in tpu memory
 *
 * @param cvk_ctx kernel structure
 * @param tl_ifmap input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tl_buf2 working buffer
 * @param tl_buf3 working buffer
 * @param tl_y0_buf atan lookup table in tpu memory
 * @param tl_slope_buf atan slope lookup table in tpu memory
 * @param tl_invert_buf lookup table in tpu memory
 * @param tl_pos_neg_buf lookup table in memory
 * @param tl_table_answer reciprocal for bf16 exponent part in tpu memory
 * @param tl_table_answer_mantissa reciprocal for bf16 fraction part in tpu memory
 * @param [out] tl_ofmap_bf16 result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @example
 * // 1. alloc in tpu memory
 * // 2.1. get reciprocal table in host
 * cvm_reciprocal_tbl(table_reciprocal_data, table_reciprocal_data_mantissa, &table_shape);
 * // 2.2. get atan table in host
 * cvm_atan_tbl(table_data_atan_y0, table_data_atan_slope, table_data_atan_invert,
 *              table_data_atan_pos_neg, &table_shape);
 * // 3. put host data to tpu memory
 * // 4. prepare command buffer
 * cvm_atan_emit(cvk_ctx, tl_ifmap, tl_buf, tl_buf2, tl_buf4, tl_y0_buf,
 * tl_slope_buf, tl_invert_buf, tl_pos_neg_buf, tl_reciprocal_table_answer,
 * tl_reciprocal_table_answer_mantissa, tl_ofmap_bf16, fmt);
 *
 * // 5. submit it
 * test_submit_comp(rt_ctx, cvk_ctx);
 * // 6. get result from tpu memory
 * uint16_t *ofmap_data = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 * @return status, 0 means success, other means generates command fail
 */
int cvm_atan_emit(cvk_context_t *cvk_ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2,
                  cvk_tl_t *tl_buf3, cvk_tl_t *tl_y0_buf, cvk_tl_t *tl_slope_buf,
                  cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf, cvk_tl_t *tl_table_answer,
                  cvk_tl_t *tl_table_answer_mantissa, cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);
// atan2 function
/**
 * @brief syntactic sugar for generate atan in degree lookup table in
 * host/cvm_atan_s_01/cvm_pos_neg_tbl
 *
 * @param [out] table_data_atan_y0 atan answer lookup table in host
 * @param [out] table_data_atan_invert lookup table in host
 * @param [out] table_data_atan_pos_neg lookup table in host
 * @param table_shape table shape
 */
void cvm_atan_fast_degree_tbl(uint16_t *table_data_atan_y0, uint16_t *table_data_atan_invert,
                              uint16_t *table_data_atan_pos_neg, cvk_tl_shape_t *table_shape);

/**
 * @brief implement atan2 by degree in tpu memory, implemented by atan. you can refer
 * [wiki](https://en.wikipedia.org/wiki/Atan2) for more details
 *
 * @param cvk_ctx kernel structure
 * @param y input tensor in tpu memory
 * @param x input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tl_buf2 working buffer
 * @param tl_buf3 working buffer
 * @param tl_y0_buf atan2 lookup table in tpu memory
 * @param tl_invert_buf lookup table in tpu memory
 * @param tl_pos_neg_buf lookup table in memory
 * @param tl_table_answer reciprocal for bf16 exponent part in tpu memory
 * @param tl_table_answer_mantissa reciprocal for bf16 fraction part in tpu memory
 * @param [out] tl_ofmap_bf16 result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @example
 * // 1. alloc in tpu memory
 * // 2.1. get reciprocal table in host
 * cvm_reciprocal_tbl(table_reciprocal_data, table_reciprocal_data_mantissa, &table_shape);
 * // 2.2. get atan table in host
 * cvm_atan_fast_degree_tbl(table_data_atan_y0, table_data_atan_slope, table_data_atan_invert,
 *              table_data_atan_pos_neg, &table_shape);
 * // 3. put host data to tpu memory
 * // 4. prepare command buffer
 * cvm_atan2_fast_degree_emit(
 *    cvk_ctx, tl_ifmap2, tl_ifmap, tl_buf, tl_buf2, tl_buf3, tl_y0_buf,
 *    tl_invert_buf, tl_pos_neg_buf, tl_reciprocal_table_answer,
 *    tl_reciprocal_table_answer_mantissa, tl_ofmap_bf16, fmt);
 *
 * // 5. submit it
 * test_submit_comp(rt_ctx, cvk_ctx);
 * // 6. get result from tpu memory
 * uint16_t *ofmap_data = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 */
void cvm_atan2_fast_degree_emit(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *x, cvk_tl_t *tl_buf,
                                cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_y0_buf,
                                cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf,
                                cvk_tl_t *tl_table_answer, cvk_tl_t *tl_table_answer_mantissa,
                                cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);
/**
 * @brief implement atan2 in tpu memory, implemented by atan. you can refer
 * [wiki](https://en.wikipedia.org/wiki/Atan2) for more details
 *
 * @param cvk_ctx kernel structure
 * @param y input tensor in tpu memory
 * @param x input tensor in tpu memory
 * @param tl_buf working buffer
 * @param tl_buf2 working buffer
 * @param tl_buf3 working buffer
 * @param tl_y0_buf atan2 lookup table in tpu memory
 * @param tl_invert_buf lookup table in tpu memory
 * @param tl_pos_neg_buf lookup table in memory
 * @param tl_table_answer reciprocal for bf16 exponent part in tpu memory
 * @param tl_table_answer_mantissa reciprocal for bf16 fraction part in tpu memory
 * @param [out] tl_ofmap_bf16 result in tpu memory
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @example
 * // 1. alloc in tpu memory
 * // 2.1. get reciprocal table in host
 * cvm_reciprocal_tbl(table_reciprocal_data, table_reciprocal_data_mantissa, &table_shape);
 * // 2.2. get atan table in host
 * cvm_atan_fast_degree_tbl(table_data_atan_y0, table_data_atan_slope, table_data_atan_invert,
 *              table_data_atan_pos_neg, &table_shape);
 * // 3. put host data to tpu memory
 * // 4. prepare command buffer
 * cvm_atan2_fast_degree_emit(
 *    cvk_ctx, tl_ifmap2, tl_ifmap, tl_buf, tl_buf2, tl_buf3, tl_y0_buf,
 *    tl_invert_buf, tl_pos_neg_buf, tl_reciprocal_table_answer,
 *    tl_reciprocal_table_answer_mantissa, tl_ofmap_bf16, fmt);
 *
 * // 5. submit it
 * test_submit_comp(rt_ctx, cvk_ctx);
 * // 6. get result from tpu memory
 * uint16_t *ofmap_data = (uint16_t *)get_bf16_tensor_l2g(rt_ctx, cvk_ctx, tl_ofmap_bf16,
 * tl_ofmap_bf16->fmt);
 */
void cvm_atan2_merge_emit(cvk_context_t *cvk_ctx, cvk_tl_t *y, cvk_tl_t *x, cvk_tl_t *tl_buf,
                          cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_y0_buf,
                          cvk_tl_t *tl_invert_buf, cvk_tl_t *tl_pos_neg_buf,
                          cvk_tl_t *tl_table_answer, cvk_tl_t *tl_table_answer_mantissa,
                          cvk_tl_t *tl_ofmap_bf16, cvk_fmt_t fmt);

/**
 * @brief get lookup table size for host alloc mamory used
 *
 * @param cvk_ctx kernel structure
 * @param table_shape table shape
 * @param fmt tensor format such as \CVK_FMT_BF16
 *
 * @return table size in bytes
 */
uint64_t cvm_lut_tbl_bytesize(cvk_context_t *cvk_ctx, cvk_tl_shape_t *table_shape, cvk_fmt_t fmt);

/**
 * @brief calculate new proper reshape channel for depthwise
 * current only support batch = 1
 *
 * @param cvk_ctx kernel structure
 * @param ic origin input shape of c
 * @param ih origin input shape of h
 * @param iw origin input shape of w
 * @param kh origin kerenl shape of h
 * @param kw origin kerenl shape of w
 * @param pad_right padding right with input
 * @param pad_left padding left with input
 * @param stride_h stride h with input
 * @param stride_w stride w with input
 * @param [out] tl_load_shape shape structure for input in tpu memory
 * @param [out] new_tl_ifmap_stride deprecated that stride for input in tpu memory
 * @param [out] new_tg_ifmap_shape shape structure for input in device memory
 * @param [out] new_tg_ifmap_stride stride structure for input in device memory
 * @param [out] new_tl_weight_shape reshape weight in tpu memory
 * @param [out] new_tl_bias_shape reshape bias in tpu memory
 * @param [out] new_tl_ofmap_shape reshape output in tpu memory
 * @param fmt the possible value is \CVK_FMT_BF16 or \CVK_FMT_I8 or \CVK_FMT_U8
 * @param eu_align currently MUST set 1 is force align with hardware
 *
 * @example
 * int align = 1; // force align
 * cvk_tiu_depthwise_pt_convolution_param_t *p;
 * // 1. get reshaped shape
 * int r = cvm_reshape_channel_same(
 *    cvk_ctx, ic, ih, iw, kh, kw, pad_right, pad_left, stride_h, stride_w,
 *    &tl_load_shape, &tl_load_stride, &tg_shape, &tg_stride, &tl_weight_shape,
 *    &tl_bias_shape, &tl_output_shape, fmt, align);
 * // reshape fail
 * if (r == -1) {
 *   return -1;
 * }
 *
 * // 2.1 load input
 * // load input into tpu memory
 * int load_align = 0; // not align for pack
 * tmp_tl_load = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_load_shape, fmt, load_align);
 * tmp_tg = test_alloc_tg_mem_comp(&rt_ctx, cvk_ctx, tg_shape, fmt);
 * tmp_tg->stride = tg_stride;

 * // int8
 * cvk_tdma_g2l_tensor_copy_param_t p1;
 * cvk_ctx->ops->tdma_g2l_tensor_copy(cvk_ctx, &p1);
 * test_submit_comp(&rt_ctx, cvk_ctx);
 * test_free_tg_mem_comp(&rt_ctx, tmp_tg);


 * // fit for hw
 * int align_in_tl = 1;
 * tmp_tl_load->stride = bmk1880v2_tensor_lmem_default_stride(
 *     cvk_ctx, tmp_tl_load->shape, fmt, align_in_tl);
 * p->ifmap = tmp_tl_load;

 * // 2.2 prepare load bias, put to tg and load back
 * if (has_bias) {
 *   // bias must i8
 *   int no_bias_align = 0;
 *   p->bias = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_bias_shape, fmt, no_bias_align);
 *
 *   // duplicate bias and replace old
 *   uint32_t *new_bias = cvm_reshape_channel_weight(
 *       (uint8_t *)bias, tl_bias_shape.n, tl_bias_shape.c, tl_bias_shape.h,
 *       tl_bias_shape.w, org_oc, fmt);
 *
 *   test_put_tensor_g2l_comp(&rt_ctx, cvk_ctx, p->bias, bias);
 * }
 *
 * // 2.3 prepare load weight, put to tg and load back
 * {
 *   int weight_align = 1;
 *   p->weight = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_weight_shape, fmt, weight_align);
 *   // duplicate kernel with c
 *   uint8_t *new_weight = cvm_reshape_channel_weight(
 *       (uint8_t *)weight, tl_weight_shape.n, tl_weight_shape.c, tl_weight_shape.h,
 *       tl_weight_shape.w, org_oc, fmt);
 *
 *   test_put_tensor_g2l_comp(&rt_ctx, cvk_ctx, p->weight, (u16 *)weight);
 * }
 *
 * // 2.4 prepard ofmap
 * {
 *   // we allocate 'same' mode shape
 *   int output_align = 1; // hw need
 *   p->ofmap = cvk_ctx->ops->lmem_alloc_tensor(cvk_ctx, tl_output_shape, fmt, output_align);
 * }
 *
 * // 3. prepare command buffer
 * cvk_ctx->ops->tiu_pt_depthwise_convolution(cvk_ctx, p);
 *
 * // 4. submit it
 * test_submit_comp(rt_ctx, cvk_ctx);
 *
 * // 5. get result from tpu memory
 * output = test_get_tensor_l2g_comp(&rt_ctx, cvk_ctx, p->ofmap, fmt);
 *
 * @return status, -1 means fail, other means reshape slice success
 */
int cvm_reshape_channel_same(cvk_context_t *cvk_ctx, int ic, int ih, int iw, int kh, int kw,
                             int pad_right, int pad_left, int stride_h, int stride_w,
                             cvk_tl_shape_t *tl_load_shape, cvk_tl_stride_t *new_tl_ifmap_stride,
                             cvk_tg_shape_t *new_tg_ifmap_shape,
                             cvk_tg_stride_t *new_tg_ifmap_stride,
                             cvk_tl_shape_t *new_tl_weight_shape, cvk_tl_shape_t *new_tl_bias_shape,
                             cvk_tl_shape_t *new_tl_ofmap_shape, cvk_fmt_t fmt, int eu_align);

/**
 * @brief re-construct bias content by reshape channel
 *
 * @param bias original bias in host memory
 * @param ni reshape bias shape of n
 * @param ci reshape bias shape of c
 * @param hi reshape bias shape of h
 * @param wi reshape bias shape of w
 * @param old_bias_c origin bias shape of c
 * @param fmt the possible value is \CVK_FMT_BF16 or \CVK_FMT_I8 or \CVK_FMT_U8
 *
 * @return bias host data
 */
uint32_t *cvm_reshape_channel_bias(uint8_t *bias, int ni, int ci, int hi, int wi, int old_bias_c,
                                   cvk_fmt_t fmt);

/**
 * @brief re-construct weight content by reshape channel
 *
 * @param weight original bias in host memory
 * @param ni reshape weight shape of n
 * @param ci reshape weight shape of c
 * @param hi reshape weight shape of h
 * @param wi reshape weight shape of w
 * @param old_weight_c origin weight shape of c
 * @param fmt the possible value is \CVK_FMT_BF16 or \CVK_FMT_I8 or \CVK_FMT_U8
 *
 * @return weight host data
 */
uint8_t *cvm_reshape_channel_weight(uint8_t *weight, int ni, int ci, int hi, int wi,
                                    int old_weight_c, cvk_fmt_t fmt);

typedef struct cvm_tiu_atan2_param {
  cvk_tl_t *a;
  cvk_tl_t *b;
  cvk_tl_t *res;
  cvk_tl_t *buf1;
  cvk_tl_t *buf2;
  cvk_tl_t *buf3;
  cvk_tl_t *buf4;
  cvk_tl_t *buf5;
  cvk_tl_t *buf6;
  cvk_tl_t *y0;
  cvk_tl_t *slope;
  cvk_tl_t *invert;
  cvk_tl_t *pos_neg_table;
  cvk_tl_t *reciprocal_table_answer;
  cvk_tl_t *reciprocal_table_answer_mantissa;
  cvk_tl_t *sqrt_table_answer;
  cvk_tl_t *sqrt_table_answer_mantissa;
  cvk_tl_t *idx_0_table;
  cvk_fmt_t fmt;
  bool output_degree;
} cvm_tiu_atan2_param_t;

typedef struct cvk_tiu_mask_param {
  cvk_tl_t *ifmap;
  cvk_tl_t *ofmap;
  cvk_tl_t *buf;
  cvk_tl_t *buf2;
  cvk_tl_t *buf3;
  cvk_tl_t *pos_neg_table;
  cvk_tl_t *idx_0_table;
  cvk_fmt_t fmt;
} cvm_tiu_mask_param_t;

typedef struct cvm_tiu_sigmoid_param {
  float scale;
  cvk_tl_t *ifmap;
  cvk_tl_t *buf;
  cvk_tl_t *table_answer;
  cvk_tl_t *table_answer_slope;
  cvk_tl_t *ofmap;
} cvm_tiu_sigmoid_param_t;

typedef struct cvm_tiu_sqrt_param {
  cvk_tl_t *a;
  cvk_tl_t *res;
  cvk_tl_t *buf;
  cvk_tl_t *sqrt_table_answer;
  cvk_tl_t *sqrt_table_answer_mantissa;
} cvm_tiu_sqrt_param_t;

/**
 * @brief get \quantized_multiplier and its \right_shift,
 * please refer
 * \https://github.com/tensorflow/tensorflow/blob/master/tensorflow/lite/kernels/internal/common.h:MultiplyByQuantizedMultiplier
 * for more details
 *
 * @param real_multiplier
 * @param quantized_multiplier
 * @param right_shift
 */
void cvm_get_chl_quan(float real_multiplier, uint32_t *quantized_multiplier, int *right_shift);

/**
 * @brief
 *
 * @param c
 * @param quantized_multiplier
 * @param right_shift
 * @param cal_data
 * @param bias_data
 * @param has_bias
 */
void cvm_fill_chl_quan_data(const uint32_t c, const uint32_t quantized_multiplier,
                            const int right_shift, uint8_t *cal_data, int32_t *bias_data,
                            bool has_bias);

/**
 * @brief
 *
 * @param c
 * @param quantized_multiplier
 * @param right_shift
 * @param bias_data
 * @param has_bias
 *
 * @return
 */
uint8_t *cvm_get_chl_quan_data(const uint32_t c, const uint32_t quantized_multiplier,
                               const int right_shift, int32_t *bias_data, bool has_bias);

/**
 * @brief get byte size of input \fmt
 *
 * @param fmt \cvk_fmt_t structure
 *
 * @example
 * int sz = cvm_bytesize_of_fmt(CVK_FMT_BF16);
 * assert (sz == 2 && "bf16 takes 2 bytes")
 *
 * sz = cvm_bytesize_of_fmt(CVK_FMT_I8);
 * assert (sz == 1 && "int8 takes 1 bytes")
 * @return byte size of fmt
 */
int cvm_bytesize_of_fmt(cvk_fmt_t fmt);

/**
 * @brief reduce multiplication for h,w
 * the possible shape will be <1, c, 1, 1>
 * you could refer [here](https://en.wikipedia.org/wiki/Reduction_Operator) for
 * more details
 *
 * @param cvk_ctx kernel structure
 * @param [out] mp_tl_mulsum input tensor in tpu memory, the shape should be <1, c, h, w>
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_reduce_hw_mul(cvk_context_t *cvk_ctx, cvk_tl_t *mp_tl_mulsum);

/**
 * @brief bf16 to fp32, ONLY move bf16 to fp32 high 16 bits part,
 * the memory layout as following:
 *
 * bf16: 0x4300
 * 0 16    (bit)
 * -----
 * 0x4300
 *
 * fp32: 0x43000000
 * -----
 * 0 16 32
 * 0x  0x43
 *
 * @param cvk_ctx kernel structure
 * @param tg_bf16 bf16 data in device memory
 * @param [out] tg_fp32 fp32 data in decive memory, the w shape SHOULD be double with
 * \tg_bf16->shape.w
 */
void cvm_bf16_fp32(cvk_context_t *cvk_ctx, cvk_tg_t *tg_bf16, cvk_tg_t *tg_fp32);

/**
 * @brief set value by mask(0/1)
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_mask mask value, it MUST be 0 or 1, it will DIRTY it
 * @param [in] tl_buf
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_set_image_by_u8mask(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                            cvk_tl_t *tl_mask, cvk_tl_t *tl_ofmap);

/**
 * @brief set value by mask(0/1) by DePthwise
 * 0 means keep \tl_ofmap one
 * 1 means overwrite with \tl_ifmap
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_mask mask value, it MUST be 0 or 1, it will DIRTY it
 * @param [in] tl_kernel for mask reverting(0/1->1/0) that the contain MUST BE -1 with int8
 * and shape SHOULD BE <1, tl_ifmap->shape.c, 1, 1>
 * @param [in] tl_bias for mask reverting(0/1->1/0) that the contain MUST BE 1 with int8,
 * seperate high/low part, and shape SHOULD BE <2, tl_ifmap->shape.c, 1, 1>
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */

int cvm_set_image_by_u8mask_dp(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_mask,
                               cvk_tl_t *tl_kernel, cvk_tl_t *tl_bias, cvk_tl_t *tl_ofmap);

/**
 * @brief set value by mask and threshold, set it
 * if \tl_mask && (int8_t)\tl_update_tbl < threshold
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_mask mask value, it MUST be 0 or 1, it will DIRTY it
 * @param [in] tl_update_tbl the value range will under int8, it will DIRTY it
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_set_image_by_two_info_i8(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf2,
                                 cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl, uint8_t threshold,
                                 cvk_tl_t *tl_ofmap);

/**
 * @brief set value by mask and threshold by DePthwise, set it
 * if \tl_mask && (int8_t)\tl_update_tbl < threshold
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_kernel set all to 1 for \tl_update_tbl * 1 - threshold
 * to test larger or smaller,
 * that MUST BE 1 with int8 and shape SHOULD BE <1, tl_ifmap->shape.c, 1, 1>
 * @param [in] tl_mask mask value, it MUST be 0 or 1, it will DIRTY it
 * @param [in] tl_update_tbl the value range will under int8, it will DIRTY it
 * @param [in] tl_threshold for boradcast \threshold to bias
 * the type MUST BE int8 and seperate high/low part and it will DIRTY it
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */

int cvm_set_image_by_two_info_i8_dp(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_kernel,
                                    cvk_tl_t *tl_mask, cvk_tl_t *tl_update_tbl,
                                    cvk_tl_t *tl_threshold, cvk_tl_t *tl_ofmap);

/**
 * @brief get abs(\tl_ifmap-tl_ifmap2)
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_ifmap2 image input, MUST uint8, it will DIRTY it
 * @param [out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, o, MUST uint8ther means generates command fail
 */
int cvm_gen_image_diff(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_ifmap2,
                       cvk_tl_t *tl_buf, cvk_tl_t *tl_buf2, cvk_tl_t *tl_ofmap);

/**
 * @brief update \tl_ofmap by \threshold_a, \threshold_b,
 *  plz refer \sample_set_val_by_mask.cpp for more details
 *
 * @param [out] tl_mask return 0/1 mask
 * @param [in] tl_update_tbl u8
 * @param [in,out] tl_ofmap image output, int8
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_update_tbl_by_threshold(cvk_context_t *ctx, cvk_tl_t *tl_mask, cvk_tl_t *tl_buf,
                                cvk_tl_t *tl_buf2, cvk_tl_t *tl_buf3, cvk_tl_t *tl_update_tbl,
                                uint8_t threshold_a, uint8_t threshold_b, cvk_tl_t *tl_ofmap);

/**
 * @brief set value by mask, update \tl_ofmap once (uint8_t)tl_update_tbl >= threshold
 *
 * @param [in] tl_ifmap image input, MUST uint8
 * @param [in] tl_update_tbl the value range will under uint8
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_set_image_by_two_info_u8(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                                 cvk_tl_t *tl_buf2, cvk_tl_t *tl_update_tbl, uint8_t threshold,
                                 cvk_tl_t *tl_ofmap);

/**
 * @brief set value by mask
 * if (int8_t)\tl_update_tbl > threshold
 *
 * @param [in] tl_ifmap image input
 * @param [in] tl_update_tbl int8, MUST uint8, it will DIRTY
 * @param [in,out] tl_ofmap image output, MUST uint8, it will DIRTY it
 *
 * @return status, 0 means success, other means generates command fail
 */
int cvm_blend_image_by_tbl(cvk_context_t *ctx, cvk_tl_t *tl_ifmap, cvk_tl_t *tl_buf,
                           cvk_tl_t *tl_buf2, cvk_tl_t *tl_update_tbl, uint8_t threshold,
                           uint8_t w1, uint8_t w2, cvk_tl_t *tl_ofmap);
/**
 * @brief get upsample 2d with nearest mode
 *
 * @param [in] tl_ifmap
 * @param [in] tl_weight upsample used that fill with 1
 * @param [out] tl_ofmap
 *
 * @return status, 0 means success, other means generates command fail
 */

int cvm_upsample2d(cvk_context_t *ctx, cvk_tl_t *tl_input, cvk_tl_t *tl_weight,
                   cvk_tl_t *tl_output);
#ifdef __cplusplus
}
#endif

#endif  // CVIMATH_INTERNAL_H
