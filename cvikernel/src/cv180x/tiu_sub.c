#include "cvkcv180x.h"

void cvkcv180x_tiu_sub(
    cvk_context_t *ctx,
    const cvk_tiu_sub_param_t *p)
{
  int8_t status = 0;
  int bf16_enable = (p->a_low->fmt == CVK_FMT_BF16) ? 1 : 0;

  if (bf16_enable) {
    /*bf16 only support 16 bit*/
    CHECK(status, !p->a_high);
    CHECK(status, !p->b_high);
    CHECK(status, !p->res_high);
    status |= check_tiu_tensor(p->a_low);
    status |= check_tiu_tensor(p->b_low);
    status |= check_tiu_tensor(p->res_low);
    status |= check_same_shape_3(p->res_low, p->a_low, p->b_low);
  } else {
    status |= check_16bit_tiu_tensor(p->a_low, p->a_high);
    status |= check_16bit_tiu_tensor(p->b_low, p->b_high);
    status |= check_tiu_tensor(p->res_low);
    status |= check_same_shape_3(p->res_low, p->a_low, p->b_low);
    CHECK(status, tensor_is_signed(p->res_low));
  }
  if (p->res_high)
    status |= check_16bit_tiu_tensor(p->res_low, p->res_high);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_SUB_FIX8B;
  reg.tsk_opd_num = 2;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_res_shift = 0;
  reg.opt_relu_typ = 0;
  fill_same_tensor_shape(&reg, p->a_low->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  int arith_shift = tensor_is_signed(p->res_low);
  reg.opt_shift_typ = arith_shift;
  reg.opt_res_shift = p->rshift_bits;

  reg.opd0_addr = p->a_low->start_address;
  reg.opt_opd0_sign = tensor_is_signed(p->a_low);
  reg.opt_opd0_seg = (p->a_high == NULL);
  reg.opd0_b_str =  bf16_enable ? 0 : (p->a_high->start_address - p->a_low->start_address);
  fill_opd0_stride(&reg, &p->a_low->stride);

  reg.opd1_addr = p->b_low->start_address;
  reg.opt_opd1_sign = tensor_is_signed(p->b_low);;
  reg.opt_opd1_seg = (p->b_high == NULL);
  reg.opd1_b_str =  bf16_enable ? 0 : (p->b_high->start_address - p->b_low->start_address);
  fill_opd1_stride(&reg, &p->b_low->stride);

  reg.res0_addr = p->res_low->start_address;
  reg.opt_res0_sign = 1;
  reg.opt_res0_seg = (p->res_high == NULL);
  fill_res0_stride(&reg, &p->res_low->stride);
  if (p->res_high)
    reg.res0_b_str =  bf16_enable ? 0 : (p->res_high->start_address - p->res_low->start_address);

  reg.layer_info = p->layer_id;

  if (status) {
    printf("cvkcv180x tiu sub: wrong parameter\n");
    return;
  }

  (void *)emit_tiu_cmdbuf(ctx, &reg);
}
