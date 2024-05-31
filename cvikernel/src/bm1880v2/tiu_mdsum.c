#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_mdsum(
    ctx_t *ctx,
    const bmk1880v2_tiu_mdsum_param_t *p)
{
  const bmk1880v2_tensor_lmem_t *res = p->res;
  const bmk1880v2_tensor_lmem_t *input = p->input;

  check_tiu_tensor_2(res, input);
  ASSERT(res->fmt == input->fmt);
  if (p->res_is_int8)
    ASSERT(res->shape.n == 1);
  else
    ASSERT(res->shape.n == 2);
  ASSERT(res->shape.c == input->shape.c);
  ASSERT(res->shape.h == 1);
  ASSERT(res->shape.w == 1);

  int res_addr = res->start_address;

  tiu_reg_t reg;
  reset_tiu_reg(&reg);

  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_TENSOR_ARITH_FIX8B;
  reg.tens_mdsum = 1;
  reg.tsk_opd_num = 1;
  reg.opt_relu = 0;

  int arith_shift = tensor_is_signed(res);
  reg.opt_shift_typ = arith_shift;
  reg.opt_right_shift = p->rshift_bits;

  reg.opd0_addr = input->start_address;
  reg.opt_opd0_sign = tensor_is_signed(input);
  reg.opt_opd0_int8 = 1;
  reg.opd0_n = input->shape.n;
  reg.opd0_c = input->shape.c;
  reg.opd0_h = input->shape.h;
  reg.opd0_w = input->shape.w;
  reg.opd0_n_str = input->stride.n;
  reg.opd0_c_str = input->stride.c;
  reg.opd0_h_str = input->stride.h;
  reg.opd0_w_str = 1;

  reg.res0_addr = res_addr;
  reg.opt_res0_sign = tensor_is_signed(res);
  reg.opt_res0_int8 = p->res_is_int8;
  reg.res0_n = 1;
  reg.res0_c = res->shape.c;
  reg.res0_h = 1;
  reg.res0_w = 1;
  reg.short_res0_str = 0b01;

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
