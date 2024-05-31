#include "kernel_1822.h"

bmk1822_op_t * bmk1822_tiu_min_pooling(
    ctx_t *ctx,
    const bmk1822_tiu_min_pooling_param_t *p)
{
  check_tiu_tensor_2(p->ifmap, p->ofmap);
  ASSERT(p->kh * p->kw > 1);
  ASSERT(p->ifmap->shape.n == p->ofmap->shape.n);
  ASSERT(p->ifmap->shape.c == p->ofmap->shape.c);
  assert_stride_type_0(ctx, p->ifmap);
  assert_stride_type_0(ctx, p->ofmap);

  int opd0_sign = tensor_is_signed(p->ifmap);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_DEPTHWISE_POOL_FIX8B;
  reg.tsk_eu_typ = 3;
  reg.opt_relu_typ = 0; /* Hardware relu function not validated. */
  reg.opt_res_shift = 0;
  reg.opt_shift_typ = opd0_sign;
  reg.tsk_opd_num = 1;

  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = opd0_sign;
  reg.opt_res0_seg = 1;
  reg.res0_n = p->ofmap->shape.n;
  reg.res0_c = p->ofmap->shape.c;
  reg.res0_h = p->ofmap->shape.h;
  reg.res0_w = p->ofmap->shape.w;

  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_seg = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.conv_opd0_up_pad = p->pad_top;
  reg.conv_opd0_dn_pad = p->pad_bottom;
  reg.conv_opd0_lf_pad = p->pad_left;
  reg.conv_opd0_rt_pad = p->pad_right;
  if (opd0_sign)
    reg.opd0_ins_val = (uint16_t)127;
  else
    reg.opd0_ins_val = (uint16_t)255;
  reg.opt_opd1_seg = 1;
  reg.opd1_h = p->kh;
  reg.opd1_w = p->kw;
  reg.conv_op_x_str = p->stride_w;
  reg.conv_op_y_str = p->stride_h;

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}

bmk1822_op_t * bmk1822_tiu_bf16_min_pooling(
    ctx_t *ctx,
    const bmk1822_tiu_min_pooling_param_t *p)
{
  int bf16_enable = (p->ifmap->fmt == FMT_BF16) ? 1 : 0;

  check_tiu_tensor_2(p->ifmap, p->ofmap);
  ASSERT(p->kh * p->kw > 1);
  ASSERT(p->ifmap->shape.n == p->ofmap->shape.n);
  ASSERT(p->ifmap->shape.c == p->ofmap->shape.c);
  if (bf16_enable) {
    assert_bf16_stride_type_0(ctx, p->ifmap);
    assert_bf16_stride_type_0(ctx, p->ofmap);
  } else {
    assert_stride_type_0(ctx, p->ifmap);
    assert_stride_type_0(ctx, p->ofmap);
  }
  int opd0_sign = tensor_is_signed(p->ifmap);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_DEPTHWISE_POOL_FIX8B;
  reg.tsk_eu_typ = 3;
  reg.opt_relu_typ = 0; /* Hardware relu function not validated. */
  reg.opt_res_shift = 0;
  reg.opt_shift_typ = opd0_sign;
  reg.tsk_opd_num = 1;
  reg.opd_typ = bf16_enable ? 1: 0;

  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = opd0_sign;
  reg.opt_res0_seg = 1;
  reg.res0_n = p->ofmap->shape.n;
  reg.res0_c = p->ofmap->shape.c;
  reg.res0_h = p->ofmap->shape.h;
  reg.res0_w = p->ofmap->shape.w;
  reg.opd0_ins_val = p->ins_fp;
  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_seg = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.conv_opd0_up_pad = p->pad_top;
  reg.conv_opd0_dn_pad = p->pad_bottom;
  reg.conv_opd0_lf_pad = p->pad_left;
  reg.conv_opd0_rt_pad = p->pad_right;

  reg.opt_opd1_seg = 1;
  reg.opd1_h = p->kh;
  reg.opd1_w = p->kw;
  reg.conv_op_x_str = p->stride_w;
  reg.conv_op_y_str = p->stride_h;

  /* [15:0] layer id */
  reg.layer_info = p->layer_id;
  return emit_tiu_cmdbuf(ctx, &reg);
}
