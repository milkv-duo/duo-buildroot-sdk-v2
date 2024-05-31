#include "cvkcv180x.h"

void cvkcv180x_tiu_max(
    cvk_context_t *ctx,
    const cvk_tiu_max_param_t *p)
{
  int8_t status = 0;
  int bf16_enable = (p->a->fmt == CVK_FMT_BF16) ? 1 : 0;

  status |= check_tiu_tensor_2(p->max, p->a);
  status |= check_same_shape(p->max, p->a);

  if (p->b_is_const && !bf16_enable) {
    if (tensor_is_signed(p->a))
      CHECK(status, p->b_const.is_signed);
    else
      CHECK(status, !p->b_const.is_signed);
  } else if (!p->b_is_const) {
    status |= check_tiu_tensor(p->b);
    status |= check_same_shape(p->max, p->b);
    CHECK(status, p->a->fmt == p->b->fmt);
  }
  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_MAX_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_res_shift = 0;
  reg.opt_relu_typ = 0;
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

  reg.res0_addr = p->max->start_address;
  reg.opt_res0_sign = tensor_is_signed(p->max);
  fill_res0_stride(&reg, &p->max->stride);

  reg.layer_info = p->layer_id;

  if (status) {
    printf("cvkcv180x tiu max: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
