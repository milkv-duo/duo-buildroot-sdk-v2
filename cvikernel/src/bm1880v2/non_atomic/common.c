/**
 * \breif common wrap function for lut
 */
#include "gen_lut.h"
#include <bmkernel/bm1880v2/1880v2_fp_convert.h>

void bf16_table_shape(ctx_t *ctx, bmk1880v2_tensor_lmem_shape_t *s) { 
  // MUST valid
  assert(s);

  uint32_t npu_num = ctx->chip_info.npu_num;
  s->n = 1;
  s->c = npu_num;
  s->h = bf16_table_h();
  s->w = bf16_table_w(); // hard code for hw, hw:32x8
}

void bf16_table_check(bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tbl_answer,
  bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16) {

  // MUST valid
  assert(tl_ofmap_bf16);
  assert(tl_ifmap);
  assert(tbl_answer);
  assert(tbl_answer_mantissa);

  // shape should be same
  assert_same_shape(tl_ifmap, tl_ofmap_bf16);

  // TODO table channel should be great equal input

  // currently ONLY support bf16
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tbl_answer->fmt == FMT_BF16);
  assert(tbl_answer_mantissa->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  // table shape should fix
  assert(is_1880v2_tbl_shape(&tbl_answer->shape));
  assert(is_1880v2_tbl_shape(&tbl_answer_mantissa->shape));
}

static void _bf16_table_check(bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_buf,
  bmk1880v2_tensor_lmem_t *tbl_answer,
  bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16) {

  // check table / input / output
  bf16_table_check(tl_ifmap, tbl_answer, tbl_answer_mantissa, tl_ofmap_bf16);

  assert_same_shape_3(tl_ifmap, tl_buf, tl_ofmap_bf16);

  // check buf
  assert(tl_buf);
  assert(tl_buf->fmt == FMT_BF16);

  // TODO: remove assert for -O2

}

int _bf16_lut_exp_mantissa(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_buf,
  bmk1880v2_tensor_lmem_t *tbl_answer,
  bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  uint8_t is_dirty_ifmap
) {
  bmk1880v2_tensor_lmem_t* tmp = tl_buf;
  if (is_dirty_ifmap) {
      tmp = tl_ifmap;
  }

  // check table / input / output
  _bf16_table_check(tl_ifmap, tl_ifmap, tbl_answer, tbl_answer_mantissa, tl_ofmap_bf16);

  // issue lut cmd
  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  // remove low 8 bits by int8 copy with stride
  // <! get index(pow)
  memset(&p10, 0x00, sizeof(bmk1880v2_tdma_l2l_tensor_copy_param_t));
  p10.dst = tl_ofmap_bf16;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false; // MUST init by ifself in soc
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  // <! get f(x0) = 2^(x0*-0.5)
  bmk1880v2_tiu_lookup_table_param_t p12;
  memset(&p12, 0, sizeof(p12));
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = tl_ofmap_bf16;
  p12.table = tbl_answer;
  p12.layer_id = ctx->layer_id;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  // <! get mantissa value
  p12.ofmap = tmp;
  p12.ifmap = tl_ifmap;
  p12.table = tbl_answer_mantissa;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  // sqrt = (2^exp) * mantissa
  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ofmap_bf16;
  p1.b_is_const = 0;
  p1.b = tmp;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  p1.layer_id = ctx->layer_id;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  return 0;
}

int bf16_lut_exp_mantissa(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_buf,
  bmk1880v2_tensor_lmem_t *tbl_answer,
  bmk1880v2_tensor_lmem_t *tbl_answer_mantissa,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
) {
  return _bf16_lut_exp_mantissa(ctx,
      tl_ifmap,
      tl_buf,
      tbl_answer,
      tbl_answer_mantissa,
      tl_ofmap_bf16,
      false);
}

// \int8_rnd_mode 1 is rounding to 0, e.g: 1.3->1, -1.3->-1, -1.5->-2
//                0 is rounding to nearset even, e.g: 1.3->1, -1.3->-1, -1.7->-2
// \return convert bf16 as int8 and locate to lower part
// e.g.: 24 = 0x18 = 1.5* 2^4 = 0x41C0
// bf16_get_tbl_idx(0x41C0,FMT_U8) = 0x0018
void _bf16_get_tbl_idx(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t src_fmt, int int8_rnd_mode) {

  assert((int8_rnd_mode == 0 || int8_rnd_mode == 1) && "only support 2 mode");

  assert_same_shape(tl_ifmap, tl_ofmap_bf16);
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  // get index
  tl_shape_t tl_ofmap_A_idx_int8_shape = {tl_ofmap_bf16->shape.n, tl_ofmap_bf16->shape.c,
    tl_ofmap_bf16->shape.h * tl_ofmap_bf16->shape.w, 1};

  bmk1880v2_tensor_lmem_t dst;
  bmk1880v2_tensor_lmem_s_copy(&dst, tl_ofmap_bf16);
  dst.start_address = tl_ofmap_bf16->start_address;
  dst.fmt = src_fmt;
  dst.shape = tl_ofmap_A_idx_int8_shape;
  dst.stride = bmk1880v2_tensor_lmem_default_stride(ctx, dst.shape, dst.fmt, CTRL_NULL);
  dst.stride.h = dst.stride.h * 2;
  dst.int8_rnd_mode = int8_rnd_mode;

  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  memset(&p10, 0, sizeof(p10));
  p10.dst = &dst;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false; // MUST init by ifself in soc
  p10.mv_lut_idx = false;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
}

void bf16_get_u8_tbl_idx(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
    ) {

  _bf16_get_tbl_idx(ctx, tl_ifmap, tl_ofmap_bf16, FMT_U8, 0);
}

/*
 * \brief get bf16 decimal part, bf16_get_dec(12.3) = 12.0
 * it leverages bf16->int8 get integer and move to bf16
 * \tl_ifmap should be FMT_BF16 format / size
 */
void bf16_get_dec(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
    ) {

  assert_same_shape_3(tl_ifmap, tl_buf, tl_ofmap_bf16);
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  memset(&p10, 0, sizeof(p10));
  bmk1880v2_tensor_lmem_t dst, src;
  bmk1880v2_tensor_lmem_s_copy(&src, tl_ifmap);
  bmk1880v2_tensor_lmem_s_copy(&dst, tl_buf);

  dst.fmt = FMT_I8;
  dst.stride = bmk1880v2_tensor_lmem_default_stride(ctx, dst.shape, 
      dst.fmt, CTRL_AL);

  // bf16 -> int8
  p10.dst = &dst;
  p10.src = &src;
  p10.mv_lut_base = false; // MUST init by ifself in soc
  p10.mv_lut_idx = false;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);

  // int8 -> bf16
  //src.fmt = FMT_I8;
  //tl_shape_t tl_ofmap_A_idx_int8_shape = {tl_ofmap_bf16->shape.n, tl_ofmap_bf16->shape.c,
  //  tl_ofmap_bf16->shape.h * tl_ofmap_bf16->shape.w, 1};
  //src.shape = tl_ofmap_A_idx_int8_shape;
  //src.stride = bmk1880v2_tensor_lmem_default_stride(ctx, src.shape, /*eu_align*/ 1, src.fmt);
  //src.stride.w = 2;

  //tl_shape_t tl_dst_reshape = {tl_ofmap_bf16->shape.n, tl_ofmap_bf16->shape.c,
  //  1, tl_ofmap_bf16->shape.h * tl_ofmap_bf16->shape.w};

  p10.dst = tl_ofmap_bf16;
  p10.src = &dst;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
}

// \return decimal fractions / mantissa_as_idx,
// e.g: bf16_get_dec_fractions(12.341) = 0.341
// NOTICE: we use bf16->i8, the decimal part should be -127 ~ +127
void bf16_get_dec_fractions(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT buf,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
    ) {

  assert_same_shape_3(tl_ifmap, buf, tl_ofmap_bf16);
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  // idx(i8) to bf16 format to sub it
  bf16_get_dec(ctx, tl_ifmap, tl_ofmap_bf16, buf);

  // mantissa part -> sub to get mantissa
  bmk1880v2_tiu_element_wise_sub_param_t p5;
  memset(&p5, 0, sizeof(p5));
  p5.res_high = 0;
  p5.res_low = tl_ofmap_bf16;
  p5.a_high = 0;
  p5.a_low = tl_ifmap;
  p5.b_high = 0;
  p5.b_low = buf;
  p5.rshift_bits = 0;
  bmk1880v2_tiu_element_wise_sub(ctx, &p5);
}

/**
 * \table_shape return table shape under 1880v2 BF16
 * \return table byte size under BF16
 */
uint64_t bf16_lut_tbl_bytesize(ctx_t *ctx,
    bmk1880v2_tensor_lmem_shape_t *table_shape, fmt_t fmt) {

  assert(table_shape);

  int data_type_size = bytesize_of_fmt(fmt);
  bf16_table_shape(ctx, table_shape);
  uint64_t table_size = tl_shape_size(table_shape);

  return table_size * data_type_size;
}

/**
 * \brief f(x) = x*x
 */
int bf16_emit_square(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {

  (void)fmt;
  assert_same_shape(tl_ifmap, tl_ofmap_bf16);
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ifmap;
  p1.b_is_const = 0;
  p1.b = tl_ifmap;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  return 0;
}

/**
 * \brief f(x) = |x|
 * TODO: check tl_ifmap->start_addr != tl_ofmap_bf16->start_addr
 */
int bf16_emit_abs(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {

  (void)fmt;
  assert_same_shape(tl_ifmap, tl_ofmap_bf16);
  assert(tl_ifmap->fmt == FMT_BF16);
  assert(tl_ofmap_bf16->fmt == FMT_BF16);

  // abs it, multiply -1
  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ifmap;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(-1.0);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  // abs it, get max
  bmk1880v2_tiu_element_wise_max_param_t p;
  memset(&p, 0, sizeof(p));
  p.max = tl_ofmap_bf16;
  p.a = tl_ofmap_bf16;
  p.b_is_const = 0;
  p.b = tl_ifmap;
  bmk1880v2_tiu_element_wise_max(ctx, &p);

  return 0;
}

/**
 * \brief pythagoras p(x, y) = pow(x*x + y*y, 0.5)
 * plz refer [here](http://www.themathpage.com/Alg/pythagorean-distance.htm)
 */
int bf16_emit_pythagoras(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* y,
    bmk1880v2_tensor_lmem_t* x,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* tl_buf2,
    bmk1880v2_tensor_lmem_t* tl_sqrt_table_answer,
    bmk1880v2_tensor_lmem_t* tl_sqrt_table_answer_mantissa,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt) {

  // y0 = x * x
  bf16_emit_square(ctx, x, tl_buf, fmt);

#if 1
  // y0 = y0 + y * y
  bmk1880v2_tiu_element_wise_mac_param_t p2;
  memset(&p2, 0, sizeof(p2));
  p2.res_high = 0;
  p2.res_low = tl_buf;
  p2.res_is_int8 = 0;
  p2.a = y;
  p2.b_is_const = 0;
  p2.b = y;
  p2.lshift_bits = 0;//lshift_bits;
  p2.rshift_bits = 0;//rshift_bits;
  p2.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mac(ctx, &p2);
#else
  // y * y
  bf16_emit_square(ctx, y, tl_buf2, fmt);
  // y = x + y
  {
    bmk1880v2_tiu_element_wise_add_param_t p4;
    p4.res_high = 0;
    p4.res_low = tl_buf;
    p4.a_high = 0;
    p4.a_low = tl_buf2;
    p4.b_is_const = 0;
    p4.b_high = 0;
    p4.b_low = tl_buf;
    p4.rshift_bits = 0;
    p4.relu_enable = 0;
    bmk1880v2_tiu_element_wise_add(ctx, &p4);
  }
#endif

  // y0 = sqrt(y0)
  bf16_emit_sqrt(ctx,
      tl_buf,
      tl_buf2,
      tl_sqrt_table_answer,
      tl_sqrt_table_answer_mantissa,
      tl_ofmap_bf16
      );
  return 0;
}

void bf16_gen_0_tbl(uint16_t* OUT table_0,
    bmk1880v2_tensor_lmem_shape_t* table_shape) {

  assert(is_1880v2_tbl_shape(table_shape));

  uint32_t half = half_h_table();
  int table_hw = bf16_table_hw();
  
  table_0[0] = convert_fp32_bf16(1.0);

  for (uint32_t i = 1; i < half * 2; i++) {
    table_0[i] = convert_fp32_bf16(0.0);
  }

#ifdef DBG
  for (uint32_t i = 0; i < 2 * half; i++) {
    printf("lut [%u] is %lf, 0x%x\n", i, convert_bf16_fp32(table_0[i]),
        table_0[i]);
  }
#endif /* ifdef DBG */

  // duplicate channel #1 to #31
  //TODO: tensor copy
  for (uint64_t i = 1; i < table_shape->c; i++) {
    memcpy(&table_0[table_hw * i], &table_0[0], sizeof(uint16_t) * table_hw);
  }
}


/**
 * \brief check which element is 0, return 1 others return 0
 * e.g: input = [0, 1, -1, 2] output [1, 0, 0, 0]
 */
int bf16_emit_0_idx(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* tl_buf,
  bmk1880v2_tensor_lmem_t* tbl_answer,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {

  // check table / input / output
  _bf16_table_check(tl_ifmap, tl_buf, tbl_answer, tbl_answer, tl_ofmap_bf16);

  assert(fmt);

  // TODO: add fmt parameter?
  // abs for \bf16_get_u8_tbl_idx we use bf16->uint8_t
  //bf16_emit_abs(ctx, tl_ifmap, tl_ofmap_bf16, FMT_BF16);
  // TODO check if address == of address
  //bf16_get_u8_tbl_idx(ctx, tl_ofmap_bf16, tl_buf);
  // re-scale 0.xx to x.
  //bf16_emit_mul_const(ctx, tl_ifmap, tl_buf, fmt, 1000);

  // we directly use mantissa as index, try to add mantissa and mul to filter 2's power
  //bf16_emit_abs(ctx, tl_ifmap, tl_buf, fmt);
  //bf16_emit_add_const(ctx, tl_buf, tl_buf, fmt, convert_bf16_fp32(0x7f) + 1);
  //bf16_emit_mul(ctx, tl_ifmap, tl_buf, tl_buf, fmt);


  bmk1880v2_tiu_lookup_table_param_t p12;
  memset(&p12, 0, sizeof(p12));
#if 1
  // NOTICE: we use int8 mul to enlarge 2^n
  bmk1880v2_tensor_lmem_t src, dst;
  bmk1880v2_tensor_lmem_s_copy(&src, tl_ifmap);
  bmk1880v2_tensor_lmem_s_copy(&dst, tl_buf);

  src.fmt = FMT_U8;
  src.shape.w = src.shape.w * 2; // real size
  src.stride = bmk1880v2_tensor_lmem_default_stride(ctx, src.shape, FMT_I8, CTRL_NULL);
  dst.shape = src.shape;
  dst.fmt = src.fmt;
  dst.stride = src.stride;

  bmk1880v2_tiu_element_wise_mul_param_t p;
  memset(&p, 0, sizeof(p));
  p.res_high = NULL;
  p.res_low = &dst;
  p.a = &src;
  p.b_is_const = 1;
  p.b_const.val = 255; // saturate
  p.b_const.is_signed = 0;
  p.rshift_bits = 2; // avoid unnormal
  p.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p);


  // get 2^x and 0
  p12.ofmap = tl_buf;
  p12.ifmap = tl_buf;
  p12.table = tbl_answer;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  //bf16_get_u8_tbl_idx(ctx, tl_buf, tl_ofmap_bf16);
  _bf16_get_tbl_idx(ctx, tl_ifmap, tl_ofmap_bf16, FMT_I8, 0);

  // get 0<x<1 and 0
  // directly consider mantissa as index
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = tl_ofmap_bf16;
  p12.table = tbl_answer;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  bf16_emit_mul(ctx, tl_buf, tl_ofmap_bf16, tl_ofmap_bf16, fmt);
#else

  _bf16_get_tbl_idx(ctx, tl_ifmap, tl_ofmap_bf16, FMT_I8, 1);
  p12.ofmap = tl_ofmap_bf16;
  p12.ifmap = tl_ofmap_bf16;
  p12.table = tbl_answer;
  bmk1880v2_tiu_lookup_table(ctx, &p12);
#endif
  return 0;
}

/**
 * \brief max(x, const)
 * e.g.: x = [1, 2, 3, 4, -1, -2], const = 1 y = [1, 2, 3, 1, 1]
 */
int bf16_emit_max_const(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt, float b) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_max_param_t p;
  memset(&p, 0, sizeof(p));
  p.max = tl_ofmap_bf16;
  p.a = tl_ifmap;
  p.b_is_const = 1;
  p.b_const.val = convert_fp32_bf16(b);
  bmk1880v2_tiu_element_wise_max(ctx, &p);

  return 0;
}

/**
 * \brief min(x, const)
 * e.g.: x = [1, 2, 3, 4, -1, -2], const = 1 y = [1, 1, 1, -1, -2]
 */
int bf16_emit_min_const(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* tl_ifmap,
    bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
    fmt_t fmt, float b) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_min_param_t p7;
  memset(&p7, 0, sizeof(p7));
  p7.min = tl_ofmap_bf16;
  p7.a = tl_ifmap;
  p7.b_is_const = 1;
  p7.b_const.val = convert_fp32_bf16(b);
  p7.b_const.is_signed = 1;
  bmk1880v2_tiu_element_wise_min(ctx, &p7);

  return 0;
}

// pre process pos/neg
static int _bf16_emit_pre_pos_neg(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16
  ) {

  // check table / input / output
  _bf16_table_check(tl_ifmap, tl_buf, tl_pos_neg_table, tl_pos_neg_table, tl_ofmap_bf16);

  //_bf16_get_tbl_idx(ctx, tl_ifmap, tl_buf, FMT_I8);

  // seperate >=0 and < 0
  // p = table_pos_neg(x0) (>= 0 return 1, < 0 return -1)
  bmk1880v2_tdma_l2l_tensor_copy_param_t p10;
  memset(&p10, 0, sizeof(p10));
  p10.dst = tl_buf;
  p10.src = tl_ifmap;
  p10.mv_lut_base = false; // MUST init by ifself in soc
  p10.mv_lut_idx = true;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p10);
  p10.mv_lut_idx = false;

  bmk1880v2_tiu_lookup_table_param_t p12;
  memset(&p12, 0, sizeof(p12));
  p12.ofmap = tl_buf;
  p12.ifmap = tl_buf;
  p12.table = tl_pos_neg_table;
  bmk1880v2_tiu_lookup_table(ctx, &p12);

  return 0;
}

/**
 * \brief check elements are < 0 
 * \tl_pos_neg_table plz refer \bf16_atan_pos_neg
 * e.g: input = [0, 10, 6, -1, 0] output [0, 0, 0, 1, 0]
 */
int bf16_emit_neg_idx(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {

  _bf16_emit_pre_pos_neg(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_ofmap_bf16);

  // sub 1, [1 -1] -> [0 -2]
  bf16_emit_add_const(ctx, tl_buf, tl_buf, fmt, -1.0);

  // abs, [0 -2] -> [0 2]
  bf16_emit_abs(ctx, tl_buf, tl_ofmap_bf16, fmt);

  // mul 1/2 [0 2] -> [0 1]
  bf16_emit_mul_const(ctx, tl_ofmap_bf16, tl_ofmap_bf16, fmt, 0.5);

  return 0;
}

/**
 * \brief check elements are >= 0 
 * \tl_pos_neg_table plz refer \bf16_atan_pos_neg
 * e.g: input = [0, 10, 6, -1, 0] output [0, 1, 1, 0, 0]
 */
int bf16_emit_pos_idx(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {

  _bf16_emit_pre_pos_neg(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_ofmap_bf16);
  
  // add 1, [1 -1] -> [2 0]
  bf16_emit_add_const(ctx, tl_buf, tl_buf, fmt, 1.0);

  // mul 1/2 [2 0] -> [1 0]
  bf16_emit_mul_const(ctx, tl_buf, tl_ofmap_bf16, fmt, 0.5);

  return 0;
}

/**
 * \brief invert 0/1 input
 * e.g: input = [0, 1, 1, 1, 0] output [1, 0, 0, 0, 1]
 */
int _bf16_emit_0_1_revert_input(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt, uint8_t is_dirty_ifmap
  ) {
  // [-1, -1, 0, -1, 0] = sub([0 0 1 0 1], 1)
  // [1, 1, 0, 1, 0] = abs([-1, -1, 0, -1, 0])
  bmk1880v2_tensor_lmem_t *_tl_buf = tl_buf;

  // check buf
  if (is_dirty_ifmap) {
    _tl_buf = tl_ifmap;
  }
  else {
    assert(tl_buf);
    assert(tl_buf->fmt == FMT_BF16);
    assert_same_shape(tl_buf, tl_ofmap_bf16);
  }

  // sub 1,  = add -1
  bf16_emit_add_const(ctx, tl_ifmap, _tl_buf, fmt, -1.0);

  // abs
  bf16_emit_abs(ctx, _tl_buf, tl_ofmap_bf16, fmt);

  return 0;
}

int bf16_emit_0_1_revert_input(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {
  return _bf16_emit_0_1_revert_input(ctx,
      tl_ifmap, tl_buf, tl_ofmap_bf16, fmt, false);
}
/**
 * \brief invert 0/1 value
 * e.g: input = [0, 10, 6, -1, 0] output [1, 0, 0, 0, 1]
 * the step is [0, 10, 6, -1, 0] -> [0, 1, 1, 1, 0] -> [1, 0, 0, 0, 1]
 */
int bf16_emit_0_1_revert(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* tl_buf,
  bmk1880v2_tensor_lmem_t* tbl_answer,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {
  // [-1, -1, 0, -1, 0] = sub([0 0 1 0 1], 1)
  // [1, 1, 0, 1, 0] = abs([-1, -1, 0, -1, 0])

  // check table / input / output
  _bf16_table_check(tl_ifmap, tl_buf, tbl_answer, tbl_answer, tl_ofmap_bf16);

  // check which element is 0, return 1 others return 0
  bf16_emit_0_idx(ctx, tl_ifmap, tl_buf, tbl_answer, tl_ofmap_bf16, fmt);

  bf16_emit_0_1_revert_input(ctx, tl_ofmap_bf16, tl_buf, tl_ofmap_bf16, fmt);

  return 0;
}

// \brief a(tensor) * b(tensor)
int bf16_emit_mul(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap2,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt
  ) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ifmap;
  p1.b_is_const = 0;
  p1.b = tl_ifmap2;
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  return 0;
}

// \brief a(tensor) * b(tensor)
int bf16_emit_add(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap2,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_add_param_t p4;
  memset(&p4, 0, sizeof(p4));
  p4.res_high = 0;
  p4.res_low = tl_ofmap_bf16;
  p4.a_high = 0;
  p4.a_low = tl_ifmap;
  p4.b_is_const = 0;
  p4.b_high = 0;
  p4.b_low = tl_ifmap2;
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  return 0;
}

int bf16_emit_add_const(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt, float b) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_add_param_t p4;
  memset(&p4, 0, sizeof(p4));
  p4.res_high = 0;
  p4.res_low = tl_ofmap_bf16;
  p4.a_high = 0;
  p4.a_low = tl_ifmap;
  p4.b_is_const = 1;
  p4.b_high = 0;
  p4.b_const.val = convert_fp32_bf16(b);
  p4.rshift_bits = 0;
  p4.relu_enable = 0;
  bmk1880v2_tiu_element_wise_add(ctx, &p4);

  return 0;
}

// \brief a(tensor) * b(const)
int bf16_emit_mul_const(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt, float b
  ) {

  (void)fmt;

  bmk1880v2_tiu_element_wise_mul_param_t p1;
  memset(&p1, 0, sizeof(p1));
  p1.res_high = NULL;
  p1.res_low = tl_ofmap_bf16;
  p1.a = tl_ifmap;
  p1.b_is_const = 1;
  p1.b_const.val = convert_fp32_bf16(b);
  p1.rshift_bits = 0;
  p1.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p1);

  return 0;
}

// \brief a(tensor) / b(const)
// NOTICE: it could dirty \y if \is_dirty_ifmap set true
int bf16_emit_x_over_y(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN x,
  bmk1880v2_tensor_lmem_t* IN y,
  bmk1880v2_tensor_lmem_t* IN tl_buf,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  bmk1880v2_tensor_lmem_t *tl_table_answer,
  bmk1880v2_tensor_lmem_t *tl_table_answer_mantissa,
  fmt_t fmt, uint8_t is_dirty_ifmap
  ) {

  bmk1880v2_tensor_lmem_t* tmp = tl_buf;
  if (is_dirty_ifmap) {
    tmp = NULL;
  }

  // y = reciprocal(y)
  _bf16_lut_exp_mantissa(ctx,
      y,
      tmp,
      tl_table_answer,
      tl_table_answer_mantissa,
      tl_ofmap_bf16,
      is_dirty_ifmap
      );

  // x / y = x * (1/y)
  bf16_emit_mul(ctx, x, tl_ofmap_bf16, tl_ofmap_bf16, fmt);

  return 0;
}

int _bf16_emit_mask(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t *tl_buf2,
  bmk1880v2_tensor_lmem_t *tl_buf3,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* tl_0_idx_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt, enum BF16_MASK_TYPE mask,
  uint8_t is_dirty_ifmap) {

  _bf16_table_check(tl_ifmap, tl_buf, tl_pos_neg_table, tl_0_idx_table, tl_ofmap_bf16);
  if (is_dirty_ifmap) {
    assert_same_shape(tl_buf, tl_buf2);
  }
  else {
    assert_same_shape_3(tl_buf, tl_buf2, tl_buf3);
  }


  switch (mask) {
    case BF16_MASK_TYPE_GT_0:
      // x > 0
      {
        // x >= 0
        bf16_emit_pos_idx(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_buf2, fmt);

        bmk1880v2_tensor_lmem_t *out = tl_ofmap_bf16;
        bmk1880v2_tensor_lmem_t *in = tl_ofmap_bf16;
        if (is_dirty_ifmap) {
          // x = 0
          bf16_emit_0_idx(ctx, tl_ifmap, tl_buf, tl_0_idx_table, tl_ofmap_bf16, fmt); // 0.003 could consider 1
          // !(x = 0)
          _bf16_emit_0_1_revert_input(ctx, tl_ofmap_bf16, NULL, tl_buf, fmt, true);
          in = tl_buf;
          out = tl_ofmap_bf16;
        }
        else {
          // x = 0
          bf16_emit_0_idx(ctx, tl_ifmap, tl_buf, tl_0_idx_table, tl_buf3, fmt); // 0.003 could consider 1
          // !(x = 0)
          bf16_emit_0_1_revert_input(ctx, tl_buf3, tl_buf, tl_ofmap_bf16, fmt);
        }

        // x > 0 = (x >= 0 && !(x = 0))
        bf16_emit_mul(ctx, in, tl_buf2, out, fmt);
      }
      break;
    case BF16_MASK_TYPE_GE_0:
      // y >= 0

      bf16_emit_pos_idx(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_ofmap_bf16, fmt);
      break;
    case BF16_MASK_TYPE_EQ_0:
      bf16_emit_0_idx(ctx, tl_ifmap, tl_buf, tl_0_idx_table, tl_ofmap_bf16, fmt); // 0.003 could consider 1
      break;
    case BF16_MASK_TYPE_LT_0:
      // x < 0

      // x < 0
      bf16_emit_neg_idx(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_ofmap_bf16, fmt);
      
      break;
    case BF16_MASK_TYPE_LE_0:
      // x < 0
      bf16_emit_neg_idx(ctx, tl_ifmap, tl_buf, tl_pos_neg_table, tl_ofmap_bf16, fmt);

      // x = 0
      bf16_emit_0_idx(ctx, tl_ifmap, tl_buf, tl_0_idx_table, tl_buf2, fmt); // 0.003 could consider 1

      // x <= 0 = (x < 0 || (x = 0))
      bf16_emit_add(ctx, tl_ofmap_bf16, tl_buf2, tl_ofmap_bf16, fmt);
      break;
    default:
      assert(0 && "not support yet");
  }
  return 0;
}

/**
 * \brief return > 0 mask
 * e.g.: [1 2 -1 0 -3 -4 ] -> [1 1 0 0 0 0]
 */
int bf16_emit_mask_gt0(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t *tl_buf2,
  bmk1880v2_tensor_lmem_t *tl_buf3,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* tl_0_idx_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  return bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf2,
      tl_buf3,
      tl_pos_neg_table,
      tl_0_idx_table,
      tl_ofmap_bf16, fmt, BF16_MASK_TYPE_GT_0);
}

/**
 * \brief return >= 0 mask
 * e.g.: [1 2 -1 0 -3 -4 ] -> [1 1 0 1 0 0]
 */
int bf16_emit_mask_ge0(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  return bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf, // fake
      tl_buf, // fake
      tl_pos_neg_table,
      tl_pos_neg_table, //fake
      tl_ofmap_bf16, fmt, BF16_MASK_TYPE_GE_0);
}


/**
 * \brief return <= 0 mask
 * e.g.: [1 2 -1 0 -3 -4 ] -> [0 0 1 1 0 0]
 */
int bf16_emit_mask_le0(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  return bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf, // fake
      tl_buf, // fake
      tl_pos_neg_table,
      tl_pos_neg_table, //fake
      tl_ofmap_bf16, fmt, BF16_MASK_TYPE_LE_0);
}


/**
 * \brief return = 0 mask
 * e.g.: [1 2 -1 0 -3 -4 ] -> [0 0 0 1 0 0]
 */
int bf16_emit_mask_eq0(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_0_idx_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  return bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf, // fake
      tl_buf, // fake
      tl_0_idx_table, // fake
      tl_0_idx_table,
      tl_ofmap_bf16, fmt, BF16_MASK_TYPE_EQ_0);
}


/**
 * \brief return < 0 mask
 * e.g.: [1 2 -1 0 -3 -4 ] -> [0 0 1 0 1 1]
 */
int bf16_emit_mask_lt0(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt) {

  return bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf, // fake
      tl_buf, // fake
      tl_pos_neg_table,
      tl_pos_neg_table, // fake
      tl_ofmap_bf16, fmt, BF16_MASK_TYPE_LT_0);
}

int bf16_emit_mask(ctx_t *ctx,
  bmk1880v2_tensor_lmem_t* IN tl_ifmap,
  bmk1880v2_tensor_lmem_t *tl_buf,
  bmk1880v2_tensor_lmem_t *tl_buf2,
  bmk1880v2_tensor_lmem_t *tl_buf3,
  bmk1880v2_tensor_lmem_t* tl_pos_neg_table,
  bmk1880v2_tensor_lmem_t* tl_0_idx_table,
  bmk1880v2_tensor_lmem_t* OUT tl_ofmap_bf16,
  fmt_t fmt, enum BF16_MASK_TYPE mask) {

  return _bf16_emit_mask(ctx,
      tl_ifmap,
      tl_buf,
      tl_buf2,
      tl_buf3,
      tl_pos_neg_table,
      tl_0_idx_table,
      tl_ofmap_bf16, fmt, mask, false);
}

// return x >=0 to 1, x < 0 is -1
void bf16_emit_mask_ge0_lt0(
    ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* y,
    bmk1880v2_tensor_lmem_t* index_i8,
    bmk1880v2_tensor_lmem_t* OUT tl_buf3,
    fmt_t fmt
    ) {

  bmk1880v2_tiu_element_wise_mul_param_t p;
  bmk1880v2_tdma_l2l_tensor_copy_param_t p1;
  memset(&p, 0, sizeof(p));
  memset(&p1, 0, sizeof(p1));

  // get y < 0, bf16->int8 and mul 0xff to get -128 and righshift to get 1
  bf16_emit_mul_const(ctx, y, tl_buf3, fmt, pow(2,64));
  p1.src = tl_buf3;
  p1.dst = index_i8;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);


  p.res_high = 0;
  p.res_low = index_i8;
  p.a = index_i8;
  p.b_is_const = 1;
  p.b_const.val = -128;
  p.b_const.is_signed = 1;
  p.rshift_bits = 0;
  p.relu_enable = 1;
  bmk1880v2_tiu_element_wise_mul(ctx, &p);

  p.res_high = 0;
  p.res_low = index_i8;
  p.a = index_i8;
  p.b_is_const = 1;
  p.b_const.val = 1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 1;
  bmk1880v2_tiu_element_wise_mul(ctx, &p);

  // get y < 0 indicate 1
  p1.src = index_i8;
  p1.dst = tl_buf3;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);

  // merge, y >= 0 is 1, y < 0 is -1
  bf16_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, -2.0);
  bf16_emit_add_const(ctx, tl_buf3, tl_buf3, fmt, 1.0);

#if 0
  bf16_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, -1.0);

  // get y > 0
  // y * (-1) + 1 get 0/1 map, 1 indicate xy == 0
  bf16_emit_add_const(ctx, tl_buf3, tl_buf2, fmt, 1.0);

  // reduce y == 0
  if (0)
  {
    bmk1880v2_tiu_element_wise_max_param_t p3;
    bmk1880v2_tensor_lmem_t index_i8;
    bmk1880v2_tensor_lmem_s_copy_l2l_bf16_8(ctx, index_i8, tl_ofmap_bf16, FMT_I8);
    bf16_emit_mul_const(ctx, y, tl_buf, fmt, -1);
    p3.max = tl_buf;
    p3.a = y;
    p3.b_is_const = 0;
    p3.b = tl_buf;
    bmk1880v2_tiu_element_wise_max(ctx, &p3);
    bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, convert_bf16_fp32(0x7f00));
    //bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, pow(2, 64));

    p1.src = tl_buf;
    p1.dst = index_i8;
    bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);

    p.res_high = NULL;
    p.res_low = index_i8;
    p.a = index_i8;
    p.b_is_const = 1;
    p.b_const.val = -1;
    p.b_const.is_signed = 1;
    p.rshift_bits = 7;
    p.relu_enable = 0;
    bmk1880v2_tiu_element_wise_mul(ctx, &p);


    p1.src = index_i8;
    p1.dst = tl_buf3;
    bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);

    //revert it
    bf16_emit_mul_const(ctx, tl_buf3, tl_buf3, fmt, -1.0);
    //bf16_emit_add_const(ctx, tl_buf3, tl_buf3, fmt, 1);
    bf16_emit_mul(ctx, tl_buf2, tl_buf3, tl_buf2, fmt);
  }

  bf16_emit_add(ctx, tl_buf2, tl_buf3, tl_buf3, fmt);
#endif
}

/*
 * \return -1 means others, 0 indicate 0
 */
void bf16_emit_mask_eq_0(ctx_t *ctx,
    bmk1880v2_tensor_lmem_t* y,
    bmk1880v2_tensor_lmem_t* tl_buf,
    bmk1880v2_tensor_lmem_t* index_i8,
    bmk1880v2_tensor_lmem_t* OUT tl_buf3,
    fmt_t fmt
    ) {

  bmk1880v2_tdma_l2l_tensor_copy_param_t p1;
  bmk1880v2_tiu_element_wise_mul_param_t p;
  memset(&p1, 0, sizeof(p1));
  memset(&p, 0, sizeof(p));

  bf16_emit_abs(ctx, y, tl_buf, fmt);
  //bf16_emit_mul_const(ctx, y, tl_buf, fmt, -1);
  //bmk1880v2_tiu_element_wise_max_param_t p3;
  //p3.max = tl_buf;
  //p3.a = y;
  //p3.b_is_const = 0;
  //p3.b = tl_buf;
  //bmk1880v2_tiu_element_wise_max(ctx, &p3);
  bf16_emit_mul_const(ctx, tl_buf, tl_buf, fmt, convert_bf16_fp32(0x7f00));

  p1.src = tl_buf;
  p1.dst = index_i8;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);

  p.res_high = NULL;
  p.res_low = index_i8;
  p.a = index_i8;
  p.b_is_const = 1;
  p.b_const.val = -1;
  p.b_const.is_signed = 1;
  p.rshift_bits = 7;
  p.relu_enable = 0;
  bmk1880v2_tiu_element_wise_mul(ctx, &p);

  p1.src = index_i8;
  p1.dst = tl_buf3;
  bmk1880v2_tdma_l2l_bf16_tensor_copy(ctx, &p1);
}

