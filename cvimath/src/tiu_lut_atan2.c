/**
 * \brirf implement with atan, plz refer https://en.wikipedia.org/wiki/Atan2
 * NOTICE: current epsilon set to 0.1
 */
#include <cvimath_internal.h>
#include "gen_lut.h"  // NOLINT

//#define DBG

static void _cvm_atan2_emit_case_3(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                                   cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_buf4,
                                   cvk_tl_t* tl_y0_buf, cvk_tl_t* tl_slope_buf,
                                   cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                   cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_mantissa,
                                   cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt, float b) {
  // case 3
  // atan( y / x)

  // x0 = reciprocal(x)
  cvm_emit_reciprocal(ctx, x, tl_buf2, tl_table_answer, tl_table_answer_mantissa, tl_buf);

  // y0 = x0 * y
  cvk_tiu_mul_param_t p1;
  p1.res_high = NULL;
  p1.res_low = tl_buf4;
  p1.a = y;
  p1.b_is_const = 0;
  p1.b = tl_buf;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;

  ctx->ops->tiu_mul(ctx, &p1);

  // x0 = atan(y0)
  _cvm_atan_emit(ctx, tl_buf4, tl_buf, tl_buf2, tl_buf3, tl_y0_buf, tl_slope_buf, tl_invert_buf,
                 tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa, OUT tl_ofmap_bf16,
                 fmt, b);
}

static void cvm_atan2_emit_case_3(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                                  cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_buf4,
                                  cvk_tl_t* tl_y0_buf, cvk_tl_t* tl_slope_buf,
                                  cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                  cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_mantissa,
                                  cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt) {
  _cvm_atan2_emit_case_3(ctx, y, x, tl_buf, tl_buf2, tl_buf3, tl_buf4, tl_y0_buf, tl_slope_buf,
                         tl_invert_buf, tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa,
                         tl_ofmap_bf16, fmt, 0.0);
}

// NOTICE: it could dirty \y
/**
 * atan2(y, x) should express 4 condition using atan express from
 * [here](https://en.wikipedia.org/wiki/Atan2)
 */
void cvm_atan2_emit(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                    cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_buf4, cvk_tl_t* tl_buf5,
                    cvk_tl_t* tl_buf6, cvk_tl_t* tl_y0_buf, cvk_tl_t* tl_slope_buf,
                    cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table, cvk_tl_t* tl_table_answer,
                    cvk_tl_t* tl_table_answer_mantissa, cvk_tl_t* tl_sqrt_table_answer,
                    cvk_tl_t* tl_sqrt_table_answer_mantissa, cvk_tl_t* tl_0_idx_table,
                    cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt) {
  cvm_table_check(y, tl_y0_buf, tl_slope_buf, x);
  cvm_table_check(tl_buf, tl_invert_buf, tl_pos_neg_table, tl_buf2);
  cvm_table_check(tl_buf3, tl_table_answer, tl_table_answer_mantissa, tl_buf4);
  cvm_table_check(tl_buf6, tl_table_answer, tl_0_idx_table, tl_buf5);
  cvm_table_check(y, tl_sqrt_table_answer, tl_sqrt_table_answer_mantissa, x);

  // atan(y/x), x > 0
  // atan(y/x) + PI , x < 0 and y >= 0
  // atan(y/x) - PI , x < 0 and y < 0
  // pi / 2, x = 0 and y > 0
  // -pi / 2, x = 0 and y < 0
  // 0, x = 0 and y = 0

  // atan(y/x), x > 0
  cvm_emit_max_const(ctx, x, tl_buf4, fmt, 0.0);
  cvm_atan2_emit_case_3(ctx, y, tl_buf4, tl_buf, tl_buf2, tl_buf3, tl_buf5, tl_y0_buf, tl_slope_buf,
                        tl_invert_buf, tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa,
                        tl_ofmap_bf16, fmt);

  // x > 0
  cvm_emit_mask_gt0(ctx, x, tl_buf, tl_buf3, tl_buf4, tl_pos_neg_table, tl_0_idx_table, tl_buf2,
                    fmt);

  cvm_emit_mul(ctx, tl_ofmap_bf16, tl_buf2, tl_ofmap_bf16, fmt);

  // atan(y/x) + PI , x < 0 and y >= 0
  cvm_emit_min_const(ctx, x, tl_buf4, fmt, 0.0);
  _cvm_atan2_emit_case_3(ctx, y, tl_buf4, tl_buf, tl_buf2, tl_buf3, tl_buf5, tl_y0_buf,
                         tl_slope_buf, tl_invert_buf, tl_pos_neg_table, tl_table_answer,
                         tl_table_answer_mantissa, tl_buf6, fmt, M_PI);
  // cvm_emit_add_const(ctx, tl_buf6, tl_buf6, fmt, M_PI);

  // get index map that x < 0 and y >= 0
  // !(y >= 0) = !(y < 0)
#if 0
  cvm_emit_pos_idx(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // y == 0
  cvm_emit_0_idx(ctx, y, tl_buf, tl_0_idx_table, tl_buf2, fmt);
  cvm_emit_add(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
#else
  // y >= 0
  cvm_emit_mask_ge0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf2, fmt);
#endif
  // x < 0
  cvm_emit_mask_lt0(ctx, x, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x < 0 && y >= 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul(ctx, tl_buf6, tl_buf2, tl_buf, fmt);
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // atan(y/x) - PI , x < 0 and y < 0
  cvm_emit_min_const(ctx, x, tl_buf4, fmt, 0.0);
  cvm_atan2_emit_case_3(ctx, y, tl_buf4, tl_buf, tl_buf2, tl_buf3, tl_buf5, tl_y0_buf, tl_slope_buf,
                        tl_invert_buf, tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa,
                        tl_buf6, fmt);
  cvm_emit_add_const(ctx, tl_buf6, tl_buf6, fmt, -1.0 * M_PI);
  // x < 0 and y < 0

  // we leverage x <= 0 and y <= 0 cuz we filter out x = 0 case, speed up it
  // x < 0
  cvm_emit_mask_lt0(ctx, x, tl_buf, tl_pos_neg_table, tl_buf2, fmt);
  // y < 0
  cvm_emit_mask_lt0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x < 0 && y < 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul(ctx, tl_buf6, tl_buf2, tl_buf, fmt);
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // pi / 2, x = 0 and y > 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1

  // y > 0
  cvm_emit_mask_gt0(ctx, y, tl_buf, tl_buf5, tl_buf4, tl_pos_neg_table, tl_0_idx_table, tl_buf3,
                    fmt);
  // x = 0 && y > 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, fmt, M_PI / 2.0);

  cvm_emit_add(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // -pi / 2, x = 0 and y < 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1
  // y < 0
  cvm_emit_mask_lt0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x = 0 && y < 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, fmt, -1.0 * M_PI / 2.0);

  cvm_emit_add(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // 0, x = 0 and y = 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1
  // y = 0
  cvm_emit_mask_eq0(ctx, y, tl_buf, tl_0_idx_table, tl_buf3, fmt);  // 0.003 could consider 1

  // x = 0 && y = 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf3, fmt);

  // !(x = 0 and y = 0) keep it
  cvm_emit_0_1_revert_input(ctx, tl_buf3, tl_buf, tl_buf2, fmt);
  cvm_emit_mul(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
}

// ==== fast version ===
static void __cvm_atan2_fast_emit_case_3(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x,
                                         cvk_tl_t* tl_buf, cvk_tl_t* tl_y0_buf,
                                         cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                         cvk_tl_t* tl_table_answer,
                                         cvk_tl_t* tl_table_answer_mantissa,
                                         cvk_tl_t* OUT tl_ofmap_bf16, cvk_tl_t* OUT y_over_x,
                                         cvk_fmt_t fmt, float b) {
  // case 3
  // atan( y / x)

#if 0
  // x0 = reciprocal(x)
  _cvm_lut_exp_mantissa(ctx,
      x,
      NULL,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_buf,
      true
      );

  // y0 = x0 * y
  cvm_emit_mul(ctx, y, tl_buf, tl_buf, fmt);
#else
  cvm_emit_x_over_y(ctx, y, x, NULL, tl_buf, tl_table_answer, tl_table_answer_mantissa, fmt, true);

  if (y_over_x) {
    cvm_emit_add_const(ctx, tl_buf, y_over_x, fmt, 0);
  }
#endif

  // x0 = atan(y0)
  _cvm_atan_fast_emit(ctx, tl_buf, x, NULL, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                      tl_table_answer, tl_table_answer_mantissa, OUT tl_ofmap_bf16, fmt, b, true);
}

#if 0
static void _cvm_atan2_fast_emit(cvk_context_t *ctx,
    cvk_tl_t* y,
    cvk_tl_t* tl_buf,
    cvk_tl_t* tl_buf2,
    cvk_tl_t* tl_buf4,
    cvk_tl_t* tl_y0_buf,
    cvk_tl_t* tl_invert_buf,
    cvk_tl_t* tl_pos_neg_table,
    cvk_tl_t* tl_table_answer,
    cvk_tl_t* tl_table_answer_mantissa,
    cvk_tl_t* OUT tl_ofmap_bf16,
    cvk_tl_t* OUT tl_buf3,
    cvk_fmt_t fmt) {
  // case 3
  // atan( y / x)

#if 0
  // x0 = reciprocal(tl_buf)
  _cvm_lut_exp_mantissa(ctx,
      tl_buf,
      NULL,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_buf2,
      true
      );

  // y0 = x0 * y
  cvm_emit_mul(ctx, y, tl_buf2, tl_buf2, fmt);
#else
#if 0
  cvm_emit_x_over_y(ctx, y, tl_buf, NULL, tl_buf2,
      tl_table_answer, tl_table_answer_mantissa, fmt, true);

  if (tl_buf3) {
	bf16_emit_add_const(ctx, tl_buf2, tl_buf3, fmt, 0);
  }
#else
  //if (tl_buf3) {
  //  cvm_emit_add_const(ctx, tl_buf, tl_buf3, fmt, 0);
  //}

  // get xy == 0 and y < 0, add pi
  // using xy to depend x = 0 or y = 0
  // recipical y < 0 get 0xFEFF, y > 0 get 0x7F7F,
  // 1. b = xy to get other/(x = 0 or y = 0)
  // 2. c = b * 2^64 to saturate it
  // 3. c(bf16) = c(int8) >> 10 to get 1/0 map, 1 indicate xy > 0
  // 4. c = c * -1 + 1 to invert map, 1 indicate x = 0 or y = 0
  // 5. d = b(int8) - 0x7f, 0 means y > 0
  // 6. d = d(int8) + 0xff to get inf
  cvm_emit_mul(ctx, y, tl_buf, tl_buf2, fmt);
  // get 7f7f / 0
  cvm_emit_mul_const(ctx, tl_buf2, tl_ofmap_bf16, fmt, convert_bf16_fp32(0x7f00));
  //// 1 = 0x3f80
  //bf16_emit_mul_const(ctx, tl_buf2, tl_ofmap_bf16, fmt, 0);
  //bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_buf4, fmt, 1.0);
  // bf16->uint8_t and back uint8_t->bf16 to get 0/1 map

#if 1
  cvk_tl_t index_uint8_t;
  bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &index_uint8_t, tl_buf2, CVK_FMT_U8);

  index_uint8_t.shape.w = index_uint8_t.shape.w / 2;
  index_uint8_t.stride = ctx->ops->tl_default_stride(ctx, index_uint8_t.shape,
	  CTRL_NULL, CVK_FMT_I8);

  index_uint8_t.fmt = CVK_FMT_I8;

  cvk_tdma_l2l_tensor_copy_param_t p1;
  p1.src = tl_ofmap_bf16;
  p1.dst = &index_uint8_t;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);
  cvk_tiu_mul_param_t p;

#if 0

  p.res_high = NULL;
  p.res_low = &index_uint8_t;
  p.a = &index_uint8_t;
  p.b_is_const = 1;
  p.b_const.val =-1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 0;
  p.relu_enable = 0;

  ctx->ops->tiu_mul(ctx, &p);
#else
  p.res_high = NULL;
  p.res_low = &index_uint8_t;
  p.a = &index_uint8_t;
  p.b_is_const = 1;
  p.b_const.val =-1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 0;

  ctx->ops->tiu_mul(ctx, &p);

#endif

  // get -1/0 map, -1 indicate xy != 0
  p1.src = &index_uint8_t;
  p1.dst = tl_ofmap_bf16;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);

  // x * (-1) + 1 get 0/1 map, 1 indicate xy == 0
  //bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, -1.0);
  cvm_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);

  // get y < 0, bf16->int8 and mul 0xff to get -128 and righshift to get 1
  cvm_emit_mul_const(ctx, y, tl_buf3, fmt, pow(2,64));
  p1.src = tl_buf3;
  p1.dst = &index_uint8_t;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);


  p.res_high = 0;
  p.res_low = &index_uint8_t;
  p.a = &index_uint8_t;
  p.b_is_const = 1;
  p.b_const.val =-128;
  p.b_const.is_signed = 1;
  p.rshift_bits = 0;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  p.res_high = 0;
  p.res_low = &index_uint8_t;
  p.a = &index_uint8_t;
  p.b_is_const = 1;
  p.b_const.val =1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  // get y < 0
  p1.src = &index_uint8_t;
  p1.dst = tl_buf4;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);
  cvm_emit_mul_const(ctx, tl_buf4, tl_buf4, fmt, -1.0);

  // get y > 0
  // y * (-1) + 1 get 0/1 map, 1 indicate xy == 0
  cvm_emit_add_const(ctx, tl_buf4, tl_buf2, fmt, 1.0);
  cvm_emit_add(ctx, tl_buf2, tl_buf4, tl_buf2, fmt);

  // merge y > 0 && y < 0 && x == 0
  cvm_emit_mul(ctx, tl_ofmap_bf16, tl_buf2, tl_buf3, fmt);
  //bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 0);
  //bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_buf3, fmt, M_PI);

#endif


  cvm_emit_x_over_y(ctx, y, tl_buf, NULL, tl_buf2,
      tl_table_answer, tl_table_answer_mantissa, fmt, true);
#endif
#endif

  // x0 = atan(y0)
  __cvm_atan_fast_emit(ctx,
      tl_buf2,
      tl_buf,
      tl_buf4,
      tl_y0_buf,
      tl_invert_buf,
      tl_pos_neg_table,
      tl_table_answer,
      tl_table_answer_mantissa,
      OUT tl_ofmap_bf16,
      fmt);

  // abs tl_buf3
  // revert and mul to clean !(x == 0 && (y != 0) case
  // add pi/2
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf2, fmt, -1);
  cvk_tiu_min_param_t p3;
  p3.min = tl_buf2;
  p3.a = tl_buf3;
  p3.b_is_const = 0;
  p3.b =  tl_buf2;

  ctx->ops->tiu_min(ctx, &p3);
  cvm_emit_add_const(ctx, tl_buf2, tl_buf2, fmt, 1.0);
  cvm_emit_mul(ctx, tl_ofmap_bf16, tl_buf2, tl_ofmap_bf16, fmt);

  cvm_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, M_PI_2);
  cvm_emit_add(ctx, tl_buf3, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
}
#endif

static void _cvm_atan2_fast_emit_case_3(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x,
                                        cvk_tl_t* tl_buf, cvk_tl_t* tl_y0_buf,
                                        cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                        cvk_tl_t* tl_table_answer,
                                        cvk_tl_t* tl_table_answer_mantissa,
                                        cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt, float b) {
  // case 3
  // atan( y / x)
  return __cvm_atan2_fast_emit_case_3(ctx, y, x, tl_buf, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                                      tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16,
                                      NULL, fmt, b);
}

void cvm_atan2_fast_emit(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                         cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_buf4,
                         cvk_tl_t* tl_y0_buf, cvk_tl_t* tl_slope_buf, cvk_tl_t* tl_invert_buf,
                         cvk_tl_t* tl_pos_neg_table, cvk_tl_t* tl_table_answer,
                         cvk_tl_t* tl_table_answer_mantissa, cvk_tl_t* tl_0_idx_table,
                         cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt) {
  cvm_table_check(y, tl_y0_buf, tl_slope_buf, x);
  cvm_table_check(tl_buf, tl_invert_buf, tl_pos_neg_table, tl_buf2);
  cvm_table_check(tl_buf3, tl_table_answer, tl_table_answer_mantissa, tl_buf4);
  cvm_table_check(tl_buf4, tl_table_answer, tl_0_idx_table, tl_buf4);

  // atan(y/x), x > 0
  // atan(y/x) + PI , x < 0 and y >= 0
  // atan(y/x) - PI , x < 0 and y < 0
  // pi / 2, x = 0 and y > 0
  // -pi / 2, x = 0 and y < 0
  // 0, x = 0 and y = 0

  // atan(y/x), x > 0
  cvm_emit_max_const(ctx, x, tl_buf, fmt, 0.0);
  _cvm_atan2_fast_emit_case_3(ctx, y, tl_buf, tl_buf2, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                              tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16, fmt, 0.0);

  // x > 0
  cvm_emit_mask_gt0(ctx, x, tl_buf, tl_buf2, tl_buf3, tl_pos_neg_table, tl_0_idx_table, tl_buf,
                    fmt);

  cvm_emit_mul(ctx, tl_ofmap_bf16, tl_buf, tl_ofmap_bf16, fmt);

  // atan(y/x) + PI , x < 0 and y >= 0
  cvm_emit_min_const(ctx, x, tl_buf, fmt, 0.0);
  _cvm_atan2_fast_emit_case_3(ctx, y, tl_buf, tl_buf2, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                              tl_table_answer, tl_table_answer_mantissa, tl_buf4, fmt, M_PI);
  // cvm_emit_add_const(ctx, tl_buf4, tl_buf4, fmt, M_PI);

  // get index map that x < 0 and y >= 0
  // !(y >= 0) = !(y < 0)
#if 0
  cvm_emit_pos_idx(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // y == 0
  cvm_emit_0_idx(ctx, y, tl_buf, tl_0_idx_table, tl_buf2, fmt);
  cvm_emit_add(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
#else
  // y >= 0
  cvm_emit_mask_ge0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf2, fmt);
#endif
  // x < 0
  cvm_emit_mask_lt0(ctx, x, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x < 0 && y >= 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul(ctx, tl_buf4, tl_buf2, tl_buf, fmt);
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // atan(y/x) - PI , x < 0 and y < 0
  cvm_emit_min_const(ctx, x, tl_buf, fmt, 0.0);
  _cvm_atan2_fast_emit_case_3(ctx, y, tl_buf, tl_buf2, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                              tl_table_answer, tl_table_answer_mantissa, tl_buf4, fmt, 0.0);
  cvm_emit_add_const(ctx, tl_buf4, tl_buf4, fmt, -1.0 * M_PI);
  // x < 0 and y < 0

  // we leverage x <= 0 and y <= 0 cuz we filter out x = 0 case, speed up it
  // x < 0
  cvm_emit_mask_lt0(ctx, x, tl_buf, tl_pos_neg_table, tl_buf2, fmt);
  // y < 0
  cvm_emit_mask_lt0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x < 0 && y < 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul(ctx, tl_buf4, tl_buf2, tl_buf, fmt);
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // pi / 2, x = 0 and y > 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1

  // y > 0
  // cvm_emit_mask_gt0(ctx, y, tl_buf, tl_buf5, tl_buf4, tl_pos_neg_table, tl_0_idx_table, tl_buf3,
  // fmt);
  _cvm_emit_mask(ctx, y, tl_buf, tl_buf4, NULL, tl_pos_neg_table, tl_0_idx_table, tl_buf3, fmt,
                 CVM_MASK_TYPE_GT_0, true);
  // x = 0 && y > 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, fmt, M_PI / 2.0);

  cvm_emit_add(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // -pi / 2, x = 0 and y < 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1
  // y < 0
  cvm_emit_mask_lt0(ctx, y, tl_buf, tl_pos_neg_table, tl_buf3, fmt);
  // x = 0 && y < 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);

  cvm_emit_mul_const(ctx, tl_buf2, tl_buf2, fmt, -1.0 * M_PI / 2.0);

  cvm_emit_add(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // 0, x = 0 and y = 0
  // x = 0
  cvm_emit_mask_eq0(ctx, x, tl_buf, tl_0_idx_table, tl_buf2, fmt);  // 0.003 could consider 1
  // y = 0
  cvm_emit_mask_eq0(ctx, y, tl_buf, tl_0_idx_table, tl_buf3, fmt);  // 0.003 could consider 1

  // x = 0 && y = 0
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf3, fmt);

  // !(x = 0 and y = 0) keep it
  cvm_emit_0_1_revert_input(ctx, tl_buf3, tl_buf, tl_buf2, fmt);
  cvm_emit_mul(ctx, tl_buf2, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
}

static void _x_lt_0(cvk_context_t* ctx, cvk_tl_t* x, cvk_tl_t* tl_buf, cvk_tl_t* index_i8,
                    cvk_fmt_t fmt, cvk_tl_t* OUT tl_buf2) {
  cvk_tiu_min_param_t p7;
  cvk_tiu_mul_param_t p;
  cvk_tdma_l2l_tensor_copy_param_t p1;

  // x < 0
  p7.min = tl_buf;
  p7.a = x;
  p7.b_is_const = 1;
  p7.b_const.val = 0;
  p7.b_const.is_signed = 1;

  ctx->ops->tiu_min(ctx, &p7);
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, fmt, pow(2, 64));

  p1.src = tl_buf;
  p1.dst = index_i8;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);

  p.res_high = 0;
  p.res_low = index_i8;
  p.a = index_i8;
  p.b_is_const = 1;
  p.b_const.val = -128;
  p.b_const.is_signed = 1;
  p.rshift_bits = 0;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  p.res_high = 0;
  p.res_low = index_i8;
  p.a = index_i8;
  p.b_is_const = 1;
  p.b_const.val = 1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  // get x < 0
  p1.src = index_i8;
  p1.dst = tl_buf2;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);
}

static void _cvm_atan2_merge_emit(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                                  cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_y0_buf,
                                  cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                  cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_mantissa,
                                  cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt, float degree_factor) {
  cvm_table_check(y, tl_y0_buf, tl_invert_buf, x);
  cvm_table_check(tl_buf, tl_invert_buf, tl_pos_neg_table, tl_buf2);
  cvm_table_check(tl_buf3, tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16);

  cvk_tl_t index_i8;
  bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, &index_i8, tl_buf2, CVK_FMT_I8);

  /**
   * step 1. atan(y/x)
   */
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, fmt, 0.0);
  cvm_emit_add(ctx, x, tl_buf, tl_buf, fmt);

#if 0
  // get y < 0, bf16->int8 and mul 0xff to get -128 and righshift to get 1
  cvk_tiu_mul_param_t p;
  cvk_tdma_l2l_tensor_copy_param_t p1;
  cvm_emit_mul_const(ctx, y, tl_buf3, fmt, pow(2,64));
  p1.src = tl_buf3;
  p1.dst = &index_i8;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);


  p.res_high = 0;
  p.res_low = &index_i8;
  p.a = &index_i8;
  p.b_is_const = 1;
  p.b_const.val =-128;
  p.b_const.is_signed = 1;
  p.rshift_bits = 0;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  p.res_high = 0;
  p.res_low = &index_i8;
  p.a = &index_i8;
  p.b_is_const = 1;
  p.b_const.val =1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 1;

  ctx->ops->tiu_mul(ctx, &p);

  // get y < 0
  p1.src = &index_i8;
  p1.dst = tl_buf3;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, -1.0);

  // get y > 0
  // y * (-1) + 1 get 0/1 map, 1 indicate xy == 0
  cvm_emit_add_const(ctx, tl_buf3, tl_buf2, fmt, 1.0);

  // reduce y == 0
  if (0)
  {
    cvk_tiu_max_param_t p3;
    cvk_tl_t index_i8;
    bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, &index_i8, tl_ofmap_bf16, CVK_FMT_I8);
    cvm_emit_mul_const(ctx, y, tl_buf, fmt, -1);
    p3.max = tl_buf;
    p3.a = y;
    p3.b_is_const = 0;
    p3.b =  tl_buf;

    ctx->ops->tiu_max(ctx, &p3);
    cvm_emit_mul_const(ctx, tl_buf, tl_buf, fmt, convert_bf16_fp32(0x7f00));
    //bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, pow(2, 64));

    p1.src = tl_buf;
    p1.dst = &index_i8;
    ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);

    p.res_high = NULL;
    p.res_low = &index_i8;
    p.a = &index_i8;
    p.b_is_const = 1;
    p.b_const.val =-1;
    p.b_const.is_signed = 1;
    p.rshift_bits = 7;
    p.relu_enable = 0;

    ctx->ops->tiu_mul(ctx, &p);


    p1.src = &index_i8;
    p1.dst = tl_buf3;
    ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p1);

    //revert it
    cvm_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, -1.0);
    //bf16_emit_add_const(ctx, tl_buf3, tl_buf3, fmt, 1);
    cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
  }

  cvm_emit_add(ctx, tl_buf2, tl_buf3, tl_buf3, fmt);
#endif

  cvm_emit_x_over_y(ctx, y, tl_buf, NULL, tl_buf2, tl_table_answer, tl_table_answer_mantissa, fmt,
                    true);

  // x0 = atan(y0)
  __cvm_atan_fast_emit(ctx, tl_buf2, tl_buf, tl_buf3, tl_y0_buf, tl_invert_buf, tl_pos_neg_table,
                       tl_table_answer, tl_table_answer_mantissa, OUT tl_ofmap_bf16, fmt);

  bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, &index_i8, tl_buf, CVK_FMT_I8);

  // seperate y >= 0 or < 0 to handle 0 degree / 180 degree
  cvm_emit_mask_ge0_lt0(ctx, y, &index_i8, tl_buf3, fmt);

  /**
   * step 2. set x == 0, y >=0 to pi/2, y < 0 to -pi/2
   * FIXME: atan(0) not eq PI/2
   */

  // x = 0 and y != 0
  // reset all x = 0
  // y >= 0 as pi/2, y < 0 as -pi/2
  // merge

  cvm_emit_mask_eq_0(ctx, x, tl_buf, &index_i8, tl_buf2, fmt);

  // clear x = 0
  cvm_emit_mul_const(ctx, tl_buf2, tl_buf, fmt, -1);
  cvm_emit_mul(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // get revert map, x = -x + 1 cuz original -1 menas x != 0
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf, fmt, M_PI_2 * degree_factor);
  cvm_emit_add_const(ctx, tl_buf2, tl_buf2, fmt, 1);

  cvm_emit_mul(ctx, tl_buf, tl_buf2, tl_buf, fmt);

  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  //  return;
  /**
   *   step 3. handle x < 0 && y != 0
   */

  // x < 0
  _x_lt_0(ctx, x, tl_buf, &index_i8, fmt, tl_buf2);

  // x < 0 && (y >= 1 && y < 1)
  cvm_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf, fmt);
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, fmt, M_PI * degree_factor);
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  /**
   * 4. handle x != 0 && y == 0, x>0: 0, x<0: PI, tpu atan default all pi/2
   */
  // tl_buf2 as x < 0
  // get y == 0, tl_buf3 keep y>=0 is 1, y<1 = -1
  cvm_emit_mask_eq_0(ctx, y, tl_buf, &index_i8, tl_buf3, fmt);
  // revert
  cvm_emit_mul_const(ctx, tl_buf3, tl_buf, fmt, -1.0);

  // reset y = 0 x = ? as 0, other case leave to step 5
  cvm_emit_mul(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  /**
   * 5. set y == 0 and x < 0 as pi
   */

  // get y == 0
  cvm_emit_add_const(ctx, tl_buf3, tl_buf, fmt, 1.0);
  // y == 0 && x < 0
  cvm_emit_mul(ctx, tl_buf, tl_buf2, tl_buf, fmt);
  cvm_emit_mul_const(ctx, tl_buf, tl_buf, fmt, M_PI * degree_factor);

  // merge
  cvm_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
  return;
}

/**
 * \brief reduce lut table with following step
 * 1. atan(y/x)
 * 2. handle x = 0 && y != 0, directly set pi/2, -pi/2
 * 3. handle x < 0 && y != 0
 * => y>0: PI/2, y <0: -PI/2, tpu atan default y>0: -PI/2, y <0: PI/2
 * 4. handle x != 0 && y == 0, x>0: 0, x<0: PI, tpu atan default all pi/2
 * 5. handle x = 0 && y = 0 => PI
 */
void cvm_atan2_merge_emit(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                          cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_y0_buf,
                          cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                          cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_mantissa,
                          cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt) {
  return _cvm_atan2_merge_emit(ctx, y, x, tl_buf, tl_buf2, tl_buf3, tl_y0_buf, tl_invert_buf,
                               tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa,
                               tl_ofmap_bf16, fmt, 1.0);
}

void cvm_atan2_fast_degree_emit(cvk_context_t* ctx, cvk_tl_t* y, cvk_tl_t* x, cvk_tl_t* tl_buf,
                                cvk_tl_t* tl_buf2, cvk_tl_t* tl_buf3, cvk_tl_t* tl_y0_buf,
                                cvk_tl_t* tl_invert_buf, cvk_tl_t* tl_pos_neg_table,
                                cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_mantissa,
                                cvk_tl_t* OUT tl_ofmap_bf16, cvk_fmt_t fmt) {
  return _cvm_atan2_merge_emit(ctx, y, x, tl_buf, tl_buf2, tl_buf3, tl_y0_buf, tl_invert_buf,
                               tl_pos_neg_table, tl_table_answer, tl_table_answer_mantissa,
                               tl_ofmap_bf16, fmt, 180 / M_PI);
}
