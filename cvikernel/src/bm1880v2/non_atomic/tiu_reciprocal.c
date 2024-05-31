/**
 */
#include "gen_lut.h"
#include <bmkernel/bm1880v2/1880v2_fp_convert.h>

//#define DBG

/*
 * NOTICE: it could occupy 2 lookup table size which shape is <1,32,32,8> with bf16 data type
 *
 * \tl_buf tmp buffer, the shape MUST be same with \tl_ifmap
 * \tl_ofmap_bf16 result as bf16, MUST given for tmp buffer used
 */
int bf16_emit_reciprocal(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_buf,
  bmk1880v2_tensor_lmem_t *tbl_answer,
  bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
  ) {

  return bf16_lut_exp_mantissa(ctx,
      tl_ifmap,
      tl_buf,
      tbl_answer,
      tbl_answer_mantissa,
      tl_ofmap_bf16
      );
}

// <! gen reciprocal f(x) = 1/x
static double _gen_reciprocal(int base, int p) {
  // y = x ^ -1
  double f = (double) (pow(base, -1 * p));

  if (isnan(f)) {
    assert(0);
  }
  return f;
}


void bf16_gen_reciprocal(uint16_t *table_data, bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(is_1880v2_tbl_shape(table_shape));

  int exp_start = bf16_exp_start();
  int half = half_h_table();
  int table_hw = bf16_table_hw();
  uint64_t idx = 0;

  // prepare channel 0
  double s = 0.0;
  // 0^-1 is invalid, use positive/negtive max value: 0x7F7F / 0xFF7F
  //table_data[idx] = 0xff7f; //<! convert to 0xff7f, mulitply slope[0](0.5) is feff
  table_data[idx] = 0x7F80; //<! convert to 0x7F7F
#ifdef DBG
    printf("t [%lu] is %f  bf %x\n", idx,
        convert_bf16_fp32(table_data[idx]),
        table_data[idx]);
#endif
  idx++;

  // > 0, exp from 0 -62 -61 ..  62  63
  for (int i = 0; i < half - 1; i++) {
    int shift = (exp_start + i);
    uint8_t is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = exp - 1;
    }

    double s = _gen_reciprocal(2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%lu] is %f [idx:%f][2^%f] bf %x\n", idx,
        convert_bf16_fp32(table_data[idx]),
        (float)(exp_start + i), -1 * exp,
        table_data[idx]);
#endif
    idx++;
  }

  s = _gen_reciprocal(2, -0);
  table_data[idx] = convert_fp32_bf16(s);
  table_data[idx] = 0x7F80; //<! convert to 0x7F7F
#ifdef DBG
  printf("t [%lu] is %f[%d] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), 0, table_data[idx]);
#endif
  idx++;

  // < 0, exp from 0 -62 -61 ..  62  63
  for (int i = 0; i < half - 1; i++) {
    int shift = (exp_start + i);
    uint8_t is_odd = (shift % 2);
    float exp = shift;
    if (is_odd) {
      exp = exp - 1;
    }

    double s = -1 * _gen_reciprocal(-2, exp);
    table_data[idx] = convert_fp32_bf16(s);
#ifdef DBG
    printf("t [%lu] is %f(%e - %.8lf)[(-2)^%f] bf %x\n", idx, convert_bf16_fp32(table_data[idx]), s, s, exp, table_data[idx]);
#endif
    idx++;
  }

  // idx = 255 dont care
  //s = _gen_reciprocal(2, 0);
  //table_data[idx] = convert_fp32_bf16(s);
  //printf("t [%lu] is %f[%d]\n", idx, convert_bf16_fp32(table_data[idx]), 0);
  //idx++;

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint32_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_data[i * table_hw], &table_data[0], sizeof(uint16_t) * table_hw);
  }
}

void bf16_gen_reciprocal_mantissa(uint16_t* OUT table_mantissa,
    bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(is_1880v2_tbl_shape(table_shape));

  uint32_t half = half_h_table();
  int table_hw = bf16_table_hw();
  
  int idx = 0;
  double d;
  for (uint32_t i = 0; i < half; i++) {
    d = 1 + i * 1 / 128.0;
    d = (double) pow(d, -1);
    table_mantissa[128+idx] = convert_fp32_bf16(d);

    //13=2^3x1.625=(2^2)x(2^1x1.625)
    d = 2 * (1 + i * 1 / 128.0);
    d = (double) pow(d, -1);
    table_mantissa[idx] = convert_fp32_bf16(d);
    idx++;
  }

#ifdef DBG
  for (uint32_t i = 0; i < 2 * half; i++) {
    printf("mantissa [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_mantissa[i]),
        table_mantissa[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_mantissa[table_hw * i], &table_mantissa[0], sizeof(uint16_t) * table_hw);
  }
}

void bf16_reciprocal_tbl(uint16_t *table_data, uint16_t* table_mantissa,
    bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(table_data);
  assert(table_mantissa);
  assert(table_shape);

  bf16_gen_reciprocal(table_data, table_shape);
  bf16_gen_reciprocal_mantissa(table_mantissa, table_shape);
}
