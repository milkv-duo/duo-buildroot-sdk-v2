#include "cvkcv181x.h"

void cvkcv181x_tiu_mul_qm(
    cvk_context_t *ctx,
    const cvk_tiu_mul_qm_param_t *p)
{
  int8_t status = 0;
  status |= check_tiu_tensor_2(p->res_low, p->a);
  status |= check_same_shape(p->res_low, p->a);
  if (!p->b_is_const) {
    status |= check_tiu_tensor(p->b);
    status |= check_same_shape(p->res_low, p->b);
  }
  if (p->res_high)
    status |= check_16bit_tiu_tensor(p->res_low, p->res_high);
  CHECK(status, p->relu_enable == 0 || p->relu_enable == 1);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_MUL_FIX8B;
  reg.tsk_opd_num = 2;
  int arith_shift = tensor_is_signed(p->res_low);
  reg.opt_shift_typ = arith_shift;
  reg.opt_res_shift = p->rshift_bits;
  reg.opt_relu_typ = p->relu_enable;
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
  reg.opt_res0_seg = (p->res_high == NULL);
  fill_res0_stride(&reg, &p->res_low->stride);
  if (p->res_high)
    reg.res0_b_str = p->res_high->start_address - p->res_low->start_address;
  if (p->relu_enable)
    CHECK(status, reg.opt_res0_seg);

  CHECK(status, (
    (!reg.opt_opd1_sign && !reg.opt_opd0_sign && !reg.opt_shift_typ) ||
    ((reg.opt_opd1_sign || reg.opt_opd0_sign) && reg.opt_shift_typ)
  ));

  reg.opt_chl_quan = 1;
  reg.quan_m = p->multiplier;

  reg.layer_info = p->layer_id;

  if (status) {
    printf("cvkcv181x tiu mul qm: wrong parameter\n");
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
