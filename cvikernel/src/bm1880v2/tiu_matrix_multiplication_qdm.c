#include "kernel_1880v2.h"

typedef bmk1880v2_tiu_matrix_multiplication_qdm_param_t param_t;

static void check_matrix(ctx_t *ctx, const ml_t *m)
{
  bmk1880v2_tensor_lmem_t t;
  t.start_address = m->start_address;
  t.fmt = m->fmt;
  t.shape.n = m->shape.n;
  t.shape.c = m->shape.c;
  t.shape.h = 1;
  t.shape.w = m->shape.w;
  t.stride.n = m->stride.n;
  t.stride.c = m->stride.c;
  t.stride.h = m->stride.h;
  t.stride.w = 1;

  check_tiu_tensor(&t);
  assert_stride_type_0(ctx, &t);

  uint32_t eu_num = ctx->chip_info.eu_num;
  ASSERT(m->start_address % eu_num == 0);
}

static int is_arith_shift(const param_t *p)
{
  if (p->left->fmt == FMT_I8)
    return 1;
  if (p->right->fmt == FMT_I8)
    return 1;
  if (p->bias && p->bias->fmt == FMT_I8)
    return 1;

  return 0;
}

bmk1880v2_op_t * bmk1880v2_tiu_matrix_multiplication_qdm(ctx_t *ctx, const param_t *p)
{
  const bmk1880v2_matrix_lmem_t *res = p->res;
  const bmk1880v2_matrix_lmem_t *left = p->left;
  const bmk1880v2_matrix_lmem_t *right = p->right;
  const bmk1880v2_matrix_lmem_t *bias = p->bias;

  check_matrix(ctx, res);
  check_matrix(ctx, left);
  check_matrix(ctx, right);
  if (bias)
    check_matrix(ctx, bias);

  ASSERT(p->lshift_bits < 32);
  ASSERT(!(p->relu_enable && p->add_result));
  if(p->ps32_mode & 0x2)
  {
    ASSERT(!p->relu_enable);
    ASSERT(!p->bias);
    ASSERT(!p->rshift_bits);
  }

  uint32_t left_row = left->shape.n;
  uint32_t left_col = left->shape.col;
  uint32_t right_row = right->shape.n;
  uint32_t right_col = right->shape.col;
  uint32_t res_row = res->shape.n;
  uint32_t res_col = res->shape.col;
  ASSERT(left_col == right_row);
  ASSERT(res_col == right_col);
  ASSERT(p->res_is_int8 == 1);

  if(p->ps32_mode)
  {
    ASSERT(!p->add_result);
  }
  else if (p->add_result) {
    ASSERT(res_row == left_row * 2);
    res_row = left_row;
  } else {
    ASSERT(res_row == left_row);
  }

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_FC_FIX8B;
  reg.tsk_opd_num = bias? 3: 2;
  reg.opt_shift_typ = is_arith_shift(p);
  reg.opt_right_shift = p->rshift_bits;
  reg.opt_left_shift = p->lshift_bits;
  reg.opt_relu = p->relu_enable;
  reg.opt_res_add = p->add_result;

  reg.res0_addr = res->start_address;
  reg.opt_res0_int8 = 1;
  reg.opt_res0_sign = matrix_is_signed(res);
  reg.res0_n = res_row;
  reg.res0_c = res->shape.c;
  reg.res0_h = 1;
  reg.res0_w = res->shape.w;
  reg.short_res0_str = 0;  // stride, b_stride calculated by H/W

  reg.opd0_addr = left->start_address;
  reg.opt_opd0_int8 = 1;
  reg.opt_opd0_sign = (left->fmt == FMT_I8);
  reg.opd0_n = left_row;
  reg.opd0_c = left->shape.c;
  reg.opd0_h = 1;
  reg.opd0_w = left->shape.w;
  reg.short_opd0_str = 0;

  reg.opd1_addr = right->start_address;
  reg.opt_opd1_int8 = 1;
  reg.opt_opd1_sign = (right->fmt == FMT_I8);
  reg.opd1_n = right_row;
  reg.opd1_c = right->shape.c;
  reg.opd1_h = 1;
  reg.opd1_w = left_col - left->shape.w * (left->shape.c - 1);
  reg.short_opd1_str = 0;

  reg.ps32_md = p->ps32_mode;
  if (p->ps32_mode > 0)
    reg.res0_b_str = p->res->shape.n * p->res->stride.n;
  if(reg.opd0_c == 1)
    ASSERT(reg.opd0_w == reg.opd1_w);

  // Only enable 32-bit multipler at the final post processing stage
  reg.opt_chl_quan = ((p->ps32_mode == 0) || (p->ps32_mode == 1)) ? 1 : 0;
  reg.quan_m = p->quan_m;

  // 32b bias, determined by b_stride
  if (bias) {
    ASSERT(bias->shape.n == 4);
    ASSERT(bias->shape.c == right->shape.c);
    ASSERT(bias->shape.w == right->shape.w);
    ASSERT(bias->shape.col == right->shape.col);

    reg.opd2_addr = bias->start_address;
    reg.opt_opd2_int8 = 0;
    reg.opt_opd2_sign = (bias->fmt == FMT_I8);
    reg.opd2_n = 1;
    reg.opd2_c = bias->shape.c;
    reg.opd2_h = 1;
    reg.opd2_w = bias->shape.w;
    reg.short_opd2_str = 0;
  }

  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
