#include "cvkcv181x.h"

void cvkcv181x_tiu_add(
    cvk_context_t *ctx,
    const cvk_tiu_add_param_t *p)
{
  int8_t status = 0;
  int bf16_enable = (p->a_low->fmt == CVK_FMT_BF16) ? 1 : 0;

  if (bf16_enable) {
    /*bf16 only support 16 bit*/
    CHECK(status, !p->a_high);
    CHECK(status, !(p->b.high && !p->b_is_const));
    CHECK(status, !p->res_high);
    status |= check_tiu_tensor(p->a_low);
    status |= check_tiu_tensor(p->res_low);
    status |= check_same_shape(p->res_low, p->a_low);
    if (!p->b_is_const) {
      status |= check_tiu_tensor(p->b.low);
      status |= check_same_shape(p->res_low, p->b.low);
    }
  } else {
    status |= check_16bit_tiu_tensor(p->a_low, p->a_high);
    status |= check_tiu_tensor(p->res_low);
    status |= check_same_shape(p->res_low, p->a_low);
    if (!p->b_is_const) {
      status |= check_16bit_tiu_tensor(p->b.low, p->b.high);
      status |= check_same_shape(p->res_low, p->b.low);
    }
  }
  if (p->res_high)
    status |= check_16bit_tiu_tensor(p->res_low, p->res_high);
  CHECK(status, p->relu_enable == 0 || p->relu_enable == 1);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_ADD_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_res_shift = 0;
  reg.opt_relu_typ = p->relu_enable;
  fill_same_tensor_shape(&reg, p->a_low->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  int arith_shift = tensor_is_signed(p->res_low);
  reg.opt_shift_typ = arith_shift;
  reg.opt_res_shift = p->rshift_bits;

  reg.opd0_addr = p->a_low->start_address;
  reg.opt_opd0_sign = tensor_is_signed(p->a_low);
  reg.opt_opd0_seg = (p->a_high == NULL);
  reg.opd0_b_str = bf16_enable ? 0 : (p->a_high->start_address - p->a_low->start_address);
  fill_opd0_stride(&reg, &p->a_low->stride);

  reg.opt_opd1_seg = bf16_enable ? 1 : 0; //(p->b_high == NULL); b_high is the same as b_val
  if (p->b_is_const) {
    reg.opt_opd1_const = 1;
    reg.opt_opd1_sign = !!p->b_const.is_signed;
    reg.opd1_addr = p->b_const.val;
  } else {
    reg.opt_opd1_const = 0;
    reg.opt_opd1_sign = tensor_is_signed(p->b.low);
    reg.opd1_addr = p->b.low->start_address;
    reg.opd1_b_str = bf16_enable ? 0 : (p->b.high->start_address - p->b.low->start_address);
    fill_opd1_stride(&reg, &p->b.low->stride);
  }

  reg.res0_addr = p->res_low->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->res_low);
  reg.opt_res0_seg = (p->res_high == NULL);
  fill_res0_stride(&reg, &p->res_low->stride);
  if (p->res_high)
    reg.res0_b_str = p->res_high->start_address - p->res_low->start_address;
  if (p->relu_enable)
    CHECK(status, reg.opt_res0_seg);

  reg.layer_info = p->layer_id;

  if (status) {
    printf("cvkcv181x tiu_add: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
