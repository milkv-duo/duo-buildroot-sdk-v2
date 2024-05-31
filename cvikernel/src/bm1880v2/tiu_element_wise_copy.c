#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_element_wise_copy(
    ctx_t *ctx,
    const bmk1880v2_tiu_element_wise_copy_param_t *p)
{
  int bf16_enable = (p->src->fmt == FMT_BF16) ? 1 : 0;

  check_tiu_tensor_2(p->dst, p->src);
  assert_same_shape(p->dst, p->src);
  assert_stride_range(p->dst->stride);
  assert_stride_range(p->src->stride);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tsk_eu_typ = TENSOR_COPY_FIX8B;
  reg.tsk_opd_num = 1;
  reg.opd_typ = bf16_enable ? 1: 0;
  reg.opt_right_shift = 0;
  reg.opt_shift_typ = 0;
  reg.opt_relu = 0;
  fill_same_tensor_shape(&reg, p->dst->shape);
  fill_same_tensor_stride_type(&reg, 0b11);

  reg.opd0_addr = p->src->start_address;
  reg.opt_opd0_sign = 0;
  reg.opt_opd0_int8 = 1;
  fill_opd0_stride(&reg, &p->src->stride);

  reg.res0_addr = p->dst->start_address;
  reg.opt_res0_sign = 0;
  reg.opt_res0_int8 = 1;
  fill_res0_stride(&reg, &p->dst->stride);

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
