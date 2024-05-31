#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_sub(
    ctx_t *ctx,
    const bmk1880v2_tiu_element_wise_sub_param_t *p)
{
  int bf16_enable = (p->a_low->fmt == FMT_BF16) ? 1 : 0;

  if (bf16_enable) {
    /*bf16 only support 16 bit*/
    ASSERT(!p->a_high);
    ASSERT(!p->b_high);
    ASSERT(!p->res_high);
    check_tiu_tensor(p->a_low);
    check_tiu_tensor(p->b_low);
    check_tiu_tensor(p->res_low);
    assert_same_shape_3(p->res_low, p->a_low, p->b_low);
  } else {
    check_16bit_tiu_tensor(p->a_low, p->a_high);
    check_16bit_tiu_tensor(p->b_low, p->b_high);
    check_tiu_tensor(p->res_low);
    assert_same_shape_3(p->res_low, p->a_low, p->b_low);
    ASSERT(tensor_is_signed(p->res_low));
  }
  if (p->res_high)
    check_16bit_tiu_tensor(p->res_low, p->res_high);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_SUB_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_right_shift = 0;
  reg.opt_relu = 0;
  fill_same_tensor_shape(&reg, p->a_low->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  int arith_shift = tensor_is_signed(p->res_low);
  reg.opt_shift_typ = arith_shift;
  reg.opt_right_shift = p->rshift_bits;

  reg.opd0_addr = p->a_low->start_address;
  reg.opt_opd0_sign = tensor_is_signed(p->a_low);
  reg.opt_opd0_int8 = (p->a_high == NULL);
  reg.opd0_b_str =  bf16_enable ? 0 : (p->a_high->start_address - p->a_low->start_address);
  fill_opd0_stride(&reg, &p->a_low->stride);

  reg.opd1_addr = p->b_low->start_address;
  reg.opt_opd1_sign = tensor_is_signed(p->b_low);;
  reg.opt_opd1_int8 = (p->b_high == NULL);
  reg.opd1_b_str =  bf16_enable ? 0 : (p->b_high->start_address - p->b_low->start_address);
  fill_opd1_stride(&reg, &p->b_low->stride);

  reg.res0_addr = p->res_low->start_address;
  reg.opt_res0_sign = 1;
  reg.opt_res0_int8 = (p->res_high == NULL);
  fill_res0_stride(&reg, &p->res_low->stride);
  if (p->res_high)
    reg.res0_b_str = bf16_enable ? 0 : (p->res_high->start_address - p->res_low->start_address);

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
