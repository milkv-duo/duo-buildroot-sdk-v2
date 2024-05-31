#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_mul_qdm(
    ctx_t *ctx,
    const bmk1880v2_tiu_element_wise_mul_qdm_param_t *p)
{
  check_tiu_tensor_2(p->res_low, p->a);
  assert_same_shape(p->res_low, p->a);
  if (!p->b_is_const) {
    check_tiu_tensor(p->b);
    assert_same_shape(p->res_low, p->b);
  }
  if (p->res_high)
    check_16bit_tiu_tensor(p->res_low, p->res_high);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_MUL_FIX8B;
  reg.tsk_opd_num = 2;
  int arith_shift = tensor_is_signed(p->res_low);
  reg.opt_shift_typ = arith_shift;
  reg.opt_right_shift = p->rshift_bits;
  reg.opt_relu = p->relu_enable;
  fill_same_tensor_shape(&reg, p->a->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = p->a->start_address;
  reg.opt_opd0_sign = tensor_is_signed(p->a);
  fill_opd0_stride(&reg, &p->a->stride);

  if (p->b_is_const) {
    reg.opt_opd1_const = 1;
    reg.opd1_addr = p->b_const.val;
    reg.opt_opd1_sign = !!p->b_const.is_signed;
  } else {
    reg.opt_opd1_const = 0;
    reg.opd1_addr = p->b->start_address;
    reg.opt_opd1_sign = tensor_is_signed(p->b);
    fill_opd1_stride(&reg, &p->b->stride);
  }

  reg.res0_addr = p->res_low->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->res_low);
  reg.opt_res0_int8 = (p->res_high == NULL);
  fill_res0_stride(&reg, &p->res_low->stride);
  if (p->res_high)
    reg.res0_b_str = p->res_high->start_address - p->res_low->start_address;
  if (p->relu_enable)
    ASSERT(reg.opt_res0_int8);

  ASSERT((
    (!reg.opt_opd1_sign && !reg.opt_opd0_sign && !reg.opt_shift_typ) ||
    ((reg.opt_opd1_sign || reg.opt_opd0_sign) && reg.opt_shift_typ)
  ));

  reg.opt_chl_quan = 1;
  reg.quan_m = p->multiplier;

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
