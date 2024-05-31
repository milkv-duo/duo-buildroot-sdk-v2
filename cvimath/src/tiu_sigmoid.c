/**
 * implement Linear interpolation search
 *
 * we need to pass 2 table, one is answer(lut_answer), another is slope with
 * anwser(lut_answer_slope),
 *
 * for example, we want to get x value
 * +------+----+
 * x0     x    x1
 *
 * the [Linear interpolation defined] (https://en.wikipedia.org/wiki/Linear_interpolation) as
 * flowing:
 *
 * part C  part A                     part B
 * +--+    +---+           +----------------------------------------+
 *
 * p(x) =  f(x0)     +     ( (f(x1) - f(x0)) / (x1 - x0) ) * (x - x0)
 *
 *         +---+           +-----------------------------+
 *        lut_answer              lut_answer_slope
 */

#include <cvimath_internal.h>
#include "gen_lut.h"  // NOLINT

//#define DBG
/*
 * NOTICE: it could occupy 2 lookup table size which shape is <1,32,32,8> with bf16 data type
 *
 * \tl_buf tmp buffer, the shape MUST be same with \tl_ifmap
 * \tl_ofmap_bf16 result as bf16, MUST given for tmp buffer used
 */
int cvm_emit_sigmoid(cvk_context_t* ctx, cvk_tl_t* IN tl_ifmap, cvk_tl_t* IN tl_buf,
                     cvk_tl_t* tl_table_answer, cvk_tl_t* tl_table_answer_slope,
                     cvk_tl_t* OUT tl_ofmap_bf16, float scale) {
  cvm_table_check(tl_ifmap, tl_table_answer, tl_table_answer_slope, tl_buf);

  cvk_fmt_t fmt = CVK_FMT_BF16;

  cvk_tl_shape_t tl_ofmap_A_idx_int8_shape = {1, tl_buf->shape.c, tl_buf->shape.h * tl_buf->shape.w,
                                              1};

  cvk_tdma_l2l_tensor_copy_param_t p10;

  // scale input for remap its idx(-x~x) to (-127~127), dirty tl_ifmap
  cvk_tiu_mul_param_t p1;
  p1.res_high = NULL;
  p1.res_low = tl_ifmap;
  p1.a = tl_ifmap;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(scale);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;

  ctx->ops->tiu_mul(ctx, &p1);

  // <! get idx from bf16->int8
  // save by stride
  memset(&p10, 0x00, sizeof(cvk_tdma_l2l_tensor_copy_param_t));
  cvk_tl_t dst;
  memcpy(&dst, tl_ofmap_bf16, sizeof(cvk_tl_t));
  dst.fmt = CVK_FMT_I8;
  dst.shape = tl_ofmap_A_idx_int8_shape;
  // dst.stride = ctx->ops->tl_default_stride(ctx, dst.shape, /*eu_align*/ 1,
  // dst.fmt);
  dst.stride = ctx->ops->tl_default_stride(ctx, dst.shape, dst.fmt, CTRL_NULL);
  dst.stride.h = dst.stride.h * 2;
  dst.int8_rnd_mode = 1;
  p10.dst = &dst;
  p10.src = tl_ifmap;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p10);
  dst.int8_rnd_mode = 0;  // reset

  // <! int8 to fb16 format cus for sub use, sub MUST in the same format
  memset(&p10, 0x00, sizeof(cvk_tdma_l2l_tensor_copy_param_t));
  p10.dst = tl_buf;  //<! bf16
  p10.src = &dst;
  ctx->ops->tdma_l2l_bf16_tensor_copy(ctx, &p10);

  // <! sub, diff base , a - b
  // (x - x0)
  cvk_tiu_sub_param_t p5;
  p5.res_high = 0;
  p5.res_low = tl_ifmap;
  p5.a_high = 0;
  p5.a_low = tl_ifmap;
  p5.b_high = 0;
  p5.b_low = tl_buf;
  p5.rshift_bits = 0;

  ctx->ops->tiu_sub(ctx, &p5);

  // get f(x0) and slope(x)
  // reshape, 16->16
  dst.fmt = fmt;
  dst.shape = tl_buf->shape;
  dst.stride = tl_buf->stride;

  // <! get slope by index
  // <! ( (f(x1) - f(x0)) / (x1 - x0) )
  // <! TIU MUST with same shape and stride, we leverage output map shape and stride
  cvk_tiu_lookup_table_param_t p12;
  memset(&p12, 0x0, sizeof(cvk_tiu_lookup_table_param_t));
  p12.ofmap = tl_buf;
  p12.ifmap = &dst;
  p12.table = tl_table_answer_slope;
  ctx->ops->tiu_lookup_table(ctx, &p12);

  // base f(x0)
  memset(&p12, 0x0, sizeof(cvk_tiu_lookup_table_param_t));
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = &dst;
  p12.table = tl_table_answer;
  ctx->ops->tiu_lookup_table(ctx, &p12);

  // <! mac
  // <! part A + part B, a *.b.b + res = res
  cvk_tiu_mac_param_t p2;
  p2.res_high = 0;
  p2.res_low = tl_ofmap_bf16;
  p2.res_is_int8 = 0;
  p2.a = tl_ifmap;
  p2.b_is_const = 0;
  p2.b = tl_buf;
  p2.lshift_bits = 0;  // lshift_bits;
  p2.rshift_bits = 0;  // rshift_bits;
  p2.relu_enable = 0;

  ctx->ops->tiu_mac(ctx, &p2);
  return 0;
}

static double _gen_sigmoid(float x) { return 1.0 / (1.0 + exp(-(x))); }

double* cvm_gen_sigmoid_double() {
  int table_hw = cvm_table_hw();
  return (double*)malloc(sizeof(double) * table_hw);
}

void cvm_free_sigmoid_double(double* sigmode_hw) { free(sigmode_hw); }

void cvm_gen_sigmoid(uint16_t* table_data, cvk_tl_shape_t* table_shape, double* sigmode_hw,
                     float scale, int range_start) {
  // S(x) = 1 / (1 + (e^-x))
  //<! 32*8 table, duplicate `channel` times;
  uint64_t idx = 0;
  ASSERT(is_1880v2_tbl_shape(table_shape));

  int half = half_h_table();
  int table_hw = cvm_table_hw();

  // prepare channel 0
  // x [0, 127]
  // we re-scale [-8, 8] into 256
  for (int i = 0; i < half; i++) {
    float _idx = idx / scale;
    double s = _gen_sigmoid(_idx);
    sigmode_hw[idx] = s;
    table_data[idx] = convert_fp32_bf16((float)s);
#ifdef GDB
    printf("t [%lu] is %f[%d], 0x%x fp is %f d is %.8lf, input is %f\n", idx,
           convert_bf16_fp32(table_data[idx]), i, table_data[idx], (float)s, s, _idx);
#endif
    idx++;
  }

  // x = -128
  double s = _gen_sigmoid(range_start);
  sigmode_hw[idx] = s;
  table_data[idx] = convert_fp32_bf16((double)s);
#ifdef GDB
  printf("t [%lu] is %f[%d], 0x%x fp is %f d is %.8lf input is %d\n", idx,
         convert_bf16_fp32(table_data[idx]), -128, table_data[idx], (float)s, s, range_start);
#endif
  idx++;

  // x [-128~-1], 2's complement
  for (int i = 1; i < half; i++) {
    float _idx = (i) / scale;
    double s = _gen_sigmoid(range_start + _idx);
    sigmode_hw[idx] = s;
    table_data[idx] = convert_fp32_bf16((double)s);
#ifdef GDB
    printf("t [%lu] is %f[%d], 0x%x fp is %f d is %.8lf input is %f\n", idx,
           convert_bf16_fp32(table_data[idx]), -127 + i, table_data[idx], (float)s, s,
           range_start + _idx);
#endif
    idx++;
  }

  // duplicate channel #1 to #31

  // TODO: tensor copy
  for (uint32_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(uint16_t) * table_hw);
  }
}

float cvm_sigmoid_scale(int range_start, int range_end) {
  int table_hw = cvm_table_hw();
  return table_hw / (1.0 * abs(range_start - range_end));  // 256 / 16 = 16
}

void cvm_gen_sigmoid_slope(uint16_t* OUT table_slope, cvk_tl_shape_t* table_shape,
                           double* sigmode_hw, float scale, int range_start, int range_end) {
  ASSERT(is_1880v2_tbl_shape(table_shape));

  int half = half_h_table();
  int table_hw = cvm_table_hw();

  for (int i = 0; i < table_hw; i++) {
    double x0 = sigmode_hw[i];
    // double x1 = sigmode_hw[i + 1];
    double x1;
    double delta = 1.0;
    if (i == half - 1) {
      //<! slope[127] means f(127)~f(128)
      double f = _gen_sigmoid(range_end);
      // uint16_t bf16 = convert_fp32_bf16(f);
      // x1 = convert_bf16_fp32(bf16);
      x1 = f;
    } else if (i == half) {
      // 128 index mean x1 is -129 and x0 is -128
      x1 = _gen_sigmoid(range_start - 1 / scale);
      delta = -1.0;
    } else if (i > half) {
      x0 = sigmode_hw[i];
      x1 = sigmode_hw[i - 1];
      delta = -1.0;
    } else {
      // for avoid -fsanitize=address check
      x1 = sigmode_hw[i + 1];
    }
    double s = (x1 - x0) / delta;  // x1 already scale up
    table_slope[i] = convert_fp32_bf16((float)s);
#ifdef GDB
    printf("slope table [%u] = (bf16 %f double %.8lf float %f), 0x%x, %.8lf - %.8lf(%.8lf)\n", i,
           convert_bf16_fp32(table_slope[i]), s, (float)s, table_slope[i], x1, x0, x1 - x0);
#endif
  }

  // duplicate channel #1 to #31

  // TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_slope[table_hw * i], &table_slope[0], sizeof(uint16_t) * table_hw);
  }
}

void cvm_sigmoid_tbl(uint16_t* sigmoid_table_data, uint16_t* sigmoid_table_data_slope,
                     cvk_tl_shape_t* table_shape, int range_start, int range_end) {
  ASSERT(sigmoid_table_data);
  ASSERT(sigmoid_table_data_slope);
  ASSERT(table_shape);

  double* sigmode_hw = cvm_gen_sigmoid_double();

  float scale = cvm_sigmoid_scale(range_start, range_end);

  cvm_gen_sigmoid(sigmoid_table_data, table_shape, sigmode_hw, scale, range_start);

  cvm_gen_sigmoid_slope(sigmoid_table_data_slope, table_shape, sigmode_hw, scale, range_start,
                        range_end);

  cvm_free_sigmoid_double(sigmode_hw);
}
