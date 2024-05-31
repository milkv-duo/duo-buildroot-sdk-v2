/**
 * plz refer [git](https://github.com/xiezhq-hermann/atan_lookup)
 * input range is `all real numbers` and output range is -pi/2 < x < pi/2,
 * you can refer [here](https://www.mathopenref.com/arctan.html) for more details
 */
//
// xiezhq@shanghaitech.edu.cn && wanghe@shanghaitech.edu.cn
/* Reference:
   [1] Abhisek Ukil, Vishal H Shah, Bernhard Deck,
   "Fast Computation of arctangent Functions for Embedded Applications: A
   Comparative Analysis" IEEE International Symposium on Industrial Electronics,
Pages: 1206 - 1211, DOI: 10.1109/ISIE.2011.5984330, 2011
[2] Sreeraman Rajan, Sichun Wang, Robert Inkol, and Alain Joyal
"Efficient Approximations for the Arctangent Function"
IEEE SIGNAL PROCESSING MAGAZINE [108] MAY 2006
*/

#include "gen_lut.h"
#include <bmkernel/bm1880v2/1880v2_fp_convert.h>

//#define DBG

static double LUT_d[102] = {
  0,                   0.00999966668666524, 0.0199973339731505,  0.0299910048568779,  0.0399786871232900,
  0.0499583957219428,  0.0599281551212079,  0.0698860016346425,  0.0798299857122373,  0.0897581741899505,
  0.0996686524911620,  0.109559526773944,   0.119428926018338,   0.129275004048143,   0.139095941482071,
  0.148889947609497,   0.158655262186401,   0.168390157147530,   0.178092938231198,   0.187761946513593,
  0.197395559849881,   0.206992194219821,   0.216550304976089,   0.226068387993884,   0.235544980720863,
  0.244978663126864,   0.254368058553266,   0.263711834462266,   0.273008703086711,   0.282257421981491,
  0.291456794477867,   0.300605670042395,   0.309702944542456,   0.318747560420644,   0.327738506780556,
  0.336674819386727,   0.345555580581712,   0.354379919123438,   0.363147009946176,   0.371856073848581,
  0.380506377112365,   0.389097231055278,   0.397627991522129,   0.406098058317616,   0.414506874584786,
  0.422853926132941,   0.431138740718782,   0.439360887284591,   0.447519975157170,   0.455615653211225,
  0.463647609000806,   0.471615567862328,   0.479519291992596,   0.487358579505190,   0.495133263468404,
  0.502843210927861,   0.510488321916776,   0.518068528456721,   0.525583793551610,   0.533034110177490,
  0.540419500270584,   0.547740013715902,   0.554995727338587,   0.562186743900029,   0.569313191100662,
  0.576375220591184,   0.583373006993856,   0.590306746935372,   0.597176658092678,   0.603982978252998,
  0.610725964389209,   0.617405891751573,   0.624023052976757,   0.630577757214935,   0.637070329275684,
  0.643501108793284,   0.649870449411948,   0.656178717991395,   0.662426293833151,   0.668613567927821,
  0.674740942223553,   0.680808828915828,   0.686817649758645,   0.692767835397122,   0.698659824721463,
  0.704494064242218,   0.710271007486686,   0.715991114416300,   0.721654850864761,   0.727262687996690,
  0.732815101786507,   0.738312572517228,   0.743755584298860,   0.749144624606017,   0.754480183834406,
  0.759762754875771,   0.764992832710910,   0.770170914020331,   0.775297496812126,   0.780373080066636,
  0.785398163397448,   0.790373246728302
};


void bf16_atan_y0(uint16_t *table_data_y0, bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(is_1880v2_tbl_shape(table_shape));

  int table_hw = bf16_table_hw();

  /**
   * index    0   1    2   3        60 61 62 63 64 65       123 124 125 126
   *--------
   *  exp (2) x  -62  -61 -60   ... -3 -2 -1 0  1  2   .... 60  61  62  63
   *
   * index    128 129 130 131      188 189 190 191 192 193      251 252 253 254 255
   *--------
   *  exp (-2)x   -62 -61 -60  ... -3  -2  -1   0   1   2   ... 60  61  62  63  x
   *
   */

  // [0 102) for > 1
  int lut_sz = sizeof(LUT_d) / sizeof(LUT_d[0]);
  for (int i = 0; i < lut_sz; i++) {
    table_data_y0[i] = convert_fp32_bf16(M_PI_2 - LUT_d[i]);
  }

  // [102 204) for [0 1]
  for (int i = lut_sz; i < lut_sz * 2; i++) {
    table_data_y0[i] = convert_fp32_bf16(LUT_d[i - lut_sz]);
  }

#ifdef DBG
  for (int i = 0; i < lut_sz * 2; i++) {
    printf("y0[%d] is %f(0x%x)\n", i, convert_bf16_fp32(table_data_y0[i]), table_data_y0[i]);
  }
#endif
  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint32_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_data_y0[i * table_hw], &table_data_y0[0], sizeof(uint16_t) * table_hw);
  }
}

void bf16_atan_fast_degree_y0(uint16_t *table_data_y0, bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(is_1880v2_tbl_shape(table_shape));

  int table_hw = bf16_table_hw();

  /**
   * index    0   1    2   3        60 61 62 63 64 65       123 124 125 126
   *--------
   *  exp (2) x  -62  -61 -60   ... -3 -2 -1 0  1  2   .... 60  61  62  63
   *
   * index    128 129 130 131      188 189 190 191 192 193      251 252 253 254 255
   *--------
   *  exp (-2)x   -62 -61 -60  ... -3  -2  -1   0   1   2   ... 60  61  62  63  x
   *
   */

  // [0 102) for > 1
  int lut_sz = sizeof(LUT_d) / sizeof(LUT_d[0]);
  for (int i = 0; i < lut_sz; i++) {
    table_data_y0[i] = convert_fp32_bf16((M_PI_2 - LUT_d[i]) * 180 / M_PI);
  }

  // [102 204) for [0 1]
  for (int i = lut_sz; i < lut_sz * 2; i++) {
    table_data_y0[i] = convert_fp32_bf16(LUT_d[i - lut_sz] * 180 / M_PI);
  }

#ifdef DBG
  for (int i = 0; i < lut_sz * 2; i++) {
    printf("y0[%d] is %f(0x%x)\n", i, convert_bf16_fp32(table_data_y0[i]), table_data_y0[i]);
  }
#endif
  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint32_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_data_y0[i * table_hw], &table_data_y0[0], sizeof(uint16_t) * table_hw);
  }
}

void bf16_atan_slope(uint16_t* OUT table_slope, bmk1880v2_tensor_lmem_shape_t* table_shape) {

  int table_hw = bf16_table_hw();

  int lut_sz = sizeof(LUT_d) / sizeof(LUT_d[0]) - 1;
  for (volatile int i = 0; i < lut_sz; i++) {
    table_slope[i] = convert_fp32_bf16(LUT_d[i+1] - LUT_d[i]);
  }

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_slope[table_hw * i], &table_slope[0], sizeof(uint16_t) * table_hw);
  }
}

// 'bf16_atan_s_01' means atan split [0 1] and (1,
// data in [0-1] mutilply 1, > 1 mutiply with -1
void bf16_atan_s_01(uint16_t* OUT table_invert, bmk1880v2_tensor_lmem_shape_t* table_shape) {
  int half = half_h_table();
  int table_hw = bf16_table_hw();

  // data in [0, 1], mutilply 1
#if 1
  for (uint32_t i = 0; i < 63; i++) {
    table_invert[i] = convert_fp32_bf16(1.0);
    table_invert[i+half] = convert_fp32_bf16(1.0);
  }

  // data > 1
  for (int i = 63; i < half; i++) {
    table_invert[i] = convert_fp32_bf16(-1.0);
    table_invert[i+half] = convert_fp32_bf16(-1.0);
  }
#endif

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_invert[table_hw * i], &table_invert[0], sizeof(uint16_t) * table_hw);
  }
}

// 'pos_neg' means data is positive(>=0) is 1 or negtive(<0) is -1
void bf16_atan_pos_neg(uint16_t* OUT table_pos_neg, bmk1880v2_tensor_lmem_shape_t* table_shape) {

  uint32_t half = half_h_table();
  int table_hw = bf16_table_hw();

  // data >= 0
  for (uint32_t i = 0; i < half; i++) {
    table_pos_neg[i] = convert_fp32_bf16(1.0);
  }

  // data < 0
  for (uint32_t i = half; i < half * 2; i++) {
    table_pos_neg[i] = convert_fp32_bf16(-1.0);
  }

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_pos_neg[table_hw * i], &table_pos_neg[0], sizeof(uint16_t) * table_hw);
  }
}

/* Syntactic sugar for get more precision
 * raw implement code :

 double re_x = 1 / x;
 int index = round(re_x * 100);
 return (M_PI_2 - (LUT_d[index] + (re_x * 100 - index) * (LUT_d[index + 1] - LUT_d[index])));
 and we want to get `(LUT_d[index] + (re_x * 100 - index)` part
 */
int bf16_atan_slope_multipilier(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_buf3,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {

  (void)fmt;

  bf16_get_dec(ctx, tl_buf, tl_buf2, tl_buf3);
  // z = (min(x,y) * 100 - index) * slope(index)

  // fill to 100
  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_buf2;
  p1.a = tl_buf;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(0);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  // add
  bmk1880v2_tiu_element_wise_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_buf2;
  p4.a_high = 0;
  p4.a_low = tl_buf2;
  p4.b_is_const = 1;
  p4.b_high = 0;
  p4.b_const.val = convert_fp32_bf16(-100.0);
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  bmk1880v2_tiu_element_wise_mac_param_t p2;
  p2.res_high = 0;
  p2.res_low = tl_buf3;
  p2.res_is_int8 = 0;
  p2.a = tl_ifmap;
  p2.b_is_const = 0;
  p2.b = tl_buf2;
  p2.lshift_bits = 0;//lshift_bits;
  p2.rshift_bits = 0;//rshift_bits;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(ctx, &p2);

  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_buf3;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(-1.0);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  return 0;
}

/** issue atan >= 0
 *  \b for more precision, we use mac for atan2
 *  if (x > 1) {
 *    x = 1 / x
 *  }
 *  int index = round(x * 100);
 *  double r = (x * 100 - index) * (LUT_d[index + 1] - LUT_d[index]);
 *  double shift = LUT_d[index];
 *  if (x > 1) {
 *    shift = M_PI_2 - LUT_d[index];
 *  }
 *  return r + shift;
 *  FIXME: reduce temp buffer count
 */
int _bf16_atan_emit(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_buf3,
    bmk1880v2_tensor_lmem_t* tl_y0_buf,
    bmk1880v2_tensor_lmem_t* tl_slope_buf,
    bmk1880v2_tensor_lmem_t* tl_invert_buf,
    bmk1880v2_tensor_lmem_t* tl_pos_neg_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt, float b) {

  bf16_table_check(tl_ifmap, tl_y0_buf, tl_slope_buf, tl_ifmap);
  bf16_table_check(tl_buf, tl_invert_buf, tl_pos_neg_buf, tl_buf2);
  bf16_table_check(tl_buf3, tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16);

  // x = abs(x0)
  // y = 1 / x
  // index = 100 * min(x, y)
  // z = (min(x,y) * 100 - index) * slope(index)
  // invert = table_0_102(x) ([0-1] return 1, >1 return -1)
  // invert = invert * z
  // t = 64 * (table_0_102 + 1)
  // shift_index = t(index) ([0-1] return 102, >1 return 0)
  // shift = y0(shift_index + index) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
  // p = table_pos_neg(x0) (>= 0 return 1, < 0 return -1)
  // p(shift + invert * z)

  bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);

  // y = 1 / x
  bf16_emit_reciprocal(ctx,
      tl_buf,
      tl_buf2,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16
      );

  bmk1880v2_tiu_element_wise_min_param_t p7;
  p7.min = tl_ofmap_bf16;
  p7.a = tl_buf;
  p7.b_is_const = 0;
  p7.b = tl_ofmap_bf16;
  bmk1880v2_tiu_element_wise_min(ctx, &p7);

  // get index
  bmk1880v2_tiu_element_wise_mul_param_t p1;
  p1.res_high = NULL;
  p1.res_low = tl_buf;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(100.0);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  bf16_atan_slope_multipilier(ctx, tl_buf, tl_buf2, tl_buf3, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  // get int8 index of x2
  bf16_get_u8_tbl_idx(ctx, tl_buf, tl_buf2);

  // x0 = base[x2] + (0.x * (slope[x2])
  // TODO: use mac

  // get slope[x2]
  bmk1880v2_tiu_lookup_table_param_t p12;
  p12.ofmap = tl_buf3;
  p12.ifmap = tl_buf2;
  p12.table = tl_slope_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  // z = (min(x,y) * 100 - index) * slope(index)
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 0;
  p1.b = tl_buf3;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  // get index from exp,
  // mv_lut_base get exp as index, remove mantissa
  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  p10.dst = tl_buf3;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  // invert = table_0_102(x) ([0-1] return 1, >1 return -1)
  p12.ofmap = tl_buf3;
  p12.ifmap = tl_buf3;
  p12.table = tl_invert_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  // z = invert * z
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 0;
  p1.b = tl_buf3;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  // t = 51 * (invert + 1), -> invert + 1
  bmk1880v2_tiu_element_wise_add_param_t p4;
  p4.res_high = 0;
  p4.res_low = tl_buf3;
  p4.a_high = 0;
  p4.a_low = tl_buf3;
  p4.b_is_const = 1;
  p4.b_high = 0;
  p4.b_const.val = convert_fp32_bf16(1.0);
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  // t = 51 * (invert + 1)
  p1.res_high = NULL;
  p1.res_low = tl_buf3;
  p1.a = tl_buf3;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(51.0);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

#if 1
  // avoid rounding, we first round org index
  bf16_get_u8_tbl_idx(ctx, tl_buf, tl_buf2);
  tl_buf2->fmt = FMT_U8;
  tl_shape_t t = tl_buf2->shape;
  bmk1880v2_tensor_lmem_stride_t s = tl_buf2->stride;
  tl_buf2->shape.h = tl_buf2->shape.h * tl_buf2->shape.w;
  tl_buf2->shape.w = 1;
  tl_buf2->stride.h = 2;
  tl_buf2->stride.c = tl_buf2->shape.h * tl_buf2->shape.w;
  tl_buf2->stride.c = tl_buf2->shape.c * tl_buf2->shape.h * tl_buf2->shape.w;
  p10.dst = tl_buf;
  p10.src = tl_buf2;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = false;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  tl_buf2->fmt = FMT_BF16;
  tl_buf2->shape = t;
  tl_buf2->stride = s;
#else
#endif
  // t = t + index
  p4.res_high = 0;
  p4.res_low = tl_buf3;
  p4.a_high = 0;
  p4.a_low = tl_buf3;
  p4.b_is_const = 0;
  p4.b_high = 0;
  p4.b_low = tl_buf;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  // get int8 index for lut
  bf16_get_u8_tbl_idx(ctx, tl_buf3, tl_buf);

  // shift = y0(t) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
  p12.ofmap = tl_buf3;
  p12.ifmap = tl_buf;
  p12.table = tl_y0_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  // z = base[x2] + (0.x * (slope[x2])
  p4.res_high = 0;
  p4.res_low = tl_buf2;
  p4.a_high = 0;
  p4.a_low = tl_ofmap_bf16;
  p4.b_is_const = 0;
  p4.b_high = 0;
  p4.b_low = tl_buf3;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  // get pos neg, use mv_lut_idx
  p10.dst = tl_buf3;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  // p = table_pos_neg(x0) (>= 0 return 1, < 0 return -1)
  p12.ofmap = tl_buf3;
  p12.ifmap = tl_buf3;
  p12.table = tl_pos_neg_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);
#if 0
  // p * z
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_buf2;
  p1.b_is_const = 0;
  p1.b = tl_buf3;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);
#else

  // add pi/-pi for atan2
  bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 0.0);
  bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, b);

  // p * z + pi
  bmk1880v2_tiu_element_wise_mac_param_t p2;
  p2.res_high = 0;
  p2.res_low = tl_ofmap_bf16;
  p2.res_is_int8 = 0;
  p2.a = tl_buf2;
  p2.b_is_const = 0;
  p2.b = tl_buf3;
  p2.lshift_bits = 0;//lshift_bits;
  p2.rshift_bits = 0;//rshift_bits;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(ctx, &p2);
#endif

  return 0;
}

int bf16_atan_emit(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_buf3,
    bmk1880v2_tensor_lmem_t* tl_y0_buf,
    bmk1880v2_tensor_lmem_t* tl_slope_buf,
    bmk1880v2_tensor_lmem_t* tl_invert_buf,
    bmk1880v2_tensor_lmem_t* tl_pos_neg_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {
  return _bf16_atan_emit(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf2,
      tl_buf3,
      tl_y0_buf,
      tl_slope_buf,
      tl_invert_buf,
      tl_pos_neg_buf,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16,
      fmt, 0.0);
}

/**
 * \table_data_atan_slope is optional, NULL for not assign it
 */
void bf16_atan_tbl(uint16_t *table_data_atan_y0,
    uint16_t* table_data_atan_slope, uint16_t* table_data_atan_invert, uint16_t* table_data_atan_pos_neg,
    bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(table_data_atan_y0);
  //assert(table_data_atan_slope);
  assert(table_data_atan_invert);
  assert(table_data_atan_pos_neg);
  assert(table_shape);

  bf16_atan_y0(table_data_atan_y0, table_shape);
  if (table_data_atan_slope) {
    bf16_atan_slope(table_data_atan_slope, table_shape);
  }
  bf16_atan_s_01(table_data_atan_invert, table_shape);
  bf16_atan_pos_neg(table_data_atan_pos_neg, table_shape);
}

void bf16_atan_fast_degree_tbl(uint16_t *table_data_atan_y0,
    uint16_t* table_data_atan_invert, uint16_t* table_data_atan_pos_neg,
    bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(table_data_atan_y0);
  assert(table_data_atan_invert);
  assert(table_data_atan_pos_neg);
  assert(table_shape);

  bf16_atan_fast_degree_y0(table_data_atan_y0, table_shape);
  bf16_atan_s_01(table_data_atan_invert, table_shape);
  bf16_atan_pos_neg(table_data_atan_pos_neg, table_shape);
}

/** issue atan >= 0
 *  for fast version, we discard slope
 *  tl_y0_buf[0-102) put 'LUT[index]', [102-204) for 'M_PI_2 - LUT[index]'
 */
int _bf16_atan_fast_emit(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_y0_buf,
    bmk1880v2_tensor_lmem_t* tl_invert_buf,
    bmk1880v2_tensor_lmem_t* tl_pos_neg_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt, float b, uint8_t is_dirty_ifmap) {

  bf16_table_check(tl_ifmap, tl_y0_buf, tl_y0_buf, tl_ifmap);
  bf16_table_check(tl_buf, tl_invert_buf, tl_pos_neg_buf, tl_buf);
  bf16_table_check(tl_buf, tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16);

  bmk1880v2_tiu_lookup_table_param_t p12;
  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;

  // plz refer https://github.com/xiezhq-hermann/atan_lookup/blob/master/atan.cpp
  // for faster version
  bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);

  // y = 1 / x
  _bf16_lut_exp_mantissa(ctx,
      tl_buf,
      NULL,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16,
      true
      );

  // once again cuz recipical's input dirtied
  bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);

  bmk1880v2_tiu_element_wise_min_param_t p7;
  p7.min = tl_buf;
  p7.a = tl_buf;
  p7.b_is_const = 0;
  p7.b = tl_ofmap_bf16;
  bmk1880v2_tiu_element_wise_min(ctx, &p7);

  // get index
  bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, 100.0);

  // get index from exp,
  // mv_lut_base get exp as index, remove mantissa
  p10.dst = tl_ofmap_bf16;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  bmk1880v2_tensor_lmem_t* tmp = tl_buf2;
  if (is_dirty_ifmap) {
    tmp = tl_ifmap;
  }

  // get pos neg, use mv_lut_idx
  p10.dst = tmp;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  // p = table_pos_neg(x0) (>= 0 return 1, < 0 return -1)
  p12.ofmap = tmp;
  p12.ifmap = tmp;
  p12.table = tl_pos_neg_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);


  // get index of LUT[index] or (M_PI_2 - LUT[index])
  {

    // invert = table_0_102(x) ([0-1] return 1, >1 return -1)
    p12.ofmap = tl_ofmap_bf16;
    p12.ifmap = tl_ofmap_bf16;
    p12.table = tl_invert_buf;
    bmk1880v2_tiu_lookup_table(ctx, &p12);

    bmk1880v2_tensor_lmem_t *out = tl_buf;
#if 1
    // t = 51 * (invert + 1), -> invert + 1
    bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);

    // t = 51 * (invert + 1)
    bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 51.0);

    // t = t + index
    bf16_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

    // get int8 index for lut
    //bf16_get_u8_tbl_idx(ctx, tl_ofmap_bf16, tl_buf);
    //_bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

    //// shift = y0(t) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
    //p12.ofmap = tl_buf;
    //p12.ifmap = tl_buf;
    //p12.table = tl_y0_buf;
    //bmk1880v2_tiu_lookup_table(ctx, &p12);

    _bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

#else
    // index, output is uint8 format
    _bf16_get_tbl_idx(ctx, tl_buf, tl_buf, FMT_U8, 0);

    // mask value from bf16 -> int8, we add as bf16
    // int8 format (51*(mask + 1) + index) is real remap index for table
    // mask = mask + 1
    bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);

    // mask = 51 * mask
    bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 51.0);

    // mask value change to int8 format for lut
    _bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_ofmap_bf16, FMT_U8, 0);

    // int8 format (51*(mask) + index) is real remap index for table
    if (1)
    {
      bmk1880v2_tensor_lmem_t index_u8, mask_u8, fake_u8, out_u8;
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &index_u8, tl_buf, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &out_u8, tl_buf, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &mask_u8, tl_ofmap_bf16, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &fake_u8, tmp, FMT_U8);
      //fake_u8.start_address =
      //    ctx->chip_info.lmem_size - (fake_u8.shape.h * fake_u8.shape.w);
      //    //tl_buf->start_address + 1;
      //    //tl_ifmap->start_address;

      // mask + index
      // its safe we only need low part value, so we give fake high part

      bmk1880v2_tensor_lmem_t * a = bmk1880v2_lmem_alloc_tensor(
          ctx,
          out_u8.shape,
          FMT_U8, CTRL_NULL);
#if 1
      bmk1880v2_tiu_element_wise_add_param_t p4;
      p4.res_high = 0;
      //p4.res_low = &mask_u8;
      p4.res_low = &index_u8;
      p4.a_high = a;
      p4.a_low = &index_u8;
      p4.b_is_const = 0;
      p4.b_high = a;
      p4.b_low = &mask_u8;
      p4.rshift_bits = 0;
      p4.relu_enable = 0;
      bmk1880v2_tiu_element_wise_add(ctx, &p4);
      //out = tl_ofmap_bf16;
#else
      {
        bmk1880v2_tiu_element_wise_mul_param_t p;
        p.res_high = NULL;
        p.res_low = a;
        p.a = a;
        p.b_is_const = 1;
        p.b_const.val = 0;
        p.b_const.is_signed = 0;
        p.rshift_bits = 0;
        p.relu_enable = 0;
        bmk1880v2_tiu_element_wise_mul(ctx, &p);
      }

      out = tl_ofmap_bf16;
      bmk1880v2_tiu_element_wise_mac_param_t p2;
      p2.res_high = a;
      p2.res_low = &mask_u8;
      p2.res_is_int8 = 0;
      p2.a = &index_u8;
      p2.b_is_const = 1;
      p2.b = 0;
      p2.b_const.val = 1;
      p2.b_const.is_signed = 0;
      p2.lshift_bits = 0;
      p2.rshift_bits = 0;
      p2.relu_enable = 0;
      bmk1880v2_tiu_element_wise_mac(ctx, &p2);
#endif
      bmk1880v2_lmem_free_tensor(
          ctx, a);
    }
    else {
      // move bak to bf16
      //bmk1880v2_tensor_lmem_t index_u8, mask_u8;
      //bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &index_u8, tl_buf, FMT_U8);
      //bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &mask_u8, tl_ofmap_bf16, FMT_U8);

      //p10.dst = tl_buf;
      //p10.src = &index_u8;
      //p10.mv_lut_base = false;
      //p10.mv_lut_idx = false;
      //bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);

      //p10.dst = tl_ofmap_bf16;
      //p10.src = &mask_u8;
      //bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);

      //bf16_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
      //_bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

    }
#endif

    // shift = y0(t) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
    p12.ofmap = out;
    p12.ifmap = out;
    p12.table = tl_y0_buf;
    bmk1880v2_tiu_lookup_table(ctx, &p12);
  }

#if 0
  // add pi/-pi for atan2
  bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 0.0);
  bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, b);

  // p * z + pi
  bmk1880v2_tiu_element_wise_mac_param_t p2;
  p2.res_high = 0;
  p2.res_low = tl_ofmap_bf16;
  p2.res_is_int8 = 0;
  p2.a = tl_buf;
  p2.b_is_const = 0;
  p2.b = tmp;
  p2.lshift_bits = 0;//lshift_bits;
  p2.rshift_bits = 0;//rshift_bits;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(ctx, &p2);
#else
  bf16_emit_mul(ctx, tl_buf, tmp, tl_ofmap_bf16, fmt);
  bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, b);
#endif

  return 0;
}

/**
 * \brief using \tl_buf2 as temp buffer for uint8_t add
 * \NOTICE: it dirties input: \tl_ifmap
 */
int __bf16_atan_fast_emit(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_y0_buf,
    bmk1880v2_tensor_lmem_t* tl_invert_buf,
    bmk1880v2_tensor_lmem_t* tl_pos_neg_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {

  bf16_table_check(tl_ifmap, tl_y0_buf, tl_y0_buf, tl_ifmap);
  bf16_table_check(tl_buf, tl_invert_buf, tl_pos_neg_buf, tl_buf);
  bf16_table_check(tl_buf, tl_table_answer, tl_table_answer_mantissa, tl_ofmap_bf16);

  bmk1880v2_tiu_lookup_table_param_t p12;

  // plz refer https://github.com/xiezhq-hermann/atan_lookup/blob/master/atan.cpp
  // for faster version
  bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);

  // y = 1 / x
  _bf16_lut_exp_mantissa(ctx,
      tl_buf,
      NULL,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16,
      true
      );

  // once again cuz recipical's input dirtied
  bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);

  bmk1880v2_tiu_element_wise_min_param_t p7;
  p7.min = tl_buf;
  p7.a = tl_buf;
  p7.b_is_const = 0;
  p7.b = tl_ofmap_bf16;
  bmk1880v2_tiu_element_wise_min(ctx, &p7);

  // get index
  bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, 100.0);

  // get index from exp,
  // mv_lut_base get exp as index, remove mantissa
#if 1
  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  p10.dst = tl_ofmap_bf16;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;
#else
  bf16_emit_abs(ctx, tl_ifmap, tl_ofmap_bf16, fmt);
  bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 0.5);
#endif

  bmk1880v2_tensor_lmem_t* tmp = tl_buf2;
  tmp = tl_ifmap;

#if 0
  // get pos neg, use mv_lut_idx
  p10.dst = tmp;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false;
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  // p = table_pos_neg(x0) (>= 0 return 1, < 0 return -1)
  p12.ofmap = tmp;
  p12.ifmap = tmp;
  p12.table = tl_pos_neg_buf;
  bmk1880v2_tiu_lookup_table(ctx, &p12);
  //p12.ofmap = tl_ofmap_bf16;
  //p12.ifmap = tmp;
  //p12.table = tl_pos_neg_buf;
  //bmk1880v2_tiu_lookup_table(ctx, &p12);
  //return 0;
#else
  // dirty input is ok
  bmk1880v2_tensor_lmem_t index_i8;
  bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, &index_i8, tl_buf2, FMT_I8);
  bf16_emit_mask_ge0_lt0(ctx, tmp, &index_i8, tmp, fmt);
  //bf16_emit_mask_ge0_lt0(ctx, tmp, &index_i8, tl_ofmap_bf16, fmt);
  //return 0;
#endif

  // get index of LUT[index] or (M_PI_2 - LUT[index])
  {

#if 1
    // invert = table_0_102(x) ([0-1] return 1, >1 return -1)
    p12.ofmap = tl_ofmap_bf16;
    p12.ifmap = tl_ofmap_bf16;
    p12.table = tl_invert_buf;
    bmk1880v2_tiu_lookup_table(ctx, &p12);
#else
    {
      bmk1880v2_tensor_lmem_t index_i8;
      bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, &index_i8, tl_buf2, FMT_I8);
      // 1. abs
      // 2. add 0.5 to round bf16->int8
      // 3. leave (0,1) and others, rightshift 1 to get 0, others
      // 4. saturate to int max, and transform from int8 to bf16

      //bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, convert_bf16_fp32(0x7f00));
      bmk1880v2_tdma_l2l_tensor_copy_param_t p1;
      p1.src = tl_ofmap_bf16;
      p1.dst = &index_i8;
      bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);

      bmk1880v2_tiu_element_wise_mul_param_t p;
      p.res_high = NULL;
      p.res_low = &index_i8;
      p.a = &index_i8;
      p.b_is_const = 1;
      p.b_const.val = 1;
      p.b_const.is_signed = 0;
      p.rshift_bits = 1;
      p.relu_enable = 0;
      bmk1880v2_tiu_element_wise_mul(ctx, &p);

      //p.res_high = NULL;
      //p.res_low = &index_i8;
      //p.a = &index_i8;
      //p.b_is_const = 1;
      //p.b_const.val = -1;
      //p.b_const.is_signed = 1;
      //p.rshift_bits = 7;
      //p.relu_enable = 0;
      //bmk1880v2_tiu_element_wise_mul(ctx, &p);

      p1.src = &index_i8;
      p1.dst = tl_ofmap_bf16;
      bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);
      return 0;

      //bf16_emit_mask_eq_0(ctx, tl_ofmap_bf16, tl_ofmap_bf16, &index_i8, tl_ofmap_bf16, fmt);
      bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 2.0);
      bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);
    }
#endif

    bmk1880v2_tensor_lmem_t *out = tl_buf;
#if 0
    // t = 51 * (invert + 1), -> invert + 1
    bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);

    // t = 51 * (invert + 1)
    bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 51.0);

    // t = t + index
    bf16_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

    // get int8 index for lut
    //bf16_get_u8_tbl_idx(ctx, tl_ofmap_bf16, tl_buf);
    //_bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

    //// shift = y0(t) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
    //p12.ofmap = tl_buf;
    //p12.ifmap = tl_buf;
    //p12.table = tl_y0_buf;
    //bmk1880v2_tiu_lookup_table(ctx, &p12);

    _bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

#else
    // index, output is uint8 format
    _bf16_get_tbl_idx(ctx, tl_buf, tl_buf, FMT_U8, 0);

    // mask value from bf16 -> int8, we add as bf16
    // int8 format (51*(mask + 1) + index) is real remap index for table
    // mask = mask + 1
    bf16_emit_add_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 1.0);

    // mask = 51 * mask
    bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 51.0);

    // mask value change to int8 format for lut
    _bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_ofmap_bf16, FMT_U8, 0);

    // int8 format (51*(mask) + index) is real remap index for table
    if (1)
    {
      bmk1880v2_tensor_lmem_t index_u8, mask_u8, fake_u8, out_u8;
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &index_u8, tl_buf, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &out_u8, tl_buf, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &mask_u8, tl_ofmap_bf16, FMT_U8);
      bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &fake_u8, tl_buf2, FMT_U8);
#if 0
      // mask + index
      // its safe we only need low part value, so we give fake high part
      bmk1880v2_tiu_element_wise_add_param_t p4;
      p4.res_high = 0;
      p4.res_low = &index_u8;
      p4.a_high = &fake_u8;
      p4.a_low = &index_u8;
      p4.b_is_const = 0;
      p4.b_high = &fake_u8;
      p4.b_low = &mask_u8;
      p4.rshift_bits = 0;
      p4.relu_enable = 0;
      bmk1880v2_tiu_element_wise_add(ctx, &p4);
#else
      bmk1880v2_tiu_element_wise_mac_param_t p2;
      p2.res_high = &fake_u8;
      p2.res_low = &index_u8;
      p2.res_is_int8 = 0;
      p2.a = &mask_u8;
      p2.b_is_const = 1;
      p2.b = 0;
      p2.b_const.val = 1;
      p2.b_const.is_signed = 0;
      p2.lshift_bits = 0;
      p2.rshift_bits = 0;
      p2.relu_enable = 0;
      bmk1880v2_tiu_element_wise_mac(ctx, &p2);
#endif

    }
    else {
      // move bak to bf16
      //bmk1880v2_tensor_lmem_t index_u8, mask_u8;
      //bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &index_u8, tl_buf, FMT_U8);
      //bmk1880v2_tensor_lmem_s_copy_bf16_8(ctx, &mask_u8, tl_ofmap_bf16, FMT_U8);

      //p10.dst = tl_buf;
      //p10.src = &index_u8;
      //p10.mv_lut_base = false;
      //p10.mv_lut_idx = false;
      //bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);

      //p10.dst = tl_ofmap_bf16;
      //p10.src = &mask_u8;
      //bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);

      //bf16_emit_add(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
      //_bf16_get_tbl_idx(ctx, tl_ofmap_bf16, tl_buf, FMT_U8, 0);

    }
#endif

    // shift = y0(t) ([0-1] return  LUT_d[index], >1 return M_PI_2 - LUT_d[index]
    p12.ofmap = out;
    p12.ifmap = out;
    p12.table = tl_y0_buf;
    bmk1880v2_tiu_lookup_table(ctx, &p12);
  }

  bf16_emit_mul(ctx, tl_buf, tmp, tl_ofmap_bf16, fmt);

  return 0;
}

int bf16_atan_fast_emit(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_y0_buf,
    bmk1880v2_tensor_lmem_t* tl_invert_buf,
    bmk1880v2_tensor_lmem_t* tl_pos_neg_buf,
    bmk1880v2_tensor_lmem_t *tl_table_answer,
    bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt, uint8_t is_dirty_ifmap) {

  return _bf16_atan_fast_emit(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf2,
      tl_y0_buf,
      tl_invert_buf,
      tl_pos_neg_buf,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16,
      fmt, 0.0, is_dirty_ifmap);
}
