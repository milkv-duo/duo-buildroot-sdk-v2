#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_arith_shift(
    ctx_t *ctx,
    const bmk1880v2_tiu_element_wise_arith_shift_param_t *p)
{
  check_16bit_tiu_tensor(p->a_low, p->a_high);
  check_16bit_tiu_tensor(p->res_low, p->res_high);
  check_tiu_tensor(p->bits);
  assert_same_shape_3(p->res_low, p->a_low, p->bits);
  ASSERT(tensor_is_signed(p->a_low));
  ASSERT(tensor_is_signed(p->bits));

  int res_high_addr = p->res_high->start_address;
  int res_low_addr = p->res_low->start_address;
  ASSERT(res_high_addr > res_low_addr);
  int res_b_stride = res_high_addr - res_low_addr;

  int a_high_addr = p->a_high->start_address;
  int a_low_addr = p->a_low->start_address;
  ASSERT(a_high_addr > a_low_addr);
  int a_b_stride = a_high_addr - a_low_addr;

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_SHIFT_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opt_right_shift = 0;
  reg.opt_rshift_typ = 0;
  reg.opt_relu = 0;
  fill_same_tensor_shape(&reg, p->a_low->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = a_low_addr;
  reg.opt_opd0_sign = 1;
  reg.opt_opd0_int8 = 0;
  reg.opd0_b_str = a_b_stride;
  fill_opd0_stride(&reg, &p->a_low->stride);

  reg.opd1_addr = p->bits->start_address;
  reg.opt_opd1_sign = 1;
  reg.opt_opd1_int8 = 1;
  fill_opd1_stride(&reg, &p->bits->stride);

  reg.res0_addr = res_low_addr;
  reg.opt_res0_sign = 1;
  reg.opt_res0_int8 = 0;
  reg.res0_b_str = res_b_stride;
  fill_res0_stride(&reg, &p->res_low->stride);

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
