#include "kernel_1880v2.h"

bmk1880v2_op_t * bmk1880v2_tiu_depthwise_convolution_qdm(
    ctx_t *ctx,
    const bmk1880v2_tiu_depthwise_convolution_qdm_param_t *p)
{
  uint32_t eu_num = ctx->chip_info.eu_num;
  bool isMulConst = (p->weight_is_const == 1) ? 1 : 0;

  if(isMulConst) {
    check_tiu_tensor_2(p->ifmap, p->ofmap);
  } else {
    check_tiu_tensor_3(p->ifmap, p->ofmap, p->weight);
  }
  assert_stride_type_0(ctx, p->ifmap);
  if(!isMulConst)
    assert_stride_type_0(ctx, p->weight);
  check_tiu_tensor(p->chl_quan_param);
  assert_stride_type_2(ctx, p->chl_quan_param);

  ASSERT((p->ofmap->stride.n % eu_num) == 0);
  ASSERT(p->chl_quan_param->start_address % eu_num == 0);
  ASSERT(p->ifmap->shape.n == p->ofmap->shape.n);
  ASSERT(p->ifmap->shape.c == p->ofmap->shape.c);
  if(!isMulConst){
    ASSERT(p->ifmap->shape.c == p->weight->shape.c);
    ASSERT(p->weight->shape.n == 1);
  }

  int opd0_sign = tensor_is_signed(p->ifmap);

  tiu_reg_t reg;
  reset_tiu_reg(&reg);
  reg.cmd_en = 1;
  reg.tsk_typ = DCR_TYPE_DEPTHWISE_POOL_FIX8B;
  reg.tsk_eu_typ = 2;
  reg.opt_relu = p->relu_enable;
  reg.opt_shift_typ = 1;
  reg.tsk_opd_num = 2;

  int res0_sign = tensor_is_signed(p->ofmap);
  reg.res0_addr = p->ofmap->start_address;
  reg.opt_res0_sign = res0_sign;
  reg.opt_res0_int8 = 1;
  reg.res0_n = p->ofmap->shape.n;
  reg.res0_c = p->ofmap->shape.c;
  reg.res0_h = p->ofmap->shape.h;
  reg.res0_w = p->ofmap->shape.w;
  reg.res0_n_str = p->ofmap->stride.n;
  reg.res0_c_str = p->ofmap->stride.c;
  reg.res0_h_str = p->ofmap->stride.h;
  reg.res0_w_str = p->ofmap->stride.w;
  reg.short_res0_str = 3;   // Manual instead of h/w

  reg.opd0_addr = p->ifmap->start_address;
  reg.opt_opd0_sign = opd0_sign;
  reg.opt_opd0_int8 = 1;
  reg.opd0_n = p->ifmap->shape.n;
  reg.opd0_c = p->ifmap->shape.c;
  reg.opd0_h = p->ifmap->shape.h;
  reg.opd0_w = p->ifmap->shape.w;
  reg.opd0_ins_val = (uint32_t)p->ins_val;
  reg.opd0_n_str = p->ifmap->stride.n;
  reg.opd0_c_str = p->ifmap->stride.c;
  reg.opd0_h_str = p->ifmap->stride.h;
  reg.opd0_w_str = p->ifmap->stride.w;
  reg.short_opd0_str = 3;   // Manual instead of h/w
  reg.conv_opd0_up_pad = p->pad_top;
  reg.conv_opd0_dn_pad = p->pad_bottom;
  reg.conv_opd0_lf_pad = p->pad_left;
  reg.conv_opd0_rt_pad = p->pad_right;
  reg.conv_opd0_x_ins0 = p->ins_w;
  reg.conv_opd0_y_ins0 = p->ins_h;
  reg.conv_opd0_x_ins0_last = p->ins_last_w;
  reg.conv_opd0_y_ins0_last = p->ins_last_h;

  reg.opt_opd1_sign = 1;
  reg.opt_opd1_int8 = 1;
  reg.conv_opd1_x_ins0 = p->dilation_w - 1;
  reg.conv_opd1_y_ins0 = p->dilation_h - 1;
  if (isMulConst) {
    reg.opt_opd1_const = 1;
    reg.opt_opd1_sign = p->weight_const.is_signed;
    reg.opd1_addr = p->weight_const.val;
    reg.opd1_n = p->weight->shape.n;
    reg.opd1_c = p->weight->shape.c;
    reg.opd1_h = p->weight->shape.h;
    reg.opd1_w = p->weight->shape.w;
  } else {
    reg.opd1_addr = p->weight->start_address;
    reg.opd1_n = p->weight->shape.n;
    reg.opd1_c = p->weight->shape.c;
    reg.opd1_h = p->weight->shape.h;
    reg.opd1_w = p->weight->shape.w;
  }
  reg.conv_op_x_str = p->stride_w;
  reg.conv_op_y_str = p->stride_h;

  ASSERT(p->chl_quan_param->shape.n == 1);
  ASSERT(p->chl_quan_param->shape.c == p->ofmap->shape.c);
  ASSERT(p->chl_quan_param->shape.h == 1);
  ASSERT(p->chl_quan_param->shape.w == 1);
  reg.opt_chl_quan = 1;
  reg.opt_right_shift = 0;  // useless
  reg.opd2_addr = p->chl_quan_param->start_address;
  reg.opd2_n = p->chl_quan_param->shape.n;
  reg.opd2_c = p->chl_quan_param->shape.c;
  reg.opd2_h = p->chl_quan_param->shape.h;
  reg.opd2_w = p->chl_quan_param->shape.w;
  reg.opt_opd2_int8 = 1;  // useless, force to 1 to skip b_stride check
  reg.short_opd2_str = 2; // useless
  reg.opd2_b_str = 0;     // useless
  
  if (p->has_bias) {
    reg.tsk_opd_num = 3;
    reg.opt_opd2_sign = 1;
  }

  reg.layer_info = p->layer_id;

  return emit_tiu_cmdbuf(ctx, &reg);
}
