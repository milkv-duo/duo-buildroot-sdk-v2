#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_min(
    ctx_t *ctx,
    const bmk1880v2_tiu_element_wise_min_param_t *p)
{
  int bf16_enable = (p->a->fmt == FMT_BF16) ? 1 : 0;

  check_tiu_tensor_2(p->min, p->a);
  assert_same_shape(p->min, p->a);
  if (p->b_is_const && !bf16_enable) {
    if (tensor_is_signed(p->a))
      ASSERT(p->b_const.is_signed);
    else
      ASSERT(!p->b_const.is_signed);
  } else if (!p->b_is_const) {
    check_tiu_tensor(p->b);
    assert_same_shape(p->min, p->b);
    ASSERT(p->a->fmt == p->b->fmt);
  }

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_MIN_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_right_shift = 0;
  reg.opt_relu = 0;
  fill_same_tensor_shape(&reg, p->a->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = p->a->start_address;
  reg.opt_opd0_sign = tensor_is_signed(p->a);
  fill_opd0_stride(&reg, &p->a->stride);

  if (p->b_is_const) {
    reg.opt_opd1_const = 1;
    reg.opd1_addr = bf16_enable ? p->b_const.val : (p->b_const.val & 0xFF);
    reg.opt_opd1_sign = !!p->b_const.is_signed;
  } else {
    reg.opt_opd1_const = 0;
    reg.opd1_addr = p->b->start_address;
    reg.opt_opd1_sign = tensor_is_signed(p->b);
    fill_opd1_stride(&reg, &p->b->stride);
  }

  reg.res0_addr = p->min->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->min);
  fill_res0_stride(&reg, &p->min->stride);

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
