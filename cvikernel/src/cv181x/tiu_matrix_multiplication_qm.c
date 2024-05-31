#include "cvkcv181x.h"

static int8_t check_matrix(cvk_context_t *ctx, const cvk_ml_t *m)
{
  int8_t status = 0;
  cvk_tl_t t;
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

  status |= check_tiu_tensor(&t);
  status |= check_stride_type_0(ctx, &t);

  uint32_t eu_num = ctx->info.eu_num;
  CHECK(status, m->start_address % eu_num == 0);

  return status;
}

static int is_arith_shift(const cvk_tiu_matrix_multiplication_qm_param_t *p)
{
  if (p->left->fmt == CVK_FMT_I8)
    return 1;
  if (p->right->fmt == CVK_FMT_I8)
    return 1;
  if (p->bias && p->bias->fmt == CVK_FMT_I8)
    return 1;

  return 0;
}

void cvkcv181x_tiu_matrix_multiplication_qm(cvk_context_t *ctx, const cvk_tiu_matrix_multiplication_qm_param_t *p)
{
  int8_t status = 0;
  const cvk_ml_t *res = p->res;
  const cvk_ml_t *left = p->left;
  const cvk_ml_t *right = p->right;
  const cvk_ml_t *bias = p->bias;

  status |= check_matrix(ctx, res);
  status |= check_matrix(ctx, left);
  status |= check_matrix(ctx, right);
  if (bias)
    status |= check_matrix(ctx, bias);

  CHECK(status, p->lshift_bits < 32);
  CHECK(status, !(p->relu_enable && p->add_result));
  if(p->ps32_mode & 0x2)
  {
    CHECK(status, !p->relu_enable);
    CHECK(status, !p->bias);
    CHECK(status, !p->rshift_bits);
  }
  CHECK(status, p->relu_enable == 0 || p->relu_enable == 1);

  uint32_t left_row = left->shape.n;
  uint32_t left_col = left->shape.col;
  uint32_t right_row = right->shape.n;
  uint32_t right_col = right->shape.col;
  uint32_t res_row = res->shape.n;
  uint32_t res_col = res->shape.col;
  CHECK(status, left_col == right_row);
  CHECK(status, res_col == right_col);
  CHECK(status, p->res_is_int8 == 1);

  if(p->ps32_mode)
  {
    CHECK(status, !p->add_result);
  }
  else if (p->add_result) {
    CHECK(status, res_row == left_row * 2);
    res_row = left_row;
  } else {
    CHECK(status, res_row == left_row);
  }

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_FC_FIX8B;
  reg.tsk_opd_num = bias? 3: 2;
  reg.opt_shift_typ = is_arith_shift(p);
  reg.opt_res_shift = p->rshift_bits;
  reg.opt_left_shift = p->lshift_bits;
  reg.opt_relu_typ = p->relu_enable;
  reg.opt_res_add = p->add_result;

  reg.res0_addr = res->start_address;
  reg.opt_res0_seg = 1;
  reg.opt_res0_sign = matrix_is_signed(res);
  reg.res0_n = res_row;
  reg.res0_c = res->shape.c;
  reg.res0_h = 1;
  reg.res0_w = res->shape.w;
  reg.short_res0_str = 0;  // stride, b_stride calculated by H/W

  reg.opd0_addr = left->start_address;
  reg.opt_opd0_seg = 1;
  reg.opt_opd0_sign = (left->fmt == CVK_FMT_I8);
  reg.opd0_n = left_row;
  reg.opd0_c = left->shape.c;
  reg.opd0_h = 1;
  reg.opd0_w = left->shape.w;
  reg.short_opd0_str = 0;

  reg.opd1_addr = right->start_address;
  reg.opt_opd1_seg = 1;
  reg.opt_opd1_sign = (right->fmt == CVK_FMT_I8);
  reg.opd1_n = right_row;
  reg.opd1_c = right->shape.c;
  reg.opd1_h = 1;
  reg.opd1_w = left_col - left->shape.w * (left->shape.c - 1);
  reg.short_opd1_str = 0;

  reg.ps32_md = p->ps32_mode;
  if (p->ps32_mode > 0)
    reg.res0_b_str = p->res->shape.n * p->res->stride.n;
  if(reg.opd0_c == 1)
    CHECK(status, reg.opd0_w == reg.opd1_w);

  // Only enable 32-bit multiplier at the final post processing stage
  reg.opt_chl_quan = ((p->ps32_mode == 0) || (p->ps32_mode == 1)) ? 1 : 0;
  reg.quan_m = p->quan_m;

  // 32b bias, determined by b_stride
  if (bias) {
    CHECK(status, bias->shape.n == 4);
    CHECK(status, bias->shape.c == right->shape.c);
    CHECK(status, bias->shape.w == right->shape.w);
    CHECK(status, bias->shape.col == right->shape.col);

    reg.opd2_addr = bias->start_address;
    reg.opt_opd2_seg = 0;
    reg.opt_opd2_sign = (bias->fmt == CVK_FMT_I8);
    reg.opd2_n = 1;
    reg.opd2_c = bias->shape.c;
    reg.opd2_h = 1;
    reg.opd2_w = bias->shape.w;
    reg.short_opd2_str = 0;
  }

  reg.layer_info = p->layer_id;

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
