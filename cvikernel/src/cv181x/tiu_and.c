#include "cvkcv181x.h"

void cvkcv181x_tiu_and_int8(
    cvk_context_t *ctx,
    const cvk_tiu_and_int8_param_t *p)
{
  int8_t status = 0;
  status |= check_tiu_tensor_3(p->res, p->a, p->b);
  status |= check_same_shape_3(p->res, p->a, p->b);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_AND_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opt_res_shift = 0;
  reg.opt_shift_typ = 0;
  reg.opt_relu_typ = 0;
  fill_same_tensor_shape(&reg, p->a->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = p->a->start_address;
  reg.opt_opd0_sign = 0;
  reg.opt_opd0_seg = 1;
  fill_opd0_stride(&reg, &p->a->stride);

  reg.opd1_addr = p->b->start_address;
  reg.opt_opd1_sign = 0;
  reg.opt_opd1_seg = 1;
  fill_opd1_stride(&reg, &p->b->stride);

  reg.res0_addr = p->res->start_address;
  reg.opt_res0_sign = 0;
  reg.opt_res0_seg = 1;
  fill_res0_stride(&reg, &p->res->stride);

  reg.layer_info = p->layer_id;

  if (status) {
    printf("cvkcv181x tiu and: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}

void cvkcv181x_tiu_and_int16(
    cvk_context_t *ctx,
    const cvk_tiu_and_int16_param_t *p)
{
  int8_t status = 0;
  status |= check_16bit_tiu_tensor(p->a_low, p->a_high);
  status |= check_16bit_tiu_tensor(p->b_low, p->b_high);
  status |= check_16bit_tiu_tensor(p->res_low, p->res_high);
  status |= check_same_shape_3(p->res_low, p->a_low, p->b_low);

  int res_high_addr = p->res_high->start_address;
  int res_low_addr = p->res_low->start_address;
  CHECK(status, res_high_addr > res_low_addr);
  int res_b_stride = res_high_addr - res_low_addr;

  int a_high_addr = p->a_high->start_address;
  int a_low_addr = p->a_low->start_address;
  CHECK(status, a_high_addr > a_low_addr);
  int a_b_stride = a_high_addr - a_low_addr;

  int b_high_addr = p->b_high->start_address;
  int b_low_addr = p->b_low->start_address;
  CHECK(status, b_high_addr > b_low_addr);
  int b_b_stride = b_high_addr - b_low_addr;

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_AND_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opt_res_shift = 0;
  reg.opt_shift_typ = 0;
  reg.opt_relu_typ = 0;
  fill_same_tensor_shape(&reg, p->a_low->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = a_low_addr;
  reg.opt_opd0_sign = 0;
  reg.opt_opd0_seg = 0;
  reg.opd0_b_str = a_b_stride;
  fill_opd0_stride(&reg, &p->a_low->stride);

  reg.opd1_addr = b_low_addr;
  reg.opt_opd1_sign = 0;
  reg.opt_opd1_seg = 0;
  reg.opd1_b_str = b_b_stride;
  fill_opd1_stride(&reg, &p->b_low->stride);

  reg.res0_addr = res_low_addr;
  reg.opt_res0_sign = 0;
  reg.opt_res0_seg = 0;
  reg.res0_b_str = res_b_stride;
  fill_res0_stride(&reg, &p->res_low->stride);

  if (status) {
    printf("cvkcv181x tiu and: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
