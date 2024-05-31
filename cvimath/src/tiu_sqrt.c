/**
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
int cvm_emit_sqrt(cvk_context_t* ctx, cvk_tl_t* IN tl_ifmap, cvk_tl_t* IN tl_buf,
                  cvk_tl_t* tbl_answer, cvk_tl_t* tbl_answer_mantissa,
                  cvk_tl_t* OUT tl_ofmap_bf16) {
  return cvm_lut_exp_mantissa(ctx, tl_ifmap, tl_buf, tbl_answer, tbl_answer_mantissa,
                              tl_ofmap_bf16);
}

static double _gen_sqrt(int base, int p) {
  // y = x ^ 0.5
  double f = (double)(pow(base, p * 0.5));

  if (isnan(f)) {
    ASSERT(0);
  }
  return f;
}

void cvm_gen_sqrt(uint16_t* table_data, cvk_tl_shape_t* table_shape) {
  ASSERT(is_1880v2_tbl_shape(table_shape));

  int exp_start = cvm_exp_start();
  int half = half_h_table();
  int table_hw = cvm_table_hw();
  uint64_t idx = 0;

  // prepare channel 0
  double s = 0.0;
  table_data[idx] = convert_fp32_bf16(s);  // 0^0.5 = 0
#ifdef DBG
  printf("t [%lu] is %f(%.8lf)[idx:%f][2^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s,
         (float)exp_start, (float)(exp_start / 2), table_data[idx]);
#endif
  idx++;

  // > 0, exp from 0 -62 -61 ..  62  63
  for (int i = 0; i < half; i++) {
    int shift = (exp_start + i);
    bool is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = exp - 1;
    }

    double s = _gen_sqrt(2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%lu] is %f [idx:%f][2^%f(%f)] bf %x\n", idx, convert_bf16_fp32(table_data[idx]),
           (float)(exp_start + i), exp / 2, (exp_start + i) / 2.0, table_data[idx]);
#endif
    idx++;
  }

  //// idx = 127 dont care
  // duplicate channel #1 to #channel
  // TODO: tensor copy

  for (uint32_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(uint16_t) * table_hw);
  }
}

void cvm_gen_sqrt_mantissa(uint16_t* OUT table_mantissa, cvk_tl_shape_t* table_shape) {
  ASSERT(is_1880v2_tbl_shape(table_shape));

  uint32_t half = half_h_table();
  int table_hw = cvm_table_hw();

  int idx = 0;
  double d;
  for (uint32_t i = 0; i < half; i++) {
    d = 1 + i * 1 / 128.0;
    d = (double)pow(d, 0.5);
    table_mantissa[128 + idx] = convert_fp32_bf16(d);
#ifdef DBG
    // printf(", [%u] is %lf\n", i+128, d);
#endif /* ifdef DBG */

    // 13=2^3x1.625=(2^2)x(2^1x1.625)
    d = 2 * (1 + i * 1 / 128.0);
    d = (double)pow(d, 0.5);
    table_mantissa[idx] = convert_fp32_bf16(d);
#ifdef DBG
    // printf("mantissa [%u] is %lf", i, d);
#endif /* ifdef DBG */
    idx++;
  }
#ifdef DBG
  for (uint32_t i = 0; i < 2 * half; i++) {
    printf("mantissa [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_mantissa[i]),
           table_mantissa[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  // TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_mantissa[table_hw * i], &table_mantissa[0], sizeof(uint16_t) * table_hw);
  }
}

void cvm_sqrt_tbl(uint16_t* sqrt_table_data, uint16_t* sqrt_table_data_mantissa,
                  cvk_tl_shape_t* table_shape) {
  ASSERT(sqrt_table_data);
  ASSERT(sqrt_table_data_mantissa);
  ASSERT(table_shape);

  cvm_gen_sqrt(sqrt_table_data, table_shape);
  cvm_gen_sqrt_mantissa(sqrt_table_data_mantissa, table_shape);
}
